#ifndef VOICE_SYSTEM_H
#define VOICE_SYSTEM_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h>
#include <tensorflow/lite/experimental/micro/micro_error_reporter.h>
#include <tensorflow/lite/experimental/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>
#include "model.h"

static const char* VOICE_PC_IP = "192.168.201.147";
static const int VOICE_PC_PORT = 12345;

#define VOICE_MIC_I2S_SCK 14
#define VOICE_MIC_I2S_WS  15
#define VOICE_MIC_I2S_SD  32
#define VOICE_SPK_I2S_BCLK 25
#define VOICE_SPK_I2S_LRC  26
#define VOICE_SPK_I2S_DIN  33

#define MEMO_BUF_SIZE 512
#define REMIND_BUF_SIZE 256  // ▼▼▼ 新增：提醒文本缓冲 ▼▼▼

class VoiceSystem {
private:
    enum VoiceState { STATE_WAKEUP, STATE_RECORDING, STATE_WAITING, STATE_PLAYING };
    VoiceState currentState = STATE_WAKEUP;

    static const int BLOCK_SIZE = 512;
    int16_t samples[BLOCK_SIZE];
    double vReal[BLOCK_SIZE];
    double vImag[BLOCK_SIZE];
    ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BLOCK_SIZE, 44100);
    
    int record_count = -1;
    float ai_features[50][8];

    tflite::MicroInterpreter* tflInterpreter = nullptr;
    uint8_t* tensorArena = nullptr;
    const int tensorArenaSize = 16 * 1024;

    WiFiUDP udp;
    unsigned long recordStartTime = 0;
    unsigned long lastPlayPacketTime = 0;
    const unsigned long RECORD_DURATION = 5000;

    void (*_onActionTrigger)(int) = nullptr;
    void (*_onMemoReceived)(const char*) = nullptr;
    char memoBuf[MEMO_BUF_SIZE];
    volatile bool hasPendingMemo = false;

    // ▼▼▼ 新增：提醒 TTS 请求缓冲 ▼▼▼
    char remindBuf[REMIND_BUF_SIZE];
    volatile bool hasPendingRemind = false;

    void i2s_install() {
        i2s_config_t mic_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = 44100,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
            .communication_format = I2S_COMM_FORMAT_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 8, .dma_buf_len = 64, .use_apll = true
        };
        i2s_driver_install(I2S_NUM_0, &mic_config, 0, NULL);
        i2s_pin_config_t mic_pins = {.bck_io_num = VOICE_MIC_I2S_SCK, .ws_io_num = VOICE_MIC_I2S_WS, .data_out_num = -1, .data_in_num = VOICE_MIC_I2S_SD};
        i2s_set_pin(I2S_NUM_0, &mic_pins);

        i2s_config_t spk_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = 44100,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
            .communication_format = I2S_COMM_FORMAT_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 8, .dma_buf_len = 64, .use_apll = true, .tx_desc_auto_clear = true
        };
        i2s_driver_install(I2S_NUM_1, &spk_config, 0, NULL);
        i2s_pin_config_t spk_pins = {.bck_io_num = VOICE_SPK_I2S_BCLK, .ws_io_num = VOICE_SPK_I2S_LRC, .data_out_num = VOICE_SPK_I2S_DIN, .data_in_num = -1};
        i2s_set_pin(I2S_NUM_1, &spk_pins);
    }

    bool run_ai_inference() {
        TfLiteTensor* input = tflInterpreter->input(0);
        for (int i = 0; i < 400; i++) input->data.f[i] = ((float*)ai_features)[i] / 128.0;
        if (tflInterpreter->Invoke() != kTfLiteOk) return false;
        return (tflInterpreter->output(0)->data.f[1] > 0.75);
    }

