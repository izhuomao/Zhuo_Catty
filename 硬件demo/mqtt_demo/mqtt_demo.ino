#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ================= 1. WiFi 配置 (仅需修改这里) =================
const char* WIFI_SSID = "茁猫阿姨洗铁路";      // TODO: 填入你的WiFi名称
const char* WIFI_PASS = "12345678";  // TODO: 填入你的WiFi密码

// ================= 2. 阿里云 MQTT 连接参数 (已根据你提供的信息填好) =================
// Broker 地址
const char* MQTT_SERVER = "a1k6uay65rc.iot-as-mqtt.cn-shanghai.aliyuncs.com";
const int   MQTT_PORT   = 1883;

// 客户端 ID
const char* MQTT_CLIENTID = "a1k6uay65rc.Catty01|securemode=2,signmethod=hmacsha256,timestamp=1770002813613|";

// 用户名
const char* MQTT_USERNAME = "Catty01&a1k6uay65rc";

// 密码
const char* MQTT_PASSWORD = "51f7675239c324b826bad77604438a1a9dbe41a07c64506094865ed1b0b01955";

// ================= 3. Topic 定义 (需与云流转规则对应) =================
// 注意：设备名是 Catty01，ProductKey 是 a1k6uay65rc

// [接收] ESP32 订阅此 Topic 接收 App 指令
// 你的云流转规则必须配置：App发送的 Topic -> 转发到 -> 这个 Topic
const char* TOPIC_SUB = "/a1k6uay65rc/Catty01/user/control"; 

// [发送] ESP32 向此 Topic 发送状态 (电量、心情)
// 你的云流转规则必须配置：这个 Topic -> 转发到 -> App订阅的 Topic
const char* TOPIC_PUB = "/a1k6uay65rc/Catty01/user/status";

// ================= 全局变量 =================
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsgTime = 0; // 用于心跳计时

// ================= 函数定义 =================

// 连接 WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("正在连接 WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA); // 设置为 Station 模式
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi 连接成功!");
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());
}

// 收到消息的回调函数 (解析 App 发来的指令)
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("收到消息 [Topic: ");
  Serial.print(topic);
  Serial.print("] Payload: ");

  // 将 payload 转换为 String 方便处理
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // 使用 ArduinoJson 解析 JSON
  // 分配内存，200字节通常足够处理简单指令
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("JSON 解析失败: ");
    Serial.println(error.c_str());
    return;
  }

  // --- 1. 处理移动指令 (对应 App: "move") ---
  if (doc.containsKey("move")) {
    const char* moveCmd = doc["move"];
    Serial.print(">> 执行移动: ");
    Serial.println(moveCmd);
    
    if (strcmp(moveCmd, "forward") == 0) {
      // TODO: 这里写电机前进代码
    } else if (strcmp(moveCmd, "backward") == 0) {
      // TODO: 这里写电机后退代码
    } else if (strcmp(moveCmd, "left") == 0) {
      // TODO: 左转
    } else if (strcmp(moveCmd, "right") == 0) {
      // TODO: 右转
    }
  }

  // --- 2. 处理动作指令 (对应 App: "action") ---
  if (doc.containsKey("action")) {
    const char* actionCmd = doc["action"];
    Serial.print(">> 执行动作: ");
    Serial.println(actionCmd);

    if (strcmp(actionCmd, "stand") == 0) {
      // 站立
    } else if (strcmp(actionCmd, "squat") == 0) {
      // 蹲下
    } else if (strcmp(actionCmd, "sleep") == 0) {
      // 睡觉
    } else if (strcmp(actionCmd, "greet") == 0) {
      // 打招呼
    }
  }

  // --- 3. 处理模式指令 (对应 App: "mode") ---
  if (doc.containsKey("mode")) {
    const char* modeCmd = doc["mode"];
    Serial.print(">> 切换模式: ");
    Serial.println(modeCmd);
  }
}

// MQTT 重连函数
void reconnect() {
  // 循环直到连接成功
  while (!client.connected()) {
    Serial.print("尝试连接阿里云 MQTT...");
    
    // 使用你提供的参数进行连接
    if (client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("连接成功!");
      
      // 连接成功后，务必重新订阅 Topic
      client.subscribe(TOPIC_SUB);
      Serial.print("已订阅 Topic: ");
      Serial.println(TOPIC_SUB);
    } else {
      Serial.print("失败, 错误码 rc=");
      Serial.print(client.state());
      Serial.println(" -> 5秒后重试");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // 初始化 WiFi
  setup_wifi();
  
  // 初始化 MQTT
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback); // 设置收到消息的处理函数
  
  // 调整 buffer 大小，防止长 JSON 截断 (视情况调整，512通常足够)
  client.setKeepAlive(60);
  client.setBufferSize(512); 
}

void loop() {
  // 检查 MQTT 连接，断线自动重连
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // 保持 MQTT 心跳

  // --- 定时发送状态给 App (例如每3秒) ---
  unsigned long now = millis();
  if (now - lastMsgTime > 3000) {
    lastMsgTime = now;

    // 构建要发送的 JSON: {"mood":"开心", "battery": 85}
    StaticJsonDocument<200> doc;
    
    // 模拟数据 (你可以替换为真实的传感器读数)
    doc["mood"] = "兴奋"; 
    doc["battery"] = random(80, 100); // 随机生成 80-100 的电量用于测试

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // 发送消息
    client.publish(TOPIC_PUB, jsonBuffer);
    // Serial.print("上报状态: "); // 调试用，不想刷屏可注释掉
    // Serial.println(jsonBuffer);
  }
}
