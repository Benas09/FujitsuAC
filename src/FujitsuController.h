/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include <Arduino.h>
#include <stdint.h>
#include "RegistryTable.h"
#include "Buffer.h"
#include "Enums.h"

#pragma once

namespace FujitsuAC {

    class FujitsuController {

      public:
        FujitsuController(Stream &uart);

        bool setup();
        bool loop();

        void setPower(Enums::Power power);
        void setMinimumHeat(Enums::MinimumHeat minimumHeat);
        void setMode(Enums::Mode mode);
        void setFanSpeed(Enums::FanSpeed fanSpeed);
        void setVerticalAirflow(Enums::VerticalAirflow verticalAirflow);
        void setVerticalSwing(Enums::VerticalSwing verticalSwing);
        void setHorizontalAirflow(Enums::HorizontalAirflow horizontalAirflow);
        void setHorizontalSwing(Enums::HorizontalSwing horizontalSwing);
        void setPowerful(Enums::Powerful powerful);
        void setEconomy(Enums::EconomyMode economy);
        void setEnergySavingFan(Enums::EnergySavingFan energySavingFan);
        void setOutdoorUnitLowNoise(Enums::OutdoorUnitLowNoise outdoorUnitLowNoise);
        void setHumanSensor(Enums::HumanSensor humanSensor);
        void setTemp(const char *temp);

        bool isPoweredOn();
        bool isHorizontalSwingSupported();
        bool isHumanSensorSupported();

        void setOnRegisterChangeCallback(std::function<void(const Register* reg)> onRegisterChangeCallback);
        void setDebugCallback(std::function<void(const char* name, const char* message)> debugCallback);
        const Register* getAllRegisters(size_t &outSize) const;
        Register* getRegister(Address address);

      private:
        std::function<void(const Register *reg)> onRegisterChangeCallback;
        std::function<void(const char* name, const char* message)> debugCallback;
        
        Stream &uart;
        RegistryTable registryTable;
        Buffer buffer;
        uint32_t lastRequestMillis = 0;
        bool lastResponseReceived = true;
        bool noResponseNotified = false;
        bool initialized = false;
        bool terminated = false;

        bool humanSensorSupported = false;

        enum class FrameType: int {
          None = -1,
          Init1 = 0,
          Init2 = 1,
          InitialRegistries1 = 2,
          InitialRegistries2 = 3,
          InitialRegistries3 = 4,
          FrameA = 5,
          FrameB = 6,
          FrameC = 7,
          SendRegistries = 8,
          CheckRegistries = 9,
        };

        FrameType lastFrameSent = FrameType::None;

        struct Frame {
            FrameType type;
            size_t size;
            Address registries[19];
        };

        struct FrameSendRegistries {
            FrameType type;
            size_t size;
            Address registries[19];
            uint16_t values[19];
        };

        Frame initialRegistries1 = {FrameType::InitialRegistries1, 2, {
              Address::Initial0,
              Address::Initial1,
            }
        };

        Frame initialRegistries2 = {FrameType::InitialRegistries2, 14, {
              Address::Initial2,
              Address::Initial3,  
              Address::Initial4,  
              Address::Initial5,  
              Address::Initial6,  
              Address::Initial7,  
              Address::Initial8,  
              Address::Initial9,  
              Address::Initial10,  
              Address::Initial11,  
              Address::Initial12,  
              Address::Initial13,  
              Address::HorizontalSwingSupported,  
              Address::Initial15,
            }
        };

        Frame initialRegistries3 = {FrameType::InitialRegistries3, 10, {
              Address::Initial16,
              Address::Initial17,  
              Address::HumanSensorSupported,  
              Address::Initial19,  
              Address::Initial20,  
              Address::Initial21,  
              Address::Initial22,  
              Address::Initial23,  
              Address::Initial24,  
              Address::Initial25,
            }
        };

        Frame frameA = {FrameType::FrameA, 13, {
              Address::Power,
              Address::Mode,
              Address::SetpointTemp,
              Address::FanSpeed,
              Address::VerticalAirflowSetterRegistry,
              Address::VerticalSwing,
              Address::VerticalAirflow,
              Address::HorizontalAirflowSetterRegistry,
              Address::HorizontalSwing,
              Address::HorizontalAirflow,
              Address::Register11,
              Address::ActualTemp,
              Address::Register13,
            }
        };

        Frame frameB = {FrameType::FrameB, 19, {
              Address::EconomyMode,  
              Address::MinimumHeat,  
              Address::HumanSensor,  
              Address::Register17,  
              Address::Register18,  
              Address::Register19,  
              Address::Register20,  
              Address::Register21,  
              Address::EnergySavingFan,  
              Address::Register23,  
              Address::Powerful,  
              Address::OutdoorUnitLowNoise,  
              Address::Register26,  
              Address::Register27,  
              Address::Register28,  
              Address::Register29,  
              Address::Register30,  
              Address::Register31,  
              Address::Register32,
            }
        };

        Frame frameC = {FrameType::FrameC, 12, {
              Address::Register33,  
              Address::Register34,  
              Address::Register35,  
              Address::Register36,  
              Address::Register37,  
              Address::Register38,  
              Address::Register39,  
              Address::Register40,  
              Address::Register41,  
              Address::OutdoorTemp,  
              Address::Register43,  
              Address::Register44,
            }
        };

        FrameSendRegistries frameSendRegistries = {FrameType::SendRegistries, 0, {}, {}};

        bool isMinimumHeatEnabled();

        void sendRequest();
        void requestRegistries(Frame frame);
        void sendRegistries();
        void onFrame(uint8_t buffer[128], int size, bool isValid);
        void updateRegistries(uint8_t buffer[128], int size);
        void debug(const char* name, const char* message);
        const char* toHexStr(uint8_t buffer[128], int size);
    };

}