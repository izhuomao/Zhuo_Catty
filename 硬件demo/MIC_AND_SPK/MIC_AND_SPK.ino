#include <WiFi.h>
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
#include "RgbLed.h"

// =================配置区域=================
const char* ssid = "茁猫阿姨洗铁路"; 
const char* password = "12345678";
const char* pc_ip = "192.168.201.147";
const int pc_port = 12345;

#define MIC_I2S_SCK 14
#define MIC_I2S_WS  15
#define MIC_I2S_SD  32
#define SPK_I2S_BCLK 25
#define SPK_I2S_LRC  26
#define SPK_I2S_DIN  33

RgbLed myLed(13, 19, 27);

enum State { STATE_WAKEUP, STATE_RECORDING, STATE_WAITING, STATE_PLAYING };
State currentState = STATE_WAKEUP;

// AI & FFT 参数
const int BLOCK_SIZE = 512;
int16_t samples[BLOCK_SIZE];
double vReal[BLOCK_SIZE];
double vImag[BLOCK_SIZE];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BLOCK_SIZE, 44100);
int record_count = -1;
float ai_features[50][8]; 

// TFLite
tflite::MicroErrorReporter tflErrorReporter;
tflite::ops::micro::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
constexpr int tensorArenaSize = 16 * 1024;
byte tensorArena[tensorArenaSize];

// UDP & 计时
WiFiUDP udp;
unsigned long recordStartTime = 0;
unsigned long lastPlayPacketTime = 0;
const unsigned long RECORD_DURATION = 5000; 

// ================= I2S 初始化 =================
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
  i2s_pin_config_t mic_pins = {.bck_io_num = MIC_I2S_SCK, .ws_io_num = MIC_I2S_WS, .data_out_num = -1, .data_in_num = MIC_I2S_SD};
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
  i2s_pin_config_t spk_pins = {.bck_io_num = SPK_I2S_BCLK, .ws_io_num = SPK_I2S_LRC, .data_out_num = SPK_I2S_DIN, .data_in_num = -1};
  i2s_set_pin(I2S_NUM_1, &spk_pins);
}

// ================= 推理逻辑 =================
bool run_ai_inference() {
  TfLiteTensor* input = tflInterpreter->input(0);
  for (int i = 0; i < 400; i++) input->data.f[i] = ((float*)ai_features)[i] / 128.0;
  if (tflInterpreter->Invoke() != kTfLiteOk) return false;
  return (tflInterpreter->output(0)->data.f[1] > 0.75); // 稍微提高一点阈值防止误触
}

// ================= 主循环 =================
void setup() {
  Serial.begin(115200);
  myLed.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  udp.begin(pc_port);
  i2s_install();
  tflModel = tflite::GetModel(model);
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);
  tflInterpreter->AllocateTensors();
  Serial.println("System Ready!");
}

void loop() {
  switch (currentState) {
    
    case STATE_WAKEUP: {
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
          Serial.println(">>> 唤醒成功!");
          myLed.showRed(80);
          udp.beginPacket(pc_ip, pc_port);
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
        udp.beginPacket(pc_ip, pc_port);
        udp.write((uint8_t*)mic_buff, read);
        udp.endPacket();
      }
      if (millis() - recordStartTime > RECORD_DURATION) {
        udp.beginPacket(pc_ip, pc_port);
        udp.write((uint8_t*)"END", 3);
        udp.endPacket();
        Serial.println("录音结束，等待服务器回复...");
        myLed.showBlue(50);
        currentState = STATE_WAITING;
        recordStartTime = millis(); // 借用这个时间做等待超时判断
      }
      break;
    }

    case STATE_WAITING: {
      int packetSize = udp.parsePacket();
      if (packetSize > 0) {
        // 收到第一个数据包！
        Serial.printf("收到首包，大小: %d。进入播放模式...\n", packetSize);
        lastPlayPacketTime = millis();
        currentState = STATE_PLAYING;
        myLed.showGreen(50);
        
        // 立即处理这第一个包，不要丢掉
        char first_buff[1460];
        int len = udp.read(first_buff, 1460);
        size_t written;
        i2s_write(I2S_NUM_1, first_buff, len, &written, portMAX_DELAY);
      }
      // 如果服务器 15 秒都没给任何 UDP 包，回退到唤醒
      if (millis() - recordStartTime > 15000) {
        Serial.println("等待超时，服务器未响应。");
        currentState = STATE_WAKEUP;
        myLed.turnOff();
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
          lastPlayPacketTime = millis(); // 只有真正播出了声音，才重置计时器
        }
      }

      // 重点：增加判断，只有连续 2 秒没收到新包，才认为播放彻底结束
      if (millis() - lastPlayPacketTime > 2000) {
        Serial.println("数据流中断超过2秒，播放结束。");
        myLed.turnOff();
        currentState = STATE_WAKEUP;
      }
      break;
    }
  }
}
