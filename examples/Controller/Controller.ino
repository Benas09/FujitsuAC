/*
FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
Copyright (c) 2025 Benas Ragauskas. All rights reserved.

Project home: https://github.com/Benas09/FujitsuAC
*/
#include <FujitsuAC.h>

#define RXD2 16
#define TXD2 17

/*
Uncomment any of these if you connected required hardware.
LED_R: 3V3 -> LED POSITIVE | LED NEGATIVE -> RESISTOR -> LED_R Pin (Used for WiFi indication)
LED_W: 3V3 -> LED POSITIVE | LED NEGATIVE -> RESISTOR -> LED_W Pin (Used for Mqtt indication)
RESET_BUTTON: When this pin is shorted to GND via push button, credentials are cleared and dongle reboots
*/

// #define LED_W 6
// #define LED_R 7
// #define RESET_BUTTON 20

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