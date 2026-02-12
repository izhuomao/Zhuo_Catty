#ifndef RGB_LED_H
#define RGB_LED_H

#include <Arduino.h>

class RgbLed {
  private:
    // 硬件引脚
    int _pinR, _pinG, _pinB;
    // ESP32 PWM 通道
    int _chR, _chG, _chB;
    
    // 内部函数：设置底层 PWM
    void _setRaw(int r, int g, int b);

  public:
    // 构造函数：告诉它引脚在哪里
    RgbLed(int pinR, int pinG, int pinB);

    // 初始化：在 setup() 里调用
    void begin();

    // 基础功能
    void showRed();
    void showGreen();
    void showBlue();
    void turnOff();

    // 高级功能：如果你以后想调节亮度 (0-255)
    // 比如 showRed(100) 表示暗一点的红
    void showRed(int brightness);
    void showGreen(int brightness);
    void showBlue(int brightness);
};

#endif
