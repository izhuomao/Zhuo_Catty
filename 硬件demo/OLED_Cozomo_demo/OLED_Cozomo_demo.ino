#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- 屏幕配置 (SPI) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_DC     16
#define OLED_CS     5
#define OLED_RESET  4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// 定义眼睛的标准参数
int eye_width = 40;
int eye_height = 45;
int eye_gap = 20;         // 两眼之间的距离
int eye_radius = 8;       // 圆角大小
int x_center = SCREEN_WIDTH / 2;
int y_center = SCREEN_HEIGHT / 2;

void setup() {
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    for(;;);
  }
}

void loop() {
  // 1. 正常状态：四处张望 + 眨眼
  centerEyes();
  delay(1000);
  blink(1);       // 眨眼1次
  lookLeft();
  delay(800);
  lookRight();
  delay(800);
  centerEyes();
  delay(500);

  // 2. 变身：愤怒模式！
  // 模拟 Cozmo 遇到障碍物时的反应
  animateToAngry(); 
  delay(2000);      // 保持愤怒 2秒
  
  // 3. 变身：委屈模式...
  // 1. 先慢慢变难过 (复用之前的函数)
  animateToSad();
  delay(500); // 保持难过，酝酿一下
  // 2. 掉第一滴泪
  animateToCry();
  // 3. 稍微停顿，吸一下鼻子
  delay(200);
  // 4. 掉第二滴泪
  animateToCry();
  // 5. 哭完了，觉得不好意思，眨眨眼恢复正常
  delay(1000);
  centerEyes();
  blink(2);
  delay(2000);
  
  animateToHappy();
  delay(2000);

  // 4. 恢复正常
  centerEyes();
  delay(1000);

}

// ==========================================
//           基础绘图函数 (核心层)
// ==========================================

// 画一对基本的眼睛 (参数：左眼X, 左眼Y, 宽, 高)
// 这里的 x, y 是左眼的左上角坐标
void drawRawEyes(int x, int y, int w, int h) {
  // 左眼
  display.fillRoundRect(x, y, w, h, eye_radius, WHITE);
  // 右眼 (算出右眼的位置)
  display.fillRoundRect(x + w + eye_gap, y, w, h, eye_radius, WHITE);
}

// ==========================================
//           表情实现逻辑 (应用层)
// ==========================================

// --- 1. 正常居中 ---
void centerEyes() {
  display.clearDisplay();
  // 计算居中坐标
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  
  drawRawEyes(start_x, start_y, eye_width, eye_height);
  display.display();
}

// --- 2. 眨眼动作 ---
void blink(int times) {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;

  for(int t=0; t<times; t++) {
    // 闭眼过程 (高度从 45 -> 2)
    for(int h=eye_height; h>=2; h-=8) {
      display.clearDisplay();
      // 保持垂直居中：高度变小了，Y坐标要相应增加
      int current_y = start_y + (eye_height - h) / 2;
      drawRawEyes(start_x, current_y, eye_width, h);
      display.display();
    }
    
    // 睁眼过程 (高度从 2 -> 45)
    for(int h=2; h<=eye_height; h+=8) {
      display.clearDisplay();
      int current_y = start_y + (eye_height - h) / 2;
      drawRawEyes(start_x, current_y, eye_width, h);
      display.display();
    }
    delay(100); // 两次眨眼间隔
  }
}

// --- 3. 向左看 ---
void lookLeft() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  
  // 简单的位移动画
  for(int offset=0; offset<20; offset+=5) {
    display.clearDisplay();
    drawRawEyes(start_x - offset, start_y, eye_width, eye_height);
    display.display();
  }
}

// --- 4. 向右看 ---
void lookRight() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  
  for(int offset=0; offset<20; offset+=5) {
    display.clearDisplay();
    drawRawEyes(start_x + offset, start_y, eye_width, eye_height);
    display.display();
  }
}

// --- 5. 愤怒模式 (重点技巧：遮罩) ---
void animateToAngry() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;

  // 动画：眼皮慢慢压下来
  for(int drop=0; drop<25; drop+=2) {
    display.clearDisplay();
    drawRawEyes(start_x, start_y, eye_width, eye_height);

    // 【关键】画两个黑色的三角形盖在眼睛上方，形成"倒八字"
    
    // 左眼遮罩 (三角形)
    // 三个顶点：(左上, 右上偏下, 右上)
    display.fillTriangle(
      start_x, start_y - 5,                 // 顶点1: 左上角上方
      start_x + eye_width, start_y + drop,  // 顶点2: 右上角往下压 (形成斜度)
      start_x + eye_width, start_y - 5,     // 顶点3: 右上角上方 (填补空隙)
      BLACK); // 使用黑色！

    // 右眼遮罩 (镜像)
    display.fillTriangle(
      right_eye_x, start_y + drop,          // 顶点1: 左上角往下压
      right_eye_x + eye_width, start_y - 5, // 顶点2: 右上角上方
      right_eye_x, start_y - 5,             // 顶点3: 左上角上方
      BLACK);

    display.display();
  }
}

