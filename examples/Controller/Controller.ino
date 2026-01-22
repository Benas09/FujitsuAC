/*
FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
Copyright (c) 2025 Benas Ragauskas. All rights reserved.

Project home: https://github.com/Benas09/FujitsuAC
*/
//EEPROM
#include <Preferences.h>

//WiFi
#include <WiFi.h>

// Access point
#include <NetworkClient.h>
#include <WiFiAP.h>

//OTA
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

//MQTT
#include <PubSubClient.h>

#define HARDWARE_UART 1 //0 for SoftwareSerial

#if HARDWARE_UART
#include <Uart.h>
#else
#include <SoftwareSerial.h>
#endif

#include <FujitsuController.h>
#include <MqttBridge.h>

#define RXD2 16
#define TXD2 17

// #define LED_W 6
// #define LED_R 7
// #define RESET_BUTTON 20

String uniqueId;

String wifiSsid;
String wifiPw;
String mqttIp;
String mqttPort;
String mqttUser;
String mqttPw;
String deviceName;
String otaPw;

Preferences preferences;
NetworkServer server(80);

#if HARDWARE_UART
FujitsuAC::Uart uart = FujitsuAC::Uart(UART_NUM_2, RXD2, TXD2); //RX, TX
#else
SoftwareSerial uart(RXD2, TXD2, true); //RX, TX
#endif

FujitsuAC::FujitsuController controller = FujitsuAC::FujitsuController(uart);
FujitsuAC::MqttBridge* bridge = nullptr;

WiFiClient espClient;
PubSubClient mqttClient = PubSubClient(espClient);

void generateUniqueId() {
    char buf[13];
    snprintf(buf, sizeof(buf), "%012llX", ESP.getEfuseMac());
    uniqueId = buf;
    uniqueId.toLowerCase();
}

void initIO() {
#ifdef LED_R
    ledcAttach(LED_R, 12000, 8);
    ledcWrite(LED_R, 253);
#endif

#ifdef LED_W
    ledcAttach(LED_W, 12000, 8);
    ledcWrite(LED_W, 255);
#endif

#ifdef RESET_BUTTON
    pinMode(RESET_BUTTON, INPUT_PULLUP);
#endif
}

void loadPreferences() {
    preferences.begin("fujitsu_ac", false);

    wifiSsid = preferences.getString("wifi-ssid", "");
    wifiPw = preferences.getString("wifi-pw", "");
    mqttIp = preferences.getString("mqtt-ip", "");
    mqttPort = preferences.getString("mqtt-port", "");
    mqttUser = preferences.getString("mqtt-user", "");
    mqttPw = preferences.getString("mqtt-pw", "");
    deviceName = preferences.getString("device-name", "");
    otaPw = preferences.getString("ota-pw", "");

    preferences.end();
}

void createAP() {
    IPAddress apIP(192,168,1,1);
    IPAddress apGateway(192,168,1,1);
    IPAddress apSubnet(255,255,255,0);

    WiFi.softAPConfig(apIP, apGateway, apSubnet);

    char accessPointName[64];
    snprintf(accessPointName, sizeof(accessPointName), "FujitsuAC-%s", uniqueId.c_str());

    if (!WiFi.softAP(accessPointName)) {
        ESP.restart();
    }

    server.begin();
}

void connectToWifi()
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    uint32_t start = millis();

    WiFi.disconnect(true, true);
    WiFi.setHostname(deviceName.c_str());
    WiFi.mode(WIFI_STA);

    int bestNetwork = -1;
    int bestRSSI = -1000;

    int networkCount = WiFi.scanNetworks();

    for (int i = 0; i < networkCount; i++) {
        uint8_t* bssid = WiFi.BSSID(i);
        int rssi = WiFi.RSSI(i);

        if (
            WiFi.SSID(i) == wifiSsid
            && rssi > bestRSSI
        ) {
            bestRSSI = rssi;
            bestNetwork = i;
        }
    }

    if (-1 != bestNetwork) {
        uint8_t* bestBssid = WiFi.BSSID(bestNetwork);
        int channel = WiFi.channel(bestNetwork);

        WiFi.begin(wifiSsid, wifiPw, channel, bestBssid, true);
    }

    while (WiFi.status() != WL_CONNECTED) {
        handleResetButton();
#ifdef LED_R
        ledcWrite(LED_R, 255);
        delay(500);
        ledcWrite(LED_R, 253);
        delay(500);
#else
        delay(50);
#endif

        if (millis() - start > 600000) {
            clearConfig();
        }
    }
}

