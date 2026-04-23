/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "TFSXJ4Bridge.h"

namespace FujitsuAC {
    TFSXJ4Bridge::TFSXJ4Bridge(
        Config &config,
        PubSubClient &mqttClient
    ):
        IMqttBridge(config, mqttClient)
    {}

    void TFSXJ4Bridge::setup() {
        IMqttBridge::setup();
    }

    void TFSXJ4Bridge::loop() {
        IMqttBridge::loop();

        if (TFSXJ4Bridge::UartStatus::Initialized == _uartStatus) {
            _controller->loop();

            return;
        }

        this->initializeUart();
    }

    void TFSXJ4Bridge::initializeUart() {
        if (TFSXJ4Bridge::UartStatus::Start == _uartStatus) {
            _uartStatus = TFSXJ4Bridge::UartStatus::High;
            _uartTimer = millis();

            digitalWrite(_config.getTxPin(), HIGH);

            this->debug("info", "TFSXJ4: UartStatus::High");

            return;
        } else if (TFSXJ4Bridge::UartStatus::High == _uartStatus) {
            if (millis() - _uartTimer >= 8700) {
                _uartStatus = TFSXJ4Bridge::UartStatus::Low;
                _uartTimer = millis();

                digitalWrite(_config.getTxPin(), LOW);

                this->debug("info", "TFSXJ4: UartStatus::Low");
            }

            return;
        } else if (TFSXJ4Bridge::UartStatus::Low == _uartStatus) {
            if (millis() - _uartTimer >= 11000) {
                _uartStatus = TFSXJ4Bridge::UartStatus::Initialized;

                _uart = new Uart(UART_NUM_2, _config.getRxPin(), _config.getTxPin());
                _controller = new TFSXJ4Controller(*_uart);

                _controller->setOnRegisterChangeCallback([this](const RegistryTable::Register* reg) {
                    this->onRegisterChange(reg);
                });

                _controller->setDebugCallback([this](const char* name, const char* message) {
                    this->debug(name, message);
                });

                _controller->setup();

                this->debug("info", "TFSXJ4: UartStatus::Initialized");
            }
        }
    }

    void TFSXJ4Bridge::handleMqttCommand(const char *property, const char* payload) {

    }

    void TFSXJ4Bridge::onRegisterChange(const RegistryTable::Register *reg) {

    }
}