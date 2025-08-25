/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "Uart.h"

namespace FujitsuAC {

    Uart::Uart(uart_port_t port, int rxPin, int txPin) : _uart_port(port) {
        const uart_config_t uart_config = {
          .baud_rate = 9600,
          .data_bits = UART_DATA_8_BITS,
          .parity    = UART_PARITY_DISABLE,
          .stop_bits = UART_STOP_BITS_1,
          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
          .source_clk = UART_SCLK_APB,
        };

        uart_driver_install(_uart_port, 1024, 0, 0, NULL, 0);
        uart_param_config(_uart_port, &uart_config);
        uart_set_pin(_uart_port, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_set_line_inverse(_uart_port, UART_SIGNAL_TXD_INV | UART_SIGNAL_RXD_INV);
    }

    int Uart::available() {
        size_t bytesInBuffer;
        uart_get_buffered_data_len(_uart_port, &bytesInBuffer);

        return bytesInBuffer;
    }

    int Uart::read() {
        uint8_t b;
        int len = uart_read_bytes(_uart_port, &b, 1, 1 / portTICK_PERIOD_MS);

        return len > 0 ? b : -1;
    }

    int Uart::peek() {
        return -1;
    }

    void Uart::flush() {
        uart_flush(_uart_port);
    }

    size_t Uart::write(uint8_t byte) {
        return uart_write_bytes(_uart_port, &byte, 1);
    }

    size_t Uart::write(const uint8_t* buffer, size_t size) {
        return uart_write_bytes(_uart_port, buffer, size);
    }

}