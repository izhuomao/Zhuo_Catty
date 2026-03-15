#ifndef MOTION_SYSTEM_H
#define MOTION_SYSTEM_H

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "Config.h" // 确保包含配置，以防未来用到全局参数

class MotionSystem {
private:
    Adafruit_PWMServoDriver pwm;

    // === 1. 引脚定义 ===
    const int RF = 0;   // 右前 (Right Front)
    const int LF = 1;   // 左前 (Left Front)
    const int RB = 2;   // 右后 (Right Behind)
    const int LB = 3;   // 左后 (Left Behind)
    const int TAIL = 4; // 尾巴

    // === 2. 脉宽与校准参数 ===
    const int SERVO_MIN = 120;
    const int SERVO_MAX = 550;

    // 专属补偿数据 (已校准)
    int servoOffsets[5] = {-5, 5, 5, -5, 0};
    int currentAngles[5] = {90, 90, 90, 90, 90}; 

    // === 3. 控制参数 ===
    int stepSpeed = 3;           // 数值越小动作越快
    int pauseBetweenSteps = 100; // 矩阵每步之间的停顿

    // === 4. 动作矩阵 ===
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
    // 初始化舵机驱动
    void init() {
        Wire.begin(); // 如果 Config.h 里定义了 SDA SCL，可以使用 Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
        pwm = Adafruit_PWMServoDriver();
        pwm.begin();
        pwm.setPWMFreq(50); // SG90标准频率

        setServoAngle(TAIL, 90);
        stand(); 
    }

    // ★★★ 核心：总控接口，连接大脑和四肢 ★★★
    void execAction(int actionId) {
        switch (actionId) {
            case 0:  // 待机模式
            case 3:  // 立正
                stand();
                break;
            case 14: //趴下
                getDown();
                break;
            case 12: // 停止
                stand(); 
                break;
            case 1:  // 睡觉
                sleep(); 
                break;
            case 2:  // 坐下
                sit(); 
                break;
            case 4:  // 撒娇
                cute(2); 
                break;
            case 5:  // 前进
                walkForward_STM32(2); 
                stand();
                break;
            case 6:  // 后退
                walkBackward_STM32(2); 
                stand();
                break;
            case 7:  // 左转
                turnLeft_STM32(2); 
                stand();
                break;
            case 8:  // 右转
                turnRight_STM32(2); 
                stand();
                break;
            case 9:  // 摇摆(开心)
                swing(2); 
                break;
            case 10: // 打招呼
                hello_wave(); 
                stand();
                break;
            case 11: // 摇尾巴
                swingTail(4); 
                break;
            case 13: // 自由模式(走一步)
                // 自由漫步时只走1个周期，方便主循环检查超声波
                walkForward_STM32(1); 
                break;
            case 22: // ▼▼▼ 新增：备忘录提醒动作（摇尾巴+摆动吸引注意） ▼▼▼
                swingTail(3);
                hello_wave();
                stand();
                break;
            default:
                stand();
                break;
        }
    }

private:
    // ==========================================
    // 底层驱动与平滑算法
    // ==========================================
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

    // ==========================================
    // 动作姿态库
    // ==========================================
    void stand() { moveAllServosSmoothly(90, 90, 90, 90); }
    void sit() { moveAllServosSmoothly(90, 90, 160, 20); }
    void getDown() { moveAllServosSmoothly(160, 20, 20, 160); }
    void sleep() { sit(); moveAllServosSmoothly(160, 20, 160, 20); }

    void hello_wave() {
        sit(); delay(300);
        for(int i=0; i<3; i++) {
            moveAllServosSmoothly(90, 90, 160, 20); // 抬高
            delay(150);
            moveAllServosSmoothly(180, 90, 160, 20); // 压低
            delay(150);
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

    // ==========================================
    // 步态执行函数
    // ==========================================
    void walkForward_STM32(int cycles) {
        for (int c = 0; c < cycles; c++) {
            for (int s = 0; s < 8; s++) {
                moveAllServosSmoothly(stm32_advance_matrix[s][0], stm32_advance_matrix[s][1], stm32_advance_matrix[s][2], stm32_advance_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
        }
    }

    void walkBackward_STM32(int cycles) {
        for (int c = 0; c < cycles; c++) {
            for (int s = 0; s < 8; s++) {
                moveAllServosSmoothly(stm32_back_matrix[s][0], stm32_back_matrix[s][1], stm32_back_matrix[s][2], stm32_back_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
        }
    }

    void turnLeft_STM32(int cycles) {
        for (int c = 0; c < cycles; c++) {
            for (int s = 0; s < 4; s++) {
                moveAllServosSmoothly(stm32_turn_left_matrix[s][0], stm32_turn_left_matrix[s][1], stm32_turn_left_matrix[s][2], stm32_turn_left_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
        }
    }

    void turnRight_STM32(int cycles) {
        for (int c = 0; c < cycles; c++) {
            for (int s = 0; s < 4; s++) {
                moveAllServosSmoothly(stm32_turn_right_matrix[s][0], stm32_turn_right_matrix[s][1], stm32_turn_right_matrix[s][2], stm32_turn_right_matrix[s][3]);
                delay(pauseBetweenSteps);
            }
        }
    }
};

#endif
