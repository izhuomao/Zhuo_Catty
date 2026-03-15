#include "RgbLed.h"
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h>
#include <tensorflow/lite/experimental/micro/micro_error_reporter.h>
#include <tensorflow/lite/experimental/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>
#include "model.h"

// --- 宏定义与常量 ---
#define I2S_PORT I2S_NUM_1
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14

#define MAX_STEP     5    // 每次移动的步长（帧数）
#define SAMPLE_NUMS  50   // 识别窗口的总长度（帧数）

RgbLed myLed(13, 19, 27); // 假设沿用之前的引脚

const int BLOCK_SIZE = 512;
const double SAMPLING_FREQUENCY = 44100;
int16_t samples[BLOCK_SIZE];

// FFT 变量
double vReal[BLOCK_SIZE];
double vImag[BLOCK_SIZE];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BLOCK_SIZE, SAMPLING_FREQUENCY);
int bands[8] = {0};
const uint8_t amplitude = 150;

// 推理相关变量
int sign = 0;
int Inputindex = 0;
int Inputbands[SAMPLE_NUMS][8] = {0};
int steps = MAX_STEP;

unsigned long wakeTimer = 0;         // 记录触发唤醒时的时间戳
bool isAwake = false;                // 标记当前是否处于“已唤醒”状态
const unsigned long WAKE_DURATION = 5000; // 保持唤醒的时长（5秒 = 5000毫秒）


tflite::MicroErrorReporter tflErrorReporter;
tflite::ops::micro::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;
constexpr int tensorArenaSize = 10 * 1024; // 稍微调大一点点确保稳定
byte tensorArena[tensorArenaSize];

const char* VOICES[] = {
  "silence",
  "Zhuo_Catty", 
  "noise"
};
#define NUM_VOICES (sizeof(VOICES) / sizeof(VOICES[0]))

// --- 函数声明 ---
void i2s_init();
void i2s_setpin();
void FFT_Operation();
void constrain128();
void move_Inputindex();
void detection();
void application();

void setup() {
  Serial.begin(115200);
  i2s_init();
  i2s_setpin();
  myLed.begin();

  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    while (1);
  }
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);
  tflInterpreter->AllocateTensors();
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
  
  Serial.println("System Ready. Continuous Listening Mode...");
}

void loop() {
  size_t bytes_read = 0;
  // 从 I2S 读取音频数据
  i2s_read(I2S_PORT, (void *)samples, BLOCK_SIZE * 2, &bytes_read, portMAX_DELAY);

  if (bytes_read > 0) {
      FFT_Operation();
      constrain128();
      
      move_Inputindex(); // 填充滑动窗口

      if (sign == 1) {
          detection();   // 模型推理
          application(); // 执行动作
          sign = 0;      // 重置标志位
      }
  }
}

// --- 功能实现 ---

void FFT_Operation() {
  for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
    vReal[i] = (double)samples[i] * 256; 
    vImag[i] = 0.0; 
  }
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude(); 

  for (int i = 0; i < 8; i++) bands[i] = 0;

  for (int i = 2; i < (BLOCK_SIZE/2); i++) { 
    if (vReal[i] > 2000) { 
      if (i > 2   && i <= 4 )   bands[0] = max(bands[0], (int)(vReal[i]/amplitude));
      if (i > 4   && i <= 8 )   bands[1] = max(bands[1], (int)(vReal[i]/amplitude));
      if (i > 8   && i <= 16 )  bands[2] = max(bands[2], (int)(vReal[i]/amplitude));
      if (i > 16  && i <= 32 )  bands[3] = max(bands[3], (int)(vReal[i]/amplitude));
      if (i > 32  && i <= 64 )  bands[4] = max(bands[4], (int)(vReal[i]/amplitude));
      if (i > 64  && i <= 128 ) bands[5] = max(bands[5], (int)(vReal[i]/amplitude));
      if (i > 128 && i <= 256 ) bands[6] = max(bands[6], (int)(vReal[i]/amplitude));
      if (i > 256           )   bands[7] = max(bands[7], (int)(vReal[i]/amplitude));
    }
  }
}

void constrain128() {
  for (int j = 0; j < 8; j++) {
    bands[j] = int(bands[j] * (128.0 / 100000));
    bands[j] = constrain(bands[j], 0, 128);
  }
}

void move_Inputindex() {
  // 初始填充阶段
  if (Inputindex < SAMPLE_NUMS) {
    for (int b = 0; b < 8; b++) {
      Inputbands[Inputindex][b] = (int)bands[b];
    }
    Inputindex++;
    if (Inputindex == SAMPLE_NUMS) sign = 1;
  } 
  // 持续滚动阶段
  else {
    steps++;
    if (steps > MAX_STEP) { // 达到步长，平移旧数据
      for (int mo = 0; mo < SAMPLE_NUMS - MAX_STEP; mo++) {
        for (int b = 0; b < 8; b++) {
          Inputbands[mo][b] = Inputbands[mo + MAX_STEP][b];
        }
      }   
      steps = 1; // 重置步长计数
    }
    
    // 将最新一帧放入窗口末尾对应位置
    for (int b = 0; b < 8; b++) {
      Inputbands[SAMPLE_NUMS - MAX_STEP + (steps - 1)][b] = (int)bands[b];
    }

    if (steps == MAX_STEP) sign = 1; // 填满一个步长，触发识别
  }
}

void detection() {
  for (int i = 0; i < SAMPLE_NUMS; i++) {
    for (int j = 0; j < 8; j++) {
      tflInputTensor->data.f[i * 8 + j] = Inputbands[i][j] / 128.0;
    }
  }
  if (tflInterpreter->Invoke() != kTfLiteOk) {
    Serial.println("Invoke failed!");
  }
}

void application() {
  // --- 1. 获取得分（保留逻辑计算，但不打印） ---
  float silence_score = tflOutputTensor->data.f[0];
  float zhuo_score    = tflOutputTensor->data.f[1];
  float noise_score   = tflOutputTensor->data.f[2];

  unsigned long currentMillis = millis(); 

  // --- 2. 唤醒逻辑判断 ---
  if (zhuo_score > 0.6 && zhuo_score > noise_score && zhuo_score > silence_score) {
      
      // 只有在之前不是唤醒状态时，才打印提示（防止你之前看到的五六次连发刷屏）
      if (!isAwake) {
          Serial.println(">>> [EVENT] 检测到唤醒词: Zhuo_Catty!");
          Serial.println(">>> [ACTION] LED已开启，进入5秒等待期...");
      }

      myLed.showRed(50); 
      isAwake = true;    
      wakeTimer = currentMillis; // 刷新计时器
  } 
  
  // --- 3. 状态保持与超时逻辑 ---
  if (isAwake) {
      if (currentMillis - wakeTimer >= WAKE_DURATION) {
          isAwake = false;   
          myLed.turnOff();   
          Serial.println(">>> [SYSTEM] 5秒已到，自动关灯。");
      }
  } else {
      // 确保非唤醒状态下灯是灭的
      myLed.turnOff();
  }
}



// --- I2S 初始化 ---
void i2s_init() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = true
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}
