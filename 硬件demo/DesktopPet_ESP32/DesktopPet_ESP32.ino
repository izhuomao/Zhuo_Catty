/*
 * 桌宠主程序 - 双核并行版 (备忘录+提醒+情感记忆版)
 */
#include "Config.h"
#include "FaceSystem.h"
#include "SensorSystem.h"
#include "LedSystem.h"
#include "NetworkSystem.h"
#include "WifiSystem.h"
#include "MotionSystem.h"
#include "VoiceSystem.h"

WifiSystem wifi;
FaceSystem face;
NetworkSystem network;
LedSystem led;
SensorSystem sensor;
MotionSystem motion;
VoiceSystem voice;   

volatile int pendingActionId = -1;
TaskHandle_t VoiceTaskHandle = NULL;
TaskHandle_t MotionTaskHandle = NULL;

#define MEMO_FORWARD_BUF_SIZE 512
char memoForwardBuf[MEMO_FORWARD_BUF_SIZE];
volatile bool memoReadyToPublish = false;

#define REMIND_FORWARD_BUF_SIZE 256
char remindForwardBuf[REMIND_FORWARD_BUF_SIZE];
volatile bool hasRemindRequest = false;

// ▼▼▼ 新增：情感状态全局变量 ▼▼▼
volatile int globalMood = 50;
volatile int globalIntimacy = 20;
volatile bool emotionUpdated = false;

// ▼▼▼ 新增：APP 互动通知标志 ▼▼▼
volatile bool hasAppInteract = false;

void setGlobalState(int id);
void onMqttCommand(int id);  // ▼▼▼ 新增：MQTT 指令专用回调 ▼▼▼
void onMemoFromVoice(const char* memoJson);
void onReminderFromMqtt(const char* reminderText);
void onEmotionFromVoice(int mood, int intimacy);

// ================= 核心 1：动作、表情、传感器 =================
void MotionTask(void * pvParameters) {
  for(;;) {
    if (pendingActionId != -1) {
      int id = pendingActionId;
      pendingActionId = -1;
      face.execAction(id);
      motion.execAction(id);
    }
    
    // ▼▼▼ 新增：应用情感数据到表情和舵机 ▼▼▼
    if (emotionUpdated) {
      emotionUpdated = false;
      int mood = globalMood;
      face.setMood(mood);
      motion.setMoodSpeed(mood);
    }
    
    face.keepAlive();
    sensor.init();
    vTaskDelay(10 / portTICK_PERIOD_MS); 
  }
}

// ================= 核心 0：语音、WiFi、网络 =================
void VoiceTask(void * pvParameters) {
  for(;;) {
    voice.update();
    wifi.maintain();
    network.update();
    
    if (memoReadyToPublish) {
      network.publishMemo(memoForwardBuf);
      memoReadyToPublish = false;
    }
    
    if (hasRemindRequest) {
      voice.requestRemindTTS(remindForwardBuf);
      hasRemindRequest = false;
    }
    
    // ▼▼▼ 新增：转发 APP 互动通知给 PC ▼▼▼
    if (hasAppInteract) {
      voice.requestAppInteract();
      hasAppInteract = false;
    }
    
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("上次重启原因: %d\n", reason);

  wifi.init(); 
  motion.init();
  face.init();
  network.init(onMqttCommand, onReminderFromMqtt);  // ▼▼▼ 改用 onMqttCommand ▼▼▼
  led.init(); 
  sensor.init();
  
  // ▼▼▼ 修改：传入情感数据回调 ▼▼▼
  voice.init(setGlobalState, onMemoFromVoice, onEmotionFromVoice);

  Serial.println(">>> 桌宠启动成功 (双核+备忘录+提醒+情感记忆) <<<");

  xTaskCreatePinnedToCore(VoiceTask, "VoiceTask", 10000, NULL, 3, &VoiceTaskHandle, 0);
  xTaskCreatePinnedToCore(MotionTask, "MotionTask", 10000, NULL, 1, &MotionTaskHandle, 1);

  setGlobalState(3);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// ▼▼▼ 新增：MQTT 指令专用回调（APP 遥控时触发） ▼▼▼
void onMqttCommand(int id) {
    Serial.printf("📱 [主程序] 收到 MQTT 指令: %d，标记 APP 互动\n", id);
    setGlobalState(id);
    hasAppInteract = true;
}

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

void onMemoFromVoice(const char* memoJson) {
    Serial.printf("📝 [主程序] 收到语音备忘录，准备通过 MQTT 转发\n");
    int len = strlen(memoJson);
    if (len < MEMO_FORWARD_BUF_SIZE - 1) {
        strncpy(memoForwardBuf, memoJson, MEMO_FORWARD_BUF_SIZE - 1);
        memoForwardBuf[MEMO_FORWARD_BUF_SIZE - 1] = '\0';
        memoReadyToPublish = true;
    }
}

void onReminderFromMqtt(const char* reminderText) {
    Serial.printf("🔔 [主程序] 收到 MQTT 提醒: %s\n", reminderText);
    int len = strlen(reminderText);
    if (len < REMIND_FORWARD_BUF_SIZE - 1) {
        strncpy(remindForwardBuf, reminderText, REMIND_FORWARD_BUF_SIZE - 1);
        remindForwardBuf[REMIND_FORWARD_BUF_SIZE - 1] = '\0';
        hasRemindRequest = true;
    }
}

// ▼▼▼ 新增：情感数据回调 ▼▼▼
void onEmotionFromVoice(int mood, int intimacy) {
    Serial.printf("💖 [主程序] 收到情感数据: mood=%d, intimacy=%d\n", mood, intimacy);
    globalMood = mood;
    globalIntimacy = intimacy;
    emotionUpdated = true;
    
    // 把情感数据同步给 NetworkSystem，通过 MQTT 上报给手机 APP
    network.setEmotionData(mood, intimacy);
}
