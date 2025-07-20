/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <SoftwareSerial.h>
#include "EspMQTTClient.h"
#include <DummyUnit.h>

//mosquitto_sub -h 192.168.1.100 -t fujitsu/dummy/#

EspMQTTClient client = EspMQTTClient(
    "your-ssid",
    "your-password",
    "192.168.1.100",
    "",
    "",
    "FujitsuDummy",
    1883
);

SoftwareSerial controllerUart(16, 17, true); //RX, TX
DummyUnit dummyUnit = DummyUnit(controllerUart, client);

void setup() {
    Serial.begin(9600);

    while (!Serial) {}

    client.enableOTA();
}

void onConnectionEstablished()
{
    controllerUart.begin(9600);
    dummyUnit.setup();
}

void loop() {
    client.loop();

    dummyUnit.loop();
}