// --- 6. 委屈/难过模式 ---
void animateToSad() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;

  // 动画：眼角耷拉下来 (八字眉)
  for(int drop=0; drop<25; drop+=2) {
    display.clearDisplay();
    drawRawEyes(start_x, start_y, eye_width, eye_height);

    // 左眼遮罩：这次遮住"左上角"，形成八字
    display.fillTriangle(
      start_x, start_y + drop,              // 左上角往下压
      start_x + eye_width, start_y - 5,     // 右上角不动
      start_x, start_y - 5,                 // 填补
      BLACK);

    // 右眼遮罩：遮住"右上角"
    display.fillTriangle(
      right_eye_x, start_y - 5,             // 左上角不动
      right_eye_x + eye_width, start_y + drop, // 右上角往下压
      right_eye_x + eye_width, start_y - 5,
      BLACK);

    display.display();
  }
}

// --- 7. 开心模式 (眯眼笑) ---
void animateToHappy() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;

  // 这里的 offset 控制"脸颊"往上抬多高
  for(int offset=0; offset<12; offset+=2) {
    display.clearDisplay();
    
    // 1. 先画正常的白眼睛
    drawRawEyes(start_x, start_y, eye_width, eye_height);

    // 2. 画两个黑色的圆，代表"鼓起来的脸颊"
    // 圆心在眼睛正下方的某个位置，随着 offset 增加，圆心往上移，把眼睛下半部分"吃"掉
    
    int cheek_radius = eye_width * 0.8; // 脸颊圆的大小，稍微比眼睛宽一点点
    
    // 左眼遮罩 (黑色圆)
    // 关键计算：y坐标 = 眼睛底部 + 半径 - 抬起高度
    display.fillCircle(
      start_x + eye_width / 2,         // 圆心 X：左眼中间
      start_y + eye_height + 10 - offset, // 圆心 Y：从下面慢慢升上来
      cheek_radius, 
      BLACK);

    // 右眼遮罩 (黑色圆)
    display.fillCircle(
      right_eye_x + eye_width / 2,     // 圆心 X：右眼中间
      start_y + eye_height + 10 - offset, // 圆心 Y
      cheek_radius, 
      BLACK);

    display.display();
  }
}

// --- 8. 流泪模式 ---
void animateToCry() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;
  
  // 步骤 1: 先变身成"难过"的状态 (复用之前的遮罩逻辑，但这回要快一点)
  // 我们不需要慢慢变难过，直接画出难过的最终形态（眼角耷拉）
  // 难过的遮罩高度，我们固定设为 20 (drop = 20)
  int sad_drop = 20;

  // 步骤 2: 泪水积蓄 -> 掉落的动画
  // 泪水起始位置：右眼底部的中间偏左一点
  int tear_x = right_eye_x + 10; 
  int tear_start_y = start_y + eye_height - 5; // 眼睛底部
  int tear_y = tear_start_y;
  
  // 循环：让泪水从眼睛底部掉落到屏幕最下方
  while (tear_y < SCREEN_HEIGHT + 5) {
    display.clearDisplay();
    
    // --- A. 画背景：保持难过的眼睛 ---
    drawRawEyes(start_x, start_y, eye_width, eye_height);
    
    // 加上难过的遮罩 (保持不动)
    // 左眼遮罩
    display.fillTriangle(
      start_x, start_y + sad_drop,         
      start_x + eye_width, start_y - 5,     
      start_x, start_y - 5,                 
      BLACK);
    // 右眼遮罩
    display.fillTriangle(
      right_eye_x, start_y - 5,             
      right_eye_x + eye_width, start_y + sad_drop, 
      right_eye_x + eye_width, start_y - 5,
      BLACK);

    // --- B. 画前景：泪珠 ---
    // 只有当泪珠离开眼睛一点点后，才画出来，或者一直画
    // 我们画一个实心圆，半径为 3
    display.fillCircle(tear_x, tear_y, 3, WHITE);

    display.display();

    // --- C. 物理计算 ---
    tear_y += 3; // 每次下落 3 个像素 (想要掉得快就改大)
    
    // 简单的延时，控制帧率
    delay(30); 
  }
}
