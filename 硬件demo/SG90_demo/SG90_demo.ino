#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// 创建驱动对象
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// --- 配置区域 ---
// 定义舵机连接的 PCA9685 通道号 (0-15)
const int SERVO_1_CHANNEL = 0; // 第一个舵机插 0 口
const int SERVO_2_CHANNEL = 1; // 第二个舵机插 1 口
const int SERVO_3_CHANNEL = 2; // 第二个舵机插 2 口
const int SERVO_4_CHANNEL = 3; // 第二个舵机插 3 口
const int SERVO_5_CHANNEL = 4; // 第二个舵机插 3 口



// 定义脉宽范围 (根据 SG90 调整)
// 如果舵机此时还在滋滋响，微调这两个数
#define SERVOMIN  120 // 0度
#define SERVOMAX  550 // 180度

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);

  // --- 关键修改：逐个唤醒舵机 ---
  
  // 1. 唤醒第 1 个
  Serial.println("Waking up Servo 0...");
  setServoAngle(0, 90); // 设定一个中间值
  delay(500); // 【重要】给它 0.5秒 时间归位，等电流降下去

  // 2. 唤醒第 2 个
  Serial.println("Waking up Servo 1...");
  setServoAngle(1, 90);
  delay(500);

  // 3. 唤醒第 3 个
  Serial.println("Waking up Servo 2...");
  setServoAngle(2, 90);
  delay(500);

  // 4. 唤醒第 4 个
  Serial.println("Waking up Servo 3...");
  setServoAngle(3, 90);
  delay(500);

  Serial.println("Waking up Servo 3...");
  setServoAngle(4, 90);
  delay(500);

  Serial.println("All servos ready!");
}


void loop() {
  // ------------------------------------------------
  // 场景 1：完全同步 (Simultaneous)
  // 两个舵机做一模一样的动作
  // ------------------------------------------------
  Serial.println("动作 1：双舵机同步归零 (0度)");
  setServoAngle(SERVO_1_CHANNEL, 0);
  setServoAngle(SERVO_2_CHANNEL, 0);
  setServoAngle(SERVO_3_CHANNEL, 0);
  setServoAngle(SERVO_4_CHANNEL, 0);
  setServoAngle(SERVO_5_CHANNEL, 0);

  delay(1000); // 等待它们转到位

  Serial.println("动作 2：双舵机同步转中 (90度)");
  setServoAngle(SERVO_1_CHANNEL, 180);
  setServoAngle(SERVO_2_CHANNEL, 180);
  setServoAngle(SERVO_3_CHANNEL, 180);
  setServoAngle(SERVO_4_CHANNEL, 180);
  setServoAngle(SERVO_5_CHANNEL, 180);

  delay(1000);

  // ------------------------------------------------
  // 场景 2：反向动作 (Opposite)
  // 模拟机械爪开合，或者走路的样子
  // ------------------------------------------------
  // Serial.println("动作 3：反向动作 (0 vs 180)");
  // setServoAngle(SERVO_1_CHANNEL, 0);   // 舵机1 去 0度
  // setServoAngle(SERVO_2_CHANNEL, 180); // 舵机2 去 180度
  // delay(1000);

  // Serial.println("动作 4：反向动作交换 (180 vs 0)");
  // setServoAngle(SERVO_1_CHANNEL, 180);
  // setServoAngle(SERVO_2_CHANNEL, 0);
  // delay(1000);
}

// --- 封装好的辅助函数 (直接复制即可) ---
// 作用：把角度 (0-180) 转换成 PWM 脉冲并发送给 PCA9685
void setServoAngle(uint8_t channel, int angle) {
  // 1. 限制角度在安全范围内
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;

  // 2. 将角度 (0-180) 映射为 脉冲数值 (SERVOMIN - SERVOMAX)
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);

  // 3. 发送指令
  // 0 表示从周期的开头就开始供电 (ON)
  // pulse 表示在什么时候切断供电 (OFF)
  pwm.setPWM(channel, 0, pulse);
}
