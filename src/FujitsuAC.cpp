/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/
#include "FujitsuAC.h"

#define VERSION "1.1.5"

namespace FujitsuAC {

    FujitsuAC::FujitsuAC(int rxPin, int txPin, int ledWPin, int ledRPin, int resetButtonPin): 
        preferences(),
        server(80),
        uart(UART_NUM_2, rxPin, txPin),
        controller(uart),
        espClient(),
        mqttClient(espClient),
        ledWPin(ledWPin),
        ledRPin(ledRPin),
        resetButtonPin(resetButtonPin)
    {}

    void FujitsuAC::setup() {
        this->generateUniqueId();
        this->initIO();
        this->loadConfig();
        this->handleResetButton();

        if (this->createAP()) {
            return;
        }

        this->controller.setup();
        this->connectToWifi();

        ArduinoOTA.setHostname(deviceName.c_str());
        ArduinoOTA.setPassword(otaPw.c_str());
        ArduinoOTA.begin();

        this->mqttClient.setServer(mqttIp.c_str(), (uint16_t) mqttPort.toInt());
        this->mqttClient.setBufferSize(2048);
    }

    void FujitsuAC::loop() {
        this->handleResetButton();

        if (wifiSsid == "") {
            this->handleHttp();

            return;
        }

        if (WiFi.status() != WL_CONNECTED) {
            ESP.restart();
        }

        ArduinoOTA.handle();

        this->connectToMqtt();

        this->mqttClient.loop();
        this->bridge->loop();
        this->controller.loop();
    }

    void FujitsuAC::generateUniqueId() {
        char buf[13];
        snprintf(buf, sizeof(buf), "%012llX", ESP.getEfuseMac());

        uniqueId = buf;
        uniqueId.toLowerCase();
    }

    void FujitsuAC::initIO() {
        if (ledRPin > 0) {
            ledcAttach(ledRPin, 12000, 10);
            ledcWrite(ledRPin, 1020);
        }

        if (ledWPin > 0) {
            ledcAttach(ledWPin, 12000, 10);
            ledcWrite(ledWPin, 1023);
        }

        if (resetButtonPin > 0) {
            pinMode(resetButtonPin, INPUT_PULLUP);
        }
    }

    void FujitsuAC::loadConfig() {
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

    void FujitsuAC::clearConfig() {
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

    String FujitsuAC::getConfigValue(String qs, String key) {
        int start = qs.indexOf(key + "=");
        if (start == -1) return "";

        start += key.length() + 1;
        int end = qs.indexOf("&", start);
        if (end == -1) end = qs.length();

        return this->urlDecode(qs.substring(start, end));
    }

    String FujitsuAC::urlDecode(const String &s) {
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

    void FujitsuAC::parseConfig(String content) {
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

    void FujitsuAC::handleResetButton() {
        if (resetButtonPin > 0 && LOW == digitalRead(resetButtonPin)) {
            this->clearConfig();
        }
    }

    bool FujitsuAC::createAP() {
        if (wifiSsid != "") {
            return false;
        }

        IPAddress apIP(192,168,1,1);
        IPAddress apGateway(192,168,1,1);
        IPAddress apSubnet(255,255,255,0);

        WiFi.softAPConfig(apIP, apGateway, apSubnet);

        char accessPointName[64];
        snprintf(accessPointName, sizeof(accessPointName), "FujitsuAC-%s", uniqueId.c_str());

        if (!WiFi.softAP(accessPointName)) {
            ESP.restart();
        }

        this->server.begin();

        return true;
    }

    void FujitsuAC::connectToWifi()
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
            this->handleResetButton();
    
            if (ledRPin > 0) {
                ledcWrite(ledRPin, 1023);
                delay(500);

                ledcWrite(ledRPin, 1020);
                delay(500);
            } else {
                delay(50);
            }

            if (millis() - start > 600000) {
                this->clearConfig();
            }
        }
    }

    void FujitsuAC::connectToMqtt() {
        while (!mqttClient.connected()) {
            if (WiFi.status() != WL_CONNECTED) {
                ESP.restart();
            }

            if (ledWPin > 0) {
                ledcWrite(ledWPin, 1023);
            }

            char topic[64];
            snprintf(topic, sizeof(topic), "fujitsu/%s/status", uniqueId.c_str());

            bool connected = false;

            if (mqttUser == "") {
                connected = mqttClient.connect(deviceName.c_str(), topic, 0, true, "offline");
            } else {
                connected = mqttClient.connect(deviceName.c_str(), mqttUser.c_str(), mqttPw.c_str(), topic, 0, true, "offline");
            }

            if (connected) {
                if (ledWPin > 0) {
                    ledcWrite(ledWPin, 1020);
                }

                if (nullptr == bridge) {
                    bridge = new MqttBridge(mqttClient, controller, uniqueId.c_str(), deviceName.c_str(), VERSION);
                }

                bridge->setup();
            } else {
                delay(5000);
            }
        }
    }

    void FujitsuAC::handleHttp() {
        NetworkClient client = this->server.accept();

        if (client) {
            if (client.connected()) {
                String firstLine = client.readStringUntil('\n');
                
                if (firstLine.startsWith("POST /")) {
                    String body = "";

                    while (client.available()) {
                        body = client.readStringUntil('\n');
                        body.trim();

                        if (body.startsWith("wifi-ssid")) {
                            this->parseConfig(body);

                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println();
                            client.println("Saved");
                            client.println();

                            client.flush();
                            client.stop();
                            delay(200);

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
}