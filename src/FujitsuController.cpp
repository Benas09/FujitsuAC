/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "FujitsuController.h"

FujitsuController::FujitsuController(Stream &uart) : 
    uart(uart),
    registryTable(),
    buffer(uart) {
}

bool FujitsuController::setup() {
    this->initialized = true;

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
        && (this->lastRequestMillis == 0 || (now - this->lastRequestMillis) >= 400)
    ) {
        this->lastRequestMillis = now;

        switch (this->lastFrameSent) {
            case FrameType::None: {
                this->lastFrameSent = FrameType::Init1;
                this->lastResponseReceived = false;

                uint8_t payload[] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFB};
                uart.write(payload, sizeof(payload));

                break;
            }

            case FrameType::Init1: {
                this->lastFrameSent = FrameType::Init2;
                this->lastResponseReceived = false;

                uint8_t payload[] = {0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0xFF, 0xF5};
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
            case FrameType::CheckRegistries:
                if (this->frameSendRegistries.size > 0) {
                    this->sendRegistries();

                    break;
                }

                this->requestRegistries(this->frameB);

                break;

            case FrameType::SendRegistries: {
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

        uint8_t addrHigh = (addr >> 8) & 0xFF;
        uint8_t addrLow = addr & 0xFF;

        int index = 5 + i * 2;

        request[index] = addrHigh;
        request[index + 1] = addrLow;

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
        - 0x03
        - (4 * frame.size)
    ;

    for (int i = 0; i < frame.size; i++) {
        Address addr = frame.registries[i];

        uint8_t addrHigh = (addr >> 8) & 0xFF;
        uint8_t addrLow = addr & 0xFF;

        int index = 5 + i * 4;

        request[index] = addrHigh;
        request[index + 1] = addrLow;
        request[index + 2] = (frame.values[i] >> 8) & 0xFF;
        request[index + 3] = frame.values[i] & 0xFF;

        checksum -= request[index];
        checksum -= request[index + 1];
    }

    request[bufferSize - 2] = (checksum >> 8) & 0xFF;
    request[bufferSize - 1] = checksum & 0xFF;

    uart.write(request, bufferSize);
}

void FujitsuController::onFrame(uint8_t buffer[128], int size, bool isValid) {
    if (!this->initialized) {
        return;
    }

    if (!isValid) {
        this->debug("error", this->toHexStr(buffer, size));

        return;
    }

    if (this->terminated) {
        this->debug("after_termination", this->toHexStr(buffer, size));

        return;
    }

    this->noResponseNotified = false;

    if (FrameType::Init1 == this->lastFrameSent) {
        uint8_t expectedResponse[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFD};

        if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) > 0) {
            this->debug("error", this->toHexStr(buffer, size));
            this->debug("error", "terminated");

            this->terminated = true;
        } else {
            this->lastResponseReceived = true;
        }

        return;
    }

    if (FrameType::Init2 == this->lastFrameSent) {
        uint8_t expectedResponse[8] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFC};

        if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) > 0) {
            this->debug("error", this->toHexStr(buffer, size));
            this->debug("error", "terminated");

            this->terminated = true;
        } else {
            this->lastResponseReceived = true;
        }

        return;
    }

    if (0x03 == buffer[0]) {
        if (0x01 != buffer[5]) {
            this->debug("error", this->toHexStr(buffer, size));
        }

        this->lastResponseReceived = true;
        this->updateRegistries(buffer, size);

        return;
    }

    if (0x02 == buffer[0]) {
        if (0x01 != buffer[4]) {
            this->debug("error", this->toHexStr(buffer, size));

            return;
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

bool FujitsuController::isPowerOn() {
    Register* reg = this->registryTable.getRegister(Address::Power);

    return Enums::Power::On == static_cast<Enums::Power>(reg->value);
}

void FujitsuController::setPower(Enums::Power power) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::Power;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(power);
}

void FujitsuController::setMode(Enums::Mode mode) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::Mode;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(mode);
}

void FujitsuController::setFanSpeed(Enums::FanSpeed fanSpeed) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::Fan;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(fanSpeed);
}

void FujitsuController::setVerticalAirflow(Enums::VerticalAirflow verticalAirflow) {
    this->frameSendRegistries.size = 2;
    this->frameSendRegistries.registries[0] = Address::VerticalSwing;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(Enums::VerticalSwing::Off);
    this->frameSendRegistries.registries[1] = Address::VerticalAirflow;
    this->frameSendRegistries.values[1] = static_cast<uint16_t>(verticalAirflow);
}

void FujitsuController::setVerticalSwing(Enums::VerticalSwing verticalSwing) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::VerticalSwing;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(verticalSwing);
}

void FujitsuController::setPowerful(Enums::Powerful powerful) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::Powerful;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(powerful);
}

void FujitsuController::setEconomy(Enums::Economy economy) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::EconomyMode;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(economy);
}

void FujitsuController::setEnergySavingFan(Enums::EnergySavingFan energySavingFan) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::EnergySavingFan;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(energySavingFan);
}

void FujitsuController::setOutdoorUnitLowNoise(Enums::OutdoorUnitLowNoise outdoorUnitLowNoise) {
    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::OutdoorUnitLowNoise;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(outdoorUnitLowNoise);
}

void FujitsuController::setTemp(const char *temp) {
    double number = std::strtod(temp, nullptr);
    int result = static_cast<int>(number * 10 + 0.5);

    result = (result + 2) / 5 * 5;

    if (result < 180) {
        result = 180;
    } else if (result > 300) {
        result = 300;
    };

    this->frameSendRegistries.size = 1;
    this->frameSendRegistries.registries[0] = Address::SetpointTemp;
    this->frameSendRegistries.values[0] = static_cast<uint16_t>(result);
}

void FujitsuController::setOnRegisterChangeCallback(std::function<void(Register* reg)> onRegisterChangeCallback) {
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