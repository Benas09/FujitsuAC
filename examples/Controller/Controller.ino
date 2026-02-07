/*
FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
Copyright (c) 2025 Benas Ragauskas. All rights reserved.

Project home: https://github.com/Benas09/FujitsuAC
*/
#include <FujitsuAC.h>

#define RXD2 16
#define TXD2 17

#if CONFIG_IDF_TARGET_ESP32S3
    #define LED_W 6
    #define LED_R 7
    #define RESET_BUTTON 20
#elif CONFIG_IDF_TARGET_ESP32
    #define LED_W 18
    #define LED_R 19
    #define RESET_BUTTON 21
#endif

#ifndef LED_W
    #define LED_W -1
#endif

#ifndef LED_R
    #define LED_R -1
#endif

#ifndef RESET_BUTTON
    #define RESET_BUTTON -1
#endif

FujitsuAC::FujitsuAC fujitsuAC = FujitsuAC::FujitsuAC(RXD2, TXD2, LED_W, LED_R, RESET_BUTTON);

void setup() {
    fujitsuAC.setup();
}

void loop() {
    fujitsuAC.loop();
}