#ifndef WIFI_SYSTEM_H
#define WIFI_SYSTEM_H

#include <WiFi.h>
#include "Config.h"

class WifiSystem {
private:
    unsigned long lastCheckTime = 0;

public:
    void init() {
        // 设置为 Station 模式
        WiFi.mode(WIFI_STA);
        
        Serial.println();
        Serial.print("[WiFi] Connecting to: ");
        Serial.println(WIFI_SSID);

        // ★★★ 显式使用账号密码连接 ★★★
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        // 等待连接 (阻塞式，最多等10秒)
        int retry = 0;
        while (WiFi.status() != WL_CONNECTED && retry < 20) {
            delay(500);
            Serial.print(".");
            retry++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WiFi] Connected!");
            Serial.print("[WiFi] IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\n[WiFi] Connection Failed! Check password.");
        }
    }

    // 在 loop 中调用，负责掉线自动重连
    void maintain() {
        // 每 5 秒检查一次状态
        if (millis() - lastCheckTime > 5000) {
            lastCheckTime = millis();
            
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WiFi] Lost connection. Reconnecting...");
                WiFi.disconnect();
                WiFi.reconnect();
            }
        }
    }
};

#endif
