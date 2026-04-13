/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "TFSXW1Bridge.h"

namespace FujitsuAC {
    TFSXW1Bridge::TFSXW1Bridge(
        Config &config,
        PubSubClient &mqttClient,
        Stream &uart
    ):
        controller(uart),
        IMqttBridge(
            config,
            mqttClient
        )
    {}

    void TFSXW1Bridge::setup() {
        IMqttBridge::setup();

        this->registerBaseEntities();
        this->registerSwitch(TFSXW1Controller::Address::Power);

        this->controller.setOnRegisterChangeCallback([this](const RegistryTable::Register* reg) {
            this->onRegisterChange(reg);
        });

        this->controller.setDebugCallback([this](const char* name, const char* message) {
            this->debug(name, message);
        });

        this->controller.setup();

        //Send initial registry values after MQTT connection
        size_t registryCount;
        const RegistryTable::Register* registers = this->controller.getAllRegisters(registryCount);

        for (size_t i = 0; i < registryCount; ++i) {
            if (
                registers[i].address == TFSXW1Controller::Address::ActualTemp
                || registers[i].address == TFSXW1Controller::Address::OutdoorTemp
            ) {
                continue;
            }

            this->onRegisterChange(&registers[i]);
        }
    }

    void TFSXW1Bridge::loop() {
        IMqttBridge::loop();
        this->controller.loop();

        if (this->isPoweringOn && !this->controller.isPoweredOn()) {
            this->controller.setPower(TFSXW1Enums::Power::On);
        }
    }

    void TFSXW1Bridge::registerBaseEntities() {
        char topic[128];

        String p = "{";
        p += "\"name\": \"climate\",";
        p += "\"unique_id\": \"" + _config.getUniqueId() + "_climate\",";
        p += "\"icon\": \"mdi:air-conditioner\",";

        p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";

        p += "\"mode_command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/mode\",";
        p += "\"mode_state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/mode\",";

        p += "\"temperature_command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/temp\",";
        p += "\"temperature_state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/temp\",";

        p += "\"fan_mode_command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/fan\",";
        p += "\"fan_mode_state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/fan\",";

        p += "\"current_temperature_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/actual_temp\",";
        p += "\"current_humidity_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/humidity\",";

        p += "\"min_temp\": 18,";
        p += "\"max_temp\": 30,";
        p += "\"temp_step\": 0.5,";
        p += "\"modes\": [\"off\", \"auto\", \"cool\", \"dry\", \"fan_only\", \"heat\"],";
        p += "\"fan_modes\": [\"auto\", \"quiet\", \"low\", \"medium\", \"high\"],";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/climate/%s_climate/config", _config.getUniqueId().c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"actual_temp\",";
        p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/actual_temp\",";
        p += "\"unit_of_measurement\": \"°C\",";
        p += "\"unique_id\": \"" + _config.getUniqueId() + "_actual_temp\",";
        p += "\"device_class\": \"temperature\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_actual_temp/config", _config.getUniqueId().c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"outdoor_temp\",";
        p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/outdoor_temp\",";
        p += "\"unit_of_measurement\": \"°C\",";
        p += "\"unique_id\": \"" + _config.getUniqueId() + "_outdoor_temp\",";
        p += "\"device_class\": \"temperature\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_outdoor_temp/config", _config.getUniqueId().c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        this->debug("info", "Base entities registered");
    }

