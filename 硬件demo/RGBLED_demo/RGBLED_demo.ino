#include "RgbLed.h" // 引入刚才写的头文件

// 实例化一个灯对象：定义引脚 (R, G, B)
// 假设接在 GPIO 25, 26, 27
RgbLed myLed(13, 19, 27);

void setup() {
  Serial.begin(115200);
  
  // 启动灯
  myLed.begin();
  Serial.println("System Ready.");
}

void loop() {
  Serial.println("Red");
  myLed.showRed(50);
  delay(1000);

  Serial.println("Green");
  myLed.showGreen(50);
  delay(1000);

  Serial.println("Blue");
  myLed.showBlue(50);
  delay(1000);

  Serial.println("Off");
  myLed.turnOff();
  delay(1000);

  // 甚至可以尝试调节亮度（如果不想太刺眼）
  // myLed.showRed(50); // 50% 亮度的红色
}
