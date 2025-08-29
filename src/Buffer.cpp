/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "Buffer.h"

namespace FujitsuAC {

    Buffer::Buffer(Stream &uart): uart(uart) {}

    bool Buffer::loop(std::function<void(uint8_t buffer[128], int size, bool isValid)> callback) {
        int previousIndex = this->currentIndex;

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
        
        if (previousIndex != this->currentIndex && this->debugCallback) {
            this->debugCallback("buffer", this->getCurrentBufferAsHexString());
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

    const char* Buffer::getCurrentBufferAsHexString() {
        static char hexStr[384];
        int offset = 0;

        for (int i = 0; i < this->currentIndex && offset < sizeof(hexStr) - 3; ++i) {
            offset += snprintf(
                hexStr + offset, 
                sizeof(hexStr) - offset,
                (i < this->currentIndex - 1) 
                    ? "%02X " 
                    : "%02X", buffer[i]
            );
        }

        return hexStr;
    }

    void Buffer::setDebugCallback(std::function<void(const char* name, const char* message)> debugCallback) {
        this->debugCallback = debugCallback;
    }

}