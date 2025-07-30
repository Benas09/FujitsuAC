/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

namespace FujitsuAC {

    namespace Enums {
        enum class Power: uint16_t {
            On = 0x0001,
            Off = 0x0000
        };

        enum class Mode: uint16_t {
            Auto = 0x0000,
            Cool = 0x0001,
            Dry = 0x0002,
            Fan = 0x0003,
            Heat = 0x0004,
          //    MinimumHeat = 0x0001 adresas 17 1/
        };

        enum class FanSpeed: uint16_t {
            Auto = 0x0000,
            Quiet = 0x0002,
            Low = 0x0005,
            Medium = 0x0008,
            High = 0x000B,
        };

        enum class VerticalAirflow: uint16_t {
            Position1 = 0x0001,
            Position2 = 0x0002,
            Position3 = 0x0003,
            Position4 = 0x0004,
            Position5 = 0x0005,
            Position6 = 0x0006,
        };

        enum class VerticalSwing: uint16_t {
            Off = 0x0000,
            On = 0x0001,
        };

        enum class Powerful: uint16_t {
            Off = 0x0000,
            On = 0x0001,
        };

        enum class EconomyMode: uint16_t {
            Off = 0x0000,
            On = 0x0001,
        };

        enum class EnergySavingFan: uint16_t {
            Off = 0x0000,
            On = 0x0001,
        };

        enum class OutdoorUnitLowNoise: uint16_t {
            Off = 0x0000,
            On = 0x0001,
        };
    }

}