void setup() {
    generateUniqueId();
    initIO();
    loadPreferences();
    handleResetButton();

    if (wifiSsid == "") {
        createAP();
        return;
    }
    

#if HARDWARE_UART != 1
    uart.begin(9600);
#endif

    controller.setup();

    connectToWifi();

    ArduinoOTA.setHostname(deviceName.c_str());
    ArduinoOTA.setPassword(otaPw.c_str());
    ArduinoOTA.begin();

    mqttClient.setServer(mqttIp.c_str(), (uint16_t) mqttPort.toInt());
    mqttClient.setBufferSize(2048);
}

void reconnect() {
    while (!mqttClient.connected()) {
#ifdef LED_W
        ledcWrite(LED_W, 255);
#endif

        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/status", uniqueId.c_str());

        bool connected = false;

        if (mqttUser == "") {
            connected = mqttClient.connect(deviceName.c_str(), topic, 0, true, "offline");
        } else {
            connected = mqttClient.connect(deviceName.c_str(), mqttUser.c_str(), mqttPw.c_str(), topic, 0, true, "offline");
        }

        if (connected) {
#ifdef LED_W
            ledcWrite(LED_W, 254);
#endif
            if (nullptr == bridge) {
                bridge = new FujitsuAC::MqttBridge(mqttClient, controller, uniqueId.c_str(), deviceName.c_str());
            }

            bridge->setup();
        } else {
            delay(5000);
        }
    }
}

void loop() {
    handleResetButton();

    if (wifiSsid == "") {
        handleHttp();
        return;
    }

    ArduinoOTA.handle();

    if (!mqttClient.connected()) {
        reconnect();
    }

    mqttClient.loop();
    bridge->loop();
    controller.loop();
}

String urlDecode(const String &s) {
    String out;
    out.reserve(s.length());

    for (int i = 0; i < s.length(); i++) {
        char c = s[i];

        if (c == '+') {
            out += ' ';
        } else if (c == '%' && i + 2 < s.length()) {
            char h1 = s[i + 1];
            char h2 = s[i + 2];

            int v1 = isdigit(h1) ? h1 - '0' : toupper(h1) - 'A' + 10;
            int v2 = isdigit(h2) ? h2 - '0' : toupper(h2) - 'A' + 10;

            out += char((v1 << 4) | v2);
            i += 2;
        } else {
            out += c;
        }
    }
    
    return out;
}

String getConfigValue(String qs, String key) {
    int start = qs.indexOf(key + "=");
    if (start == -1) return "";

    start += key.length() + 1;
    int end = qs.indexOf("&", start);
    if (end == -1) end = qs.length();

    return urlDecode(qs.substring(start, end));
}

void parseConfig(String content) {
    preferences.begin("fujitsu_ac", false);

    preferences.putString("wifi-ssid", getConfigValue(content, "wifi-ssid")); 
    preferences.putString("wifi-pw", getConfigValue(content, "wifi-pw")); 
    preferences.putString("mqtt-ip", getConfigValue(content, "mqtt-ip")); 
    preferences.putString("mqtt-port", getConfigValue(content, "mqtt-port")); 
    preferences.putString("mqtt-user", getConfigValue(content, "mqtt-user")); 
    preferences.putString("mqtt-pw", getConfigValue(content, "mqtt-pw")); 
    preferences.putString("device-name", getConfigValue(content, "device-name")); 
    preferences.putString("ota-pw", getConfigValue(content, "ota-pw"));

    preferences.end();
}

