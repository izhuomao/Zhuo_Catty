#ifndef FACE_SYSTEM_H
#define FACE_SYSTEM_H

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "Config.h"      // 引用配置文件

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class FaceSystem {
  private:
    Adafruit_SSD1306 display; // 对象直接在类内实例化
    
    // 眼睛参数
    int eye_w = 40; 
    int eye_h = 45; 
    int eye_gap = 20; 
    int radius = 8;
    int centerX, centerY;
    
    // 天气变量
    String weatherText = "Loading";
    String temperature = "--";
    unsigned long lastWeatherTime = 0;
    int currentAnimId = 0;

    // --- 基础绘图封装 ---
    void drawRawEyes(int x, int y, int w, int h) {
      display.fillRoundRect(x, y, w, h, radius, SSD1306_WHITE);
      display.fillRoundRect(x + w + eye_gap, y, w, h, radius, SSD1306_WHITE);
    }
    
    void clear() { display.clearDisplay(); }
    void show() { display.display(); }

    void drawTextCenter(String text) {
      clear(); display.setCursor(10, 30); display.setTextSize(1);
      display.println(text); show();
    }

  public:
    // 构造函数：初始化列表初始化 display 对象
    FaceSystem() : display(SCREEN_WIDTH, SCREEN_HEIGHT, 
                           PIN_OLED_MOSI, PIN_OLED_CLK, 
                           PIN_OLED_DC, PIN_OLED_RES, PIN_OLED_CS) {
      centerX = SCREEN_WIDTH / 2; 
      centerY = SCREEN_HEIGHT / 2;
    }

    // 初始化系统
    void init() {
      // 1. 启动屏幕
      if(!display.begin(SSD1306_SWITCHCAPVCC)) { 
        Serial.println(F("OLED Allocation Failed")); 
        for(;;); 
      }
      display.setTextColor(SSD1306_WHITE);
      display.clearDisplay();

      // 2. 连接 WiFi (如果主程序还没连，这里负责连)
      if(WiFi.status() != WL_CONNECTED) {
        drawTextCenter("Connecting WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20) { 
          delay(500); 
          Serial.print(".");
          timeout++;
        }
      }

      // 3. 同步时间 & 获取天气
      if(WiFi.status() == WL_CONNECTED) {
          drawTextCenter("Sync Time...");
          configTime(8 * 3600, 0, "ntp1.aliyun.com");
          updateWeather();
      } else {
          drawTextCenter("WiFi Failed :(");
          delay(1000);
      }
      
      // 开机眨眼
      animateWink();
    }

    // 必须在 loop 中调用，维持天气更新和时钟
    void keepAlive() {
      // 如果当前是 ID=0 (待机模式)，显示时钟
      if (currentAnimId == 0) {
          showWeatherClock(); 
      }
      
      // 每10分钟更新一次天气
      if (millis() - lastWeatherTime > 600000) { 
          updateWeather(); 
          lastWeatherTime = millis(); 
      }
    }

    // 执行表情动作 (对应你的 Switch-Case)
    void execAction(int cmd_id) {
      currentAnimId = cmd_id;
      // Serial.print("Face Action: "); Serial.println(cmd_id);

      switch (cmd_id) {
        case 0: break; // 待机模式
        case 1: animateSleep(); break;
        case 2: animateSquat(); break;
        case 3: showNeutral(); break;
        case 4: animateBegging(); break;
        case 5: animateDetermined(); break;
        case 6: animateRetreat(); break;
        case 7: animateLookLeft(); break;
        case 8: animateLookRight(); break;
        case 9: animateHappy(); break;
        case 10: animateWink(); break;
        case 11: animateLove(); break;
        case 12: showNeutral(); break;
        case 13: { 
           long r = random(0, 3);
           if(r==0) animateLookLeft();
           else if(r==1) animateLookRight();
           else animateWink();
           break;
        }
        case 20: showWeatherClock(); break;   // 显示天气界面
        case 21: showTimeFull(); break;       // 显示时间界面
        case 22: showMemoReminder(); break;   // ▼▼▼ 新增：备忘录提醒动画 ▼▼▼
        default: showNeutral(); break;
      }
    }

    // ================= 动画实现区 (搬运自你的代码) =================
    
    void showNeutral() { 
      clear(); 
      drawRawEyes((SCREEN_WIDTH-(eye_w*2+eye_gap))/2, (SCREEN_HEIGHT-eye_h)/2, eye_w, eye_h); 
      show(); 
    }

    void animateSleep() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2;
       for(int h=eye_h; h>=2; h-=4) {
         clear(); drawRawEyes(sx, sy+(eye_h-h)/2, eye_w, h); show();
       }
       clear();
       display.fillRect(sx, centerY+10, eye_w, 4, SSD1306_WHITE);
       display.fillRect(sx+eye_w+eye_gap, centerY+10, eye_w, 4, SSD1306_WHITE);
       display.setCursor(centerX-10, 10); display.setTextSize(2); display.print("Zzz");
       show();
    }

    void animateSquat() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2;
       for(int offset=0; offset<=15; offset+=3) {
         clear(); drawRawEyes(sx, sy+offset, eye_w, eye_h); show();
       }
    }

    void animateBegging() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; 
       int sy = (SCREEN_HEIGHT-eye_h)/2;
       int rx = sx + eye_w + eye_gap; // 右眼的起始X坐标

       // 1. 蓄力前摇：眼睛先向下微缩，表现出“委屈巴巴”的低姿态
       for(int i = 0; i <= 8; i+=4) {
         clear();
         drawRawEyes(sx, sy + i, eye_w, eye_h - i);
         show();
         delay(50);
       }
       
       // 2. 展开撒娇：水汪汪的星星眼 + 八字眉形态 + 泪光微动
       for(int bounce = 0; bounce < 3; bounce++) { // 循环3次产生眼巴巴的跳动感
         for(int offset = 0; offset <= 3; offset += 3) {
           clear();
           
           // 画基础眼眶：稍微变圆一点，位置跟随 offset 微动
           int current_sy = sy + 2 + offset;
           display.fillRoundRect(sx, current_sy, eye_w, eye_h - 4, radius + 4, SSD1306_WHITE);
           display.fillRoundRect(rx, current_sy, eye_w, eye_h - 4, radius + 4, SSD1306_WHITE);
           
           // 【核心精髓】切掉外上角，形成“八字眉”的无辜感 (🥺 委屈形态)
           // 左眼切左上角
           display.fillTriangle(sx, current_sy - 2, sx + 20, current_sy - 2, sx, current_sy + 16, SSD1306_BLACK);
           // 右眼切右上角
           display.fillTriangle(rx + eye_w, current_sy - 2, rx + eye_w - 20, current_sy - 2, rx + eye_w, current_sy + 16, SSD1306_BLACK);

           // 画巨大的黑色瞳孔，位置偏上（模拟抬头眼巴巴看人的感觉）
           int pupil_r = 14;
           int pupil_y = current_sy + eye_h/2 - 2;
           display.fillCircle(sx + eye_w/2, pupil_y, pupil_r, SSD1306_BLACK);
           display.fillCircle(rx + eye_w/2, pupil_y, pupil_r, SSD1306_BLACK);
           
           // 高光1：大圆（明亮的眼神）
           display.fillCircle(sx + eye_w/2 + 4, pupil_y - 5, 4, SSD1306_WHITE);
           display.fillCircle(rx + eye_w/2 + 4, pupil_y - 5, 4, SSD1306_WHITE);
           
           // 高光2：小圆（泪光闪烁动态效果）
           // 利用 bounce 参数让小高光轻微闪烁/移动，显得更加灵动
           int glint_offset = (bounce % 2 == 0) ? 0 : 1; 
           display.fillCircle(sx + eye_w/2 - 5, pupil_y + 5 + glint_offset, 2, SSD1306_WHITE);
           display.fillCircle(rx + eye_w/2 - 5, pupil_y + 5 + glint_offset, 2, SSD1306_WHITE);
           
           show();
           delay(250); // 控制撒娇动作的节奏
         }
       }
    }


    void animateDetermined() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2; int rx=sx+eye_w+eye_gap;
       for(int d=0; d<20; d+=2) { 
         clear(); drawRawEyes(sx, sy, eye_w, eye_h); 
         display.fillTriangle(sx, sy-5, sx+eye_w, sy+d, sx+eye_w, sy-5, SSD1306_BLACK);
         display.fillTriangle(rx, sy+d, rx+eye_w, sy-5, rx, sy-5, SSD1306_BLACK); show(); 
       }
    }

    void animateRetreat() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2;
       for(int s=0; s<15; s+=3) {
         clear(); drawRawEyes(sx+s, sy+s, eye_w-s*2, eye_h-s*2); show(); delay(20);
       }
       delay(500); 
       for(int s=15; s>=0; s-=3) {
         clear(); drawRawEyes(sx+s, sy+s, eye_w-s*2, eye_h-s*2); show(); 
       }
    }

    void animateLookLeft() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2;
       for(int o=0; o<=15; o+=3) { clear(); drawRawEyes(sx-o, sy, eye_w, eye_h); show(); } 
       delay(500);
       for(int o=15; o>=0; o-=3) { clear(); drawRawEyes(sx-o, sy, eye_w, eye_h); show(); }
    }

    void animateLookRight() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2;
       for(int o=0; o<=15; o+=3) { clear(); drawRawEyes(sx+o, sy, eye_w, eye_h); show(); } 
       delay(500);
       for(int o=15; o>=0; o-=3) { clear(); drawRawEyes(sx+o, sy, eye_w, eye_h); show(); }
    }

    void animateHappy() {
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2; int rx=sx+eye_w+eye_gap;
       for(int o=0; o<15; o+=3) { 
         clear(); drawRawEyes(sx, sy, eye_w, eye_h);
         display.fillCircle(sx+eye_w/2, sy+eye_h+10-o, eye_w*0.8, SSD1306_BLACK);
         display.fillCircle(rx+eye_w/2, sy+eye_h+10-o, eye_w*0.8, SSD1306_BLACK); show(); 
       }
    }

    void animateWink() {
       clear();
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = (SCREEN_HEIGHT-eye_h)/2;
       display.fillRoundRect(sx, sy, eye_w, eye_h, radius, SSD1306_WHITE);
       display.fillRect(sx+eye_w+eye_gap, centerY, eye_w, 6, SSD1306_WHITE);
       show();
       delay(800);
       showNeutral();
    }

    void animateLove() {
       clear();
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2; int sy = centerY-10;
       auto drawHeart = [&](int x, int y, int s) {
          display.fillCircle(x-s/2, y, s/2, SSD1306_WHITE);
          display.fillCircle(x+s/2, y, s/2, SSD1306_WHITE);
          display.fillTriangle(x-s, y+s/4, x+s, y+s/4, x, y+s*1.5, SSD1306_WHITE);
       };
       drawHeart(sx+eye_w/2, sy, 18);
       drawHeart(sx+eye_w+eye_gap+eye_w/2, sy, 18);
       show();
    }

    // ▼▼▼ 新增：备忘录提醒动画 ▼▼▼
    void showMemoReminder() {
       // 第一阶段：闪烁感叹号警示 (吸引注意力)
       for(int flash = 0; flash < 4; flash++) {
         clear();
         display.setTextSize(3);
         display.setCursor(16, 5);
         display.print("!! ");
         display.setCursor(70, 5);
         display.print(" !!");
         // 中间画铃铛形状
         display.fillCircle(64, 18, 10, SSD1306_WHITE);
         display.fillRect(58, 28, 12, 4, SSD1306_WHITE);
         display.fillRect(62, 32, 4, 4, SSD1306_WHITE);
         display.setTextSize(1);
         display.setCursor(30, 48);
         display.print("== REMINDER ==");
         show();
         delay(300);
         
         clear();
         // 只画大眼睛（闪烁效果）
         int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2;
         int sy = (SCREEN_HEIGHT-eye_h)/2;
         drawRawEyes(sx, sy, eye_w, eye_h);
         show();
         delay(200);
       }
       
       // 第二阶段：稳定显示提醒图标 + 文字（持续到语音播完）
       clear();
       // 上半部分：画两个睁大的"惊讶"眼睛
       int sx = (SCREEN_WIDTH-(eye_w*2+eye_gap))/2;
       int sy = 2;
       display.fillRoundRect(sx, sy, eye_w, eye_h - 8, radius + 4, SSD1306_WHITE);
       display.fillRoundRect(sx + eye_w + eye_gap, sy, eye_w, eye_h - 8, radius + 4, SSD1306_WHITE);
       // 在眼睛里画大瞳孔
       display.fillCircle(sx + eye_w/2, sy + (eye_h-8)/2, 10, SSD1306_BLACK);
       display.fillCircle(sx + eye_w + eye_gap + eye_w/2, sy + (eye_h-8)/2, 10, SSD1306_BLACK);
       // 高光
       display.fillCircle(sx + eye_w/2 + 3, sy + (eye_h-8)/2 - 4, 3, SSD1306_WHITE);
       display.fillCircle(sx + eye_w + eye_gap + eye_w/2 + 3, sy + (eye_h-8)/2 - 4, 3, SSD1306_WHITE);
       // 下半部分：提醒文字
       display.drawLine(0, 42, 128, 42, SSD1306_WHITE);
       display.setTextSize(2);
       display.setCursor(10, 48);
       display.print("MEMO!!");
       show();
    }

    void updateWeather() {
      if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;
        String url = "http://api.seniverse.com/v3/weather/now.json?key=" + String(WEATHER_API_KEY) + "&location=" + String(WEATHER_CITY) + "&language=en&unit=c";
        http.begin(url);
        if (http.GET() > 0) {
          String payload = http.getString();
          DynamicJsonDocument doc(1024);
          if (!deserializeJson(doc, payload)) {
            weatherText = doc["results"][0]["now"]["text"].as<String>();
            temperature = doc["results"][0]["now"]["temperature"].as<String>();
          }
        }
        http.end();
      }
    }

    void showWeatherClock() {
      struct tm timeinfo;
      if(!getLocalTime(&timeinfo)) return;
      clear();
      display.drawLine(0, 40, 128, 40, SSD1306_WHITE);
      char timeStr[6]; sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      display.setTextSize(3); display.setCursor(20, 10); display.print(timeStr);
      display.setTextSize(1);
      display.setCursor(0, 45); display.printf("Temp: %s C", temperature.c_str());
      display.setCursor(0, 55); display.print(weatherText); 
      show();
    }
    void showTimeFull() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return;
    clear();
    char timeStr[6]; 
    sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    display.setTextSize(3); 
    display.setCursor(20, 8); 
    display.print(timeStr);
    
    char dateStr[16];
    sprintf(dateStr, "%04d/%02d/%02d", 
            timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday);
    display.setTextSize(1);
    display.setCursor(25, 45);
    display.print(dateStr);
    show();
    }

};

#endif
