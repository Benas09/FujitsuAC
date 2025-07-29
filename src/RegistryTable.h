/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <stddef.h>
#include "Register.h"

#pragma once

namespace FujitsuAC {

    class RegistryTable {
        public:
            RegistryTable ();
            Register* getRegister(Address address);
            const Register* getAllRegisters(size_t &outSize) const;

        private:
            Register registerTable[70] = {
                {Address::Initial0, 0x0000, false},
                {Address::Initial1, 0x0000, false},
                
                {Address::Initial2, 0x0000, false},
                {Address::Initial3, 0x0000, false},
                {Address::Initial4, 0x0000, false},
                {Address::Initial5, 0x0000, false},
                {Address::Initial6, 0x0000, false},
                {Address::Initial7, 0x0000, false},
                {Address::Initial8, 0x0000, false},
                {Address::Initial9, 0x0000, false},
                {Address::Initial10, 0x0000, false},
                {Address::Initial11, 0x0000, false},
                {Address::Initial12, 0x0000, false},
                {Address::Initial13, 0x0000, false},
                {Address::Initial14, 0x0000, false},
                {Address::Initial15, 0x0000, false},
                
                {Address::Initial16, 0x0000, false},
                {Address::Initial17, 0x0000, false},
                {Address::Initial18, 0x0000, false},
                {Address::Initial19, 0x0000, false},
                {Address::Initial20, 0x0000, false},
                {Address::Initial21, 0x0000, false},
                {Address::Initial22, 0x0000, false},
                {Address::Initial23, 0x0000, false},
                {Address::Initial24, 0x0000, false},
                {Address::Initial25, 0x0000, false},
                
                {Address::Power, 0x0000, false},
                {Address::Mode, 0x0000, false},
                {Address::SetpointTemp, 0x0000, false},
                {Address::FanSpeed, 0x0000, false},
                {Address::VerticalAirflow, 0x0000, false},
                {Address::VerticalSwing, 0x0000, false},
                {Address::Register7, 0x0000, false},
                {Address::Register8, 0x0000, false},
                {Address::Register9, 0x0000, false},
                {Address::Register10, 0x0000, false},
                {Address::Register11, 0x0000, false},
                {Address::ActualTemp, 0x0000, false},
                {Address::Register13, 0x0000, false},
                
                {Address::EconomyMode, 0x0000, false},
                {Address::Register15, 0x0000, false},
                {Address::Register16, 0x0000, false},
                {Address::Register17, 0x0000, false},
                {Address::Register18, 0x0000, false},
                {Address::Register19, 0x0000, false},
                {Address::Register20, 0x0000, false},
                {Address::Register21, 0x0000, false},
                {Address::EnergySavingFan, 0x0000, false},
                {Address::Register23, 0x0000, false},
                {Address::Powerful, 0x0000, false},
                {Address::OutdoorUnitLowNoise, 0x0000, false},
                {Address::Register26, 0x0000, false},
                {Address::Register27, 0x0000, false},
                {Address::Register28, 0x0000, false},
                {Address::Register29, 0x0000, false},
                {Address::Register30, 0x0000, false},
                {Address::Register31, 0x0000, false},
                {Address::Register32, 0x0000, false},
                
                {Address::Register33, 0x0000, false},
                {Address::Register34, 0x0000, false},
                {Address::Register35, 0x0000, false},
                {Address::Register36, 0x0000, false},
                {Address::Register37, 0x0000, false},
                {Address::Register38, 0x0000, false},
                {Address::Register39, 0x0000, false},
                {Address::Register40, 0x0000, false},
                {Address::Register41, 0x0000, false},
                {Address::Register42, 0x0000, false},
                {Address::Register43, 0x0000, false},
                {Address::Register44, 0x0000, false},
            };
    };

}