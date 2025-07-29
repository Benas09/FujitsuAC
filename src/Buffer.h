/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <SoftwareSerial.h>

#pragma once

namespace FujitsuAC {

    class Buffer {
        public:
            Buffer(Stream &uart);

            bool setup();
            bool loop(std::function<void(uint8_t buffer[128], int size, bool isValid)> callback);

        private:
            Stream &uart;

            uint32_t lastMillis = 0;
            uint8_t buffer[128];
            int currentIndex = 0;

            bool isValidFrame(uint8_t buffer[128], int size);
    };

}