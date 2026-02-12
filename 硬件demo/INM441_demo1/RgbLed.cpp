#include "RgbLed.h"

// 构造函数
RgbLed::RgbLed(int pinR, int pinG, int pinB) {
  _pinR = pinR;
  _pinG = pinG;
  _pinB = pinB;
  // 分配 PWM 通道 (ESP32 共有16个通道，这里占用前3个)
  _chR = 0;
  _chG = 1;
  _chB = 2;
}

// 初始化
void RgbLed::begin() {
  // 设置 PWM 参数
  ledcSetup(_chR, 5000, 8); // 5000Hz, 8位分辨率
  ledcSetup(_chG, 5000, 8);
  ledcSetup(_chB, 5000, 8);

  // 绑定引脚
  ledcAttachPin(_pinR, _chR);
  ledcAttachPin(_pinG, _chG);
  ledcAttachPin(_pinB, _chB);
  
  // 默认关闭
  turnOff();
}

// --- 核心私有函数：处理共阳极逻辑 ---
void RgbLed::_setRaw(int r, int g, int b) {
  // 限制范围在 0-255
  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);

  // 共阳极关键逻辑：反转数值
  // 想要亮(255) -> 输出低电平(0)
  ledcWrite(_chR, 255 - r);
  ledcWrite(_chG, 255 - g);
  ledcWrite(_chB, 255 - b);
}

// --- 用户使用的简单函数 ---

void RgbLed::showRed()   { _setRaw(255, 0, 0); }
void RgbLed::showGreen() { _setRaw(0, 255, 0); }
void RgbLed::showBlue()  { _setRaw(0, 0, 255); }
void RgbLed::turnOff()   { _setRaw(0, 0, 0); }

// 带亮度的重载函数
void RgbLed::showRed(int brightness)   { _setRaw(brightness, 0, 0); }
void RgbLed::showGreen(int brightness) { _setRaw(0, brightness, 0); }
void RgbLed::showBlue(int brightness)  { _setRaw(0, 0, brightness); }
