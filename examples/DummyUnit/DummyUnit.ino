/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/
#include <SoftwareSerial.h>
#include <DummyUnit.h>

#define RXD2 16
#define TXD2 17

SoftwareSerial controllerUart(RXD2, TXD2, true); //RX, TX
DummyUnit dummyUnit = DummyUnit(controllerUart);

void onConnectionEstablished() {}

void setup() {
    Serial.begin(115200);
    Serial.println("Dummy start");

    controllerUart.begin(9600);
    dummyUnit.setup();
}

void loop() {
    dummyUnit.loop();
}