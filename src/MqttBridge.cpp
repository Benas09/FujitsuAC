/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "MqttBridge.h"
#include <WiFi.h>
#include "esp_system.h"

namespace FujitsuAC {

    MqttBridge::MqttBridge(
        PubSubClient &mqttClient, 
        FujitsuController &controller,
        const char* uniqueId,
        const char* name,
        const char* version
    ) : mqttClient(mqttClient), 
        controller(controller), 
        uniqueId(uniqueId),
        name(name),
        version(version) {}

    bool MqttBridge::setup() {
        this->controller.setOnRegisterChangeCallback([this](const Register* reg) {
            this->onRegisterChange(reg);
        });

        this->controller.setDebugCallback([this](const char* name, const char* message) {
            this->debug(name, message);
        });

        char topic[128];
        snprintf(topic, sizeof(topic), "fujitsu/%s/status", this->uniqueId.c_str());
        this->mqttClient.publish(topic, "online", true);

        this->debug("info", "MQTT Connected");

        this->createDeviceConfig();
        this->registerDiagnosticEntities();
        this->sendInitialDiagnosticData();
        this->sendDiagnosticData();

        this->registerBaseEntities();

        static constexpr Address switches[] = {
            Address::Power,
            Address::VerticalAirflow,
            Address::VerticalSwing,
            Address::Powerful,
            Address::EconomyMode,
            Address::EnergySavingFan,
            Address::OutdoorUnitLowNoise,
            Address::MinimumHeat
        };

        for (const auto& switch_ : switches) {
            this->registerSwitch(switch_);
        }

        this->mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
            char message[length + 1];
            memcpy(message, payload, length);
            message[length] = '\0';

            this->onMqtt(topic, message);
        });

        snprintf(topic, sizeof(topic), "fujitsu/%s/#", this->uniqueId.c_str());
        this->mqttClient.subscribe(topic);

        //Send current registry values after MQTT connection
        size_t registryCount;
        const Register* registers = this->controller.getAllRegisters(registryCount);

        for (size_t i = 0; i < registryCount; ++i) {
            if (
                registers[i].address == Address::ActualTemp
                || registers[i].address == Address::OutdoorTemp
            ) {
                continue;
            }

            this->onRegisterChange(&registers[i]);
        }

        return true;
    }

    bool MqttBridge::loop() {
        this->sendDiagnosticData();

        if (this->isPoweringOn && !this->controller.isPoweredOn()) {
            this->controller.setPower(Enums::Power::On);
        }

        return true;
    }

    void MqttBridge::setOnFirmwareUpdateRequestCallback(std::function<void()> onFirmwareUpdateRequestCallback) {
        this->onFirmwareUpdateRequestCallback = onFirmwareUpdateRequestCallback;
    }

    void MqttBridge::createDeviceConfig() {
        if (0 == this->deviceConfig.length()) {
            this->deviceConfig = "\"device\": {";
            this->deviceConfig += "\"identifiers\": [\"" + this->uniqueId + "\"],";
            this->deviceConfig += "\"manufacturer\": \"bepro.lt\",";
            this->deviceConfig += "\"model\": \"fAir\",";
            this->deviceConfig += "\"name\": \"" + this->name + "\"";
            this->deviceConfig += "}";
        }
    }

    void MqttBridge::registerDiagnosticEntities() {
        char topic[128];

        String p = "{";
        p += "\"name\": \"restart\",";
        p += "\"icon\": \"mdi:restart\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_restart\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"command_topic\": \"fujitsu/" + this->uniqueId + "/set/restart\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"payload_press\": \"restart\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/button/%s_%s/config", this->uniqueId.c_str(), "restart");
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"update_firmware\",";
        p += "\"icon\": \"mdi:update\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_update_firmware\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"command_topic\": \"fujitsu/" + this->uniqueId + "/set/update_firmware\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"payload_press\": \"update_firmware\",";
        
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/button/%s_%s/config", this->uniqueId.c_str(), "update_firmware");
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"name\",";
        p += "\"icon\": \"mdi:text-recognition\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/name\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_name\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_name/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"wifi_rssi\",";
        p += "\"icon\": \"mdi:wifi\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/wifi_rssi\",";
        p += "\"device_class\": \"signal_strength\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"unit_of_measurement\": \"dB\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_wifi_rssi\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_wifi_rssi/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"ip\",";
        p += "\"icon\": \"mdi:ip\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/ip\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_ip\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_ip/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"version\",";
        p += "\"icon\": \"mdi:git\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/version\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_version\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_version/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"latest_version\",";
        p += "\"icon\": \"mdi:git\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/latest_version\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_latest_version\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_latest_version/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"reset_reason\",";
        p += "\"icon\": \"mdi:restart\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/reset_reason\",";
        p += "\"device_class\": \"enum\",";
        p += "\"entity_category\": \"diagnostic\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_reset_reason\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_reset_reason/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        this->debug("info", "Diagnostic entities registered");
    }

    void MqttBridge::registerBaseEntities() {
        char topic[128];

        String p = "{";
        p += "\"name\": \"climate\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_climate\",";
        p += "\"icon\": \"mdi:air-conditioner\",";

        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";

        p += "\"mode_command_topic\": \"fujitsu/" + this->uniqueId + "/set/mode\",";
        p += "\"mode_state_topic\": \"fujitsu/" + this->uniqueId + "/state/mode\",";

        p += "\"temperature_command_topic\": \"fujitsu/" + this->uniqueId + "/set/temp\",";
        p += "\"temperature_state_topic\": \"fujitsu/" + this->uniqueId + "/state/temp\",";

        p += "\"fan_mode_command_topic\": \"fujitsu/" + this->uniqueId + "/set/fan\",";
        p += "\"fan_mode_state_topic\": \"fujitsu/" + this->uniqueId + "/state/fan\",";

        p += "\"current_temperature_topic\": \"fujitsu/" + this->uniqueId + "/state/actual_temp\",";
        p += "\"current_humidity_topic\": \"fujitsu/" + this->uniqueId + "/state/humidity\",";

        p += "\"min_temp\": 18,";
        p += "\"max_temp\": 30,";
        p += "\"temp_step\": 0.5,";
        p += "\"modes\": [\"off\", \"auto\", \"cool\", \"dry\", \"fan_only\", \"heat\"],";
        p += "\"fan_modes\": [\"auto\", \"quiet\", \"low\", \"medium\", \"high\"],";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/climate/%s_climate/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"actual_temp\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/actual_temp\",";
        p += "\"unit_of_measurement\": \"°C\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_actual_temp\",";
        p += "\"device_class\": \"temperature\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_actual_temp/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        p = "{";
        p += "\"name\": \"outdoor_temp\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/outdoor_temp\",";
        p += "\"unit_of_measurement\": \"°C\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_outdoor_temp\",";
        p += "\"device_class\": \"temperature\",";
        p += this->deviceConfig;
        p += "}";

        snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_outdoor_temp/config", this->uniqueId.c_str());
        this->mqttClient.publish(topic, p.c_str(), true);

        this->debug("info", "Base entities registered");
    }

    void MqttBridge::registerSwitch(Address address) {
        String propertyName = String(this->addressToString(address));
        
        String p = "{";
        p += "\"name\": \"" + propertyName + "\",";
        p += "\"unique_id\": \"" + this->uniqueId + "_" + propertyName + "\",";
        p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
        p += "\"payload_available\": \"online\",";
        p += "\"payload_not_available\": \"offline\",";
        p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/" + propertyName + "\",";
        p += "\"command_topic\": \"fujitsu/" + this->uniqueId + "/set/" + propertyName + "\",";

        if (
            Address::VerticalAirflow == address
            || Address::HorizontalAirflow == address
        ) {
            p += "\"options\": [\"1\", \"2\", \"3\", \"4\", \"5\", \"6\"],";
        } else {
            p += "\"payload_on\": \"on\",";
            p += "\"payload_off\": \"off\",";
        }
        
        p += this->deviceConfig;
        p += "}";

        char topic[128];

        if (
            Address::VerticalAirflow == address
            || Address::HorizontalAirflow == address
        ) {
            snprintf(topic, sizeof(topic), "homeassistant/select/%s_%s/config", this->uniqueId.c_str(), propertyName.c_str());
        } else {
            snprintf(topic, sizeof(topic), "homeassistant/switch/%s_%s/config", this->uniqueId.c_str(), propertyName.c_str());
        }

        this->mqttClient.publish(topic, p.c_str(), true);

        char message[64];
        snprintf(message, sizeof(message), "Switch '%s' registered", propertyName.c_str());

        this->debug("info", message);
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

        if (0 == strcmp(property, "update_firmware")) {
            if (this->onFirmwareUpdateRequestCallback) {
                this->onFirmwareUpdateRequestCallback();
            }

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::Power))) {
            this->controller.setPower(this->stringToEnum(Enums::Power::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::MinimumHeat))) {
            this->controller.setMinimumHeat(this->stringToEnum(Enums::MinimumHeat::Off, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::Mode))) {
            if (0 == strcmp(payload, "off")) {
                this->controller.setPower(Enums::Power::Off);

                return;
            }

            this->controller.setMode(this->stringToEnum(Enums::Mode::Auto, payload));

            if (!this->controller.isPoweredOn()) {
                this->isPoweringOn = true;
            }

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

        if (0 == strcmp(property, this->addressToString(Address::HorizontalAirflow))) {
            this->controller.setHorizontalAirflow(this->stringToEnum(Enums::HorizontalAirflow::Position1, payload));

            return;
        }

        if (0 == strcmp(property, this->addressToString(Address::HorizontalSwing))) {
            this->controller.setHorizontalSwing(this->stringToEnum(Enums::HorizontalSwing::Off, payload));

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

    void MqttBridge::publishState(Address address, const char* value)
    {
        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), this->addressToString(address));

        this->mqttClient.publish(topic, value, true);
    }

    void MqttBridge::onRegisterChange(const Register *reg) {
        if (reg->address == Address::ActualTemp) {
            uint32_t now = millis();

            if ((now - this->lastTempReportMillis) < 180000) {
                return;
            }
            
            this->lastTempReportMillis = now;
        }

        if (reg->address == Address::SetpointTemp && 0xFFFF == reg->value) {
            // Fan mode do not return setpoint temp
            return;
        }

        this->publishState(reg->address, this->valueToString(reg));

        if (Address::Power == reg->address) {
            if (static_cast<uint16_t>(Enums::Power::On) == reg->value) {
                this->isPoweringOn = false;
            }

            // Workaround to get shown required mode shown immediately after turn off
            Register* modeRegister = this->controller.getRegister(Address::Mode);
            this->publishState(modeRegister->address, this->valueToString(modeRegister));
        }

        if (Address::HumanSensorSupported == reg->address && this->controller.isHumanSensorSupported()) {
            this->registerSwitch(Address::HumanSensor);
        }

        if (Address::HorizontalSwingSupported == reg->address && this->controller.isHorizontalSwingSupported()) {
            this->registerSwitch(Address::HorizontalSwing);
            this->registerSwitch(Address::HorizontalAirflow);
        }
    }

    void MqttBridge::sendInitialDiagnosticData() {
        char topic[64];

        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), "name");
        this->mqttClient.publish(topic, this->name.c_str(), true);

        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), "ip");
        this->mqttClient.publish(topic, WiFi.localIP().toString().c_str(), true);

        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), "version");
        this->mqttClient.publish(topic, this->version.c_str(), true);

        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), "reset_reason");
        this->mqttClient.publish(topic, this->getResetReason(), true);
    }

    void MqttBridge::sendDiagnosticData() {
        if ((millis() - this->lastDiagnosticReportMillis) < 30000) {
            return;
        }

        char topic[64];

        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), "wifi_rssi");
        char rssi[8];
        snprintf(rssi, sizeof(rssi), "%d", WiFi.RSSI());
        this->mqttClient.publish(topic, rssi, true);

        this->lastDiagnosticReportMillis = millis();
    }

    const char* MqttBridge::getResetReason() {
        esp_reset_reason_t reason = esp_reset_reason();

        switch (reason) {
            case ESP_RST_UNKNOWN: return "UNKNOWN";
            case ESP_RST_POWERON: return "POWERON";
            case ESP_RST_EXT: return "EXTERNAL";
            case ESP_RST_SW: return "SOFTWARE";
            case ESP_RST_PANIC: return "PANIC";
            case ESP_RST_INT_WDT: return "INT_WDT";
            case ESP_RST_TASK_WDT: return "TASK_WDT";
            case ESP_RST_WDT: return "WDT";
            case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
            case ESP_RST_BROWNOUT: return "BROWNOUT";
            case ESP_RST_SDIO: return "SDIO";
            default: return "INVALID";
        }
    }

    const char* MqttBridge::addressToString(Address address) {
        switch (address) {
            case Address::Power: return "power";
            case Address::Mode: return "mode";
            case Address::FanSpeed: return "fan";
            case Address::VerticalSwing: return "vertical_swing";
            case Address::VerticalAirflow: return "vertical_airflow";
            case Address::HorizontalSwing: return "horizontal_swing";
            case Address::HorizontalAirflow: return "horizontal_airflow";
            case Address::Powerful: return "powerful";
            case Address::EconomyMode: return "economy_mode";
            case Address::EnergySavingFan: return "energy_saving_fan";
            case Address::OutdoorUnitLowNoise: return "outdoor_unit_low_noise";
            case Address::SetpointTemp: return "temp";
            case Address::ActualTemp: return "actual_temp";
            case Address::OutdoorTemp: return "outdoor_temp";
            case Address::HumanSensor: return "human_sensor";
            case Address::MinimumHeat: return "minimum_heat";
            default: {
                static char buffer[20];
                snprintf(buffer, sizeof(buffer), "address_%04X", static_cast<uint16_t>(address));

                return buffer;
            }
        }
    }

    const char* MqttBridge::valueToString(const Register *reg) {
        switch (reg->address) {
            case Address::Power:
                switch (static_cast<Enums::Power>(reg->value)) {
                    case Enums::Power::On: return "on";
                    case Enums::Power::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::MinimumHeat:
                switch (static_cast<Enums::MinimumHeat>(reg->value)) {
                    case Enums::MinimumHeat::On: return "on";
                    case Enums::MinimumHeat::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::Mode:
                if (!this->isPoweringOn && !this->controller.isPoweredOn()) {
                    return "off";
                }

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
                    case Enums::VerticalAirflow::Swing: return "1";
                    default: return "unknown";
                }

                break;
            case Address::HorizontalSwing:
                switch (static_cast<Enums::HorizontalSwing>(reg->value)) {
                    case Enums::HorizontalSwing::On: return "on";
                    case Enums::HorizontalSwing::Off: return "off";
                    default: return "unknown";
                }

                break;
            case Address::HorizontalAirflow:
                switch (static_cast<Enums::HorizontalAirflow>(reg->value)) {
                    case Enums::HorizontalAirflow::Position1: return "1";
                    case Enums::HorizontalAirflow::Position2: return "2";
                    case Enums::HorizontalAirflow::Position3: return "3";
                    case Enums::HorizontalAirflow::Position4: return "4";
                    case Enums::HorizontalAirflow::Position5: return "5";
                    case Enums::HorizontalAirflow::Position6: return "6";
                    case Enums::HorizontalAirflow::Swing: return "1";
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
            case Address::HumanSensor:
                switch (static_cast<Enums::HumanSensor>(reg->value)) {
                    case Enums::HumanSensor::On: return "on";
                    case Enums::HumanSensor::Off: return "off";
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

            case Address::OutdoorTemp: {
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

    const Enums::Power MqttBridge::stringToEnum(Enums::Power def, const char *value) {
        if (0 == strcmp(value, "on")) {
            return Enums::Power::On;
        }

        return def;
    }

    const Enums::MinimumHeat MqttBridge::stringToEnum(Enums::MinimumHeat def, const char *value) {
        if (0 == strcmp(value, "on")) {
            return Enums::MinimumHeat::On;
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

    const Enums::HorizontalAirflow MqttBridge::stringToEnum(Enums::HorizontalAirflow def, const char *value) {
        if (0 == strcmp(value, "1") ) {
            return Enums::HorizontalAirflow::Position1;
        } else if (strcmp(value, "2") == 0) {
            return Enums::HorizontalAirflow::Position2;
        } else if (strcmp(value, "3") == 0) {
            return Enums::HorizontalAirflow::Position3;
        } else if (strcmp(value, "4") == 0) {
            return Enums::HorizontalAirflow::Position4;
        } else if (strcmp(value, "5") == 0) {
            return Enums::HorizontalAirflow::Position5;
        } else if (strcmp(value, "6") == 0) {
            return Enums::HorizontalAirflow::Position6;
        }

        return def;
    }

    const Enums::HorizontalSwing MqttBridge::stringToEnum(Enums::HorizontalSwing def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::HorizontalSwing::On;
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

    const Enums::HumanSensor MqttBridge::stringToEnum(Enums::HumanSensor def, const char *value) {
        if (strcmp(value, "on") == 0) {
            return Enums::HumanSensor::On;
        }

        return def;
    }

    void MqttBridge::debug(const char* name, const char* message) {
        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/debug/%s", this->uniqueId.c_str(), name);

        this->mqttClient.publish(topic, message);
    }

    void MqttBridge::onVersionReceived(const char* version) {
        char topic[64];
        snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), "latest_version");

        this->mqttClient.publish(topic, version, true);
    }
}