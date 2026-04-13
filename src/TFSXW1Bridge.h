/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "IMqttBridge.h"
#include <Arduino.h>
#include "RegistryTable.h"
#include "TFSXW1Controller.h"

namespace FujitsuAC {

    class TFSXW1Bridge: public IMqttBridge {
        public:
            TFSXW1Bridge(
                Config &config,
                PubSubClient &mqttClient,
                Stream &uart
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

            const TFSXW1Enums::Power stringToEnum(TFSXW1Enums::Power def, const char *value);
            const TFSXW1Enums::MinimumHeat stringToEnum(TFSXW1Enums::MinimumHeat def, const char *value);
            const TFSXW1Enums::Mode stringToEnum(TFSXW1Enums::Mode def, const char *value);
            const TFSXW1Enums::FanSpeed stringToEnum(TFSXW1Enums::FanSpeed def, const char *value);
            const TFSXW1Enums::VerticalAirflow stringToEnum(TFSXW1Enums::VerticalAirflow def, const char *value);
            const TFSXW1Enums::VerticalSwing stringToEnum(TFSXW1Enums::VerticalSwing def, const char *value);
            const TFSXW1Enums::HorizontalAirflow stringToEnum(TFSXW1Enums::HorizontalAirflow def, const char *value);
            const TFSXW1Enums::HorizontalSwing stringToEnum(TFSXW1Enums::HorizontalSwing def, const char *value);
            const TFSXW1Enums::Powerful stringToEnum(TFSXW1Enums::Powerful def, const char *value);
            const TFSXW1Enums::EconomyMode stringToEnum(TFSXW1Enums::EconomyMode def, const char *value);
            const TFSXW1Enums::EnergySavingFan stringToEnum(TFSXW1Enums::EnergySavingFan def, const char *value);
            const TFSXW1Enums::OutdoorUnitLowNoise stringToEnum(TFSXW1Enums::OutdoorUnitLowNoise def, const char *value);
            const TFSXW1Enums::CoilDry stringToEnum(TFSXW1Enums::CoilDry def, const char *value);
            const TFSXW1Enums::HumanSensor stringToEnum(TFSXW1Enums::HumanSensor def, const char *value);
    };

}