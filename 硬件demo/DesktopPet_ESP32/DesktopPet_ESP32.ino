/*
 * 桌宠主程序框架 - 双核并行版 (备忘录+提醒版)
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
volatile int pendingActionId = -1;
TaskHandle_t VoiceTaskHandle = NULL;
TaskHandle_t MotionTaskHandle = NULL;

// --- 备忘录跨核心转发缓冲 ---
#define MEMO_FORWARD_BUF_SIZE 512
char memoForwardBuf[MEMO_FORWARD_BUF_SIZE];
volatile bool memoReadyToPublish = false;

// --- ▼▼▼ 新增：提醒请求缓冲（MQTT → VoiceSystem） ▼▼▼ ---
#define REMIND_FORWARD_BUF_SIZE 256
char remindForwardBuf[REMIND_FORWARD_BUF_SIZE];
volatile bool hasRemindRequest = false;

// 函数声明
void setGlobalState(int id);
void onMemoFromVoice(const char* memoJson);
void onReminderFromMqtt(const char* reminderText);

// ================= 核心 1 任务：动作、表情、传感器 =================
void MotionTask(void * pvParameters) {
  for(;;) {
    if (pendingActionId != -1) {
      int id = pendingActionId;
      pendingActionId = -1;
      face.execAction(id);
      motion.execAction(id);
    }
    face.keepAlive();
    sensor.init();
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

// ================= 核心 0 任务：语音、WiFi、网络 =================
void VoiceTask(void * pvParameters) {
  for(;;) {
    voice.update();
    wifi.maintain();
    network.update();
    
    // 检查是否有待转发的备忘录（语音录入 → MQTT → App）
    if (memoReadyToPublish) {
      network.publishMemo(memoForwardBuf);
      memoReadyToPublish = false;
    }
    
    // ▼▼▼ 新增：检查是否有待处理的提醒请求（App MQTT → VoiceSystem → Python TTS） ▼▼▼
    if (hasRemindRequest) {
      voice.requestRemindTTS(remindForwardBuf);
      hasRemindRequest = false;
    }
    
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  wifi.init(); 
  motion.init();
  face.init();
  
  // ▼▼▼ 修改：传入提醒回调 ▼▼▼
  network.init(setGlobalState, onReminderFromMqtt);
  
  led.init(); 
  sensor.init();
  voice.init(setGlobalState, onMemoFromVoice);

  Serial.println(">>> 桌宠启动成功 (双核+备忘录+提醒) <<<");

  xTaskCreatePinnedToCore(
    VoiceTask, "VoiceTask", 10000, NULL, 3, &VoiceTaskHandle, 0
  );

  xTaskCreatePinnedToCore(
    MotionTask, "MotionTask", 10000, NULL, 1, &MotionTaskHandle, 1
  );

  setGlobalState(3);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// 状态分发
void setGlobalState(int id) {
    Serial.printf(">>> 收到指令，分发 ID: %d\n", id);
    
    if (id == 16) { led.showRed(); return; } 
    if (id == 17) { led.showBlue(); return; }
    if (id == 18) { led.showGreen(); return; } 
    
    if (id == 3) {
        led.turnOff();
        pendingActionId = 3;
    } else {
        pendingActionId = id; 
    }
}

// 备忘录回调（VoiceSystem 收到 MEMO 包后调用，在核心0上执行）
void onMemoFromVoice(const char* memoJson) {
    Serial.printf("📝 [主程序] 收到语音备忘录，准备通过 MQTT 转发\n");
    int len = strlen(memoJson);
    if (len < MEMO_FORWARD_BUF_SIZE - 1) {
        strncpy(memoForwardBuf, memoJson, MEMO_FORWARD_BUF_SIZE - 1);
        memoForwardBuf[MEMO_FORWARD_BUF_SIZE - 1] = '\0';
        memoReadyToPublish = true;
    }
}

// ▼▼▼ 新增：提醒回调（NetworkSystem 收到 MQTT 提醒指令后调用） ▼▼▼
// App 闹钟触发 → MQTT → NetworkSystem → 这里 → VoiceSystem → UDP → Python TTS → 播放
void onReminderFromMqtt(const char* reminderText) {
    Serial.printf("🔔 [主程序] 收到 MQTT 提醒: %s\n", reminderText);
    int len = strlen(reminderText);
    if (len < REMIND_FORWARD_BUF_SIZE - 1) {
        strncpy(remindForwardBuf, reminderText, REMIND_FORWARD_BUF_SIZE - 1);
        remindForwardBuf[REMIND_FORWARD_BUF_SIZE - 1] = '\0';
        hasRemindRequest = true;
    }
}