    void TFSXW1Bridge::registerSwitch(TFSXW1Controller::Address address) {
        String propertyName = String(this->addressToString(address));
        
        String p = "{";
        p += "\"name\": \"" + propertyName + "\",";
        p += "\"unique_id\": \"" + _config.getUniqueId() + "_" + propertyName + "\",";
        p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/" + propertyName + "\",";
        p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/" + propertyName + "\",";

        if (
            TFSXW1Controller::Address::VerticalAirflow == address
            || TFSXW1Controller::Address::HorizontalAirflow == address
        ) {
            int count = TFSXW1Controller::Address::VerticalAirflow == address
                ? this->controller.getVerticalAirflowDirectionCount()
                : this->controller.getHorizontalAirflowDirectionCount()
            ;

            count = count > 6 ? 6 : count;

            p += "\"options\": [";

            for (int i = 1; i <= count; i++) {
                p += '"';
                p += char('0' + i);
                p += '"';

                if (i < count) {
                    p += ",";
                }
            }

            p += "],";
        } else {
            p += "\"payload_on\": \"on\",";
            p += "\"payload_off\": \"off\",";
        }
        
        p += this->deviceConfig;
        p += "}";

        char topic[128];

        if (
            TFSXW1Controller::Address::VerticalAirflow == address
            || TFSXW1Controller::Address::HorizontalAirflow == address
        ) {
            snprintf(topic, sizeof(topic), "homeassistant/select/%s_%s/config", _config.getUniqueId().c_str(), propertyName.c_str());
        } else {
            snprintf(topic, sizeof(topic), "homeassistant/switch/%s_%s/config", _config.getUniqueId().c_str(), propertyName.c_str());
        }

        this->mqttClient.publish(topic, p.c_str(), true);

        char message[64];
        snprintf(message, sizeof(message), "Switch '%s' registered", propertyName.c_str());

        this->debug("info", message);
    }

