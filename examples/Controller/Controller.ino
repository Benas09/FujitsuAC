/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <SoftwareSerial.h>
#include "EspMQTTClient.h"
#include <FujitsuController.h>
#include <MqttBridge.h>

String getUniqueId() {
    String mac = WiFi.macAddress();

    mac.replace(":", "");
    mac.toLowerCase();

    return mac;
}

const char* name = "OfficeAC";

EspMQTTClient client = EspMQTTClient(
    "your-ssid",
    "your-password",
    "192.168.1.100",
    "",
    "",
    name,
    1883
);

SoftwareSerial uart(16, 17, true); //RX, TX
FujitsuController controller = FujitsuController(uart);
MqttBridge bridge = MqttBridge(client, controller, getUniqueId().c_str(), name);

void setup() {
    pinMode(17, LOW);
    Serial.begin(9600);

    while (!Serial) {}

    client.enableOTA();
    client.setMaxPacketSize(1024);

    String topic = "fujitsu/" + getUniqueId() + "/status";
    client.enableLastWillMessage(topic.c_str(), "offline", true);
}

void onConnectionEstablished()
{
    uart.begin(9600);
    bridge.setup();
}

void loop() {
    client.loop();
    controller.loop();
}