/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "IMqttBridge.h"
#include <Arduino.h>
#include "RegistryTable.h"
#include "Enums.h"
#include "TFSXW1Controller.h"

namespace FujitsuAC {

    class TFSXW1Bridge: public IMqttBridge {
        public:
            TFSXW1Bridge(
                PubSubClient &mqttClient,
                Stream &uart,
                const char* uniqueId,
                const char* name,
                const char* version
            );

            void setup() override;
            void loop() override;

            void onRegisterChange(const RegistryTable::Register *reg);

        protected:
            const char* getProtocolName() override {
                return "UTY-TFSXW1";
            }

            void handleMqttCommand(const char *command, const char *property) override;

        private:
            TFSXW1Controller controller;
            uint32_t lastTempReportMillis = -180000;
            bool isPoweringOn = false;

            void registerBaseEntities();
            void registerSwitch(TFSXW1Controller::Address address);
            void publishState(uint16_t address, const char* value);

            static const char* addressToString(uint16_t address);
            const char* valueToString(const RegistryTable::Register *reg);

            const Enums::Power stringToEnum(Enums::Power def, const char *value);
            const Enums::MinimumHeat stringToEnum(Enums::MinimumHeat def, const char *value);
            const Enums::Mode stringToEnum(Enums::Mode def, const char *value);
            const Enums::FanSpeed stringToEnum(Enums::FanSpeed def, const char *value);
            const Enums::VerticalAirflow stringToEnum(Enums::VerticalAirflow def, const char *value);
            const Enums::VerticalSwing stringToEnum(Enums::VerticalSwing def, const char *value);
            const Enums::HorizontalAirflow stringToEnum(Enums::HorizontalAirflow def, const char *value);
            const Enums::HorizontalSwing stringToEnum(Enums::HorizontalSwing def, const char *value);
            const Enums::Powerful stringToEnum(Enums::Powerful def, const char *value);
            const Enums::EconomyMode stringToEnum(Enums::EconomyMode def, const char *value);
            const Enums::EnergySavingFan stringToEnum(Enums::EnergySavingFan def, const char *value);
            const Enums::OutdoorUnitLowNoise stringToEnum(Enums::OutdoorUnitLowNoise def, const char *value);
            const Enums::CoilDry stringToEnum(Enums::CoilDry def, const char *value);
            const Enums::HumanSensor stringToEnum(Enums::HumanSensor def, const char *value);
    };

}