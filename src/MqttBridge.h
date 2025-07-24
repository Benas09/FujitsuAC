/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "PubSubClient.h"
#include "FujitsuController.h"
#include "Enums.h"

class MqttBridge {
    public:
        MqttBridge(
            PubSubClient &mqttClient,
            FujitsuController &controller,
            const char* uniqueId,
            const char* name
        );

        bool setup();
        bool loop();

    private:
        PubSubClient &mqttClient;
        FujitsuController &controller;

        String uniqueId;
        String name;

        uint32_t waitingPowerOnFrom = 0;
        Enums::Mode modeAfterPowering = Enums::Mode::Auto;

        void onRegisterChange(Register *reg);
        void onMqtt(char* topic, char* payload);
        void debug(const char* name, const char* message);
        
        static const char* addressToString(Address address);
        static const char* valueToString(Register *reg);

        const Enums::Power stringToEnum(Enums::Power def, const char *value);
        const Enums::Mode stringToEnum(Enums::Mode def, const char *value);
        const Enums::FanSpeed stringToEnum(Enums::FanSpeed def, const char *value);
        const Enums::VerticalAirflow stringToEnum(Enums::VerticalAirflow def, const char *value);
        const Enums::VerticalSwing stringToEnum(Enums::VerticalSwing def, const char *value);
        const Enums::Powerful stringToEnum(Enums::Powerful def, const char *value);
        const Enums::Economy stringToEnum(Enums::Economy def, const char *value);
        const Enums::EnergySavingFan stringToEnum(Enums::EnergySavingFan def, const char *value);
        const Enums::OutdoorUnitLowNoise stringToEnum(Enums::OutdoorUnitLowNoise def, const char *value);
};
