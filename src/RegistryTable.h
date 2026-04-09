/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace FujitsuAC {

    class RegistryTable {
        public:
            struct Register {
                uint16_t address;
                uint16_t value;
            };

            RegistryTable(size_t size, Register *registerTable);
            Register* getRegister(uint16_t address);
            const Register* getAllRegisters(size_t &outSize) const;

        private:
            size_t _size;
            Register* _registerTable;
    };

}