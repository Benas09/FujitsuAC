/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "TFSXW1Controller.h"

namespace FujitsuAC {

    TFSXW1Controller::TFSXW1Controller(Stream &uart): IFujitsuController(uart) {}

    void TFSXW1Controller::setup() {
        this->initRegistryTable();

        this->initialized = true;
        this->lastRequestMillis = millis();
    }

    void TFSXW1Controller::loop() {
        if (!this->initialized) {
            return;
        }

        this->sendRequest();

        this->buffer.loop([this](uint8_t buffer[128], int size, bool isValid) {
            this->onFrame(buffer, size, isValid);
        });
    }

    void TFSXW1Controller::sendRequest() {
        if (this->terminated) {
            return;
        }

        uint32_t now = millis();

        if (
            !this->lastResponseReceived
            && (now - this->lastRequestMillis) >= 200
        ) {
            if (FrameType::None == this->lastFrameSent || FrameType::Init1 == this->lastFrameSent) {
                // Communication not established yet. Initial request will be repeated
                this->lastFrameSent = FrameType::None;
                this->lastResponseReceived = true;
            } else if (!this->noResponseNotified) {
                this->noResponseNotified = true;
                this->debug("error", "No response for 200 ms");
                this->debug("status", "No response for 200 ms");
            }

            return;
        }

        if (
            this->lastResponseReceived 
            && (now - this->lastRequestMillis) >= 400
        ) {
            this->lastRequestMillis = now;

            switch (this->lastFrameSent) {
                case FrameType::None: {
                    this->lastFrameSent = FrameType::Init1;
                    this->lastResponseReceived = false;

                    uint8_t payload[] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFB};
                    this->debug("status", "Init1 Send");
                    this->debug("send", this->toHexStr(payload, sizeof(payload)));

                    uart.write(payload, sizeof(payload));

                    break;
                }

                case FrameType::Init1: {
                    this->lastFrameSent = FrameType::Init2;
                    this->lastResponseReceived = false;

                    uint8_t payload[] = {0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0xFF, 0xF5};
                    this->debug("status", "Init2 Send");
                    this->debug("send", this->toHexStr(payload, sizeof(payload)));

                    uart.write(payload, sizeof(payload));

                    break;
                }

                case FrameType::Init2:
                    this->requestRegistries(this->initialRegistries1);

                    break;

                case FrameType::InitialRegistries1:
                    this->requestRegistries(this->initialRegistries2);

                    break;

                case FrameType::InitialRegistries2:
                    this->requestRegistries(this->initialRegistries3);

                    break;

                case FrameType::InitialRegistries3:
                case FrameType::FrameC:
                    this->requestRegistries(this->frameA);

                    break;

                case FrameType::FrameA:
                    if (this->sendQueueSize > 0) {
                        this->sendRegistries();

                        break;
                    }
                case FrameType::SendRegistries: {
                    if (this->frameCheckRegistries.size > 0) {
                        Frame frame = this->frameCheckRegistries;

                        this->frameCheckRegistries.size = 0;

                        this->requestRegistries(frame);

                        break;
                    }
                }

                case FrameType::CheckRegistries:
                    this->requestRegistries(this->frameB);

                    break;

                case FrameType::FrameB:
                    this->requestRegistries(this->frameC);

                    break;
            }
        }
    }

    void TFSXW1Controller::requestRegistries(Frame frame) {
        this->lastFrameSent = frame.type;
        this->lastResponseReceived = false;

        size_t bufferSize = 2 * frame.size + 7;
        uint8_t request[bufferSize] = {
            0x03, 
            0x00, 
            0x00, 
            0x00, 
            2 * frame.size,
        };

        uint16_t checksum = 
            0xFFFF 
            - 0x03
            - (2 * frame.size)
        ;

        for (int i = 0; i < frame.size; i++) {
            Address addr = frame.registries[i];

            int index = 5 + i * 2;

            request[index] = (static_cast<uint16_t>(addr) >> 8) & 0xFF;
            request[index + 1] = static_cast<uint16_t>(addr) & 0xFF;

            checksum -= request[index];
            checksum -= request[index + 1];
        }

        request[bufferSize - 2] = (checksum >> 8) & 0xFF;
        request[bufferSize - 1] = checksum & 0xFF;

        uart.write(request, bufferSize);
    }

    void TFSXW1Controller::enqueueRegistries(const FrameSendRegistries &frame) {
        // A newer command for the same registries replaces the queued one, so
        // e.g. dragging a temperature slider does not fill the queue.
        for (size_t i = 0; i < this->sendQueueSize; ++i) {
            FrameSendRegistries &queued = this->sendQueue[i];

            if (queued.size != frame.size) {
                continue;
            }

            bool sameRegistries = true;

            for (size_t j = 0; j < frame.size; ++j) {
                if (queued.registries[j] != frame.registries[j]) {
                    sameRegistries = false;

                    break;
                }
            }

            if (sameRegistries) {
                queued = frame;

                return;
            }
        }

        if (this->sendQueueSize >= SEND_QUEUE_CAPACITY) {
            this->debug("warning", "Send queue full, command dropped");

            return;
        }

        this->sendQueue[this->sendQueueSize++] = frame;
    }

    void TFSXW1Controller::sendRegistries() {
        FrameSendRegistries frame = this->sendQueue[0];

        for (size_t i = 1; i < this->sendQueueSize; ++i) {
            this->sendQueue[i - 1] = this->sendQueue[i];
        }

        this->sendQueueSize--;

        this->frameCheckRegistries.size = frame.size;

        for (size_t i = 0; i < frame.size; ++i) {
            this->frameCheckRegistries.registries[i] = frame.registries[i];
        }

        this->lastFrameSent = frame.type;
        this->lastResponseReceived = false;

        size_t bufferSize = 4 * frame.size + 7;
        uint8_t request[bufferSize] = {
            0x02, 
            0x00, 
            0x00, 
            0x00, 
            4 * frame.size,
        };

        uint16_t checksum = 
            0xFFFF 
            - 0x02
            - (4 * frame.size)
        ;

        for (int i = 0; i < frame.size; i++) {
            Address addr = frame.registries[i];

            int index = 5 + i * 4;

            request[index] = (static_cast<uint16_t>(addr) >> 8) & 0xFF;
            request[index + 1] = static_cast<uint16_t>(addr) & 0xFF;
            request[index + 2] = (frame.values[i] >> 8) & 0xFF;
            request[index + 3] = frame.values[i] & 0xFF;

            checksum -= request[index];
            checksum -= request[index + 1];
            checksum -= request[index + 2];
            checksum -= request[index + 3];
        }

        request[bufferSize - 2] = (checksum >> 8) & 0xFF;
        request[bufferSize - 1] = checksum & 0xFF;

        this->debug("send", this->toHexStr(request, bufferSize));

        uart.write(request, bufferSize);
    }

    void TFSXW1Controller::onFrame(uint8_t buffer[128], int size, bool isValid) {
        if (!this->initialized) {
            return;
        }

        if (!isValid) {
            this->debug("received", this->toHexStr(buffer, size));
            this->debug("error", "invalid checksum");

            return;
        }

        if (this->terminated) {
            this->debug("received", this->toHexStr(buffer, size));
            this->debug("error", "after termination");

            return;
        }

        if (this->noResponseNotified) {
            this->noResponseNotified = false;

            this->debug("status", "Running");
        }

        if (FrameType::Init1 == this->lastFrameSent) {
            static const uint8_t expectedResponseAfterRestart[][8] = {
                {0xFE, 0x00, 0x00, 0x00, 0x01, 0x02, 0xFE, 0xFE},
                {0xFC, 0x00, 0x00, 0x00, 0x01, 0x02, 0xFF, 0x00}
            };

            for (size_t i = 0; i < sizeof(expectedResponseAfterRestart) / sizeof(expectedResponseAfterRestart[0]); i++) {
                if (
                    size == 8 &&
                    memcmp(buffer, expectedResponseAfterRestart[i], 8) == 0
                ) {
                    // wait real initialization frame
                    this->debug("received", this->toHexStr(buffer, size));

                    return;
                }
            }

            uint8_t expectedResponse[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFD};

            if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) != 0) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Unexpected response. Terminate");
                this->debug("status", "Terminated Init1");

                this->terminated = true;
            } else {
                this->lastResponseReceived = true;
            }

            return;
        }

        if (FrameType::Init2 == this->lastFrameSent) {
            uint8_t expectedResponse[8] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFC};

            if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) != 0) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Unexpected response. Terminate");
                this->debug("status", "Terminated Init2");

                this->terminated = true;
            } else {
                this->lastResponseReceived = true;

                this->debug("status", "Running");
            }

            return;
        }

        if (0x03 == buffer[0]) {
            if (0x01 != buffer[5]) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Invalid status");
            }

            this->lastResponseReceived = true;

            if (0x01 == buffer[5]) {
                this->updateRegistries(buffer, size);
            }

            return;
        }

        if (0x02 == buffer[0]) {
            this->debug("received", this->toHexStr(buffer, size));

            if (0x01 != buffer[5]) {
                this->debug("error", "Invalid status");
            }

            this->lastResponseReceived = true;
        }
    }

    void TFSXW1Controller::updateRegistries(uint8_t buffer[128], int size) {
        int registriesCount = buffer[4] / 4;

        for (int i = 0; i < registriesCount; i++) {
            int index = 6 + i * 4;

            uint8_t addrHigh = buffer[index];
            uint8_t addrLow = buffer[index + 1];

            uint8_t valueHigh = buffer[index + 2];
            uint8_t valueLow = buffer[index + 3];

            uint16_t newValue = (static_cast<uint16_t>(valueHigh) << 8) | valueLow;

            Address address = static_cast<Address>((static_cast<uint16_t>(addrHigh) << 8) | addrLow);
            RegistryTable::Register* reg = this->registryTable->getRegister(address);

            if (reg->value != newValue) {
                char hexStr[32];
                snprintf(hexStr, sizeof(hexStr), "%04X | %04X -> %04X", reg->address, reg->value, newValue);

                this->debug("changed", hexStr);

                reg->value = newValue;

                if (this->onRegisterChangeCallback) {
                    this->onRegisterChangeCallback(reg);
                }
            }
        }
    }

    bool TFSXW1Controller::isPoweredOn() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::Power);
        
        return static_cast<uint16_t>(TFSXW1Enums::Power::On) == reg->value;
    }

    bool TFSXW1Controller::isMinimumHeatEnabled() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::MinimumHeat);

        return static_cast<uint16_t>(TFSXW1Enums::MinimumHeat::On) == reg->value;
    }

    bool TFSXW1Controller::isCoilDryEnabled() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::CoilDry);

        return 0x0001 == reg->value;
    }

    int TFSXW1Controller::getVerticalAirflowDirectionCount() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::VerticalAirflowDirectionCount);

        return reg->value;
    }

    int TFSXW1Controller::getHorizontalAirflowDirectionCount() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::HorizontalAirflowDirectionCount);

        return reg->value;
    }

    bool TFSXW1Controller::isFeatureSupported(Address address) {
        RegistryTable::Register* reg = this->registryTable->getRegister(address);
        
        if (
            address == Address::VerticalAirflowDirectionCount
            || address == Address::HorizontalAirflowDirectionCount
        ) {
            return reg->value > 0x0000;
        }

        return 0x0001 == reg->value;
    }

    void TFSXW1Controller::setPower(TFSXW1Enums::Power power) {
        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::Power;
        frame.values[0] = static_cast<uint16_t>(power);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setMinimumHeat(TFSXW1Enums::MinimumHeat minimumHeat) {
        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::MinimumHeat;
        frame.values[0] = static_cast<uint16_t>(minimumHeat);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setMode(TFSXW1Enums::Mode mode) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::Mode;
        frame.values[0] = static_cast<uint16_t>(mode);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setFanSpeed(TFSXW1Enums::FanSpeed fanSpeed) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::FanSpeed;
        frame.values[0] = static_cast<uint16_t>(fanSpeed);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setVerticalAirflow(TFSXW1Enums::VerticalAirflow verticalAirflow) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (!this->isFeatureSupported(Address::VerticalAirflowDirectionCount)) {
            this->debug("warning", "Vertical airflow is not supported");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 2, {}, {}};
        frame.registries[0] = Address::VerticalSwing;
        frame.values[0] = static_cast<uint16_t>(TFSXW1Enums::VerticalSwing::Off);
        frame.registries[1] = Address::VerticalAirflowSetterRegistry;
        frame.values[1] = static_cast<uint16_t>(verticalAirflow);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setVerticalSwing(TFSXW1Enums::VerticalSwing verticalSwing) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (!this->isFeatureSupported(Address::VerticalSwingSupported)) {
            this->debug("warning", "Vertical swing is not supported");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::VerticalSwing;
        frame.values[0] = static_cast<uint16_t>(verticalSwing);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setHorizontalAirflow(TFSXW1Enums::HorizontalAirflow horizontalAirflow) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (!this->isFeatureSupported(Address::HorizontalAirflowDirectionCount)) {
            this->debug("warning", "Horizontal airflow is not supported");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 2, {}, {}};
        frame.registries[0] = Address::HorizontalSwing;
        frame.values[0] = static_cast<uint16_t>(TFSXW1Enums::HorizontalSwing::Off);
        frame.registries[1] = Address::HorizontalAirflowSetterRegistry;
        frame.values[1] = static_cast<uint16_t>(horizontalAirflow);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setHorizontalSwing(TFSXW1Enums::HorizontalSwing horizontalSwing) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (!this->isFeatureSupported(Address::HorizontalSwingSupported)) {
            this->debug("warning", "Horizontal swing is not supported");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::HorizontalSwing;
        frame.values[0] = static_cast<uint16_t>(horizontalSwing);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setPowerful(TFSXW1Enums::Powerful powerful) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::Powerful;
        frame.values[0] = static_cast<uint16_t>(powerful);

        this->enqueueRegistries(frame);
    }

    bool TFSXW1Controller::isPowerfulEnabled() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::Powerful);

        return static_cast<uint16_t>(TFSXW1Enums::Powerful::On) == reg->value;
    }

    void TFSXW1Controller::setEconomy(TFSXW1Enums::EconomyMode economy) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::EconomyMode;
        frame.values[0] = static_cast<uint16_t>(economy);

        this->enqueueRegistries(frame);
    }

    bool TFSXW1Controller::isEconomyEnabled() {
        RegistryTable::Register* reg = this->registryTable->getRegister(Address::EconomyMode);

        return static_cast<uint16_t>(TFSXW1Enums::EconomyMode::On) == reg->value;
    }

    void TFSXW1Controller::setEnergySavingFan(TFSXW1Enums::EnergySavingFan energySavingFan) {
        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::EnergySavingFan;
        frame.values[0] = static_cast<uint16_t>(energySavingFan);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setOutdoorUnitLowNoise(TFSXW1Enums::OutdoorUnitLowNoise outdoorUnitLowNoise) {
        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::OutdoorUnitLowNoise;
        frame.values[0] = static_cast<uint16_t>(outdoorUnitLowNoise);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setCoilDry(TFSXW1Enums::CoilDry coilDry) {
        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::CoilDry;
        frame.values[0] = static_cast<uint16_t>(coilDry);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setHumanSensor(TFSXW1Enums::HumanSensor humanSensor) {
        if (!this->isFeatureSupported(Address::HumanSensorSupported)) {
            this->debug("warning", "Human sensor is not supported");

            return;
        }

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::HumanSensor;
        frame.values[0] = static_cast<uint16_t>(humanSensor);

        this->enqueueRegistries(frame);
    }

    void TFSXW1Controller::setTemp(const char *temp) {
        if (this->isCoilDryEnabled()) {
            this->debug("info", "Coil dry is on");

            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        RegistryTable::Register* modeRegistry = this->registryTable->getRegister(Address::Mode);

        if (static_cast<uint16_t>(TFSXW1Enums::Mode::Fan) == modeRegistry->value) {
            this->debug("info", "Fan mode enabled");

            return;
        }

        int minTemp = static_cast<uint16_t>(TFSXW1Enums::Mode::Heat) == modeRegistry->value
            ? 160
            : 180
        ;

        double number = std::strtod(temp, nullptr);
        int result = static_cast<int>(number * 10 + 0.5);

        result = (result + 2) / 5 * 5;

        if (result < minTemp) {
            this->debug("info", "Too small temp given");
            result = minTemp;
        } else if (result > 300) {
            this->debug("info", "Too big temp given");

            result = 300;
        };

        FrameSendRegistries frame = {FrameType::SendRegistries, 1, {}, {}};
        frame.registries[0] = Address::SetpointTemp;
        frame.values[0] = static_cast<uint16_t>(result);

        this->enqueueRegistries(frame);
    }
}