/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#ifndef FUJITSU_CONTROLLER_H
#define FUJITSU_CONTROLLER_H

#include <Arduino.h>
#include <stdint.h>
#include "Register.h"
#include "RegistryTable.h"
#include "Buffer.h"
#include "Enums.h"

class FujitsuController {

  public:
    FujitsuController(Stream &uart);

    bool setup();
    bool loop();

    bool isPowerOn();
    void setPower(Enums::Power power);
    void setMode(Enums::Mode mode);
    void setFanSpeed(Enums::FanSpeed fanSpeed);
    void setVerticalAirflow(Enums::VerticalAirflow verticalAirflow);
    void setVerticalSwing(Enums::VerticalSwing verticalSwing);
    void setPowerful(Enums::Powerful powerful);
    void setEconomy(Enums::Economy economy);
    void setEnergySavingFan(Enums::EnergySavingFan energySavingFan);
    void setOutdoorUnitLowNoise(Enums::OutdoorUnitLowNoise outdoorUnitLowNoise);
    void setTemp(const char *temp);

    void setOnRegisterChangeCallback(std::function<void(Register* reg)> onRegisterChangeCallback);
    void setDebugCallback(std::function<void(const char* name, const char* message)> debugCallback);

  private:
    std::function<void(Register *reg)> onRegisterChangeCallback;
    std::function<void(const char* name, const char* message)> debugCallback;
    
    Stream &uart;
    RegistryTable registryTable;
    Buffer buffer;
    uint32_t lastRequestMillis = 0;
    bool lastResponseReceived = true;
    bool noResponseNotified = false;
    bool initialized = false;
    bool terminated = false;

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
          Address::Initial14,  
          Address::Initial15,
        }
    };

    Frame initialRegistries3 = {FrameType::InitialRegistries3, 10, {
          Address::Initial16,
          Address::Initial17,  
          Address::Initial18,  
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
          Address::Fan,
          Address::VerticalAirflow,
          Address::VerticalSwing,
          Address::Register7,
          Address::Register8,
          Address::Register9,
          Address::Register10,
          Address::Register11,
          Address::ActualTemp,
          Address::Register13,
        }
    };

    Frame frameB = {FrameType::FrameB, 19, {
          Address::EconomyMode,  
          Address::Register15,  
          Address::Register16,  
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
          Address::Register42,  
          Address::Register43,  
          Address::Register44,
        }
    };

    FrameSendRegistries frameSendRegistries = {FrameType::SendRegistries, 0, {}, {}};

    void sendRequest();
    void requestRegistries(Frame frame);
    void sendRegistries();
    void onFrame(uint8_t buffer[128], int size, bool isValid);
    void updateRegistries(uint8_t buffer[128], int size);
    void debug(const char* name, const char* message);
    const char* toHexStr(uint8_t buffer[128], int size);
};

#endif //FUJITSU_CONTROLLER_H