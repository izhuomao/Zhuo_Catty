#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

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

// ================================================
// ▼▼▼ 请修改这里：WiFi 和 心知天气配置 ▼▼▼
// ================================================
const char* ssid     = "茁猫阿姨洗铁路";    
const char* password = "12345678";

// 心知天气设置
String seniverseKey  = "S0ByFwl5YUl7YNLY7";  // 填入你的私钥
String city          = "hangzhou";        // 城市拼音 (例如 shanghai, guangzhou)
// ================================================

// --- 时间变量 ---
const char* ntpServer = "ntp1.aliyun.com"; // 改用阿里云NTP，国内更准
const long  gmtOffset_sec = 8 * 3600;      // UTC+8
const int   daylightOffset_sec = 0;

// --- 存储变量 ---
String weatherText = "Loading..."; // 天气现象文字 (Sunny, Cloudy...)
String temperature = "--";
unsigned long lastWeatherTime = 0;
unsigned long weatherTimerDelay = 600000; // 10分钟更新一次

// --- 表情参数 ---
int eye_width = 40;
int eye_height = 45;
int eye_gap = 20;
int eye_radius = 8;
int x_center = SCREEN_WIDTH / 2;
int y_center = SCREEN_HEIGHT / 2;

// --- 状态控制 ---
bool isDemoPlayed = false;

void setup() {
  Serial.begin(115200);
  
  // 初始化屏幕
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);

  // 1. 连接 WiFi 界面
  display.clearDisplay();
  drawRawEyes(20, 10, 30, 30); 
  display.setCursor(30, 50);
  display.print("Connecting...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // 2. 连接成功
  display.clearDisplay();
  centerEyes(); 
  display.setCursor(35, 50);
  display.println("Connected!");
  display.display();
  delay(1000);

  // 3. 初始化时间 (使用阿里云NTP)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // 4. 首次获取天气
  getWeather();
}

void loop() {
  // 逻辑：先播表情，播完后一直显示时钟
  if (!isDemoPlayed) {
    runExpressionDemo();
    isDemoPlayed = true; 
  } else {
    runWeatherClock();
  }
}

// ==========================================
//           模式 B: 天气时钟流程
// ==========================================
void runWeatherClock() {
  // 检查更新天气
  if ((millis() - lastWeatherTime) > weatherTimerDelay) {
    getWeather();
    lastWeatherTime = millis();
  }

  display.clearDisplay();
  
  // 分隔线
  display.drawLine(0, 40, 128, 40, WHITE);
  
  // 显示大时间
  printLocalTime();
  
  // 显示心知天气数据
  display.setTextSize(1);
  
  // 温度
  display.setCursor(0, 45);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  
  // 天气描述 (英文)
  display.setCursor(0, 55);
  display.print(weatherText); 
  
  display.display();
  delay(1000); 
}

// ==========================================
//           心知天气获取函数 (核心修改)
// ==========================================
void getWeather() {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    
    // 构建心知天气 API URL
    // language=en 为了防止OLED无法显示中文乱码
    String url = "http://api.seniverse.com/v3/weather/now.json?key=" + seniverseKey + "&location=" + city + "&language=en&unit=c";
    
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      // JSON 解析
      // 心知天气的结构是: {"results": [{"now": {"text": "Sunny", "temperature": "25"}}]}
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        JsonObject results_0 = doc["results"][0];
        JsonObject now = results_0["now"];
        
        const char* textStr = now["text"]; // "Sunny", "Cloudy" 等
        const char* tempStr = now["temperature"];
        
        weatherText = String(textStr);
        temperature = String(tempStr);
      } else {
        Serial.print("JSON Error");
      }
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

// ==========================================
//           网络时间函数
// ==========================================
void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    display.setCursor(0, 10);
    display.println("Time Error");
    return;
  }
  display.setTextSize(3); 
  display.setCursor(20, 10);
  
  if(timeinfo.tm_hour < 10) display.print("0");
  display.print(timeinfo.tm_hour);
  display.print(":");
  if(timeinfo.tm_min < 10) display.print("0");
  display.print(timeinfo.tm_min);
}

// ==========================================
//           表情演示流程 (保持不变)
// ==========================================
void runExpressionDemo() {
  centerEyes(); delay(1000);
  blink(1); lookLeft(); delay(800);
  lookRight(); delay(800); centerEyes(); delay(500);

  animateToAngry(); delay(2000);
  
  animateToSad(); delay(500);
  animateToCry(); delay(200);
  animateToCry(); delay(1000);
  
  centerEyes(); blink(2); delay(1000);
  
  animateToHappy(); delay(2000);

  // 转场：慢慢闭眼
  centerEyes(); delay(500);
  for(int h=eye_height; h>=2; h-=4) {
      display.clearDisplay();
      int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
      int start_y = (SCREEN_HEIGHT - eye_height) / 2;
      int current_y = start_y + (eye_height - h) / 2;
      drawRawEyes(start_x, current_y, eye_width, h);
      display.display();
  }
  delay(500);
}

