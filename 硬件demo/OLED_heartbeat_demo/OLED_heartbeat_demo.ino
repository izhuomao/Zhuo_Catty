#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 1. 【关键】引入刚才创建的头文件
#include "emotions.h"

// --- 屏幕配置 ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// SPI 引脚
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_DC     2
#define OLED_CS     5
#define OLED_RESET  4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// 图片尺寸 (假设你resize到了64x64)
#define GIF_WIDTH  128
#define GIF_HEIGHT 100

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 failed"));
    for(;;);
  }
}

// void loop() {
//   // 1. 播放“愤怒”： 循环 2 次
//   // 假设愤怒动画有 24 帧
//   playAnimation(angry_allArray, 25, 3); 
  
//   // 2. 播放“平静”： 循环 5 次
//   // 假设平静动画有 24 帧
//   playAnimation(calm_allArray, 9, 3);
  
//   // 3. 播放“兴奋”： 循环 10 次 (比如兴奋的时间久一点)
//   playAnimation(excited_allArray, 24, 3);
  
//   // 4. 播放“难过”： 只播 1 次
//   playAnimation(sad_allArray, 26, 3);
  
//   // 此时所有动画播完一遍，loop 函数结束，
//   // Arduino 会自动重新运行 loop，于是又从愤怒开始循环 2 次...
// }

// // 参数说明：
// // animation:    动画数组名
// // total_frames: 总帧数
// // repeat_times: 你想循环播放几次 (比如 3 次)
// void playAnimation(const unsigned char* animation[], int total_frames, int repeat_times) {
  
//   int loops_completed = 0; // 计数器：当前已经播了几轮
//   int current_frame = 0;   // 当前播到第几帧

//   // 只要没播够次数，就一直在这个循环里转圈
//   while (loops_completed < repeat_times) {
    
//     // --- 1. 绘图 (跟以前一样) ---
//     display.clearDisplay();
//     // 居中计算 (如果你的图片和屏幕一样大，xy直接写0)
//     int x = (SCREEN_WIDTH - GIF_WIDTH) / 2;
//     int y = (SCREEN_HEIGHT - GIF_HEIGHT) / 2;
    
//     display.drawBitmap(x, y, animation[current_frame], GIF_WIDTH, GIF_HEIGHT, WHITE);
//     display.display();

//     // --- 2. 帧数递增 ---
//     current_frame++;

//     // --- 3. 【关键逻辑修改】检测是否播完一轮 ---
//     if (current_frame >= total_frames) {
//       current_frame = 0;   // 重置回第一帧
//       loops_completed++;   // 记账：我也播完一轮了！
//     }

//     // --- 4. 速度控制 ---
//     // 这里改成你觉得舒服的速度，比如 80 或 100
//     delay(100); 
//   }
// }

void loop() {
  // 睁眼状态：高度 30
  drawCozmoEyes(30); 
  delay(2000); // 保持睁眼 2秒
  
  // 闭眼过程 (眨眼)
  for(int h=30; h>=2; h-=4) {
    drawCozmoEyes(h);
    delay(10); // 速度很快
  }
  
  // 睁眼过程
  for(int h=2; h<=30; h+=4) {
    drawCozmoEyes(h);
    delay(10);
  }
  
  // 再次循环...
}

// 封装一个画眼睛的函数，height 参数决定眼睛睁多大
void drawCozmoEyes(int height) {
  display.clearDisplay();
  
  // 计算垂直居中位置，保证眨眼时是向中间闭合
  int y = (64 - height) / 2;
  
  // 左眼 (固定宽度40，高度动态)
  display.fillRoundRect(10, y, 40, height, 5, WHITE);
  
  // 右眼
  display.fillRoundRect(78, y, 40, height, 5, WHITE);
  
  display.display();
}
