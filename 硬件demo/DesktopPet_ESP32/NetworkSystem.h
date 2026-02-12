#ifndef NETWORK_SYSTEM_H
#define NETWORK_SYSTEM_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <functional> 
#include "Config.h" 

typedef std::function<void(int)> CommandCallback;

class NetworkSystem {
private:
    WiFiClient espClient;
    PubSubClient client;
    CommandCallback actionHandler; 
    
    unsigned long lastReconnectAttempt = 0;
    unsigned long lastMsgTime = 0;

public:
    NetworkSystem() : client(espClient) {}

    // 初始化：不再连WiFi，只设置 MQTT 参数
    void init(CommandCallback handler) {
        actionHandler = handler;
        client.setServer(MQTT_SERVER, MQTT_PORT);
        client.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->mqttCallback(topic, payload, length);
        });
        client.setBufferSize(1024); 
        client.setKeepAlive(60);   
        Serial.println("[MQTT] System Initialized");
    }

    // 在 loop 中调用
    void update() {
        // 1. 如果 WiFi 没连上，MQTT 啥也做不了，直接返回
        // (连网的事交给 WifiSystem 去管)
        if (WiFi.status() != WL_CONNECTED) {
            return; 
        }

        // 2. 检查 MQTT 连接
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
        
        // 解析 move
        if (doc.containsKey("move")) {
             const char* cmd = doc["move"];
             if (strcmp(cmd, "forward") == 0) targetActionId = 5;
             else if (strcmp(cmd, "backward") == 0) targetActionId = 6;
             else if (strcmp(cmd, "left") == 0) targetActionId = 7;
             else if (strcmp(cmd, "right") == 0) targetActionId = 8;
             else if (strcmp(cmd, "stop") == 0) targetActionId = 12;
        }
        // 解析 action
        if (doc.containsKey("action")) {
             const char* cmd = doc["action"];
             if (strcmp(cmd, "sleep") == 0) targetActionId = 1;
             else if (strcmp(cmd, "squat") == 0) targetActionId = 2;
             else if (strcmp(cmd, "stand") == 0) targetActionId = 3;
             else if (strcmp(cmd, "cute") == 0) targetActionId = 4;
             else if (strcmp(cmd, "sway") == 0) targetActionId = 9;
             else if (strcmp(cmd, "greet") == 0) targetActionId = 10;
             else if (strcmp(cmd, "wag") == 0) targetActionId = 11;
             else if (strcmp(cmd, "auto_patrol") == 0) targetActionId = 13;
        }

        if (targetActionId != -1 && actionHandler) actionHandler(targetActionId);
    }

    void reportStatus() {
        unsigned long now = millis();
        if (now - lastMsgTime > 3000) { 
            lastMsgTime = now;
            client.publish(TOPIC_PUB, "{\"status\":\"alive\"}");
        }
    }
};

#endif
