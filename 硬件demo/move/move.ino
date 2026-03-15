#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// --- 引脚定义 ---
const int RF = 0; const int LF = 1; const int RB = 2; const int LB = 3;
#define SERVOMIN  120 
#define SERVOMAX  550 

// ⭐️ 你的专属补偿数据 ⭐️
int servoOffsets[4] = {-5, 5, 5, -5};
int currentAngles[4] = {90, 90, 90, 90}; 

int stepSpeed = 3;         // 控制舵机平滑度 (数值越小动作越快)

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  stand();
  delay(2000);
}

void loop() {
  // --- 依次展示原作者的所有“隐藏”动作 ---
  
  Serial.println("1. 摇摆动作 (Swing)");
  swing(2);
  stand(); delay(4000);

  Serial.println("2. 伸懒腰 (Stretch)");
  stretch_full();
  stand(); delay(4000);

  Serial.println("3. 打招呼 (Hello)");
  hello_wave();
  stand(); delay(4000);

  Serial.println("4. 快速跳跃 (Jump Forward)");
  jump_forward();
  stand(); delay(4000);
}

// --- 动作实现函数 ---

// 1. 摇摆：重心整体前后晃动
void swing(int times) {
  for (int t = 0; t < times; t++) {
    // 向前晃
    moveAllServosSmoothly(140, 140, 140, 140);
    // 向后晃
    moveAllServosSmoothly(40, 40, 40, 40);
  }
}

// 2. 伸懒腰：前压+后撅
void stretch_full() {
  // 前压
  moveAllServosSmoothly(20, 20, 90, 90);
  delay(500);
  moveAllServosSmoothly(90, 90, 90, 90);
  delay(300);
  // 后撅 (撅屁股)
  moveAllServosSmoothly(90, 90, 160, 160);
  delay(500);
}

// 3. 打招呼：坐下并挥动手爪
void hello_wave() {
  // 坐下姿态
  moveAllServosSmoothly(90, 90, 20, 20); 
  delay(200);
  // 右前腿挥舞 3 次
  for(int i=0; i<3; i++) {
    moveAllServosSmoothly(150, 90, 20, 20); // 抬高
    delay(150);
    moveAllServosSmoothly(60, 90, 20, 20);  // 压低
    delay(150);
  }
}

// 4. 跳跃：模拟爆发力 (由于SG90速度限制，这里调小stepSpeed)
void jump_forward() {
  // 临时加快速度
  int oldSpeed = stepSpeed;
  stepSpeed = 1; 
  
  // 第一组爆发
  moveAllServosSmoothly(150, 90, 90, 40);
  // 第二组紧跟
  moveAllServosSmoothly(150, 150, 40, 40);
  
  stepSpeed = oldSpeed; // 恢复速度
}

// --- 底层函数 (保持不变) ---
void stand() { moveAllServosSmoothly(90, 90, 90, 90); }

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

void setServoAngle(uint8_t channel, int angle) {
  int realAngle = angle + servoOffsets[channel];
  if (realAngle < 0) realAngle = 0;
  if (realAngle > 180) realAngle = 180;
  pwm.setPWM(channel, 0, map(realAngle, 0, 180, SERVOMIN, SERVOMAX));
}