    void TFSXW1Bridge::handleMqttCommand(const char *property, const char* payload) {
        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::Power))) {
            this->controller.setPower(this->stringToEnum(TFSXW1Enums::Power::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::MinimumHeat))) {
            this->controller.setMinimumHeat(this->stringToEnum(TFSXW1Enums::MinimumHeat::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::Mode))) {
            if (0 == strcmp(payload, "off")) {
                this->controller.setPower(TFSXW1Enums::Power::Off);

                return;
            }

            this->controller.setMode(this->stringToEnum(TFSXW1Enums::Mode::Auto, payload));

            if (!this->controller.isPoweredOn()) {
                this->isPoweringOn = true;
            }

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::SetpointTemp))) {
            this->controller.setTemp(payload);

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::FanSpeed))) {
            this->controller.setFanSpeed(this->stringToEnum(TFSXW1Enums::FanSpeed::Auto, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::VerticalAirflow))) {
            this->controller.setVerticalAirflow(this->stringToEnum(TFSXW1Enums::VerticalAirflow::Position1, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::VerticalSwing))) {
            this->controller.setVerticalSwing(this->stringToEnum(TFSXW1Enums::VerticalSwing::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::HorizontalAirflow))) {
            this->controller.setHorizontalAirflow(this->stringToEnum(TFSXW1Enums::HorizontalAirflow::Position1, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::HorizontalSwing))) {
            this->controller.setHorizontalSwing(this->stringToEnum(TFSXW1Enums::HorizontalSwing::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::Powerful))) {
            this->controller.setPowerful(this->stringToEnum(TFSXW1Enums::Powerful::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::EconomyMode))) {
            this->controller.setEconomy(this->stringToEnum(TFSXW1Enums::EconomyMode::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::EnergySavingFan))) {
            this->controller.setEnergySavingFan(this->stringToEnum(TFSXW1Enums::EnergySavingFan::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::OutdoorUnitLowNoise))) {
            this->controller.setOutdoorUnitLowNoise(this->stringToEnum(TFSXW1Enums::OutdoorUnitLowNoise::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::HumanSensor))) {
            this->controller.setHumanSensor(this->stringToEnum(TFSXW1Enums::HumanSensor::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(TFSXW1Controller::Address::CoilDry))) {
            this->controller.setCoilDry(this->stringToEnum(TFSXW1Enums::CoilDry::Off, payload));

            return;
        }
    }

    void TFSXW1Bridge::onRegisterChange(const RegistryTable::Register *reg) {
        if (reg->address == TFSXW1Controller::Address::ActualTemp) {
            uint32_t now = millis();

            if ((now - this->lastTempReportMillis) < 180000) {
                return;
            }
            
            this->lastTempReportMillis = now;
        }

        if (reg->address == TFSXW1Controller::Address::SetpointTemp && 0xFFFF == reg->value) {
            // Fan mode do not return setpoint temp
            return;
        }

        this->publishState(reg->address, this->valueToString(reg));

        if (TFSXW1Controller::Address::Power == reg->address) {
            if (static_cast<uint16_t>(TFSXW1Enums::Power::On) == reg->value) {
                this->isPoweringOn = false;
            }

            // Workaround to get shown required mode shown immediately after turn off
            RegistryTable::Register* modeRegister = this->controller.getRegister(TFSXW1Controller::Address::Mode);
            this->publishState(modeRegister->address, this->valueToString(modeRegister));
        }

        struct FeatureRegistryRelation {
            TFSXW1Controller::Address featureAddress;
            TFSXW1Controller::Address registryAddress;
        };

        static constexpr FeatureRegistryRelation defaults[] = {
            { TFSXW1Controller::Address::VerticalAirflowDirectionCount, TFSXW1Controller::Address::VerticalAirflow },
            { TFSXW1Controller::Address::VerticalSwingSupported, TFSXW1Controller::Address::VerticalSwing },
            { TFSXW1Controller::Address::HorizontalAirflowDirectionCount, TFSXW1Controller::Address::HorizontalAirflow },
            { TFSXW1Controller::Address::HorizontalSwingSupported, TFSXW1Controller::Address::HorizontalSwing },
            { TFSXW1Controller::Address::PowerfulSupported, TFSXW1Controller::Address::Powerful },
            { TFSXW1Controller::Address::EconomyModeSupported, TFSXW1Controller::Address::EconomyMode },
            { TFSXW1Controller::Address::EnergySavingFanSupported, TFSXW1Controller::Address::EnergySavingFan },
            { TFSXW1Controller::Address::OutdoorUnitLowNoiseSupported, TFSXW1Controller::Address::OutdoorUnitLowNoise },
            { TFSXW1Controller::Address::MinimumHeatSupported, TFSXW1Controller::Address::MinimumHeat },
            { TFSXW1Controller::Address::HumanSensorSupported, TFSXW1Controller::Address::HumanSensor },
            { TFSXW1Controller::Address::CoilDrySupported, TFSXW1Controller::Address::CoilDry }
        };

        for (const auto& relation : defaults) {
            if (relation.featureAddress == reg->address) {
                if (this->controller.isFeatureSupported(relation.featureAddress)) {
                    this->registerSwitch(relation.registryAddress);
                }

                break;
            }
        }
    }

    void TFSXW1Bridge::publishState(uint16_t address, const char* value)
    {
        IMqttBridge::publishState(this->addressToString(address), value);
    }

    const char* TFSXW1Bridge::addressToString(uint16_t address) {
        switch (address) {
            case TFSXW1Controller::Address::Power: return "power";
            case TFSXW1Controller::Address::Mode: return "mode";
            case TFSXW1Controller::Address::FanSpeed: return "fan";
            case TFSXW1Controller::Address::VerticalSwing: return "vertical_swing";
            case TFSXW1Controller::Address::VerticalAirflow: return "vertical_airflow";
            case TFSXW1Controller::Address::HorizontalSwing: return "horizontal_swing";
            case TFSXW1Controller::Address::HorizontalAirflow: return "horizontal_airflow";
            case TFSXW1Controller::Address::Powerful: return "powerful";
            case TFSXW1Controller::Address::EconomyMode: return "economy_mode";
            case TFSXW1Controller::Address::EnergySavingFan: return "energy_saving_fan";
            case TFSXW1Controller::Address::OutdoorUnitLowNoise: return "outdoor_unit_low_noise";
            case TFSXW1Controller::Address::SetpointTemp: return "temp";
            case TFSXW1Controller::Address::ActualTemp: return "actual_temp";
            case TFSXW1Controller::Address::OutdoorTemp: return "outdoor_temp";
            case TFSXW1Controller::Address::HumanSensor: return "human_sensor";
            case TFSXW1Controller::Address::MinimumHeat: return "minimum_heat";
            case TFSXW1Controller::Address::CoilDry: return "coil_dry";
            default: {
                static char buffer[20];
                snprintf(buffer, sizeof(buffer), "address_%04X", static_cast<uint16_t>(address));

                return buffer;
            }
        }
    }

    const char* TFSXW1Bridge::valueToString(const RegistryTable::Register *reg) {
        switch (reg->address) {
            case TFSXW1Controller::Address::Power:
                switch (static_cast<TFSXW1Enums::Power>(reg->value)) {
                    case TFSXW1Enums::Power::On: return "on";
                    case TFSXW1Enums::Power::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::MinimumHeat:
                switch (static_cast<TFSXW1Enums::MinimumHeat>(reg->value)) {
                    case TFSXW1Enums::MinimumHeat::On: return "on";
                    case TFSXW1Enums::MinimumHeat::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::Mode:
                if (!this->isPoweringOn && !this->controller.isPoweredOn()) {
                    return "off";
                }

                switch (static_cast<TFSXW1Enums::Mode>(reg->value)) {
                    case TFSXW1Enums::Mode::Auto: return "auto";
                    case TFSXW1Enums::Mode::Cool: return "cool";
                    case TFSXW1Enums::Mode::Dry: return "dry";
                    case TFSXW1Enums::Mode::Fan: return "fan_only";
                    case TFSXW1Enums::Mode::Heat: return "heat";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::FanSpeed:
                switch (static_cast<TFSXW1Enums::FanSpeed>(reg->value)) {
                    case TFSXW1Enums::FanSpeed::Auto: return "auto";
                    case TFSXW1Enums::FanSpeed::Quiet: return "quiet";
                    case TFSXW1Enums::FanSpeed::Low: return "low";
                    case TFSXW1Enums::FanSpeed::Medium: return "medium";
                    case TFSXW1Enums::FanSpeed::High: return "high";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::VerticalSwing:
                switch (static_cast<TFSXW1Enums::VerticalSwing>(reg->value)) {
                    case TFSXW1Enums::VerticalSwing::On: return "on";
                    case TFSXW1Enums::VerticalSwing::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::VerticalAirflow:
                switch (static_cast<TFSXW1Enums::VerticalAirflow>(reg->value)) {
                    case TFSXW1Enums::VerticalAirflow::Position1: return "1";
                    case TFSXW1Enums::VerticalAirflow::Position2: return "2";
                    case TFSXW1Enums::VerticalAirflow::Position3: return "3";
                    case TFSXW1Enums::VerticalAirflow::Position4: return "4";
                    case TFSXW1Enums::VerticalAirflow::Position5: return "5";
                    case TFSXW1Enums::VerticalAirflow::Position6: return "6";
                    case TFSXW1Enums::VerticalAirflow::Swing: return "1";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::HorizontalSwing:
                switch (static_cast<TFSXW1Enums::HorizontalSwing>(reg->value)) {
                    case TFSXW1Enums::HorizontalSwing::On: return "on";
                    case TFSXW1Enums::HorizontalSwing::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::HorizontalAirflow:
                switch (static_cast<TFSXW1Enums::HorizontalAirflow>(reg->value)) {
                    case TFSXW1Enums::HorizontalAirflow::Position1: return "1";
                    case TFSXW1Enums::HorizontalAirflow::Position2: return "2";
                    case TFSXW1Enums::HorizontalAirflow::Position3: return "3";
                    case TFSXW1Enums::HorizontalAirflow::Position4: return "4";
                    case TFSXW1Enums::HorizontalAirflow::Position5: return "5";
                    case TFSXW1Enums::HorizontalAirflow::Position6: return "6";
                    case TFSXW1Enums::HorizontalAirflow::Swing: return "1";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::Powerful:
                switch (static_cast<TFSXW1Enums::Powerful>(reg->value)) {
                    case TFSXW1Enums::Powerful::On: return "on";
                    case TFSXW1Enums::Powerful::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::EconomyMode:
                switch (static_cast<TFSXW1Enums::EconomyMode>(reg->value)) {
                    case TFSXW1Enums::EconomyMode::On: return "on";
                    case TFSXW1Enums::EconomyMode::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::EnergySavingFan:
                switch (static_cast<TFSXW1Enums::EnergySavingFan>(reg->value)) {
                    case TFSXW1Enums::EnergySavingFan::On: return "on";
                    case TFSXW1Enums::EnergySavingFan::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::OutdoorUnitLowNoise:
                switch (static_cast<TFSXW1Enums::OutdoorUnitLowNoise>(reg->value)) {
                    case TFSXW1Enums::OutdoorUnitLowNoise::On: return "on";
                    case TFSXW1Enums::OutdoorUnitLowNoise::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::CoilDry:
                switch (static_cast<TFSXW1Enums::CoilDry>(reg->value)) {
                    case TFSXW1Enums::CoilDry::On: return "on";
                    case TFSXW1Enums::CoilDry::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::HumanSensor:
                switch (static_cast<TFSXW1Enums::HumanSensor>(reg->value)) {
                    case TFSXW1Enums::HumanSensor::On: return "on";
                    case TFSXW1Enums::HumanSensor::Off: return "off";
                    default: return "unknown";
                }

                break;
            case TFSXW1Controller::Address::SetpointTemp: {
                static char str[8];
                snprintf(str, sizeof(str), "%u.%u", reg->value / 10, reg->value % 10);

                return str;
            }
                
            case TFSXW1Controller::Address::ActualTemp: {
                static char str[8];
                snprintf(str, sizeof(str), "%u.%u", (reg->value - 5025) / 100, (reg->value - 5025) % 100);

                return str;
            }

            case TFSXW1Controller::Address::OutdoorTemp: {
                int val = reg->value - 5025;
                static char str[8];
                snprintf(str, sizeof(str), "%d.%02d", val / 100, abs(val % 100));

                return str;
            }

            default: {
                static char buffer[20];
                snprintf(buffer, sizeof(buffer), "%04X", static_cast<uint16_t>(reg->value));

                return buffer;
            }
        }
    }

    const TFSXW1Enums::Power TFSXW1Bridge::stringToEnum(TFSXW1Enums::Power def, const char *value) {
        if (0 == strcmp(value, "on")) {
            return TFSXW1Enums::Power::On;
        }

        return def;
    }

    const TFSXW1Enums::MinimumHeat TFSXW1Bridge::stringToEnum(TFSXW1Enums::MinimumHeat def, const char *value) {
        if (0 == strcmp(value, "on")) {
            return TFSXW1Enums::MinimumHeat::On;
        }

        return def;
    }

    const TFSXW1Enums::Mode TFSXW1Bridge::stringToEnum(TFSXW1Enums::Mode def, const char *value) {
        if (0 == strcmp(value, "cool")) {
            return TFSXW1Enums::Mode::Cool;
        } else if (0 == strcmp(value, "dry")) {
            return TFSXW1Enums::Mode::Dry;
        } else if (0 == strcmp(value, "fan_only")) {
            return TFSXW1Enums::Mode::Fan;
        } else if (0 == strcmp(value, "heat")) {
            return TFSXW1Enums::Mode::Heat;
        }

        return def;
    }

    const TFSXW1Enums::FanSpeed TFSXW1Bridge::stringToEnum(TFSXW1Enums::FanSpeed def, const char *value) {
        if (0 == strcmp(value, "auto")) {
            return TFSXW1Enums::FanSpeed::Auto;
        } else if (0 == strcmp(value, "quiet")) {
            return TFSXW1Enums::FanSpeed::Quiet;
        } else if (0 == strcmp(value, "low")) {
            return TFSXW1Enums::FanSpeed::Low;
        } else if (0 == strcmp(value, "medium")) {
            return TFSXW1Enums::FanSpeed::Medium;
        } else if (0 == strcmp(value, "high")) {
            return TFSXW1Enums::FanSpeed::High;
        }

        return def;
    }

    const TFSXW1Enums::VerticalAirflow TFSXW1Bridge::stringToEnum(TFSXW1Enums::VerticalAirflow def, const char *value) {
        if (0 == strcmp(value, "1") ) {
            return TFSXW1Enums::VerticalAirflow::Position1;
        } else if (strcmp(value, "2") == 0) {
            return TFSXW1Enums::VerticalAirflow::Position2;
        } else if (strcmp(value, "3") == 0) {
            return TFSXW1Enums::VerticalAirflow::Position3;
        } else if (strcmp(value, "4") == 0) {
            return TFSXW1Enums::VerticalAirflow::Position4;
        } else if (strcmp(value, "5") == 0) {
            return TFSXW1Enums::VerticalAirflow::Position5;
        } else if (strcmp(value, "6") == 0) {
            return TFSXW1Enums::VerticalAirflow::Position6;
        }

        return def;
    }

    const TFSXW1Enums::VerticalSwing TFSXW1Bridge::stringToEnum(TFSXW1Enums::VerticalSwing def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::VerticalSwing::On;
        }

        return def;
    }

    const TFSXW1Enums::HorizontalAirflow TFSXW1Bridge::stringToEnum(TFSXW1Enums::HorizontalAirflow def, const char *value) {
        if (0 == strcmp(value, "1") ) {
            return TFSXW1Enums::HorizontalAirflow::Position1;
        } else if (strcmp(value, "2") == 0) {
            return TFSXW1Enums::HorizontalAirflow::Position2;
        } else if (strcmp(value, "3") == 0) {
            return TFSXW1Enums::HorizontalAirflow::Position3;
        } else if (strcmp(value, "4") == 0) {
            return TFSXW1Enums::HorizontalAirflow::Position4;
        } else if (strcmp(value, "5") == 0) {
            return TFSXW1Enums::HorizontalAirflow::Position5;
        } else if (strcmp(value, "6") == 0) {
            return TFSXW1Enums::HorizontalAirflow::Position6;
        }

        return def;
    }

    const TFSXW1Enums::HorizontalSwing TFSXW1Bridge::stringToEnum(TFSXW1Enums::HorizontalSwing def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::HorizontalSwing::On;
        }

        return def;
    }

    const TFSXW1Enums::Powerful TFSXW1Bridge::stringToEnum(TFSXW1Enums::Powerful def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::Powerful::On;
        }

        return def;
    }

    const TFSXW1Enums::EconomyMode TFSXW1Bridge::stringToEnum(TFSXW1Enums::EconomyMode def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::EconomyMode::On;
        }

        return def;
    }

    const TFSXW1Enums::EnergySavingFan TFSXW1Bridge::stringToEnum(TFSXW1Enums::EnergySavingFan def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::EnergySavingFan::On;
        }

        return def;
    }

    const TFSXW1Enums::OutdoorUnitLowNoise TFSXW1Bridge::stringToEnum(TFSXW1Enums::OutdoorUnitLowNoise def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::OutdoorUnitLowNoise::On;
        }

        return def;
    }

    const TFSXW1Enums::CoilDry TFSXW1Bridge::stringToEnum(TFSXW1Enums::CoilDry def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::CoilDry::On;
        }

        return def;
    }

    const TFSXW1Enums::HumanSensor TFSXW1Bridge::stringToEnum(TFSXW1Enums::HumanSensor def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return TFSXW1Enums::HumanSensor::On;
        }

        return def;
    }
}