public:
    VoiceSystem() {
        memset(memoBuf, 0, MEMO_BUF_SIZE);
        memset(remindBuf, 0, REMIND_BUF_SIZE);
    }

    void init(void (*actionCallback)(int), void (*memoCallback)(const char*) = nullptr) {
        _onActionTrigger = actionCallback;
        _onMemoReceived = memoCallback;
        tensorArena = (uint8_t*)malloc(tensorArenaSize);
        i2s_install();

        static tflite::MicroErrorReporter tflErrorReporter;
        static tflite::ops::micro::AllOpsResolver tflOpsResolver;
        const tflite::Model* tflModel = tflite::GetModel(model);
        tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);
        tflInterpreter->AllocateTensors();

        udp.begin(VOICE_PC_PORT);
    }

    // ▼▼▼ 新增：请求提醒 TTS ▼▼▼
    // 由主程序调用，将提醒文本暂存，等 STATE_WAKEUP 时发给 Python
    void requestRemindTTS(const char* text) {
        int len = strlen(text);
        if (len > 0 && len < REMIND_BUF_SIZE - 1) {
            strncpy(remindBuf, text, REMIND_BUF_SIZE - 1);
            remindBuf[REMIND_BUF_SIZE - 1] = '\0';
            hasPendingRemind = true;
            Serial.printf("[Voice] 🔔 收到提醒 TTS 请求: %s\n", text);
        }
    }

    void update() {
        switch (currentState) {
            case STATE_WAKEUP: {
                // ▼▼▼ 新增：优先检查是否有待发送的提醒 TTS 请求 ▼▼▼
                if (hasPendingRemind) {
                    hasPendingRemind = false;
                    Serial.println("[Voice] 🔔 发送 REMIND 包给 Python...");

                    // 构造 REMIND:文本 包
                    String packet = "REMIND:" + String(remindBuf);
                    udp.beginPacket(VOICE_PC_IP, VOICE_PC_PORT);
                    udp.write((uint8_t*)packet.c_str(), packet.length());
                    udp.endPacket();

                    // 切换到等待状态，等待 Python 回传 CMD + 音频
                    if(_onActionTrigger) _onActionTrigger(17); // 蓝灯表示处理中
                    recordStartTime = millis();
                    currentState = STATE_WAITING;
                    break;
                }

                // 正常唤醒词检测逻辑（不变）
                size_t read;
                i2s_read(I2S_NUM_0, samples, BLOCK_SIZE * 2, &read, portMAX_DELAY);
                for (int i=0; i<BLOCK_SIZE; i++) { vReal[i] = samples[i]*256.0; vImag[i] = 0; }
                FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
                FFT.compute(FFT_FORWARD);
                FFT.complexToMagnitude();

                int bsum = 0;
                int temp[8] = {0};
                for (int i=2; i<(BLOCK_SIZE/2); i++) {
                    if (vReal[i] > 2000) {
                        int b = (i<=4)?0:(i<=8)?1:(i<=16)?2:(i<=32)?3:(i<=64)?4:(i<=128)?5:(i<=256)?6:7;
                        temp[b] = max(temp[b], (int)(vReal[i]/150));
                    }
                }
                for (int j=0; j<8; j++) {
                    temp[j] = constrain(int(temp[j]*(128.0/100000)), 0, 128);
                    bsum += temp[j];
                }

                if (bsum >= 40 && record_count == -1) record_count = 0;
                if (record_count >= 0 && record_count < 50) {
                    for(int j=0; j<8; j++) ai_features[record_count][j] = temp[j];
                    record_count++;
                }
                if (record_count == 50) {
                    if (run_ai_inference()) {
                        if(_onActionTrigger) _onActionTrigger(16);
                        udp.beginPacket(VOICE_PC_IP, VOICE_PC_PORT);
                        udp.write((uint8_t*)"START", 5);
                        udp.endPacket();
                        recordStartTime = millis();
                        currentState = STATE_RECORDING;
                    }
                    record_count = -1;
                }
                break;
            }

            case STATE_RECORDING: {
                char mic_buff[1024];
                size_t read;
                i2s_read(I2S_NUM_0, mic_buff, 1024, &read, portMAX_DELAY);
                if (read > 0) {
                    udp.beginPacket(VOICE_PC_IP, VOICE_PC_PORT);
                    udp.write((uint8_t*)mic_buff, read);
                    udp.endPacket();
                }
                if (millis() - recordStartTime > RECORD_DURATION) {
                    udp.beginPacket(VOICE_PC_IP, VOICE_PC_PORT);
                    udp.write((uint8_t*)"END", 3);
                    udp.endPacket();
                    if(_onActionTrigger) _onActionTrigger(17);
                    currentState = STATE_WAITING;
                    recordStartTime = millis();
                }
                break;
            }

            case STATE_WAITING: {
                int packetSize = udp.parsePacket();
                if (packetSize > 0) {
                    char buff[1460];
                    int len = udp.read(buff, 1460);
                    
                    if (len > 0) {
                        // 1. 动作/表情指令包 "CMD:ID"
                        if (len >= 4 && strncmp(buff, "CMD:", 4) == 0) {
                            int actionId = atoi(&buff[4]);
                            Serial.printf("收到动作指令 ID: %d\n", actionId);
                            if(_onActionTrigger) {
                                _onActionTrigger(actionId);
                            }
                            recordStartTime = millis(); 
                        }
                        // 2. 备忘录数据包 "MEMO:{json}"
                        else if (len >= 5 && strncmp(buff, "MEMO:", 5) == 0) {
                            int jsonLen = len - 5;
                            if (jsonLen > 0 && jsonLen < MEMO_BUF_SIZE - 1) {
                                memcpy(memoBuf, buff + 5, jsonLen);
                                memoBuf[jsonLen] = '\0';
                                hasPendingMemo = true;
                                Serial.printf("📝 收到备忘录: %s\n", memoBuf);
                                if (_onMemoReceived) {
                                    _onMemoReceived(memoBuf);
                                }
                            }
                            recordStartTime = millis();
                        }
                        // 3. 音频数据
                        else {
                            lastPlayPacketTime = millis();
                            currentState = STATE_PLAYING;
                            if(_onActionTrigger) _onActionTrigger(18);
                            
                            size_t written;
                            i2s_write(I2S_NUM_1, buff, len, &written, portMAX_DELAY);
                        }
                    }
                }
                
                if (millis() - recordStartTime > 15000) {
                    currentState = STATE_WAKEUP;
                    if(_onActionTrigger) _onActionTrigger(3);
                }
                break;
            }

            case STATE_PLAYING: {
                int packetSize = udp.parsePacket();
                if (packetSize > 0) {
                    char spk_buff[1460];
                    int len = udp.read(spk_buff, 1460);
                    size_t written;
                    if (len > 0) {
                        i2s_write(I2S_NUM_1, spk_buff, len, &written, portMAX_DELAY);
                        lastPlayPacketTime = millis();
                    }
                }
                if (millis() - lastPlayPacketTime > 2000) {
                    if(_onActionTrigger) _onActionTrigger(3);
                    currentState = STATE_WAKEUP;
                }
                break;
            }
        }
    }
};

#endif
