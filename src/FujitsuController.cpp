/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "FujitsuController.h"

namespace FujitsuAC {

    FujitsuController::FujitsuController(Stream &uart) : 
        uart(uart),
        registryTable(),
        buffer(uart) {
    }

    bool FujitsuController::setup() {
        this->initialized = true;
        this->lastRequestMillis = millis();

        return true;
    }

    bool FujitsuController::loop() {
        if (!this->initialized) {
            return true;
        }

        this->sendRequest();

        this->buffer.loop([this](uint8_t buffer[128], int size, bool isValid) {
            this->onFrame(buffer, size, isValid);
        });

        return true;
    }

    const Register* FujitsuController::getAllRegisters(size_t &outSize) const {
        return this->registryTable.getAllRegisters(outSize);
    }

    Register* FujitsuController::getRegister(Address address) {
        return this->registryTable.getRegister(address);
    }

    void FujitsuController::sendRequest() {
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
                    this->debug("send", this->toHexStr(payload, sizeof(payload)));

                    uart.write(payload, sizeof(payload));

                    break;
                }

                case FrameType::Init1: {
                    this->lastFrameSent = FrameType::Init2;
                    this->lastResponseReceived = false;

                    uint8_t payload[] = {0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0xFF, 0xF5};
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
                    if (this->frameSendRegistries.size > 0) {
                        this->sendRegistries();

                        break;
                    }
                case FrameType::SendRegistries: {
                    if (this->frameSendRegistries.size > 0) {
                        Frame frame = {
                            FrameType::CheckRegistries,
                            this->frameSendRegistries.size
                        };

                        this->frameSendRegistries.size = 0;

                        for (size_t i = 0; i < frame.size; ++i) {
                            frame.registries[i] = this->frameSendRegistries.registries[i];
                        }

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

    void FujitsuController::requestRegistries(Frame frame) {
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

    void FujitsuController::sendRegistries() {
        FrameSendRegistries frame = this->frameSendRegistries;

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

    void FujitsuController::onFrame(uint8_t buffer[128], int size, bool isValid) {
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

        this->noResponseNotified = false;

        if (FrameType::Init1 == this->lastFrameSent) {
            uint8_t expectedResponseAfterRestart[8] = {0xFE, 0x00, 0x00, 0x00, 0x01, 0x02, 0xFE, 0xFE};

            if (
                size == sizeof(expectedResponseAfterRestart) 
                && memcmp(buffer, expectedResponseAfterRestart, sizeof(expectedResponseAfterRestart)) == 0
            ) {
                // wait real initialization frame
                this->debug("received", this->toHexStr(buffer, size));

                return;
            }

            uint8_t expectedResponse[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFD};

            if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) > 0) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Unexpected response. Terminate");

                this->terminated = true;
            } else {
                this->lastResponseReceived = true;
            }

            return;
        }

        if (FrameType::Init2 == this->lastFrameSent) {
            uint8_t expectedResponse[8] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFC};

            if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) > 0) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Unexpected response. Terminate");

                this->terminated = true;
            } else {
                this->lastResponseReceived = true;
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

    void FujitsuController::updateRegistries(uint8_t buffer[128], int size) {
        int registriesCount = buffer[4] / 4;

        for (int i = 0; i < registriesCount; i++) {
            int index = 6 + i * 4;

            uint8_t addrHigh = buffer[index];
            uint8_t addrLow = buffer[index + 1];

            uint8_t valueHigh = buffer[index + 2];
            uint8_t valueLow = buffer[index + 3];

            uint16_t newValue = (static_cast<uint16_t>(valueHigh) << 8) | valueLow;

            Address address = static_cast<Address>((static_cast<uint16_t>(addrHigh) << 8) | addrLow);
            Register* reg = this->registryTable.getRegister(address);

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

    bool FujitsuController::isPoweredOn() {
        Register* reg = this->registryTable.getRegister(Address::Power);
        
        return static_cast<uint16_t>(Enums::Power::On) == reg->value;
    }

    bool FujitsuController::isMinimumHeatEnabled() {
        Register* reg = this->registryTable.getRegister(Address::MinimumHeat);

        return static_cast<uint16_t>(Enums::MinimumHeat::On) == reg->value;
    }

    bool FujitsuController::isHorizontalSwingSupported() {
        Register* reg = this->registryTable.getRegister(Address::HorizontalSwingSupported);

        return 0x0015 == reg->value;
    }

    bool FujitsuController::isHumanSensorSupported() {
        Register* reg = this->registryTable.getRegister(Address::HumanSensorSupported);
        
        return 0x0001 == reg->value;
    }

    void FujitsuController::setPower(Enums::Power power) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::Power;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(power);
    }

    void FujitsuController::setMinimumHeat(Enums::MinimumHeat minimumHeat) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }
        
        this->debug("warning", "Not tested yet");

        return;

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::MinimumHeat;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(minimumHeat);
    }

