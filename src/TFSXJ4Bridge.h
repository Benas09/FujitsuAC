/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "IMqttBridge.h"
#include <Arduino.h>
#include "RegistryTable.h"
#include "TFSXJ4Controller.h"
#include <Uart.h>

namespace FujitsuAC {

    class TFSXJ4Bridge: public IMqttBridge {
        public:
            TFSXJ4Bridge(
                Config &config,
                PubSubClient &mqttClient
            );

            void setup() override;
            void loop() override;

            void onRegisterChange(const RegistryTable::Register *reg);

        protected:
            const char* getProtocolName() override {
                return "UTY-TFSXJ4";
            }

            void handleMqttCommand(const char *command, const char *property) override;

        private:
            enum UartStatus: int {
                Start = 0,
                High = 1,
                Low = 2,
                Initialized = 3,
            };

            UartStatus _uartStatus = UartStatus::Start;
            uint32_t _uartTimer = 0;

            Stream *_uart = nullptr;
            TFSXJ4Controller *_controller = nullptr;

            void initializeUart();
    };

}