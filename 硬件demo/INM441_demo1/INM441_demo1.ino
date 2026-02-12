#include "RgbLed.h"
#include <driver/i2s.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h> 
#include <arduinoFFT.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h>
#include <tensorflow/lite/experimental/micro/micro_error_reporter.h>
#include <tensorflow/lite/experimental/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>
#include "model.h"
#define I2S_PORT I2S_NUM_1
#define I2S_WS 13
#define I2S_SD 32
#define I2S_SCK 14
RgbLed myLed(25, 26, 27);

const int BLOCK_SIZE = 512;
const double SAMPLING_FREQUENCY = 44100;

int16_t samples[BLOCK_SIZE];

// FFT 相关变量
double vReal[BLOCK_SIZE];
double vImag[BLOCK_SIZE];

// 【修正1】使用 arduinoFFT v2.x 的新写法 (模板类)
// 在初始化时就传入数组指针和采样率
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, BLOCK_SIZE, SAMPLING_FREQUENCY);

String labels[] = {"125", "250", "500", "1K", "2K", "4K", "8K", "16K"};
int bands[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t amplitude = 150;

const int record_num=30;
float record_125[record_num];
float record_250[record_num];
float record_500[record_num];
float record_1k[record_num];
float record_2k[record_num];
float record_4k[record_num];
float record_8k[record_num];
float record_16k[record_num];
int data_count=0;
int n=100; //声音的次数
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
  "open",
  "close",
  "noise"
};
#define NUM_VOICES (sizeof(VOICES) / sizeof(VOICES[0]))

const char * ssid = "茁猫阿姨洗铁路";
const char * password = "12345678";

// 网页代码 (保持不变)
char webpage[] PROGMEM = R"=====(
<html>
<head>
  <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>
</head>
<body onload="javascript:init()">
<div>
  <canvas id="chart" width="600" height="400"></canvas>
</div>
<script>
  var webSocket, dataPlot;
  var maxDataPoints = 20;
  const maxValue = 128;
  const maxLow = maxValue * 0.5;
  const maxMedium = maxValue * 0.2;
  const maxHigh = maxValue * 0.3;
  function init() {
    webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
    dataPlot = new Chart(document.getElementById("chart"), {
      type: 'bar',
      data: {
        labels: [],
        datasets: [{ data: [], label: "L", backgroundColor: "#D6E9C6" },
                   { data: [], label: "M", backgroundColor: "#FAEBCC" },
                   { data: [], label: "H", backgroundColor: "#EBCCD1" }]
      }, 
      options: {
          responsive: false, animation: false,
          scales: { xAxes: [{ stacked: true }], yAxes: [{ display: true, stacked: true, ticks: { beginAtZero: true, steps: 1000, stepValue: 500, max: maxValue } }] }
       }
    });
    webSocket.onmessage = function(event) {
      var data = JSON.parse(event.data);
      dataPlot.data.labels = [];
      dataPlot.data.datasets[0].data = [];
      dataPlot.data.datasets[1].data = [];
      dataPlot.data.datasets[2].data = [];
      data.forEach(function(element) {
        dataPlot.data.labels.push(element.bin);
        var lowValue = Math.min(maxLow, element.value);
        dataPlot.data.datasets[0].data.push(lowValue);
        var mediumValue = Math.min(Math.max(0, element.value - lowValue), maxMedium);
        dataPlot.data.datasets[1].data.push(mediumValue);
        var highValue = Math.max(0, element.value - lowValue - mediumValue);
        dataPlot.data.datasets[2].data.push(highValue);
      });
      dataPlot.update();
    }
  }
</script>
</body>
</html>
)=====";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

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
void Send_Msg();

void setup() {
  Serial.begin(115200);
  i2s_init();
  i2s_setpin();
  
  WiFi.begin(ssid, password);
  while(WiFi.status()!=WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nIP Address: " + WiFi.localIP().toString());

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

  server.on("/",[](){ server.send_P(200, "text/html", webpage); });
  server.begin();
  webSocket.begin();
}

void loop() {
  webSocket.loop();
  server.handleClient();

  size_t bytes_read = 0;
  i2s_read(I2S_PORT, (void *)samples, BLOCK_SIZE * 2, &bytes_read, portMAX_DELAY);

  if (bytes_read > 0) {
      FFT_Operation();
      constrain128();
      Check_Start();
      Send_Msg();
  }
}

void FFT_Operation(){
  // 填充数据
  for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
    // 加上简单的窗口平滑或直接转换，这里保持原样
    vReal[i] = (double)samples[i] * 256; // 这里稍微放大一点信号，原代码是左移8位
    vImag[i] = 0.0; 
  }

  // 【修正2】使用 v2.x 的新函数名 (首字母小写，无参数，因为初始化时已绑定)
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude(); // 结果会直接存回 vReal

  // 重置频段数据
  for (int i = 0; i < 8; i++) {
    bands[i] = 0;
  }

  // 分频段逻辑保持不变
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


void Send_Msg() {
  // 【优化】使用 ArduinoJson 生成 JSON，防止内存错误
  JsonDocument doc; // 自动管理内存 (ArduinoJson v7)
  
  for (int i = 0; i < 8; i++) {
    JsonObject obj = doc.add<JsonObject>();
    obj["bin"] = labels[i];
    obj["value"] = bands[i];
  }

  String jsonString;
  serializeJson(doc, jsonString);
  webSocket.broadcastTXT(jsonString);
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

    if(bsum>=Threshold_HIGH&&record_count==-1){
      data_count++;
      // if(data_count==n){
      //   myLed.showRed(50);
      // }else if(data_count>n){
      //   myLed.showBlue(50);
      // }else{
      //   myLed.showGreen(50);
      // }

      record_count=0;
    }

    if(record_count<record_num&&record_count!=-1)//收集30条数据
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

    if(record_count==record_num)//收集完成一次声音的30个元组数据
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
        float  silence=tflOutputTensor->data.f[0];
        float  open_score=tflOutputTensor->data.f[1];
        float  close_score=tflOutputTensor->data.f[2];
        float noise_score   = tflOutputTensor->data.f[3];
        if (open_score > 0.6 && open_score > noise_score && open_score > close_score) {
            myLed.showRed(50);
            Serial.println("Action: OPEN");
        }
        // 同理判断 Close
        else if (close_score > 0.6 && close_score > noise_score && close_score > open_score) {
            myLed.turnOff();
            Serial.println("Action: CLOSE");
        }
        // Serial.println("检测完成");
        // Serial.print("收集到的元组数为：");
        // Serial.println(record_count);

        record_count=-1;
    }


}
