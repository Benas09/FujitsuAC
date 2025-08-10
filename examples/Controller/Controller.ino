/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/
#include <WiFi.h>

#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

#include <PubSubClient.h>

#include <SoftwareSerial.h>
#include <FujitsuController.h>
#include <MqttBridge.h>


#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-pw"
#define DEVICE_NAME "OfficeAC"
#define OTA_PASSWORD "office_ac"

#define MQTT_SERVER "192.168.1.100"
#define MQTT_PORT 1883

#define RXD2 16
#define TXD2 17

SoftwareSerial uart(RXD2, TXD2, true); //RX, TX
FujitsuAC::FujitsuController controller = FujitsuAC::FujitsuController(uart);
FujitsuAC::MqttBridge* bridge = nullptr;

WiFiClient espClient;
PubSubClient mqttClient = PubSubClient(espClient);
String uniqueId = "000000000000";

void setup() {
    Serial.begin(115200);

    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    uart.begin(9600);
    controller.setup();

    WiFi.setHostname(DEVICE_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    ArduinoOTA.setHostname(DEVICE_NAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.begin();

    uniqueId = WiFi.macAddress();
    uniqueId.replace(":", "");
    uniqueId.toLowerCase();

    Serial.print("UniqueId: "); 
    Serial.println(uniqueId);

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(1024);
}

void reconnect() {
    while (!mqttClient.connected()) {
        Serial.print("Connecting to MQTT...");

        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/status", uniqueId);

        if (mqttClient.connect(DEVICE_NAME, topic, 0, true, "offline")) {
            if (nullptr == bridge) {
                bridge = new FujitsuAC::MqttBridge(mqttClient, controller, uniqueId.c_str(), DEVICE_NAME, false);
            }

            bridge->setup();
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            delay(5000);
        }
    }
}

void loop() {
    ArduinoOTA.handle();

    if (!mqttClient.connected()) {
        reconnect();
    }

    mqttClient.loop();
    controller.loop();
}
