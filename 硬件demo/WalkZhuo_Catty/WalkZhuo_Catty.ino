#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// === 1. 引脚定义 ===
const int RF = 0; // 右前 (Right Front)
const int LF = 1; // 左前 (Left Front)
const int RB = 2; // 右后 (Right Behind)
const int LB = 3; // 左后 (Left Behind)
const int TAIL = 4;//尾巴

// === 2. 脉宽与校准参数 ===
#define SERVOMIN  120 
#define SERVOMAX  550 

// ⭐️ 你的专属补偿数据 (已校准) ⭐️
int servoOffsets[5] = {-5, 5, 5, -5, 0};
int currentAngles[5] = {90, 90, 90, 90, 90}; 

// === 3. 控制参数 ===
int stepSpeed = 3;           // 数值越小动作越快
int pauseBetweenSteps = 100; // 矩阵每步之间的停顿

// ==========================================
// 4. 动作矩阵区域 (STM32 风格逻辑转换)
// ==========================================

// 8步前进矩阵
int stm32_advance_matrix[][4] = {
  {135,  90,  90,  45}, {135, 135,  45,  45},
  { 90, 135,  45,  90}, { 90,  90,  90,  90},
  { 90,  45, 135,  90}, { 45,  45, 135, 135},
  { 45,  90,  90, 135}, { 90,  90,  90,  90} 
};

// 8步后退矩阵
int stm32_back_matrix[][4] = {
  { 45,  90,  90, 135}, { 45,  45, 135, 135},
  { 90,  45, 135,  90}, { 90,  90,  90,  90},
  { 90, 135,  45,  90}, {135, 135,  45,  45},
  {135,  90,  90,  45}, { 90,  90,  90,  90} 
};

// 4步左转矩阵
int stm32_turn_left_matrix[][4] = {
  {135,  90,  90, 135}, {135,  45,  45, 135},
  { 90,  45,  45,  90}, { 90,  90,  90,  90}
};

// 4步右转矩阵
int stm32_turn_right_matrix[][4] = {
  { 90,  45,  45,  90}, {135,  45,  45, 135},
  {135,  90,  90, 135}, { 90,  90,  90,  90}
};

void setup() {
  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(50);
  
  Serial.println("Robot Ready!");
  setServoAngle(TAIL, 90);
  stand(); 
  delay(2000);
}

void loop() {
  // --- 动作串烧演示 ---

  Serial.println("摇尾巴...");
  swingTail(4); // 摇摆 4 个来回
  delay(1000);

  // Serial.println("撒娇...");
  // cute(2);
  // stand();delay(5000);

  // Serial.println("睡觉...");
  // sleep();
  // delay(5000);stand();

  // Serial.println("打招呼...");
  // hello_wave();
  // stand();delay(1000);

  // Serial.println("前进 2 个周期...");
  // walkForward_STM32(2);
  // stand(); delay(500);

  // Serial.println("左转 2 次...");
  // turnLeft_STM32(2);
  // stand(); delay(500);

  // Serial.println("坐下...");
  // sit();
  // delay(2000);stand();

  // Serial.println("趴下...");
  // getDown();
  // delay(2000);stand();

  // Serial.println("起立并摇摆...");
  // stand();
  // swing(2);
  // stand();
  
  // Serial.println("演示完毕，休息 5 秒");
  // delay(5000);
}

// ==========================================
// 5. 动作执行函数库
// ==========================================

// --- 基础姿态 ---
void stand() {
  moveAllServosSmoothly(90, 90, 90, 90);
}

void sit() { // 坐下：前腿直立，后腿收起
  moveAllServosSmoothly(90, 90, 160, 20); 
}

void getDown() { // 趴下：四腿收起
  moveAllServosSmoothly(160, 20, 20, 160);
}

// --- 互动动作 ---
void hello_wave() { // 打招呼
  sit(); delay(300);
  for(int i=0; i<3; i++) {
    moveAllServosSmoothly(90, 90, 160, 20); // 抬高右前腿
    delay(150);
    moveAllServosSmoothly(180, 90, 160, 20);  // 压低右前腿
    delay(150);
  }
}

void sleep() { // 睡觉
  sit();
  moveAllServosSmoothly(160, 20, 160, 20);
}

void swing(int times) { // 摇摆
  for (int t = 0; t < times; t++) {
    moveAllServosSmoothly(130, 130, 130, 130);
    moveAllServosSmoothly(50, 50, 50, 50);
  }
  stand();
}

void cute(int times) { // 撒娇
  for (int t = 0; t < times; t++) {
    moveAllServosSmoothly(130, 50, 130, 50);
    moveAllServosSmoothly(50, 130, 50, 130);
  }
  stand();
}

void swingTail(int times) { // 摇尾巴
  // 1. 先让尾巴平滑移动到准备位置 (30度)
  for(int i = currentAngles[TAIL]; i >= 30; i--) {
    setServoAngle(TAIL, i);
    currentAngles[TAIL] = i; 
    delay(4); // 移动速度
  }

  // 2. 开始欢快地摇摆
  for (int t = 0; t < times; t++) {
    // 从左扫到右 (30 -> 150)
    for (int i = 30; i <= 150; i++) {
      setServoAngle(TAIL, i);
      currentAngles[TAIL] = i; 
      delay(3); // 🌟 改小这个延迟，尾巴摇得越快
    }
    // 从右扫到左 (150 -> 30)
    for (int i = 150; i >= 30; i--) {
      setServoAngle(TAIL, i);
      currentAngles[TAIL] = i;
      delay(3);
    }
  }

  // 3. 摇完后，平滑回正到 90度 待机
  for(int i = 30; i <= 90; i++) {
    setServoAngle(TAIL, i);
    currentAngles[TAIL] = i;
    delay(4);
  }
}

// --- 步态移动 (STM32逻辑) ---
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

// ==========================================
// 6. 底层驱动函数 (已集成校准与平滑逻辑)
// ==========================================

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