    void FujitsuController::setMode(Enums::Mode mode) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::Mode;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(mode);
    }

    void FujitsuController::setFanSpeed(Enums::FanSpeed fanSpeed) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::FanSpeed;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(fanSpeed);
    }

    void FujitsuController::setVerticalAirflow(Enums::VerticalAirflow verticalAirflow) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        this->frameSendRegistries.size = 2;
        this->frameSendRegistries.registries[0] = Address::VerticalSwing;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(Enums::VerticalSwing::Off);
        this->frameSendRegistries.registries[1] = Address::VerticalAirflowSetterRegistry;
        this->frameSendRegistries.values[1] = static_cast<uint16_t>(verticalAirflow);
    }

    void FujitsuController::setVerticalSwing(Enums::VerticalSwing verticalSwing) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::VerticalSwing;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(verticalSwing);
    }

    void FujitsuController::setHorizontalAirflow(Enums::HorizontalAirflow horizontalAirflow) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (!this->isHorizontalSwingSupported()) {
            this->debug("warning", "Horizontal swing is not supported");

            return;
        }

        this->frameSendRegistries.size = 2;
        this->frameSendRegistries.registries[0] = Address::HorizontalSwing;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(Enums::HorizontalSwing::Off);
        this->frameSendRegistries.registries[1] = Address::HorizontalAirflowSetterRegistry;
        this->frameSendRegistries.values[1] = static_cast<uint16_t>(horizontalAirflow);
    }

    void FujitsuController::setHorizontalSwing(Enums::HorizontalSwing horizontalSwing) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (!this->isHorizontalSwingSupported()) {
            this->debug("warning", "Horizontal swing is not supported");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::HorizontalSwing;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(horizontalSwing);
    }

    void FujitsuController::setPowerful(Enums::Powerful powerful) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::Powerful;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(powerful);
    }

    void FujitsuController::setEconomy(Enums::EconomyMode economy) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::EconomyMode;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(economy);
    }

    void FujitsuController::setEnergySavingFan(Enums::EnergySavingFan energySavingFan) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::EnergySavingFan;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(energySavingFan);
    }

    void FujitsuController::setOutdoorUnitLowNoise(Enums::OutdoorUnitLowNoise outdoorUnitLowNoise) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::OutdoorUnitLowNoise;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(outdoorUnitLowNoise);
    }

    void FujitsuController::setHumanSensor(Enums::HumanSensor humanSensor) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (!this->isHumanSensorSupported()) {
            this->debug("warning", "Human sensor is not supported");

            return;
        }

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::HumanSensor;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(humanSensor);
    }

    void FujitsuController::setTemp(const char *temp) {
        if (this->frameSendRegistries.size > 0) {
            return;
        }

        if (this->isMinimumHeatEnabled()) {
            this->debug("info", "Minimum heat is on");

            return;
        }

        Register* modeRegistry = this->registryTable.getRegister(Address::Mode);

        if (static_cast<uint16_t>(Enums::Mode::Fan) == modeRegistry->value) {
            this->debug("info", "Fan mode enabled");

            return;
        }

        int minTemp = static_cast<uint16_t>(Enums::Mode::Heat) == modeRegistry->value
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

        this->frameSendRegistries.size = 1;
        this->frameSendRegistries.registries[0] = Address::SetpointTemp;
        this->frameSendRegistries.values[0] = static_cast<uint16_t>(result);
    }

    void FujitsuController::setOnRegisterChangeCallback(std::function<void(const Register* reg)> onRegisterChangeCallback) {
        this->onRegisterChangeCallback = onRegisterChangeCallback;
    }

    void FujitsuController::setDebugCallback(std::function<void(const char* name, const char* message)> debugCallback) {
        this->debugCallback = debugCallback;
    }

    void FujitsuController::debug(const char* name, const char* message) {
        if (this->debugCallback) {
            this->debugCallback(name, message);
        }
    }

    const char* FujitsuController::toHexStr(uint8_t buffer[128], int size) {
        static char hexStr[384];
        int offset = 0;

        for (int i = 0; i < size && offset < sizeof(hexStr) - 3; ++i) {
            offset += snprintf(
                hexStr + offset, 
                sizeof(hexStr) - offset,
                (i < size - 1) 
                    ? "%02X " 
                    : "%02X", buffer[i]
            );
        }

        return hexStr;
    }

}