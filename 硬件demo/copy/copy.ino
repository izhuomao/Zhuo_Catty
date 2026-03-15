/*
 * 桌宠主程序框架 - 双核并行版
 */
#include "Config.h"
#include "FaceSystem.h"
#include "SensorSystem.h"
#include "LedSystem.h"
#include "NetworkSystem.h"
#include "WifiSystem.h"
#include "MotionSystem.h"
#include "VoiceSystem.h"

// 实例化模块
WifiSystem wifi;
FaceSystem face;
NetworkSystem network;
LedSystem led;
SensorSystem sensor;
MotionSystem motion;
VoiceSystem voice;   

// --- 双核同步变量 ---
volatile int pendingActionId = -1; // 用于跨核心传递指令的变量
TaskHandle_t VoiceTaskHandle = NULL;
TaskHandle_t MotionTaskHandle = NULL;

void setGlobalState(int id);

// ================= 核心 1 任务：负责动作、表情、传感器 =================
void MotionTask(void * pvParameters) {
  for(;;) {
    // 1. 处理待执行的动作
    if (pendingActionId != -1) {
      int id = pendingActionId;
      pendingActionId = -1; // 消费掉指令

      // 执行耗时的舵机动作和表情动画
      // 因为在独立核心运行，这里的阻塞不会影响语音播放
      face.execAction(id);
      motion.execAction(id);
    }

    // 2. 维持屏幕 UI（时钟/天气）
    face.keepAlive();
    
    // 3. 传感器更新
    sensor.init(); // 注意：如果SensorSystem里有update，请改为sensor.update()

    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

// ================= 核心 0 任务：负责语音、WiFi、网络通讯 =================
void VoiceTask(void * pvParameters) {
  for(;;) {
    voice.update();   // 语音接收与播放（高优先级）
    wifi.maintain();  // 维持WiFi连接
    network.update(); // 维持MQTT等网络连接
    
    vTaskDelay(1 / portTICK_PERIOD_MS); // 极短延迟防止触发看门狗
  }
}

void setup() {
  Serial.begin(115200);

  wifi.init(); 
  motion.init();
  face.init();
  network.init(setGlobalState);
  led.init(); 
  sensor.init();
  voice.init(setGlobalState);

  Serial.println(">>> 桌宠启动成功 (双核模式) <<<");

  // 创建语音任务 (运行在核心 0)
  xTaskCreatePinnedToCore(
    VoiceTask, "VoiceTask", 10000, NULL, 3, &VoiceTaskHandle, 0
  );

  // 创建动作任务 (运行在核心 1)
  xTaskCreatePinnedToCore(
    MotionTask, "MotionTask", 10000, NULL, 1, &MotionTaskHandle, 1
  );

  setGlobalState(3); // 初始状态
}

void loop() {
  // loop 函数现在可以留空，或者处理一些极低优先级的任务
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// 修改后的 setGlobalState 变得非常轻量
// 修改后的 setGlobalState：严格区分系统状态和动作状态
void setGlobalState(int id) {
    Serial.printf(">>> 收到指令，分发 ID: %d\n", id);
    
    // 1. 系统纯灯光状态拦截 (16=唤醒, 17=处理中, 18=正在说话)
    // 匹配到这些 ID 时，改完灯光立刻 return，保护动作 ID 不被覆盖！
    if (id == 16) { led.showRed(); return; } 
    if (id == 17) { led.showBlue(); return; }
    if (id == 18) { led.showGreen(); return; } 
    
    // 2. 真正的动作/表情指令 (进入核心 1 执行)
    if (id == 3) {
        led.turnOff();
        pendingActionId = 3; // 对话结束，恢复待机动作
    } else {
        // 将来自 Python 的真实动作 ID (例如 2:坐下, 4:撒娇) 交给动作核心
        pendingActionId = id; 
    }
}

