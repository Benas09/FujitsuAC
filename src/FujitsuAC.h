/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

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

#include <Config.h>
#include <Uart.h>
#include <IMqttBridge.h>

#if defined(CONFIG_IDF_TARGET_ESP32C3)
    #define UART_NUM UART_NUM_1
#else
    #define UART_NUM UART_NUM_2
#endif


namespace FujitsuAC {

    class FujitsuAC {
    	public:
            FujitsuAC(
                uart_port_t uartPort,
                int rxPin, 
                int txPin, 
                int ledWPin, 
                int ledRPin, 
                int resetButtonPin
            );

			void setup();
	        void loop();

        private:
            Config _config;
            int resetButtonPin;

            NetworkServer server;

            Uart uart;
            
            WiFiClient espClient;
            PubSubClient _mqttClient;

            IMqttBridge* bridge = nullptr;

            uint32_t fallbackApCreatedAt = 0;

            void initIO();

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