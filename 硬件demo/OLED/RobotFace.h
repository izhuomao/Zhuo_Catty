#ifndef ROBOTFACE_H
#define ROBOTFACE_H

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

class RobotFace {
  private:
    Adafruit_SSD1306* _display;
    int screen_w, screen_h;
    // 眼睛基本参数
    int eye_w = 40; 
    int eye_h = 45; 
    int eye_gap = 20; 
    int radius = 8;
    int centerX, centerY;
    
    // 网络与天气变量
    String _ssid, _pass, _apiKey, _city;
    String weatherText = "Loading";
    String temperature = "--";
    unsigned long lastWeatherTime = 0;
    int currentMode = 0; // 0=时钟模式

    // --- 基础绘图封装 ---
    void drawRawEyes(int x, int y, int w, int h) {
      _display->fillRoundRect(x, y, w, h, radius, SSD1306_WHITE);
      _display->fillRoundRect(x + w + eye_gap, y, w, h, radius, SSD1306_WHITE);
    }
    
    void clear() { _display->clearDisplay(); }
    void show() { _display->display(); }

  public:
    RobotFace(Adafruit_SSD1306* displayObj, int w, int h) {
      _display = displayObj;
      screen_w = w; screen_h = h;
      centerX = w / 2; centerY = h / 2;
    }

    // 初始化
    void begin(const char* ssid, const char* pass, String key, String city) {
      _ssid = ssid; _pass = pass; _apiKey = key; _city = city;
      if(!_display->begin(SSD1306_SWITCHCAPVCC)) { Serial.println(F("OLED Error")); for(;;); }
      _display->setTextColor(SSD1306_WHITE);
      
      // WiFi
      drawTextCenter("Connecting...");
      WiFi.begin(_ssid, _pass);
      while (WiFi.status() != WL_CONNECTED) { delay(500); }
      
      // Time & Weather
      configTime(8 * 3600, 0, "ntp1.aliyun.com");
      updateWeather();
      
      // 开机动画
      animateWink();
    }

    // 主循环保持 (放loop里)
    void keepAlive() {
      if (currentMode == 0) showWeatherClock(); // 0号虽然表里没有，保留作为天气待机
      if (millis() - lastWeatherTime > 600000) { updateWeather(); lastWeatherTime = millis(); }
    }

    // ★★★ 核心：完全对应你的13个指令 ★★★
    void execAction(int cmd_id) {
      currentMode = cmd_id;
      Serial.print("Action ID: "); Serial.println(cmd_id);

      switch (cmd_id) {
        case 0: break; // 待机模式(天气时钟)

        // 1. 睡觉 (Sleep) -> 闭眼 Zzz
        case 1: animateSleep(); break;

        // 2. 蹲下 (Squat) -> 视线向下
        case 2: animateSquat(); break;

        // 3. 立正 (Stand) -> 标准直视
        case 3: showNeutral(); break;

        // 4. 跪下 (Kneel) -> 乞求/水汪汪眼
        case 4: animateBegging(); break;

        // 5. 前进 (Advance) -> 坚毅/生气眼
        case 5: animateDetermined(); break;

        // 6. 后退 (Retreat) -> 惊恐/缩小眼 (你特别要求的)
        case 6: animateRetreat(); break;

        // 7. 左转
        case 7: animateLookLeft(); break;

        // 8. 右转
        case 8: animateLookRight(); break;

        // 9. 摇摆 (Sway) -> 开心眯眼笑
        case 9: animateHappy(); break;

        // 10. 打招呼 (Greet) -> 眨单眼(Wink)
        case 10: animateWink(); break;

        // 11. 摇尾巴 (Wag Tail) -> 爱心眼
        case 11: animateLove(); break;

        // 12. 停止 (Stop) -> 恢复立正
        case 12: showNeutral(); break;

        // 13. 自由活动 (Free) -> 随机乱看
        case 13: {
           long r = random(0, 3);
           if(r==0) animateLookLeft();
           else if(r==1) animateLookRight();
           else animateWink();
           break;
        }
        default: showNeutral(); break;
      }
    }

    // ==========================================
    //           具体表情与动画实现区
    // ==========================================

    // ID 3, 12: 标准
    void showNeutral() { 
      clear(); 
      drawRawEyes((screen_w-(eye_w*2+eye_gap))/2, (screen_h-eye_h)/2, eye_w, eye_h); 
      show(); 
    }

