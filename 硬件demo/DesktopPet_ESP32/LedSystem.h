#ifndef LED_SYSTEM_H
#define LED_SYSTEM_H

#include "Config.h"

class LedSystem {
public:
    void init() {
        // 配置引脚模式
        pinMode(PIN_RGB_R, OUTPUT);
        pinMode(PIN_RGB_G, OUTPUT);
        pinMode(PIN_RGB_B, OUTPUT);
        
        // 默认关闭 (共阳极：HIGH是灭)
        off();
    }

    void off() {
        digitalWrite(PIN_RGB_R, HIGH);
        digitalWrite(PIN_RGB_G, HIGH);
        digitalWrite(PIN_RGB_B, HIGH);
    }

    // 简单的颜色设置函数 (以后可以扩展成呼吸灯)
    // r, g, b: 0=亮, 1=灭 (如果是数字开关)
    // 或者用 analogWrite 做调光
    void setColor(bool r_on, bool g_on, bool b_on) {
        digitalWrite(PIN_RGB_R, r_on ? LOW : HIGH);
        digitalWrite(PIN_RGB_G, g_on ? LOW : HIGH);
        digitalWrite(PIN_RGB_B, b_on ? LOW : HIGH);
    }
};

#endif
