/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

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

#include <Uart.h>
#include <IMqttBridge.h>

#include <NetworkUpdater.h>

namespace FujitsuAC {

    class FujitsuAC {
    	public:
            FujitsuAC(int rxPin, int txPin, int ledWPin, int ledRPin, int resetButtonPin);

			void setup();
	        void loop();

        private:
            Preferences preferences;
            NetworkServer server;

            Uart uart;
            
            WiFiClient espClient;
            PubSubClient mqttClient;

            IMqttBridge* bridge = nullptr;

            String uniqueId;
            String wifiSsid;
            String wifiPw;
            String mqttIp;
            String mqttPort;
            String mqttUser;
            String mqttPw;
            String deviceName;
            String otaPw;
            String protocol;

            int ledWPin;
            int ledRPin;
            int resetButtonPin;

            uint32_t fallbackApCreatedAt = 0;

            void generateUniqueId();
            void initIO();

            void loadConfig();
            void clearConfig();
            String getConfigValue(String qs, String key);
            String urlDecode(const String &s);
            void parseConfig(String content);

            void handleResetButton();

            bool isAPState();
            bool createAP();
            void handleHttp();

            void connectToWifi();
            void connectToMqtt();
    };
}