// ==========================================
//           基础绘图函数 (保持不变)
// ==========================================
void drawRawEyes(int x, int y, int w, int h) {
  display.fillRoundRect(x, y, w, h, eye_radius, WHITE);
  display.fillRoundRect(x + w + eye_gap, y, w, h, eye_radius, WHITE);
}

void centerEyes() {
  display.clearDisplay();
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  drawRawEyes(start_x, start_y, eye_width, eye_height);
  display.display();
}

void blink(int times) {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  for(int t=0; t<times; t++) {
    for(int h=eye_height; h>=2; h-=8) {
      display.clearDisplay();
      int current_y = start_y + (eye_height - h) / 2;
      drawRawEyes(start_x, current_y, eye_width, h);
      display.display();
    }
    for(int h=2; h<=eye_height; h+=8) {
      display.clearDisplay();
      int current_y = start_y + (eye_height - h) / 2;
      drawRawEyes(start_x, current_y, eye_width, h);
      display.display();
    }
    delay(100);
  }
}

void lookLeft() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  for(int offset=0; offset<20; offset+=5) {
    display.clearDisplay();
    drawRawEyes(start_x - offset, start_y, eye_width, eye_height);
    display.display();
  }
}

void lookRight() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  for(int offset=0; offset<20; offset+=5) {
    display.clearDisplay();
    drawRawEyes(start_x + offset, start_y, eye_width, eye_height);
    display.display();
  }
}

void animateToAngry() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;
  for(int drop=0; drop<25; drop+=2) {
    display.clearDisplay();
    drawRawEyes(start_x, start_y, eye_width, eye_height);
    display.fillTriangle(start_x, start_y - 5, start_x + eye_width, start_y + drop, start_x + eye_width, start_y - 5, BLACK);
    display.fillTriangle(right_eye_x, start_y + drop, right_eye_x + eye_width, start_y - 5, right_eye_x, start_y - 5, BLACK);
    display.display();
  }
}

void animateToSad() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;
  for(int drop=0; drop<25; drop+=2) {
    display.clearDisplay();
    drawRawEyes(start_x, start_y, eye_width, eye_height);
    display.fillTriangle(start_x, start_y + drop, start_x + eye_width, start_y - 5, start_x, start_y - 5, BLACK);
    display.fillTriangle(right_eye_x, start_y - 5, right_eye_x + eye_width, start_y + drop, right_eye_x + eye_width, start_y - 5, BLACK);
    display.display();
  }
}

// 【关键】修复过的 helper 函数，用于 animateToCry 里的蓄泪效果
void drawSadFrame(int x, int y, int right_x, int drop) {
  drawRawEyes(x, y, eye_width, eye_height);
  display.fillTriangle(x, y + drop, x + eye_width, y - 5, x, y - 5, BLACK);
  display.fillTriangle(right_x, y - 5, right_x + eye_width, y + drop, right_x + eye_width, y - 5, BLACK);
}

void animateToCry() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;
  int sad_drop = 20;
  int tear_x = right_eye_x + 10; 
  int tear_start_y = start_y + eye_height - 5;
  
  // 蓄泪
  for(int r=0; r<=4; r++) {
     display.clearDisplay();
     drawSadFrame(start_x, start_y, right_eye_x, sad_drop);
     display.fillCircle(tear_x, tear_start_y, r, WHITE); 
     display.display();
     delay(100);
  }
  
  // 掉落
  int tear_y = tear_start_y;
  while (tear_y < SCREEN_HEIGHT + 5) {
    display.clearDisplay();
    drawSadFrame(start_x, start_y, right_eye_x, sad_drop);
    display.fillCircle(tear_x, tear_y, 4, WHITE);
    display.display();
    tear_y += 3; 
    delay(30); 
  }
}

void animateToHappy() {
  int start_x = (SCREEN_WIDTH - (eye_width * 2 + eye_gap)) / 2;
  int start_y = (SCREEN_HEIGHT - eye_height) / 2;
  int right_eye_x = start_x + eye_width + eye_gap;
  for(int offset=0; offset<12; offset+=2) {
    display.clearDisplay();
    drawRawEyes(start_x, start_y, eye_width, eye_height);
    int cheek_radius = eye_width * 0.8; 
    display.fillCircle(start_x + eye_width / 2, start_y + eye_height + 10 - offset, cheek_radius, BLACK);
    display.fillCircle(right_eye_x + eye_width / 2, start_y + eye_height + 10 - offset, cheek_radius, BLACK);
    display.display();
  }
}
