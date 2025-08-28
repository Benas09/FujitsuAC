/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <Arduino.h>
#include "driver/uart.h"

namespace FujitsuAC {
    class Uart : public Stream {
        public:
            Uart(uart_port_t port, int rxPin, int txPin);

            int available() override;
            int read() override;
            int peek() override;
            void flush() override;

            size_t write(uint8_t byte) override;
            size_t write(const uint8_t* buffer, size_t size) override;
        private:
            uart_port_t _uart_port;
    };
}