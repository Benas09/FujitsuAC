/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "EspMQTTClient.h"
#include "FujitsuController.h"
#include "Enums.h"

class MqttBridge {
    public:
        MqttBridge(
            EspMQTTClient &mqttClient,
            FujitsuController &controller,
            const char* uniqueId,
            const char* name
        );

        bool setup();
        bool loop();

    private:
        EspMQTTClient &mqttClient;
        FujitsuController &controller;

        String uniqueId;
        String name;

        uint32_t waitingPowerOnFrom = 0;
        Enums::Mode modeAfterPowering = Enums::Mode::Auto;

        void onRegisterChange(Register *reg);
        void onMqtt(const String &topic, const String &payload);
        void debug(const char* name, const char* message);
        
        static const char* addressToString(Address address);
        static const char* valueToString(Register *reg);

        const Enums::Power stringToEnum(Enums::Power def, const String &value);
        const Enums::Mode stringToEnum(Enums::Mode def, const String &value);
        const Enums::FanSpeed stringToEnum(Enums::FanSpeed def, const String &value);
        const Enums::VerticalAirflow stringToEnum(Enums::VerticalAirflow def, const String &value);
        const Enums::VerticalSwing stringToEnum(Enums::VerticalSwing def, const String &value);
        const Enums::Powerful stringToEnum(Enums::Powerful def, const String &value);
        const Enums::Economy stringToEnum(Enums::Economy def, const String &value);
        const Enums::EnergySavingFan stringToEnum(Enums::EnergySavingFan def, const String &value);
        const Enums::OutdoorUnitLowNoise stringToEnum(Enums::OutdoorUnitLowNoise def, const String &value);
};
