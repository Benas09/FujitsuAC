/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <WiFi.h>

#include <ESPmDNS.h>
#include <NetworkUdp.h>

#include <PubSubClient.h>

#include <SoftwareSerial.h>
#include <Buffer.h>

#define WIFI_SSID "your-ssid"
#define WIFI_PASSWORD "your-pw"
#define DEVICE_NAME "FujitsuSniffer"

#define MQTT_SERVER "192.168.1.100"
#define MQTT_PORT 1883

//mosquitto_sub -h 192.168.1.100 -t fujitsu/sniffer/# -v >> log.txt

SoftwareSerial controllerUart(16, 25, true); //RX, TX
FujitsuAC::Buffer controllerBuffer = FujitsuAC::Buffer(controllerUart);

SoftwareSerial acUart(17, 26, true); //RX, TX
FujitsuAC::Buffer acBuffer = FujitsuAC::Buffer(acUart);

WiFiClient espClient;
PubSubClient mqttClient = PubSubClient(espClient);

void setup() {
    WiFi.setHostname(DEVICE_NAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(1024);

    controllerBuffer.setup();
    controllerUart.begin(9600);

    acBuffer.setup();
    acUart.begin(9600);
}

void onControllerFrame(uint8_t buffer[128], int size, bool isValid) {
    String msg = String("");

    for (int i = 0; i < size; i++) {
        msg += String(buffer[i], HEX);
        if (i < size - 1) msg += " ";
    }

    msg.toUpperCase();

    mqttClient.publish("fujitsu/sniffer/tx", msg.c_str());
}

void onAcFrame(uint8_t buffer[128], int size, bool isValid) {
    String msg = String("");

    for (int i = 0; i < size; i++) {
        msg += String(buffer[i], HEX);
        if (i < size - 1) msg += " ";
    }

    msg.toUpperCase();

    mqttClient.publish("fujitsu/sniffer/rx", msg.c_str());
}

void reconnect() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(DEVICE_NAME)) {
        mqttClient.publish("fujitsu/sniffer/debug", "Connected");
    }
  }
}

void loop() {
    if (!mqttClient.connected()) {
      reconnect();
    }

    controllerBuffer.loop(onControllerFrame);
    acBuffer.loop(onAcFrame);
}