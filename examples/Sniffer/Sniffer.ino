/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <SoftwareSerial.h>
#include <Buffer.h>
#include "EspMQTTClient.h"

//mosquitto_sub -h 192.168.1.100 -t fujitsu/sniffer/# -v >> log.txt

EspMQTTClient client = EspMQTTClient(
    "your-ssid",
    "your-password",
    "192.168.1.100",
    "",
    "",
    "FujitsuSniffer",
    1883
);

SoftwareSerial controllerUart(17, 26, true); //RX, TX
Buffer controllerBuffer = Buffer(controllerUart);

SoftwareSerial acUart(16, 25, true); //RX, TX
Buffer acBuffer = Buffer(acUart);

void setup() {
    Serial.begin(9600);

    while (!Serial) {}

    client.enableOTA();

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

    client.publish("fujitsu/sniffer/controller", msg);
}

void onAcFrame(uint8_t buffer[128], int size, bool isValid) {
    String msg = String("");

    for (int i = 0; i < size; i++) {
        msg += String(buffer[i], DEC);
        if (i < size - 1) msg += " ";
    }

    msg.toUpperCase();

    client.publish("fujitsu/sniffer/ac", msg);
}

void onConnectionEstablished()
{
    client.publish("fujitsu/sniffer", "Connection established");
}

void loop() {
    client.loop();

    controllerBuffer.loop(onControllerFrame);
    acBuffer.loop(onAcFrame);
}