    // ID 1: 睡觉
    void animateSleep() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       // 慢慢闭眼
       for(int h=eye_h; h>=2; h-=4) {
         clear(); drawRawEyes(sx, sy+(eye_h-h)/2, eye_w, h); show();
       }
       // 显示Zzz
       clear();
       _display->fillRect(sx, centerY+10, eye_w, 4, SSD1306_WHITE);
       _display->fillRect(sx+eye_w+eye_gap, centerY+10, eye_w, 4, SSD1306_WHITE);
       _display->setCursor(centerX-10, 10); _display->setTextSize(2); _display->print("Zzz");
       show();
    }

    // ID 2: 蹲下 (向下看)
    void animateSquat() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       // 眼睛下移
       for(int offset=0; offset<=15; offset+=3) {
         clear(); drawRawEyes(sx, sy+offset, eye_w, eye_h); show();
       }
    }

    // ID 4: 跪下 (乞求眼 - 大瞳孔)
    void animateBegging() {
       clear();
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       drawRawEyes(sx, sy, eye_w, eye_h);
       // 加高光，看起来水汪汪
       _display->fillCircle(sx+10, sy+10, 6, SSD1306_BLACK);
       _display->fillCircle(sx+eye_w+eye_gap+10, sy+10, 6, SSD1306_BLACK);
       show();
    }

    // ID 5: 前进 (坚毅/生气 - 三角眼)
    void animateDetermined() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2; int rx=sx+eye_w+eye_gap;
       for(int d=0; d<20; d+=2) { 
         clear(); drawRawEyes(sx, sy, eye_w, eye_h); 
         _display->fillTriangle(sx, sy-5, sx+eye_w, sy+d, sx+eye_w, sy-5, SSD1306_BLACK);
         _display->fillTriangle(rx, sy+d, rx+eye_w, sy-5, rx, sy-5, SSD1306_BLACK); show(); 
       }
    }

    // ID 6: 后退 (惊恐 - 瞳孔缩小) ★★★
    void animateRetreat() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       // 模拟瞳孔收缩：眼睛变小
       for(int s=0; s<15; s+=3) {
         clear(); 
         // 绘制越来越小的眼睛
         drawRawEyes(sx+s, sy+s, eye_w-s*2, eye_h-s*2); 
         show(); delay(20);
       }
       delay(500); // 保持惊恐状态
       // 慢慢恢复
       for(int s=15; s>=0; s-=3) {
         clear(); drawRawEyes(sx+s, sy+s, eye_w-s*2, eye_h-s*2); show(); 
       }
    }

    // ID 7: 左转
    void animateLookLeft() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       for(int o=0; o<=15; o+=3) { clear(); drawRawEyes(sx-o, sy, eye_w, eye_h); show(); } 
       delay(500);
       for(int o=15; o>=0; o-=3) { clear(); drawRawEyes(sx-o, sy, eye_w, eye_h); show(); }
    }

    // ID 8: 右转
    void animateLookRight() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       for(int o=0; o<=15; o+=3) { clear(); drawRawEyes(sx+o, sy, eye_w, eye_h); show(); } 
       delay(500);
       for(int o=15; o>=0; o-=3) { clear(); drawRawEyes(sx+o, sy, eye_w, eye_h); show(); }
    }

    // ID 9: 摇摆 (开心笑)
    void animateHappy() {
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2; int rx=sx+eye_w+eye_gap;
       for(int o=0; o<15; o+=3) { 
         clear(); drawRawEyes(sx, sy, eye_w, eye_h);
         _display->fillCircle(sx+eye_w/2, sy+eye_h+10-o, eye_w*0.8, SSD1306_BLACK);
         _display->fillCircle(rx+eye_w/2, sy+eye_h+10-o, eye_w*0.8, SSD1306_BLACK); show(); 
       }
    }

    // ID 10: 打招呼 (眨单眼 Wink)
    void animateWink() {
       clear();
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = (screen_h-eye_h)/2;
       // 左眼睁开，右眼闭合(画线)
       _display->fillRoundRect(sx, sy, eye_w, eye_h, radius, SSD1306_WHITE);
       _display->fillRect(sx+eye_w+eye_gap, centerY, eye_w, 6, SSD1306_WHITE);
       show();
       delay(800);
       showNeutral();
    }

    // ID 11: 摇尾巴 (爱心眼)
    void animateLove() {
       clear();
       int sx = (screen_w-(eye_w*2+eye_gap))/2; int sy = centerY-10;
       // 简单的爱心绘制Lambda
       auto drawHeart = [&](int x, int y, int s) {
          _display->fillCircle(x-s/2, y, s/2, SSD1306_WHITE);
          _display->fillCircle(x+s/2, y, s/2, SSD1306_WHITE);
          _display->fillTriangle(x-s, y+s/4, x+s, y+s/4, x, y+s*1.5, SSD1306_WHITE);
       };
       drawHeart(sx+eye_w/2, sy, 18);
       drawHeart(sx+eye_w+eye_gap+eye_w/2, sy, 18);
       show();
    }

    // 辅助: 天气与文字
    void updateWeather() {
      if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;
        String url = "http://api.seniverse.com/v3/weather/now.json?key=" + _apiKey + "&location=" + _city + "&language=en&unit=c";
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
      _display->drawLine(0, 40, 128, 40, SSD1306_WHITE);
      char timeStr[6]; sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
      _display->setTextSize(3); _display->setCursor(20, 10); _display->print(timeStr);
      _display->setTextSize(1);
      _display->setCursor(0, 45); _display->printf("Temp: %s C", temperature.c_str());
      _display->setCursor(0, 55); _display->print(weatherText); 
      show();
    }

    void drawTextCenter(String text) {
      clear(); _display->setCursor(10, 30); _display->setTextSize(1);
      _display->println(text); show();
    }
};

#endif
