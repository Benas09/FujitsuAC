/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "PubSubClient.h"
#include <WiFi.h>
#include "esp_system.h"
#include "NetworkUpdater.h"

namespace FujitsuAC {

    class IMqttBridge {
        public:
            IMqttBridge(
                PubSubClient &mqttClient,
                const char* uniqueId,
                const char* name,
                const char* version
            ): 
                mqttClient(mqttClient),
                uniqueId(uniqueId),
                name(name),
                version(version) 
            {}

            virtual ~IMqttBridge() = default;

            virtual void setup() {
                char topic[128];
                snprintf(topic, sizeof(topic), "fujitsu/%s/status", this->uniqueId.c_str());
                this->mqttClient.publish(topic, "online", true);

                this->debug("info", "MQTT Connected");

                this->createDeviceConfig();
                this->registerDiagnosticEntities();
                this->sendInitialDiagnosticData();
                this->sendDiagnosticData();

                this->mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
                    char message[length + 1];
                    memcpy(message, payload, length);
                    message[length] = '\0';

                    this->onMqtt(topic, message);
                });

                snprintf(topic, sizeof(topic), "fujitsu/%s/#", this->uniqueId.c_str());
                this->mqttClient.subscribe(topic);

                this->networkUpdater = new NetworkUpdater();
                this->networkUpdater->setDebugCallback([this](const char* name, const char* message) {
                    this->debug(name, message);
                });

                this->networkUpdater->setOnVersionReceivedCallback([this](const char* version) {
                    this->publishState("latest_version", version);
                });

                this->networkUpdater->setup();
            }

            virtual void loop() {
                this->sendDiagnosticData();
            }

            void publishState(const char* name, const char* value) {
                char topic[64];

                snprintf(topic, sizeof(topic), "fujitsu/%s/state/%s", this->uniqueId.c_str(), name);
                this->mqttClient.publish(topic, value, true);
            }

            void debug(const char* name, const char* message) {
                if (strcmp(name, "status") == 0) {
                    this->publishState(name, message);
                    
                    return;
                }

                char topic[64];
                snprintf(topic, sizeof(topic), "fujitsu/%s/debug/%s", this->uniqueId.c_str(), name);
                this->mqttClient.publish(topic, message);
            }

        protected:
            PubSubClient &mqttClient;

            String uniqueId;
            String name;
            String version;
            String deviceConfig;

            virtual const char* getProtocolName() = 0;
            virtual void handleMqttCommand(const char *property, const char *payload) = 0;

        private:
            NetworkUpdater* networkUpdater = nullptr;

            uint32_t lastDiagnosticReportMillis = -30000;

            void createDeviceConfig() {
                if (0 == this->deviceConfig.length()) {
                    this->deviceConfig = "\"device\": {";
                    this->deviceConfig += "\"identifiers\": [\"" + this->uniqueId + "\"],";
                    this->deviceConfig += "\"manufacturer\": \"bepro.lt\",";
                    this->deviceConfig += "\"model\": \"faircon\",";
                    this->deviceConfig += "\"name\": \"" + this->name + "\"";
                    this->deviceConfig += "}";
                }
            }

            void registerDiagnosticEntities() {
                char topic[128];

                String p = "{";
                p += "\"name\": \"status\",";
                p += "\"icon\": \"mdi:information\",";
                p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/status\",";
                p += "\"device_class\": \"enum\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + this->uniqueId + "_status\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_status/config", this->uniqueId.c_str());
                this->mqttClient.publish(topic, p.c_str(), true);

                p = "{";
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
                p += "\"name\": \"mac\",";
                p += "\"icon\": \"mdi:identifier\",";
                p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/mac\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + this->uniqueId + "_mac\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_mac/config", this->uniqueId.c_str());
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
                p += "\"name\": \"protocol\",";
                p += "\"icon\": \"mdi:git\",";
                p += "\"availability_topic\": \"fujitsu/" + this->uniqueId + "/status\",";
                p += "\"payload_available\": \"online\",";
                p += "\"payload_not_available\": \"offline\",";
                p += "\"state_topic\": \"fujitsu/" + this->uniqueId + "/state/protocol\",";
                p += "\"entity_category\": \"diagnostic\",";
                p += "\"unique_id\": \"" + this->uniqueId + "_protocol\",";
                p += this->deviceConfig;
                p += "}";

                snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_protocol/config", this->uniqueId.c_str());
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

            void sendInitialDiagnosticData() {
                this->publishState("status", "MqttBridge started");
                this->publishState("name", this->name.c_str());
                this->publishState("ip", WiFi.localIP().toString().c_str());
                this->publishState("mac", WiFi.macAddress().c_str());
                this->publishState("version", this->version.c_str());
                this->publishState("protocol", this->getProtocolName());
                this->publishState("reset_reason", this->getResetReason());
            }
            
            void sendDiagnosticData() {
                if ((millis() - this->lastDiagnosticReportMillis) < 30000) {
                    return;
                }

                char rssi[8];
                snprintf(rssi, sizeof(rssi), "%d", WiFi.RSSI());

                this->publishState("wifi_rssi", rssi);

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

                if (strcmp(command, "set") > 0) {
                    return;
                }

                if (0 == strcmp(property, "restart")) {
                    ESP.restart();

                    return;
                }

                if (0 == strcmp(property, "update_firmware")) {
                    this->networkUpdater->updateFirmware();

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