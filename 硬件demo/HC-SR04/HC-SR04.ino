/*
 * ESP32 超声波模块测试 + RGB灯复位程序
 * 
 * 硬件接线:
 * Trig -> GPIO 2
 * Echo -> GPIO 34
 * RGB R -> GPIO 13
 * RGB G -> GPIO 19
 * RGB B -> GPIO 27
 */

// --- 引脚定义 ---
const int trigPin = 2;
const int echoPin = 34;

const int pinR = 13;
const int pinG = 19;
const int pinB = 27;

// --- 参数配置 ---
#define SOUND_SPEED 0.034
long duration;
float distanceCm;

void setup() {
  Serial.begin(115200);
  
  // 1. 配置超声波引脚
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // 2. ⚡️ 强制配置 RGB 引脚，防止微亮 ⚡️
  pinMode(pinR, OUTPUT);
  pinMode(pinG, OUTPUT);
  pinMode(pinB, OUTPUT);
  
  // 3. 关闭 RGB 灯
  // 如果你的灯是【共阳极】(公共端接VCC)，HIGH是灭，LOW是亮
  // 如果你的灯是【共阴极】(公共端接GND)，LOW是灭，HIGH是亮
  // 建议先试 HIGH，如果灯全亮瞎眼了，就改成 LOW
  digitalWrite(pinR, HIGH); 
  digitalWrite(pinG, HIGH); 
  digitalWrite(pinB, HIGH); 
  
  Serial.println(">>> 初始化完成，RGB已关闭，开始测距 <<<");
}

void loop() {
  // --- 超声波测距逻辑 ---
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  
  if (distanceCm == 0 || distanceCm > 400) {
    Serial.println("超出量程或未连接");
  } else {
    Serial.print("距离: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
  }
  
  delay(1000); 
}
