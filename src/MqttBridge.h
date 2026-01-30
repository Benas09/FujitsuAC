/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "PubSubClient.h"
#include "FujitsuController.h"
#include "Enums.h"

#pragma once

namespace FujitsuAC {

    class MqttBridge {
        public:
            MqttBridge(
                PubSubClient &mqttClient,
                FujitsuController &controller,
                const char* uniqueId,
                const char* name,
                const char* version
            );

            bool setup();
            bool loop();
            void registerSwitch(Address address);

        private:
            PubSubClient &mqttClient;
            FujitsuController &controller;

            String uniqueId;
            String name;
            String version;
            String deviceConfig;
            uint32_t lastTempReportMillis = -180000;
            uint32_t lastDiagnosticReportMillis = -30000;

            Enums::Mode poweringToMode = Enums::Mode::None;

            void createDeviceConfig();
            void registerDiagnosticEntities();
            void registerBaseEntities();

            void sendInitialDiagnosticData();
            void sendDiagnosticData();
            void onRegisterChange(const Register *reg);
            void publishState(Address address, const char* value);
            void onMqtt(char* topic, char* payload);
            void debug(const char* name, const char* message);
            
            static const char* getResetReason();
            static const char* addressToString(Address address);
            const char* valueToString(const Register *reg);

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
            const Enums::HumanSensor stringToEnum(Enums::HumanSensor def, const char *value);
    };

}