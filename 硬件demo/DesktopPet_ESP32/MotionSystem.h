#ifndef MOTION_SYSTEM_H
#define MOTION_SYSTEM_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Config.h"

class MotionSystem {
private:
    Adafruit_PWMServoDriver pwm;

    const int RF = 0;
    const int LF = 1;
    const int RB = 2;
    const int LB = 3;
    const int TAIL = 4;

    const int SERVO_MIN = 120;
    const int SERVO_MAX = 550;

    int servoOffsets[5] = {-5, 5, 5, -5, 0};
    int currentAngles[5] = {90, 90, 90, 90, 90}; 

    int stepSpeed = 3;
    int pauseBetweenSteps = 100;

    const int stm32_advance_matrix[8][4] = {
      {135,  90,  90,  45}, {135, 135,  45,  45},
      { 90, 135,  45,  90}, { 90,  90,  90,  90},
      { 90,  45, 135,  90}, { 45,  45, 135, 135},
      { 45,  90,  90, 135}, { 90,  90,  90,  90} 
    };

    const int stm32_back_matrix[8][4] = {
      { 45,  90,  90, 135}, { 45,  45, 135, 135},
      { 90,  45, 135,  90}, { 90,  90,  90,  90},
      { 90, 135,  45,  90}, {135, 135,  45,  45},
      {135,  90,  90,  45}, { 90,  90,  90,  90} 
    };

    const int stm32_turn_left_matrix[4][4] = {
      {135,  90,  90, 135}, {135,  45,  45, 135},
      { 90,  45,  45,  90}, { 90,  90,  90,  90}
    };

    const int stm32_turn_right_matrix[4][4] = {
      { 90,  45,  45,  90}, {135,  45,  45, 135},
      {135,  90,  90, 135}, { 90,  90,  90,  90}
    };

public:
    void init() {
        Wire.begin();
        pwm = Adafruit_PWMServoDriver();
        pwm.begin();
        pwm.setPWMFreq(50);
        setServoAngle(TAIL, 90);
        stand(); 
    }

    // ▼▼▼ 新增：根据 mood 值调整舵机速度 ▼▼▼
    void setMoodSpeed(int mood) {
        if (mood > 70) {
            stepSpeed = 2;
            pauseBetweenSteps = 70;
        } else if (mood < 30) {
            stepSpeed = 5;
            pauseBetweenSteps = 150;
        } else {
            stepSpeed = 3;
            pauseBetweenSteps = 100;
        }
        Serial.printf("🦿 [运动] 速度更新: mood=%d → stepSpeed=%d, pause=%d\n", mood, stepSpeed, pauseBetweenSteps);
    }

    void execAction(int actionId) {
        switch (actionId) {
            case 0: case 3: stand(); break;
            case 14: getDown(); break;
            case 12: stand(); break;
            case 1:  sleep(); break;
            case 2:  sit(); break;
            case 4:  cute(2); break;
            case 5:  walkForward_STM32(2); stand(); break;
            case 6:  walkBackward_STM32(2); stand(); break;
            case 7:  turnLeft_STM32(2); stand(); break;
            case 8:  turnRight_STM32(2); stand(); break;
            case 9:  swing(2); break;
            case 10: hello_wave(); stand(); break;
            case 11: swingTail(4); break;
            case 13: walkForward_STM32(1); break;
            case 22: swingTail(3); hello_wave(); stand(); break;
            default: stand(); break;
        }
    }

private:
    void setServoAngle(uint8_t channel, int angle) {
        int realAngle = angle + servoOffsets[channel];
        if (realAngle < 0) realAngle = 0;
        if (realAngle > 180) realAngle = 180;
        pwm.setPWM(channel, 0, map(realAngle, 0, 180, SERVO_MIN, SERVO_MAX));
    }

    void moveAllServosSmoothly(int tRF, int tLF, int tRB, int tLB) {
        int targets[4] = {tRF, tLF, tRB, tLB};
        bool allReached = false;
        while (!allReached) {
            allReached = true;
            for (int i = 0; i < 4; i++) {
                if (currentAngles[i] < targets[i]) { currentAngles[i]++; allReached = false; }
                else if (currentAngles[i] > targets[i]) { currentAngles[i]--; allReached = false; }
                setServoAngle(i, currentAngles[i]);
            }
            if (!allReached) delay(stepSpeed);
        }
    }

    void stand() { moveAllServosSmoothly(90, 90, 90, 90); }
    void sit() { moveAllServosSmoothly(90, 90, 160, 20); }
    void getDown() { moveAllServosSmoothly(160, 20, 20, 160); }
    void sleep() { sit(); moveAllServosSmoothly(160, 20, 160, 20); }

    void hello_wave() {
        sit(); delay(300);
        for(int i=0; i<3; i++) {
            moveAllServosSmoothly(90, 90, 160, 20); delay(150);
            moveAllServosSmoothly(180, 90, 160, 20); delay(150);
        }
    }

    void swing(int times) {
        for (int t = 0; t < times; t++) {
            moveAllServosSmoothly(130, 130, 130, 130);
            moveAllServosSmoothly(50, 50, 50, 50);
        }
        stand();
    }

    void cute(int times) {
        for (int t = 0; t < times; t++) {
            moveAllServosSmoothly(130, 50, 130, 50);
            moveAllServosSmoothly(50, 130, 50, 130);
        }
        stand();
    }

    void swingTail(int times) {
        for(int i = currentAngles[TAIL]; i >= 30; i--) {
            setServoAngle(TAIL, i); currentAngles[TAIL] = i; delay(4);
        }
        for (int t = 0; t < times; t++) {
            for (int i = 30; i <= 150; i++) {
                setServoAngle(TAIL, i); currentAngles[TAIL] = i; delay(3);
            }
            for (int i = 150; i >= 30; i--) {
                setServoAngle(TAIL, i); currentAngles[TAIL] = i; delay(3);
            }
        }
        for(int i = 30; i <= 90; i++) {
            setServoAngle(TAIL, i); currentAngles[TAIL] = i; delay(4);
        }
    }

    void walkForward_STM32(int cycles) {
        for (int c = 0; c < cycles; c++)
            for (int s = 0; s < 8; s++) {
                moveAllServosSmoothly(stm32_advance_matrix[s][0], stm32_advance_matrix[s][1], stm32_advance_matrix[s][2], stm32_advance_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
    }

    void walkBackward_STM32(int cycles) {
        for (int c = 0; c < cycles; c++)
            for (int s = 0; s < 8; s++) {
                moveAllServosSmoothly(stm32_back_matrix[s][0], stm32_back_matrix[s][1], stm32_back_matrix[s][2], stm32_back_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
    }

    void turnLeft_STM32(int cycles) {
        for (int c = 0; c < cycles; c++)
            for (int s = 0; s < 4; s++) {
                moveAllServosSmoothly(stm32_turn_left_matrix[s][0], stm32_turn_left_matrix[s][1], stm32_turn_left_matrix[s][2], stm32_turn_left_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
    }

    void turnRight_STM32(int cycles) {
        for (int c = 0; c < cycles; c++)
            for (int s = 0; s < 4; s++) {
                moveAllServosSmoothly(stm32_turn_right_matrix[s][0], stm32_turn_right_matrix[s][1], stm32_turn_right_matrix[s][2], stm32_turn_right_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
    }
};

#endif
