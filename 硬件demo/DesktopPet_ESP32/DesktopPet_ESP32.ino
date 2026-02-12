/*
 * 桌宠主程序框架
 */
#include "Config.h"
#include "FaceSystem.h"
#include "SensorSystem.h"
#include "LedSystem.h"
#include "NetworkSystem.h"
#include "WifiSystem.h"
// 未来你会这里加: #include "MotionSystem.h" 等

// 实例化模块
WifiSystem wifi;
FaceSystem face;
NetworkSystem network;
LedSystem led;
SensorSystem sensor;

int currentActionId = 0;   // 当前动作ID
void setGlobalState(int id);

void setup() {
  Serial.begin(115200);

  wifi.init(); 
  
  // 初始化屏幕模块 (内部会自动连WiFi、对时)
  face.init();
  network.init(setGlobalState);
  led.init();//强制把RGB灯先熄灭
  sensor.init();

  Serial.println(">>> 桌宠启动成功 <<<");
  // Serial.println("输入数字 0-13 控制表情");
  
}

void loop() {
  // 1. 串口指令测试 (模拟外部指令)
  // if (Serial.available() > 0) {
  //   int cmd = Serial.parseInt();
  //   while(Serial.available()) Serial.read(); 
    
  //   // 调用 FaceSystem 的接口
  //   face.execAction(cmd);
  // }

  // 2. 维持后台任务 (必须有！)
  // 这行代码会让屏幕在没事干的时候显示时钟和天气
  face.keepAlive();
  wifi.maintain();
  network.update();
}
void setGlobalState(int id) {
    Serial.printf(">>> 收到指令，执行 ID: %d\n", id);

    // 这里是你统一调用其他模块的地方
    face.execAction(id);
    // motion.execAction(id);
    // led.setMood(id);
    
    // 比如：MQTT 收到 {"action":"greet"} -> 翻译成 10 -> 调用此函数 -> 执行打招呼
}