/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "PubSubClient.h"
#include <WiFi.h>
#include "esp_system.h"
#include "Config.h"
#include "NetworkUpdater.h"

namespace FujitsuAC {

    class IMqttBridge {
        public:
            IMqttBridge(
                Config &config,
                PubSubClient &mqttClient
            ): 
                _config(config),
                mqttClient(mqttClient)
            {}

            virtual ~IMqttBridge() = default;

            virtual void setup() {
                this->configureMqtt();

                this->networkUpdater = new NetworkUpdater();
                this->networkUpdater->setDebugCallback([this](const char* name, const char* message) {
                    this->debug(name, message);
                });

                this->networkUpdater->setOnVersionReceivedCallback([this](const char* version) {
                    this->publishState("latest_version", version);
                });

                this->networkUpdater->setup();
            }

            void configureMqtt() {
                char topic[128];
                snprintf(topic, sizeof(topic), "fujitsu/%s/status", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, "online", true);

                this->debug("info", "MQTT Connected");

                this->createDeviceConfig();
                this->registerDiagnosticEntities();
                this->sendInitialDiagnosticData();
                this->sendDiagnosticData();

                _config.setLedsStatus(_config.isLedsOn());

                this->mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
                    char message[length + 1];
                    memcpy(message, payload, length);
                    message[length] = '\0';

                    this->onMqtt(topic, message);
                });

