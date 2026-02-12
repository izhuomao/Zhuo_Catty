#include <WiFi.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>

// ==========================================
//              用户配置区域 (请修改这里)
// ==========================================
const char* ssid = "茁猫阿姨洗铁路";         // 请修改
const char* password = "12345678";     // 请修改
const char* pc_ip = "192.168.57.147";       // 请修改为你电脑的IPv4地址
const int pc_port = 12345;               // 端口号，保持与Python端一致
// ==========================================

// --- 引脚定义 ---
// 麦克风 (INMP441)
#define I2S_MIC_SCK 14
#define I2S_MIC_WS  15
#define I2S_MIC_SD  32

// 扬声器 (MAX98357)
#define I2S_SPK_BCLK 25
#define I2S_SPK_LRC  26
#define I2S_SPK_DIN  33

// --- 全局变量 ---
WiFiUDP udp;
bool isRecording = false;      // 内部状态：是否正在录音传输
bool remoteRecordState = false; // 控制指令状态：由串口 'r'/'s' 控制

// --- I2S 初始化配置 ---
void i2s_install() {
  // 1. 配置麦克风输入 (I2S_NUM_0)
  const i2s_config_t i2s_mic_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100, // 采样率 16kHz
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // INMP441 L/R接地即为左声道
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_driver_install(I2S_NUM_0, &i2s_mic_config, 0, NULL);
  const i2s_pin_config_t mic_pin_config = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };
  i2s_set_pin(I2S_NUM_0, &mic_pin_config);

  // 2. 配置扬声器输出 (I2S_NUM_1)
  const i2s_config_t i2s_spk_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, 
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };
  i2s_driver_install(I2S_NUM_1, &i2s_spk_config, 0, NULL);
  const i2s_pin_config_t spk_pin_config = {
    .bck_io_num = I2S_SPK_BCLK,
    .ws_io_num = I2S_SPK_LRC,
    .data_out_num = I2S_SPK_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_NUM_1, &spk_pin_config);
}

void setup() {
  Serial.begin(115200);
  
  // 连接WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // 启动UDP
  udp.begin(pc_port);
  
  // 启动I2S音频
  i2s_install();

  Serial.println("=========================================");
  Serial.println("系统就绪！请在串口监视器输入指令：");
  Serial.println("输入 'r' + 回车 -> 开始录音 (Record)");
  Serial.println("输入 's' + 回车 -> 停止录音并发送 (Stop)");
  Serial.println("=========================================");
}

void loop() {
  // === 1. 监听串口指令 (核心逻辑) ===
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    // 过滤掉换行符，只处理 r 和 s
    if (cmd == 'r' || cmd == 'R') {
      remoteRecordState = true;
      Serial.println("[指令] 开始录音... (请说话)");
    } 
    else if (cmd == 's' || cmd == 'S') {
      remoteRecordState = false;
      Serial.println("[指令] 停止录音，正在发送结束信号...");
    }
  }

  // === 2. 录音与发送逻辑 ===
  if (remoteRecordState) { 
    // 如果刚开始录音，发送 START 标志
    if (!isRecording) {
      isRecording = true;
      udp.beginPacket(pc_ip, pc_port);
      udp.write((uint8_t*)"START", 5);
      udp.endPacket();
    }

    // 从麦克风读取数据
    char i2s_read_buff[1024];
    size_t bytes_read;
    i2s_read(I2S_NUM_0, (char*)i2s_read_buff, 1024, &bytes_read, portMAX_DELAY);
    
    // 如果读到了声音，发给电脑
    if (bytes_read > 0) {
      udp.beginPacket(pc_ip, pc_port);
      udp.write((uint8_t*)i2s_read_buff, bytes_read);
      udp.endPacket();
    }
  } 
  else { 
    // 如果刚才在录音，现在停止了，发送 END 标志
    if (isRecording) {
      isRecording = false;
      udp.beginPacket(pc_ip, pc_port);
      udp.write((uint8_t*)"END", 3);
      udp.endPacket();
      Serial.println("[状态] 录音结束，等待电脑回复...");
    }
  }

  // === 3. 接收电脑回复并播放 (始终运行) ===
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Serial.println("收到音频数据包..."); // 调试用，太吵可以注释掉
    char i2s_write_buff[1024];
    int len = udp.read(i2s_write_buff, 1024);
    
    if (len > 0) {
      size_t bytes_written;
      // 写入扬声器播放
      i2s_write(I2S_NUM_1, i2s_write_buff, len, &bytes_written, portMAX_DELAY);
    }
  }
}
