#include <Arduino.h>
#include <WiFi.h>
#include "Audio.h"

// 1. 设置你的 WiFi (必须支持2.4G)
#define SSID_NAME     "茁猫阿姨洗铁路"      // <--- 请修改这里
#define SSID_PASSWORD "12345678"      // <--- 请修改这里

// 2. 定义引脚 (根据我们刚才商量的)
#define I2S_DOUT      33
#define I2S_BCLK      25
#define I2S_LRC       26

Audio audio;

void setup() {
    Serial.begin(115200);

    // --- 连接 WiFi 部分 (之前缺的就是这块) ---
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_NAME, SSID_PASSWORD);
    
    Serial.println("正在连接 WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi 连接成功!");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());

    // --- 音频初始化 ---
    // 这里的顺序必须是: BCLK, LRC, DOUT
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // 把音量调大一点测试
    audio.setVolume(5); // 最大是21
    
    // 播放一个比较稳定的流 (MP3)
    // 如果这个不响，可以换下面的备用链接
    audio.connecttohost("http://music.163.com/song/media/outer/url?id=2747802379.mp3");
}

void loop() {
    audio.loop();
    
    // 在串口打印状态，方便调试
    // 如果串口有哗啦啦的数据输出，说明程序在运行
}

// 可选：添加一些回调函数看报错信息
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