void clearConfig() {
    preferences.begin("fujitsu_ac", false);

    preferences.putString("wifi-ssid", ""); 
    preferences.putString("wifi-pw", ""); 
    preferences.putString("mqtt-ip", ""); 
    preferences.putString("mqtt-port", ""); 
    preferences.putString("mqtt-user", ""); 
    preferences.putString("mqtt-pw", ""); 
    preferences.putString("device-name", ""); 
    preferences.putString("ota-pw", "");

    preferences.end();

    delay(1000);
    ESP.restart();
}

void handleResetButton() {
#ifdef RESET_BUTTON
    if (LOW == digitalRead(RESET_BUTTON)) {
        clearConfig();
    }
#endif
}

void handleHttp() {
    NetworkClient client = server.accept();

    if (client) {
        if (client.connected()) {
            String firstLine = client.readStringUntil('\n');
            
            if (firstLine.startsWith("POST /")) {
                String body = "";

                while (client.available()) {
                    body = client.readStringUntil('\n');
                    body.trim();

                    if (body.startsWith("wifi-ssid")) {
                        parseConfig(body);

                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.println("Saved");
                        client.println();

                        ESP.restart();
                    }
                }
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            const char *content = R"rawliteral(
                <html
                    <head>
                        <title>FujitsuAC Config</title>

                        <style>
                            * {
                                box-sizing: border-box;
                                font-family: Arial, Helvetica, sans-serif;
                            }

                            body {
                                background: #f4f6f8;
                                margin: 0;
                                padding: 20px;
                                color: #333;
                            }

                            h1 {
                                text-align: center;
                                margin-bottom: 20px;
                                font-size: 24px;
                            }

                            form {
                                max-width: 420px;
                                margin: 0 auto;
                                background: #ffffff;
                                padding: 20px;
                                border-radius: 6px;
                                box-shadow: 0 2px 6px rgba(0,0,0,0.1);
                            }

                            label {
                                display: block;
                                font-weight: 600;
                                margin-bottom: 4px;
                                font-size: 14px;
                            }

                            input[type="text"] {
                                width: 100%;
                                padding: 8px 10px;
                                margin-bottom: 14px;
                                border: 1px solid #ccc;
                                border-radius: 4px;
                                font-size: 14px;
                            }

                            input[type="text"]:focus {
                                outline: none;
                                border-color: #4a90e2;
                            }

                            input[type="submit"] {
                                width: 100%;
                                padding: 10px;
                                background: #4a90e2;
                                border: none;
                                border-radius: 4px;
                                color: #fff;
                                font-size: 15px;
                                font-weight: 600;
                                cursor: pointer;
                            }

                            input[type="submit"]:hover {
                                background: #3b7dc4;
                            }

                            span {
                                display: block;
                                text-align: center;
                                margin-top: 16px;
                                font-size: 13px;
                            }

                            a {
                                color: #4a90e2;
                                text-decoration: none;
                            }

                            a:hover {
                                text-decoration: underline;
                            }
                        </style>

                        <meta name="viewport" content="width=device-width, initial-scale=1.0">
                    </head>
                    <body>
                        <h1>FujitsuAC</h1>

                        <form name="config" method="post">
                            <label>Wifi SSID</label>
                            <input type="text" name="wifi-ssid" required>

                            <label>Wifi password</label>
                            <input type="text" name="wifi-pw" required>

                            <label>MQTT Server IP</label>
                            <input type="text" name="mqtt-ip" value="192.168.1.100" required>

                            <label>MQTT Server port</label>
                            <input type="text" name="mqtt-port" value="1883" required>

                            <label>MQTT User</label>
                            <input type="text" name="mqtt-user">

                            <label>MQTT Password</label>
                            <input type="text" name="mqtt-pw">

                            <label>Device name</label>
                            <input type="text" name="device-name" value="LivingRoomAC" required>

                            <label>Device password</label>
                            <input type="text" name="ota-pw" value="living_room_ac" required>

                            <input type="submit" value="Submit">
                        </form>

                        <br/>
                        <span><a href="https://github.com/Benas09/FujitsuAC">Fujitsu AC</a></span>
                    </body>
                </html>
            )rawliteral";

            client.print(content);
            client.println();
        }

        client.stop();
    }
}