                snprintf(topic, sizeof(topic), "fujitsu/%s/#", _config.getUniqueId().c_str());
                this->mqttClient.subscribe(topic);
            }

            virtual void loop() {
                this->networkUpdater->loop();
                this->sendDiagnosticData();
            }

            void publishState(const char* name, const char* value) {
                char topic[64];

                snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", _config.getUniqueId().c_str(), name);
                this->mqttClient.publish(topic, value, true);
            }

            void debug(const char* name, const char* message) {
                if (strcmp(name, "status") == 0) {
                    this->publishState(name, message);
                    
                    return;
                }

                char topic[64];
                snprintf(topic, sizeof(topic), "fujitsu/%s/debug/%s", _config.getUniqueId().c_str(), name);
                this->mqttClient.publish(topic, message);
            }

        protected:
            Config &_config;
            PubSubClient &mqttClient;
            String deviceConfig;

            virtual const char* getProtocolName() = 0;
            virtual void handleMqttCommand(const char *property, const char *payload) = 0;

        private:
            NetworkUpdater* networkUpdater = nullptr;

            uint32_t lastDiagnosticReportMillis = -60000;

            void createDeviceConfig() {
                if (0 == this->deviceConfig.length()) {
                    this->deviceConfig = "\"device\": {";
                    this->deviceConfig += "\"identifiers\": [\"" + _config.getUniqueId() + "\"],";
                    this->deviceConfig += "\"manufacturer\": \"bepro.lt\",";
                    this->deviceConfig += "\"model\": \"faircon\",";
                    this->deviceConfig += "\"name\": \"" + _config.getDeviceName() + "\"";
                    this->deviceConfig += "}";
                }
            }

            void registerDiagnosticEntities() {
                char topic[128];

                String p = "{";
                p += "\"name\": \"status\",";
                p += "\"icon\": \"mdi:information\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/status\",";
                p += "\"device_class\": \"enum\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_status\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_status/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"name\",";
                p += "\"icon\": \"mdi:text-recognition\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/name\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_name\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_name/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"wifi_rssi\",";
                p += "\"icon\": \"mdi:wifi\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/wifi_rssi\",";
                p += "\"device_class\": \"signal_strength\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unit_of_measurement\": \"dB\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_wifi_rssi\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_wifi_rssi/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"ip\",";
                p += "\"icon\": \"mdi:ip\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/ip\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_ip\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_ip/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"mac\",";
                p += "\"icon\": \"mdi:identifier\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/mac\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_mac\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_mac/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"version\",";
                p += "\"icon\": \"mdi:git\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/version\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_version\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_version/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"latest_version\",";
                p += "\"icon\": \"mdi:git\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/latest_version\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_latest_version\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_latest_version/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"protocol\",";
                p += "\"icon\": \"mdi:git\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/protocol\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_protocol\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_protocol/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"reset_reason\",";
                p += "\"icon\": \"mdi:restart\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/reset_reason\",";
                p += "\"device_class\": \"enum\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_reset_reason\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_reset_reason/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"cpu_temp\",";
                p += "\"icon\": \"mdi:thermometer\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/cpu_temp\",";
                p += "\"device_class\": \"temperature\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unit_of_measurement\": \"°C\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_cpu_temp\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_cpu_temp/config", _config.getUniqueId().c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                this->debug("info", "Diagnostic entities registered");

                p = "{";
                p += "\"name\": \"restart\",";
                p += "\"icon\": \"mdi:restart\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_restart\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/restart\",";
                p += "\"entity_category\": \"config\",";
                p += "\"payload_press\": \"restart\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/button/%s_%s/config", _config.getUniqueId().c_str(), "restart");
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"update_firmware\",";
                p += "\"icon\": \"mdi:update\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_update_firmware\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/update_firmware\",";
                p += "\"entity_category\": \"config\",";
                p += "\"payload_press\": \"master\",";
                
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/button/%s_%s/config", _config.getUniqueId().c_str(), "update_firmware");
                this->mqttClient.publish(topic, p.c_str(), true);

                if (_config.getLedRPin() > 0) {
                    p = "{";
                    p += "\"name\": \"leds\",";
                    p += "\"icon\": \"mdi:led-outline\",";
                    p += "\"unique_id\": \"" + _config.getUniqueId() + "_leds\",";
                    p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                    p += "\"payload_available\": \"online\",";
                    p += "\"payload_not_available\": \"offline\",";
                    p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/leds\",";
                    p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/leds\",";
                    p += "\"entity_category\": \"config\",";
                    p += "\"payload_on\": \"on\",";
                    p += "\"payload_off\": \"off\",";
                    
                    p += this->deviceConfig;
                    p += "}";

                    snprintf(topic, sizeof(topic), "homeassistant/switch/%s_%s/config", _config.getUniqueId().c_str(), "leds");
                    this->mqttClient.publish(topic, p.c_str(), true);
                }

                p = "{";
                p += "\"name\": \"wifi_sleep\",";
                p += "\"icon\": \"mdi:wifi-arrow-down\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_wifi_sleep\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/wifi_sleep\",";
                p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/wifi_sleep\",";
                p += "\"entity_category\": \"config\",";
                p += "\"payload_on\": \"on\",";
                p += "\"payload_off\": \"off\",";
                
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/switch/%s_%s/config", _config.getUniqueId().c_str(), "wifi_sleep");
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"low_cpu_speed\",";
                p += "\"icon\": \"mdi:speedometer-slow\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_slow_cpu\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + _config.getUniqueId() + "/state/low_cpu_speed\",";
                p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/low_cpu_speed\",";
                p += "\"entity_category\": \"config\",";
                p += "\"payload_on\": \"on\",";
                p += "\"payload_off\": \"off\",";
                
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/switch/%s_%s/config", _config.getUniqueId().c_str(), "low_cpu_speed");
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
                p += "\"name\": \"clear_credentials\",";
                p += "\"icon\": \"mdi:delete-alert\",";
                p += "\"unique_id\": \"" + _config.getUniqueId() + "_clear_credentials\",";
                p += "\"availability_topic\": \"fujitsu/" + _config.getUniqueId() + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"command_topic\": \"fujitsu/" + _config.getUniqueId() + "/set/clear_credentials\",";
                p += "\"entity_category\": \"config\",";
                p += "\"payload_press\": \"clear_credentials\",";
                
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/button/%s_%s/config", _config.getUniqueId().c_str(), "clear_credentials");
                this->mqttClient.publish(topic, p.c_str(), true);

                this->debug("info", "Configuration entities registered");
            }

            void sendInitialDiagnosticData() {
                this->publishState("status", "MqttBridge started");
                this->publishState("name", _config.getDeviceName().c_str());
                this->publishState("ip", WiFi.localIP().toString().c_str());
                this->publishState("mac", WiFi.macAddress().c_str());
                this->publishState("version", _config.getVersion());
                this->publishState("protocol", this->getProtocolName());
                this->publishState("reset_reason", this->getResetReason());
                this->publishState("wifi_sleep", _config.isWifiSleepEnabled() ? "on" : "off");
                this->publishState("low_cpu_speed", _config.isLowCpuSpeedEnabled() ? "on" : "off");

                if (_config.getLedRPin() > 0) {
                    this->publishState("leds", _config.isLedsOn() ? "on" : "off");
                }
            }
            
            void sendDiagnosticData() {
                if ((millis() - this->lastDiagnosticReportMillis) < 60000) {
                    return;
                }

                char buffer[8];

                snprintf(buffer, sizeof(buffer), "%d", WiFi.RSSI());
                this->publishState("wifi_rssi", buffer);

                float cpuTemp = temperatureRead();
                
                if (!isnan(cpuTemp) && cpuTemp > -20 && cpuTemp < 150) {
                    snprintf(buffer, sizeof(buffer), "%.2f", cpuTemp);
                    this->publishState("cpu_temp", buffer);
                }

                this->lastDiagnosticReportMillis = millis();
            }
            
            void onMqtt(char* topic, char* payload) {
                String t = String(topic);

                int lastSlash = t.lastIndexOf('/');
                int secondLastSlash = t.lastIndexOf('/', lastSlash - 1);

                String com = t.substring(secondLastSlash + 1, lastSlash);
                String prop = t.substring(lastSlash + 1).c_str();

                const char *command = com.c_str();
                const char *property = prop.c_str();

                if (0 != strcmp(command, "set")) {
                    return;
                }

                if (0 == strcmp(property, "restart")) {
                    ESP.restart();

                    return;
                }

                if (0 == strcmp(property, "clear_credentials")) {
                    _config.clear();

                    delay(1000);
                    ESP.restart();

                    return;
                }

                if (0 == strcmp(property, "update_firmware")) {
                    this->networkUpdater->updateFirmware(payload);

                    return;
                }

                if (0 == strcmp(property, "leds")) {
                    _config.setLedsStatus(0 == strcmp(payload, "on"));
                    this->publishState("leds", _config.isLedsOn() ? "on" : "off");

                    return;
                }

                if (0 == strcmp(property, "wifi_sleep")) {
                    _config.setWifiSleepEnabled(0 == strcmp(payload, "on"));
                    this->publishState("wifi_sleep", _config.isWifiSleepEnabled() ? "on" : "off");

                    return;
                }

                if (0 == strcmp(property, "low_cpu_speed")) {
                    _config.setLowCpuSpeedEnabled(0 == strcmp(payload, "on"));
                    this->publishState("low_cpu_speed", _config.isLowCpuSpeedEnabled() ? "on" : "off");

                    return;
                }

                this->handleMqttCommand(property, payload);
            }

            static const char* getResetReason() {
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
    };

}