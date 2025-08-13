/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <stdint.h>

#pragma once

namespace FujitsuAC {

    enum class Address: uint16_t {
        Initial0 = 0x0001,
        Initial1 = 0x0101,

        Initial2 = 0x0110,
        Initial3 = 0x0111,  
        Initial4 = 0x0112,  
        Initial5 = 0x0113,  
        Initial6 = 0x0114,  
        Initial7 = 0x0115,  
        Initial8 = 0x0117,  
        Initial9 = 0x011A,  
        Initial10 = 0x011D,  
        Initial11 = 0x0120,  
        Initial12 = 0x0130,
        Initial13 = 0x0131,  
        Initial14 = 0x0142,  
        Initial15 = 0x0143,  

        Initial16 = 0x0150,
        Initial17 = 0x0151,  
        HumanSensorSupported = 0x0152,
        Initial19 = 0x0153,  
        Initial20 = 0x0154,  
        Initial21 = 0x0155,  
        Initial22 = 0x0156,  
        Initial23 = 0x0170,  
        Initial24 = 0x0171,  
        Initial25 = 0x0193,  

        Power = 0x1000,
        Mode      = 0x1001,
        SetpointTemp = 0x1002,
        FanSpeed = 0x1003,  
        VerticalAirflowSetterRegistry = 0x1010, 
        VerticalSwing = 0x1011,  
        VerticalAirflow  = 0x10A0,
        HorizontalAirflowSetterRegistry = 0x1022,  
        HorizontalSwing = 0x1023,
        HorizontalAirflow = 0x10A9,
        Register11 = 0x1031,  
        ActualTemp = 0x1033,  
        Register13 = 0x1034,  

        EconomyMode = 0x1100,  
        MinimumHeat = 0x1101,  
        HumanSensor = 0x1102,
        Register17 = 0x1103,  
        Register18 = 0x1104,  
        Register19 = 0x1105,  
        Register20 = 0x1106,  
        Register21 = 0x1107,  
        EnergySavingFan = 0x1108,  
        Register23 = 0x1109,  
        Powerful = 0x1120,  
        OutdoorUnitLowNoise = 0x1121,  
        Register26 = 0x1144,  
        Register27 = 0x1200,  
        Register28 = 0x1201,  
        Register29 = 0x1202,  
        Register30 = 0x1203,  
        Register31 = 0x1204,  
        Register32 = 0x1141, 

        Register33 = 0x1400,  
        Register34 = 0x1401,  
        Register35 = 0x1402,  
        Register36 = 0x1403,  
        Register37 = 0x1404,  
        Register38 = 0x1405,  
        Register39 = 0x1406,  
        Register40 = 0x140E,  
        Register41 = 0x2000,  
        Register42 = 0x2020,  
        Register43 = 0x2021,  
        Register44 = 0xF001,
    };

    struct Register {
        Address address;
        uint16_t value;
        bool writable;
    };

}