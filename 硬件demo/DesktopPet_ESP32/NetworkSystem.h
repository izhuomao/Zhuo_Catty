#ifndef NETWORK_SYSTEM_H
#define NETWORK_SYSTEM_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional> 
#include "Config.h" 

typedef std::function<void(int)> CommandCallback;
typedef std::function<void(const char*)> ReminderCallback;

class NetworkSystem {
private:
    WiFiClient espClient;
    PubSubClient client;
    CommandCallback actionHandler; 
    ReminderCallback reminderHandler;
    
    unsigned long lastReconnectAttempt = 0;
    unsigned long lastMsgTime = 0;

    // ▼▼▼ 新增：缓存情感数据，随心跳一起上报给手机APP ▼▼▼
    int cachedMood = 50;
    int cachedIntimacy = 20;

public:
    NetworkSystem() : client(espClient) {}

    void init(CommandCallback handler, ReminderCallback remHandler = nullptr) {
        actionHandler = handler;
        reminderHandler = remHandler;
        client.setServer(MQTT_SERVER, MQTT_PORT);
        client.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->mqttCallback(topic, payload, length);
        });
        client.setBufferSize(1024); 
        client.setKeepAlive(60);   
        Serial.println("[MQTT] System Initialized");
    }

    void update() {
        if (WiFi.status() != WL_CONNECTED) return; 
        if (!client.connected()) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 5000) { 
                lastReconnectAttempt = now;
                reconnectMqtt(); 
            }
        } else {
            client.loop();  
            reportStatus(); 
        }
    }

    // ▼▼▼ 新增：更新情感缓存 + 立即上报 ▼▼▼
    void setEmotionData(int mood, int intimacy) {
        cachedMood = mood;
        cachedIntimacy = intimacy;
        Serial.printf("[MQTT] 💖 情感数据已缓存: mood=%d, intimacy=%d\n", mood, intimacy);
        
        if (client.connected()) {
            char buf[128];
            snprintf(buf, sizeof(buf), 
                "{\"status\":\"alive\",\"mood\":%d,\"intimacy\":%d}", 
                cachedMood, cachedIntimacy);
            client.publish(TOPIC_PUB, buf);
            Serial.printf("[MQTT] 💖 情感状态已即时上报\n");
        }
    }

    void publishMemo(const char* memoJson) {
        if (client.connected()) {
            bool ok = client.publish(TOPIC_MEMO_PUB, memoJson);
            if (ok) Serial.printf("[MQTT] 📝 备忘录已发布: %s\n", memoJson);
            else Serial.println("[MQTT] ❌ 备忘录发布失败");
        } else {
            Serial.println("[MQTT] ⚠️ 未连接，无法发布备忘录");
        }
    }

private:
    void reconnectMqtt() {
        Serial.print("[MQTT] Connecting... ");
        if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
            Serial.println("Success!");
            client.subscribe(TOPIC_SUB);
        } else {
            Serial.print("Failed rc=");
            Serial.println(client.state());
        }
    }

    void mqttCallback(char* topic, byte* payload, unsigned int length) {
        String msg;
        for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
        Serial.printf("[MQTT] Recv: %s\n", msg.c_str());
        
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, msg);
        if (error) return;

        int targetActionId = -1;
        
        if (doc.containsKey("reminder")) {
            const char* reminderText = doc["reminder"];
            Serial.printf("[MQTT] 🔔 收到提醒指令: %s\n", reminderText);
            if (reminderHandler) reminderHandler(reminderText);
            return;
        }

        if (doc.containsKey("move")) {
             const char* cmd = doc["move"];
             if (strcmp(cmd, "forward") == 0) targetActionId = 5;
             else if (strcmp(cmd, "backward") == 0) targetActionId = 6;
             else if (strcmp(cmd, "left") == 0) targetActionId = 7;
             else if (strcmp(cmd, "right") == 0) targetActionId = 8;
             else if (strcmp(cmd, "stop") == 0) targetActionId = 12;
        }
        if (doc.containsKey("action")) {
             const char* cmd = doc["action"];
             if (strcmp(cmd, "sleep") == 0) targetActionId = 1;
             else if (strcmp(cmd, "sit") == 0) targetActionId = 2;
             else if (strcmp(cmd, "stand") == 0) targetActionId = 3;
             else if (strcmp(cmd, "cute") == 0) targetActionId = 4;
             else if (strcmp(cmd, "sway") == 0) targetActionId = 9;
             else if (strcmp(cmd, "greet") == 0) targetActionId = 10;
             else if (strcmp(cmd, "wag") == 0) targetActionId = 11;
             else if (strcmp(cmd, "auto_patrol") == 0) targetActionId = 13;
             else if (strcmp(cmd, "lie") == 0) targetActionId = 14;
        }

        if (targetActionId != -1 && actionHandler) actionHandler(targetActionId);
    }

    // ▼▼▼ 修改：心跳包带上情感数据 ▼▼▼
    void reportStatus() {
        unsigned long now = millis();
        if (now - lastMsgTime > 30000) {
            lastMsgTime = now;
            char buf[128];
            snprintf(buf, sizeof(buf), 
                "{\"status\":\"alive\",\"mood\":%d,\"intimacy\":%d}", 
                cachedMood, cachedIntimacy);
            client.publish(TOPIC_PUB, buf);
        }
    }
};

#endif
