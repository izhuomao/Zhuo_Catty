#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// --- 引脚定义 ---
const int RF = 0; // 右前 (Right Front)
const int LF = 1; // 左前 (Left Front)
const int RB = 2; // 右后 (Right Behind)
const int LB = 3; // 左后 (Left Behind)

// --- 脉宽映射参数 ---
#define SERVOMIN  120 
#define SERVOMAX  550 

// ==========================================
// ⭐️ 核心微调区：舵机角度补偿数组 ⭐️
// 分别对应 {RF右前, LF左前, RB右后, LB左后}
// 正数代表顺时针加角度，负数代表逆时针减角度
// ==========================================
int servoOffsets[4] = {-5, 5, 5, -5}; 


void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50); // SG90标准频率 50Hz

  Serial.println("=== 机器狗 90度 校准模式已启动 ===");
  Serial.println("请观察四条腿是否垂直。");
  Serial.println("如不垂直，请修改 servoOffsets 数组并重新烧录。");
}

void loop() {
  // 不断发送 90 度指令，让舵机处于“锁齿”发力状态
  setServoAngle(RF, 90);
  setServoAngle(LF, 90);
  setServoAngle(RB, 90);
  setServoAngle(LB, 90);
  
  delay(1000); // 每秒刷新一次
}

// ==========================================
// 底层驱动函数 (已加入补偿防越界逻辑)
// ==========================================
void setServoAngle(uint8_t channel, int angle) {
  // 1. 加上对应的偏移补偿量
  int realAngle = angle + servoOffsets[channel];
  
  // 2. 安全限幅：防止补偿后角度超出 0~180 范围，导致扫齿
  if (realAngle < 0) realAngle = 0;
  if (realAngle > 180) realAngle = 180;
  
  // 3. 映射并输出 PWM
  int pulse = map(realAngle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulse);
}

