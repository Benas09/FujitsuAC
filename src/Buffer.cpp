/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "Buffer.h"

namespace FujitsuAC {

    Buffer::Buffer(Stream &uart): uart(uart) {}

    bool Buffer::loop(std::function<void(uint8_t buffer[128], int size, bool isValid)> callback) {
        while (this->uart.available()) {
            uint8_t b = uart.read();
            uint32_t now = millis();

            if ((now - this->lastMillis) >= 20) {
                this->currentIndex = 0;
            }

            this->lastMillis = now;
            this->buffer[this->currentIndex] = b;
            this->currentIndex++;

            if (this->currentIndex > 4 && this->currentIndex == (int) this->buffer[4] + 7) {
                if (callback) {
                    callback(
                        this->buffer, (int) this->buffer[4] + 7,
                        this->isValidFrame(this->buffer, (int) this->buffer[4] + 7)
                    );
                }
            }
        }
        
        return true;
    }

    bool Buffer::isValidFrame(uint8_t buffer[128], int size) {
        uint16_t frameChecksum = (buffer[size - 2] << 8) | buffer[size - 1];
        
        uint16_t checksum = 0xFFFF; 
        
        for (int i = 0; i < size - 2; i++) {
            checksum -= buffer[i];
        }
        
        return frameChecksum == checksum;
    }

}