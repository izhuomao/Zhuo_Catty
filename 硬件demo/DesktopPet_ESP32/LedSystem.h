#ifndef LED_SYSTEM_H
#define LED_SYSTEM_H

#include "Config.h"

class LedSystem {
public:
    void init() {
        pinMode(PIN_RGB_R, OUTPUT);
        pinMode(PIN_RGB_G, OUTPUT);
        pinMode(PIN_RGB_B, OUTPUT);
        turnOff(); // 初始熄灭
    }

    // 熄灭灯光
    void turnOff() {
        digitalWrite(PIN_RGB_R, HIGH);
        digitalWrite(PIN_RGB_G, HIGH);
        digitalWrite(PIN_RGB_B, HIGH);
    }

    // 红色：唤醒/监听中
    void showRed() {
        digitalWrite(PIN_RGB_R, LOW);
        digitalWrite(PIN_RGB_G, HIGH);
        digitalWrite(PIN_RGB_B, HIGH);
    }

    // 蓝色：思考中
    void showBlue() {
        digitalWrite(PIN_RGB_R, HIGH);
        digitalWrite(PIN_RGB_G, HIGH);
        digitalWrite(PIN_RGB_B, LOW);
    }

    // 绿色：说话中/成功
    void showGreen() {
        digitalWrite(PIN_RGB_R, HIGH);
        digitalWrite(PIN_RGB_G, LOW);
        digitalWrite(PIN_RGB_B, HIGH);
    }

    // 自定义颜色
    void setColor(bool r_on, bool g_on, bool b_on) {
        digitalWrite(PIN_RGB_R, r_on ? LOW : HIGH);
        digitalWrite(PIN_RGB_G, g_on ? LOW : HIGH);
        digitalWrite(PIN_RGB_B, b_on ? LOW : HIGH);
    }
};

#endif
