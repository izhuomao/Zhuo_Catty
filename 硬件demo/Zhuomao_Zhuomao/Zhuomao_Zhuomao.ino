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

#define I2S_PORT I2S_NUM_1
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
RgbLed myLed(13, 19, 27);

const int BLOCK_SIZE = 512;
const double SAMPLING_FREQUENCY = 44100;

int16_t samples[BLOCK_SIZE];

// FFT 相关变量
double vReal[BLOCK_SIZE];
double vImag[BLOCK_SIZE];

// 使用 arduinoFFT v2.x 的新写法 (模板类)
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BLOCK_SIZE, SAMPLING_FREQUENCY);

String labels[] = {"125", "250", "500", "1K", "2K", "4K", "8K", "16K"};
int bands[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t amplitude = 150;

const int record_num = 50;
float record_125[record_num];
float record_250[record_num];
float record_500[record_num];
float record_1k[record_num];
float record_2k[record_num];
float record_4k[record_num];
float record_8k[record_num];
float record_16k[record_num];
int data_count = 0;
int n = 100; // 声音的次数

tflite::MicroErrorReporter tflErrorReporter;
tflite::ops::micro::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

const char* VOICES[] = {
  "silence",
  "Zhuo_Catty", // 【已修复】补上了缺失的逗号
  "noise"
};
#define NUM_VOICES (sizeof(VOICES) / sizeof(VOICES[0]))


void i2s_init(){
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = true
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin(){
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

void FFT_Operation();
void constrain128();
void Check_Start();

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
  
  Serial.println("System Initialized. Listening...");
}

void loop() {
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, (void *)samples, BLOCK_SIZE * 2, &bytes_read, portMAX_DELAY);

  if (bytes_read > 0) {
      FFT_Operation();
      constrain128();
      Check_Start();
  }
}

void FFT_Operation(){
  // 填充数据
  for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
    vReal[i] = (double)samples[i] * 256; // 放大信号
    vImag[i] = 0.0; 
  }

  // 使用 arduinoFFT v2.x 的新函数名
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude(); 

  // 重置频段数据
  for (int i = 0; i < 8; i++) {
    bands[i] = 0;
  }

  // 分频段逻辑
  for (int i = 2; i < (BLOCK_SIZE/2); i++){ 
    if (vReal[i] > 2000) { 
      if (i > 2   && i <= 4 )   bands[0] = max(bands[0], (int)(vReal[i]/amplitude)); // 125Hz
      if (i > 4   && i <= 8 )   bands[1] = max(bands[1], (int)(vReal[i]/amplitude)); // 250Hz
      if (i > 8   && i <= 16 )  bands[2] = max(bands[2], (int)(vReal[i]/amplitude)); // 500Hz
      if (i > 16  && i <= 32 )  bands[3] = max(bands[3], (int)(vReal[i]/amplitude)); // 1000Hz
      if (i > 32  && i <= 64 )  bands[4] = max(bands[4], (int)(vReal[i]/amplitude)); // 2000Hz
      if (i > 64  && i <= 128 ) bands[5] = max(bands[5], (int)(vReal[i]/amplitude)); // 4000Hz
      if (i > 128 && i <= 256 ) bands[6] = max(bands[6], (int)(vReal[i]/amplitude)); // 8000Hz
      if (i > 256           )   bands[7] = max(bands[7], (int)(vReal[i]/amplitude)); // 16000Hz
    }
  }
}

void constrain128(){
    for(int j=0;j<8;j++){
      bands[j]=int(bands[j]*(128.0/100000));
      bands[j]=constrain(bands[j],0,128);
    }
}

int bsum=0;
int Threshold_HIGH=40;
int Threshold_LOW=20;
int smooth_count=0;
int record_count=-1;

void Check_Start(){
    bsum=0;
    for(int j=0;j<8;j++){
      bsum=bsum+bands[j];
    }

    if(bsum>=Threshold_HIGH && record_count==-1){
      data_count++;
      record_count=0;
    }

    if(record_count<record_num && record_count!=-1)//收集50条数据
    {
        record_125[record_count]= bands[0];
        record_250[record_count]= bands[1];
        record_500[record_count]= bands[2];
        record_1k[record_count]= bands[3];
        record_2k[record_count]= bands[4];
        record_4k[record_count]= bands[5];
        record_8k[record_count]= bands[6];
        record_16k[record_count]= bands[7];
        record_count++;
    }

    if(record_count==record_num)//收集完成一次声音的50个元组数据
    {
        Serial.println("数据收集完毕，开始 AI 推理...");
        for(int k=0;k<record_count;k++){
          tflInputTensor->data.f[k * 8 + 0] = (record_125[k]) / 128.0;
          tflInputTensor->data.f[k * 8 + 1] = (record_250[k]) / 128.0;
          tflInputTensor->data.f[k * 8 + 2] = (record_500[k]) / 128.0;
          tflInputTensor->data.f[k * 8 + 3] = (record_1k[k] ) / 128.0;
          tflInputTensor->data.f[k * 8 + 4] = (record_2k[k] ) / 128.0;
          tflInputTensor->data.f[k * 8 + 5] = (record_4k[k] ) / 128.0;
          tflInputTensor->data.f[k * 8 + 6] = (record_8k[k] ) / 128.0;
          tflInputTensor->data.f[k * 8 + 7] = (record_16k[k]) / 128.0;
        }
        
        Serial.println("正在调用 Invoke()..."); 
        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk) {
            Serial.println("Invoke failed!");
            while (1);
            return;
        }
        
        Serial.println("推理完成，结果如下："); 
        for (int i = 0; i < NUM_VOICES; i++) {
            Serial.print(VOICES[i]);
            Serial.print(": ");
            Serial.println(tflOutputTensor->data.f[i], 6);
        }
        
        float silence_score = tflOutputTensor->data.f[0];
        float zhuo_score    = tflOutputTensor->data.f[1];
        float noise_score   = tflOutputTensor->data.f[2];
        
        if (zhuo_score > 0.6 && zhuo_score > noise_score && zhuo_score > silence_score) {
            myLed.showRed(50); // 识别到唤醒词，亮红灯
            Serial.println("====== 唤醒成功: Zhuo_Catty! ======");
        } else {
            // 没有听到唤醒词，或者只是背景噪音，关灯
            myLed.turnOff();
        }
        record_count=-1;
    }
}
