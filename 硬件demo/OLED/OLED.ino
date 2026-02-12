#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 1. 引用封装库
#include "RobotFace.h" 

// --- 配置区 ---
#define SSID      "茁猫阿姨洗铁路"
#define PASS      "12345678"
#define API_KEY   "S0ByFwl5YUl7YNLY7"  
#define CITY      "hangzhou"
// -------------

// 硬件引脚
Adafruit_SSD1306 display(128, 64, 23, 18, 16, 4, 5);
// 创建对象
RobotFace face(&display, 128, 64);

void setup() {
  Serial.begin(115200);

  // ★ 一行代码搞定所有初始化 (WiFi, 屏幕, 天气, 时间) ★
  face.begin(SSID, PASS, API_KEY, CITY);
  
  Serial.println("System Ready! Input 0-12 to control.");
}

void loop() {
  // 1. 串口控制逻辑
  if (Serial.available() > 0) {
    int cmd = Serial.parseInt();
    while(Serial.available()) Serial.read(); // 清缓存
    
    if(cmd >= 0 && cmd <= 13) {
      // ★ 一行代码执行动作 ★
      face.execAction(cmd);
    }
  }

  // 2. 维持后台服务 (时钟刷新、天气自动更新)
  // ★ 必须放在 loop 里一直跑 ★
  face.keepAlive();
}
