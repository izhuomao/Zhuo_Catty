#ifndef CONFIG_H
#define CONFIG_H

// ================= 1. 硬件引脚定义 =================
// OLED (SPI) - 对应你之前测试成功的引脚
#define PIN_OLED_MOSI   23  // D1
#define PIN_OLED_CLK    18  // D0
#define PIN_OLED_DC     16  // DC
#define PIN_OLED_RES    4   // RES
#define PIN_OLED_CS     5   // CS

// 其他引脚保持不变...
#define PIN_I2C_SDA     21
#define PIN_I2C_SCL     22
#define PIN_I2S_BCLK    25
#define PIN_I2S_LRC     26
#define PIN_I2S_DIN     33
#define PIN_I2S_MIC_SCK 14
#define PIN_I2S_MIC_WS  15
#define PIN_I2S_MIC_SD  32
#define PIN_RGB_R       13
#define PIN_RGB_G       19
#define PIN_RGB_B       27
#define PIN_TRIG        2
#define PIN_ECHO        34

// ================= 2. 网络与服务配置 =================
#define WIFI_SSID       "茁猫阿姨洗铁路"
#define WIFI_PASS       "12345678"

// 心知天气 API
#define WEATHER_API_KEY "S0ByFwl5YUl7YNLY7" 
#define WEATHER_CITY    "hangzhou"

// ================= MQTT 配置 (阿里云) =================
#define MQTT_SERVER     "a1k6uay65rc.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "a1k6uay65rc.Catty01|securemode=2,signmethod=hmacsha256,timestamp=1770605465276|"
#define MQTT_USER       "Catty01&a1k6uay65rc"
#define MQTT_PASS       "5174c8b0140c1cad663963ff639a8013525ee5f41b6a882e772233fc3ccc7fa1"

// Topic (请确保与云端规则一致)
#define TOPIC_SUB       "/a1k6uay65rc/Catty01/user/control"  // 收指令
#define TOPIC_PUB       "/a1k6uay65rc/Catty01/user/status"   // 发状态

#endif
