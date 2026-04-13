/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/
#pragma once
#include "FujitsuAC.h"
#include "TFSXW1Bridge.h"
// #include "TFSXJ4Bridge.h"

#define VERSION "1.3.0"

RTC_NOINIT_ATTR bool isFallbackAp;

namespace FujitsuAC {

    FujitsuAC::FujitsuAC(int rxPin, int txPin, int ledWPin, int ledRPin, int resetButtonPin): 
        _config(VERSION, ledWPin, ledRPin),
        server(80),
        uart(UART_NUM_2, rxPin, txPin),
        espClient(),
        mqttClient(espClient),
        ledWPin(ledWPin),
        ledRPin(ledRPin),
        resetButtonPin(resetButtonPin)
    {}

    void FujitsuAC::setup() {
        this->initIO();
        this->handleResetButton();
        this->generateUniqueId();

        if (this->createAP()) {
            return;
        }

        this->connectToWifi();

        ArduinoOTA.setHostname(deviceName.c_str());
        ArduinoOTA.setPassword(otaPw.c_str());
        ArduinoOTA.begin();

        this->mqttClient.setServer(mqttIp.c_str(), (uint16_t) mqttPort.toInt());
        this->mqttClient.setBufferSize(2048);
    }

    void FujitsuAC::loop() {
        this->handleResetButton();

        if (this->isAPState()) {
            this->handleHttp();

            if (isFallbackAp && millis() - fallbackApCreatedAt > 300000) {
                isFallbackAp = false;

                ESP.restart();
            }

            return;
        }

        if (WiFi.status() != WL_CONNECTED) {
            ESP.restart();
        }

        ArduinoOTA.handle();

        this->connectToMqtt();

        this->mqttClient.loop();
        this->bridge->loop();
    }

    void FujitsuAC::generateUniqueId() {
        char buf[13];
        snprintf(buf, sizeof(buf), "%012llX", ESP.getEfuseMac());

        String uniqueId = buf;
        uniqueId.toLowerCase();

        _config.setUniqueId(uniqueId);
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

    void FujitsuAC::clearConfig() {
        _config.clear();

        isFallbackAp = false;

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
        _config.setValue("wifi-ssid", getConfigValue(content, "wifi-ssid"));
        _config.setValue("wifi-pw", getConfigValue(content, "wifi-pw")); 
        _config.setValue("mqtt-ip", getConfigValue(content, "mqtt-ip")); 
        _config.setValue("mqtt-port", getConfigValue(content, "mqtt-port")); 
        _config.setValue("mqtt-user", getConfigValue(content, "mqtt-user")); 
        _config.setValue("mqtt-pw", getConfigValue(content, "mqtt-pw")); 
        _config.setValue("device-name", getConfigValue(content, "device-name")); 
        _config.setValue("ota-pw", getConfigValue(content, "ota-pw"));
        _config.setValue("protocol", getConfigValue(content, "protocol"));

        isFallbackAp = false;
    }

    void FujitsuAC::handleResetButton() {
        if (resetButtonPin > 0 && LOW == digitalRead(resetButtonPin)) {
            this->clearConfig();
        }
    }

    bool FujitsuAC::isAPState() {
        return isFallbackAp || _config.isEmpty();
    }

    bool FujitsuAC::createAP() {
        if (ESP_RST_POWERON == esp_reset_reason()) {
            isFallbackAp = false;
        }

        if (!this->isAPState()) {
            return false;
        }

        IPAddress apIP(192,168,1,1);
        IPAddress apGateway(192,168,1,1);
        IPAddress apSubnet(255,255,255,0);

        WiFi.softAPConfig(apIP, apGateway, apSubnet);

        char accessPointName[64];
        snprintf(accessPointName, sizeof(accessPointName), "faircon-%s", _config.getUniqueId().c_str());

        if (!WiFi.softAP(accessPointName)) {
            ESP.restart();
        }

        this->server.begin();

        if (isFallbackAp) {
            fallbackApCreatedAt = millis();
        }

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
                WiFi.SSID(i) == _config.getWifiSsid()
                && rssi > bestRSSI
            ) {
                bestRSSI = rssi;
                bestNetwork = i;
            }
        }

        if (-1 != bestNetwork) {
            uint8_t* bestBssid = WiFi.BSSID(bestNetwork);
            int channel = WiFi.channel(bestNetwork);

            WiFi.begin(_config.getWifiSsid(), _config.getWifiPw(), channel, bestBssid, true);
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

            if (millis() - start > 60000) {
                isFallbackAp = true;

                ESP.restart();
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
            snprintf(topic, sizeof(topic), "fujitsu/%s/status", _config.getUniqueId().c_str());

            bool connected = false;

            if (_config.getMqttUser() == "") {
                connected = mqttClient.connect(_config.getDeviceName().c_str(), topic, 0, true, "offline");
            } else {
                connected = mqttClient.connect(_config.getDeviceName().c_str(), _config.getMqttUser().c_str(), _config.getMqttPw().c_str(), topic, 0, true, "offline");
            }

            if (connected) {
                if (ledWPin > 0) {
                    ledcWrite(ledWPin, 1020);
                }

                if (nullptr == bridge) {
                    if (_config.getProtocol() == "UTY-TFSXJ4") {
                        // bridge = new TFSXJ4Bridge(_config, mqttClient, uart);
                        // bridge->setup();
                    } else {
                        bridge = new TFSXW1Bridge(_config, mqttClient, uart);
                        bridge->setup();
                    }
                }
            } else {
                this->handleResetButton();

                delay(5000);
            }
        }
    }

    void FujitsuAC::handleHttp() {
        NetworkClient client = this->server.accept();

        if (client) {
            const char *contentStart = R"rawliteral(
                <html>
                    <head>
                        <title>faircon</title>

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

                            h3 {
                                text-align: center;
                                margin-bottom: 20px;
                                font-size: 18px;
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

                            input[type="text"], select {
                                width: 100%;
                                padding: 8px 10px;
                                margin-bottom: 14px;
                                border: 1px solid #ccc;
                                border-radius: 4px;
                                font-size: 14px;
                            }

                            input[type="text"]:focus, select:focus {
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
                        <h1>faircon</h1>
            )rawliteral";

            const char *formBody = R"rawliteral(
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
                    
                    <label>Protocol</label>
                    <select name="protocol">
                        <option value="UTY-TFSXW1">UTY-TFSXW1</option>
                        <option value="UTY-TFSXJ4">UTY-TFSXJ4</option>
                    </select>

                    <input type="submit" value="Submit">
                </form>
            )rawliteral";

            const char *successBody = R"rawliteral(
                <h3>Config saved successfuly!</h3>
            )rawliteral";

            const char *contentEnd = R"rawliteral(
                        <br/>
                        <span><a href="https://faircon.lt">faircon</a></span>
                    </body>
                </html>
            )rawliteral";

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
                            
                            client.print(contentStart);
                            client.print(successBody);
                            client.print(contentEnd);

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

                client.print(contentStart);
                client.print(formBody);
                client.print(contentEnd);

                client.println();
            }

            client.stop();
        }
    }
}