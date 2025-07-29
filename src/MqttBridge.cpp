/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "MqttBridge.h"

namespace FujitsuAC {

    MqttBridge::MqttBridge(
        PubSubClient &mqttClient, 
        FujitsuController &controller,
        const char* uniqueId,
        const char* name
    ) : mqttClient(mqttClient), 
        controller(controller), 
        uniqueId(uniqueId),
        name(name) {}

    bool MqttBridge::setup() {
        this->controller.setOnRegisterChangeCallback([this](Register* reg) {
            this->onRegisterChange(reg);
        });

        this->controller.setDebugCallback([this](const char* name, const char* message) {
            this->debug(name, message);
        });

        Serial.println("MQTT Connected");

        char topic[128];
        snprintf(topic, sizeof(topic), "fujitsu/%s/status", this->uniqueId);
        this->mqttClient.publish(topic, "online", true);

        String deviceConfig = "\"device\": {";
        deviceConfig += "\"identifiers\": [\"" + this->uniqueId + "\"],";
        deviceConfig += "\"manufacturer\": \"https://github.com/Benas09/FujitsuAC\",";
        deviceConfig += "\"model\": \"Fujitsu AC\",";
        deviceConfig += "\"name\": \"" + this->name + "\"";
        deviceConfig += "}";

        String p = "{";
        p += "\"name\": \"climate\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_climate\",";
        p += "\"icon\": \"mdi:air-conditioner\",";

        p += "\"mode_command_topic\": \"fujitsu/" + this->uniqueId + "/set/mode\",";
        p += "\"mode_state_topic\": \"fujitsu/" + this->uniqueId + "/state/mode\",";

        p += "\"temperature_command_topic\": \"fujitsu/" + this->uniqueId + "/set/temp\",";
        p += "\"temperature_state_topic\": \"fujitsu/" + this->uniqueId + "/state/temp\",";

        p += "\"fan_mode_command_topic\": \"fujitsu/" + this->uniqueId + "/set/fan\",";
        p += "\"fan_mode_state_topic\": \"fujitsu/" + this->uniqueId + "/state/fan\",";

        p += "\"current_temperature_topic\": \"fujitsu/" + this->uniqueId + "/state/actual_temp\",";

        p += "\"min_temp\": 18,";
        p += "\"max_temp\": 30,";
        p += "\"temp_step\": 0.5,";
        p += "\"modes\": [\"auto\", \"cool\", \"dry\", \"fan_only\", \"heat\"],";
        p += "\"fan_modes\": [\"auto\", \"quiet\", \"low\", \"medium\", \"high\"],",
        p += deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/climate/%s_climate/config", this->uniqueId);
        this->mqttClient.publish(topic, p.c_str(), true);

        static constexpr Address switches[] = {
            Address::Power,
            Address::VerticalAirflow,
            Address::VerticalSwing,
            Address::Powerful,
            Address::EconomyMode,
            Address::EnergySavingFan,
            Address::OutdoorUnitLowNoise
        };

        for (const auto& switch_ : switches) {
            String propertyName = String(this->addressToString(switch_));
            String t = "";

            p = "{";
            p += "\"name\": \"" + propertyName + "\",";
            p += "\"unique_id\": \"" + this->uniqueId + "_" + propertyName + "\",";
            p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/" + propertyName + "\",";
            p += "\"command_topic\": \"fujitsu/" + this->uniqueId + "/set/" + propertyName + "\",";

            if (Address::VerticalAirflow == switch_) {
                p += "\"options\": [\"1\", \"2\", \"3\", \"4\", \"5\", \"6\"],";
            } else {
                p += "\"payload_on\": \"on\",";
                p += "\"payload_off\": \"off\",";
            }
            
            p += deviceConfig;
            p += "}";


            if (Address::VerticalAirflow == switch_) {
                t = "homeassistant/select/" + this->uniqueId + "_" + propertyName + "/config";
            } else {
                t = "homeassistant/switch/" + this->uniqueId + "_" + propertyName + "/config";
            }

            this->mqttClient.publish(t.c_str(), p.c_str(), true);
        }

        p = "{";
        p += "\"name\": \"restart\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_restart\",";
        p += "\"command_topic\": \"fujitsu/" + this->uniqueId + "/set/restart\",";
        p += "\"payload_press\": \"restart\",";
        p += deviceConfig;
        p += "}";

        String t = "homeassistant/button/" + this->uniqueId + "_restart/config";
        this->mqttClient.publish(t.c_str(), p.c_str(), true);

        this->mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
            char message[length + 1];
            memcpy(message, payload, length);
            message[length] = '\0';

            this->onMqtt(topic, message);
        });

        t = "fujitsu/" + this->uniqueId + "/#";
        this->mqttClient.subscribe(t.c_str());

        return true;
    }

    void MqttBridge::onMqtt(char* topic, char* payload) {
        String t = String(topic);

        int lastSlash = t.lastIndexOf('/');
        int secondLastSlash = t.lastIndexOf('/', lastSlash - 1);

        String com = t.substring(secondLastSlash + 1, lastSlash);
        String prop = t.substring(lastSlash + 1).c_str();

        const char *command = com.c_str();
        const char *property = prop.c_str();

        if (strcmp(command, "set") > 0) {
            return;
        }

        if (0 == strcmp(property, "restart")) {
            ESP.restart();

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::Power))) {
            this->controller.setPower(this->stringToEnum(Enums::Power::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::Mode))) {
            this->controller.setMode(this->stringToEnum(Enums::Mode::Auto, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::SetpointTemp))) {
            this->controller.setTemp(payload);

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::FanSpeed))) {
            this->controller.setFanSpeed(this->stringToEnum(Enums::FanSpeed::Auto, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::VerticalAirflow))) {
            this->controller.setVerticalAirflow(this->stringToEnum(Enums::VerticalAirflow::Position1, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::VerticalSwing))) {
            this->controller.setVerticalSwing(this->stringToEnum(Enums::VerticalSwing::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::Powerful))) {
            this->controller.setPowerful(this->stringToEnum(Enums::Powerful::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::EconomyMode))) {
            this->controller.setEconomy(this->stringToEnum(Enums::EconomyMode::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::EnergySavingFan))) {
            this->controller.setEnergySavingFan(this->stringToEnum(Enums::EnergySavingFan::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::OutdoorUnitLowNoise))) {
            this->controller.setOutdoorUnitLowNoise(this->stringToEnum(Enums::OutdoorUnitLowNoise::Off, payload));

            return;
        }
    }

    void MqttBridge::onRegisterChange(Register *reg) {
        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId, this->addressToString(reg->address));

        this->mqttClient.publish(
            topic, 
            this->valueToString(reg), 
            true
        );
    }

    const char* MqttBridge::addressToString(Address address) {
        switch (address) {
            case Address::Power: return "power";
            case Address::Mode: return "mode";
            case Address::FanSpeed: return "fan";
            case Address::VerticalSwing: return "vertical_swing";
            case Address::VerticalAirflow: return "vertical_airflow";
            case Address::Powerful: return "powerful";
            case Address::EconomyMode: return "economy_mode";
            case Address::EnergySavingFan: return "energy_saving_fan";
            case Address::OutdoorUnitLowNoise: return "outdoor_unit_low_noise";
            case Address::SetpointTemp: return "temp";
            case Address::ActualTemp: return "actual_temp";
            default: {
                static char buffer[20];
                snprintf(buffer, sizeof(buffer), "address_%04X", static_cast<uint16_t>(address));

                return buffer;
            }
        }
    }

    const char* MqttBridge::valueToString(Register *reg) {
        switch (reg->address) {
            case Address::Power:
                switch (static_cast<Enums::Power>(reg->value)) {
                    case Enums::Power::On: return "on";
                    case Enums::Power::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::Mode:
                switch (static_cast<Enums::Mode>(reg->value)) {
                    case Enums::Mode::Auto: return "auto";
                    case Enums::Mode::Cool: return "cool";
                    case Enums::Mode::Dry: return "dry";
                    case Enums::Mode::Fan: return "fan_only";
                    case Enums::Mode::Heat: return "heat";
                    default: return "unknown";
                }

                break;
            case Address::FanSpeed:
                switch (static_cast<Enums::FanSpeed>(reg->value)) {
                    case Enums::FanSpeed::Auto: return "auto";
                    case Enums::FanSpeed::Quiet: return "quiet";
                    case Enums::FanSpeed::Low: return "low";
                    case Enums::FanSpeed::Medium: return "medium";
                    case Enums::FanSpeed::High: return "high";
                    default: return "unknown";
                }

                break;
            case Address::VerticalSwing:
                switch (static_cast<Enums::VerticalSwing>(reg->value)) {
                    case Enums::VerticalSwing::On: return "on";
                    case Enums::VerticalSwing::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::VerticalAirflow:
                switch (static_cast<Enums::VerticalAirflow>(reg->value)) {
                    case Enums::VerticalAirflow::Position1: return "1";
                    case Enums::VerticalAirflow::Position2: return "2";
                    case Enums::VerticalAirflow::Position3: return "3";
                    case Enums::VerticalAirflow::Position4: return "4";
                    case Enums::VerticalAirflow::Position5: return "5";
                    case Enums::VerticalAirflow::Position6: return "6";
                    default: return "unknown";
                }

                break;
            case Address::Powerful:
                switch (static_cast<Enums::Powerful>(reg->value)) {
                    case Enums::Powerful::On: return "on";
                    case Enums::Powerful::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::EconomyMode:
                switch (static_cast<Enums::EconomyMode>(reg->value)) {
                    case Enums::EconomyMode::On: return "on";
                    case Enums::EconomyMode::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::EnergySavingFan:
                switch (static_cast<Enums::EnergySavingFan>(reg->value)) {
                    case Enums::EnergySavingFan::On: return "on";
                    case Enums::EnergySavingFan::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::OutdoorUnitLowNoise:
                switch (static_cast<Enums::OutdoorUnitLowNoise>(reg->value)) {
                    case Enums::OutdoorUnitLowNoise::On: return "on";
                    case Enums::OutdoorUnitLowNoise::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::SetpointTemp: {
                static char str[8];
                snprintf(str, sizeof(str), "%u.%u", reg->value / 10, reg->value % 10);

                return str;
            }
                
            case Address::ActualTemp: {
                static char str[8];
                snprintf(str, sizeof(str), "%u.%u", (reg->value - 5025) / 100, (reg->value - 5025) % 100);

                return str;
            }

            default: {
                static char buffer[20];
                snprintf(buffer, sizeof(buffer), "%04X", static_cast<uint16_t>(reg->value));

                return buffer;
            }
        }
    }

    const Enums::Power MqttBridge::stringToEnum(Enums::Power def, const char *value) {
        if (0 == strcmp(value, "on")) {
            return Enums::Power::On;
        }

        return def;
    }

    const Enums::Mode MqttBridge::stringToEnum(Enums::Mode def, const char *value) {
        if (0 == strcmp(value, "cool")) {
            return Enums::Mode::Cool;
        } else if (0 == strcmp(value, "dry")) {
            return Enums::Mode::Dry;
        } else if (0 == strcmp(value, "fan_only")) {
            return Enums::Mode::Fan;
        } else if (0 == strcmp(value, "heat")) {
            return Enums::Mode::Heat;
        }

        return def;
    }

    const Enums::FanSpeed MqttBridge::stringToEnum(Enums::FanSpeed def, const char *value) {
        if (0 == strcmp(value, "auto")) {
            return Enums::FanSpeed::Auto;
        } else if (0 == strcmp(value, "quiet")) {
            return Enums::FanSpeed::Quiet;
        } else if (0 == strcmp(value, "low")) {
            return Enums::FanSpeed::Low;
        } else if (0 == strcmp(value, "medium")) {
            return Enums::FanSpeed::Medium;
        } else if (0 == strcmp(value, "high")) {
            return Enums::FanSpeed::High;
        }

        return def;
    }

    const Enums::VerticalAirflow MqttBridge::stringToEnum(Enums::VerticalAirflow def, const char *value) {
        if (0 == strcmp(value, "1") ) {
            return Enums::VerticalAirflow::Position1;
        } else if (strcmp(value, "2") == 0) {
            return Enums::VerticalAirflow::Position2;
        } else if (strcmp(value, "3") == 0) {
            return Enums::VerticalAirflow::Position3;
        } else if (strcmp(value, "4") == 0) {
            return Enums::VerticalAirflow::Position4;
        } else if (strcmp(value, "5") == 0) {
            return Enums::VerticalAirflow::Position5;
        } else if (strcmp(value, "6") == 0) {
            return Enums::VerticalAirflow::Position6;
        }

        return def;
    }

    const Enums::VerticalSwing MqttBridge::stringToEnum(Enums::VerticalSwing def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::VerticalSwing::On;
        }

        return def;
    }

    const Enums::Powerful MqttBridge::stringToEnum(Enums::Powerful def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::Powerful::On;
        }

        return def;
    }

    const Enums::EconomyMode MqttBridge::stringToEnum(Enums::EconomyMode def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::EconomyMode::On;
        }

        return def;
    }

    const Enums::EnergySavingFan MqttBridge::stringToEnum(Enums::EnergySavingFan def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::EnergySavingFan::On;
        }

        return def;
    }

    const Enums::OutdoorUnitLowNoise MqttBridge::stringToEnum(Enums::OutdoorUnitLowNoise def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::OutdoorUnitLowNoise::On;
        }

        return def;
    }

    void MqttBridge::debug(const char* name, const char* message) {
        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/debug/%s", this->uniqueId, name);

        this->mqttClient.publish(topic, message);
    }

}