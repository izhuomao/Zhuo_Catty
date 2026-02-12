#ifndef SENSOR_SYSTEM_H
#define SENSOR_SYSTEM_H

#include "Config.h"

class SensorSystem {
private:
    float soundSpeed = 0.034;

public:
    void init() {
        pinMode(PIN_TRIG, OUTPUT);
        pinMode(PIN_ECHO, INPUT);
        digitalWrite(PIN_TRIG, LOW); // 初始拉低
    }

    float getDistance() {
        // 1. 发送触发信号
        digitalWrite(PIN_TRIG, LOW);
        delayMicroseconds(2);
        digitalWrite(PIN_TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(PIN_TRIG, LOW);

        // 2. 接收回响 (设置30ms超时，约对应5米，防止卡死)
        long duration = pulseIn(PIN_ECHO, HIGH, 30000); 

        // 3. 计算距离
        if (duration == 0) {
            return -1.0; // 表示超时或未检测到
        } else {
            return duration * soundSpeed / 2.0;
        }
    }
};

#endif
