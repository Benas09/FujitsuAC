/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "DummyUnit.h"
#include <Arduino.h>

DummyUnit::DummyUnit(Stream &uart, EspMQTTClient &mqttClient)
    : uart(uart),
      mqttClient(mqttClient),
      registryTable(),
      buffer(uart)
{}

bool DummyUnit::setup() {
    this->createDefaultRegistryValues();
    
    this->mqttClient.publish("fujitsu/dummy/info", "Setup complete");

    return true;
}

bool DummyUnit::loop() {
    this->buffer.loop([this](uint8_t buffer[128], int size, bool isValid) {
        this->onFrame(buffer, size, isValid);
    });

    return true;
}

void DummyUnit::onFrame(uint8_t buffer[128], int size, bool isValid) {
    switch (buffer[0]) {
        case 0x00: {
            this->mqttClient.publish("fujitsu/dummy/info", "Controller initialized connection");

            uint8_t response[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFD};
            uart.write(response, 8);
            break;
        }
            
        case 0x01: {
            uint8_t response[8] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFC};

            this->mqttClient.publish("fujitsu/dummy/info", "Controller connected");

            uart.write(response, 8);
            break;
        }

        case 0x02: 
            this->setRegistryValues(buffer, size);
            break;

        case 0x03:
            this->sendRegistryValues(buffer, size);

            break;
    }
}

bool DummyUnit::setRegistryValues(uint8_t buffer[128], int size) {
    String hexStr = String("");

    for (int i = 0; i < size; i++) {
        hexStr += String(buffer[i], HEX);

        if (i < size - 1) {hexStr += " ";}
    }

    hexStr.toUpperCase();

    this->mqttClient.publish("fujitsu/dummy/debug/set-registry", hexStr);

    int registriesCount = buffer[4] / 4;

    for (int i = 0; i < registriesCount; i++) {
        int index = 5 + i * 4;

        uint8_t addrHigh = buffer[index];
        uint8_t addrLow = buffer[index + 1];

        uint8_t valueHigh = buffer[index + 2];
        uint8_t valueLow = buffer[index + 3];

        uint16_t newValue = (static_cast<uint16_t>(valueHigh) << 8) | valueLow;

        Address address = static_cast<Address>((static_cast<uint16_t>(addrHigh) << 8) | addrLow);
        Register* reg = this->registryTable.getRegister(address);
        reg->value = newValue;
    }

    uint8_t response[8] = {0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFB};
    uart.write(response, 8);

    return true;
}

bool DummyUnit::sendRegistryValues(uint8_t buffer[128], int size) {
    int registriesCount = buffer[4] / 2;
    int responseSize = 4 * registriesCount + 8;

    uint8_t response[128] = {
        0x03, 
        0x00, 
        0x00, 
        0x00, 
        4 * registriesCount + 1, 
        0x01
    };

    uint16_t checksum = 
        0xFFFF 
        - 0x03
        - (4 * registriesCount + 1)
        - 0x01
    ;

    for (int i = 0; i < registriesCount; i++) {
        uint8_t addrHigh = buffer[5 + i * 2];
        uint8_t addrLow = buffer[5 + i * 2 + 1];

        Address address = static_cast<Address>((static_cast<uint16_t>(addrHigh) << 8) | addrLow);
        Register* reg = this->registryTable.getRegister(address);

        int index = 6 + i * 4;

        response[index] = addrHigh;
        response[index + 1] = addrLow;
        response[index + 2] = (reg->value >> 8) & 0xFF;
        response[index + 3] = reg->value & 0xFF;

        checksum -= response[index];
        checksum -= response[index + 1];
        checksum -= response[index + 2];
        checksum -= response[index + 3];
    }

    response[responseSize - 2] = (checksum >> 8) & 0xFF;
    response[responseSize - 1] = checksum & 0xFF;

    uart.write(response, responseSize);

    return true;
}

void DummyUnit::createDefaultRegistryValues() {
    struct RegVal {
        Address address;
        uint16_t value;
    };

    static constexpr RegVal defaults[] = {
        { Address::Initial0, 0x0000 },
        { Address::Initial1, 0x0000 },

        { Address::Initial2, 0x0001 },
        { Address::Initial3, 0x0001 },
        { Address::Initial4, 0x0001 },
        { Address::Initial5, 0x0001 },
        { Address::Initial6, 0x0001 },
        { Address::Initial7, 0x0001 },
        { Address::Initial8, 0x0001 },
        { Address::Initial9, 0x0001 },
        { Address::Initial10, 0x0001 },
        { Address::Initial11, 0x0001 },
        { Address::Initial12, 0x0006 },
        { Address::Initial13, 0x0001 },
        { Address::Initial14, 0x0000 },
        { Address::Initial15, 0x0000 },

        { Address::Initial16, 0x0001 },
        { Address::Initial17, 0x0001 },
        { Address::Initial18, 0x0000 },
        { Address::Initial19, 0x0001 },
        { Address::Initial20, 0x0000 },
        { Address::Initial21, 0x0000 },
        { Address::Initial22, 0x0000 },
        { Address::Initial23, 0x0001 },
        { Address::Initial24, 0x0001 },
        { Address::Initial25, 0x0000 },

        { Address::Power, 0x0001 },
        { Address::Mode, 0x0001 },
        { Address::SetpointTemp, 0x00FA },
        { Address::Fan, 0x0002 },
        { Address::VerticalAirflow, 0x0001 },
        { Address::VerticalSwing, 0x0000 },
        { Address::Register7, 0x0001 },
        { Address::Register8, 0xFFFF },
        { Address::Register9, 0xFFFF },
        { Address::Register10, 0xFFFF },
        { Address::Register11, 0x0181 },
        { Address::ActualTemp, 0x1B71 }, //0x1D97 25.5, is praeito batcho 0x1C6B -> 22.5, 0x1B71 -> 20.0
        { Address::Register13, 0x0000 },

        { Address::EconomyMode, 0x0000 },
        { Address::Register15, 0x0000 },
        { Address::Register16, 0xFFFF },
        { Address::Register17, 0xFFFF },
        { Address::Register18, 0xFFFF },
        { Address::Register19, 0xFFFF },
        { Address::Register20, 0xFFFF },
        { Address::Register21, 0xFFFF },
        { Address::EnergySavingFan, 0x0001 },
        { Address::Register23, 0x0000 },
        { Address::Powerful, 0x0000 },
        { Address::OutdoorUnitLowNoise, 0x0000 },
        { Address::Register26, 0xFFFF },
        { Address::Register27, 0x0000 },
        { Address::Register28, 0x0000 },
        { Address::Register29, 0x0000 },
        { Address::Register30, 0x0000 },
        { Address::Register31, 0x0000 },
        { Address::Register32, 0xFFFF },

        { Address::Register33, 0x0000 },
        { Address::Register34, 0x0000 },
        { Address::Register35, 0x0000 },
        { Address::Register36, 0x0000 },
        { Address::Register37, 0x0000 },
        { Address::Register38, 0x0000 },
        { Address::Register39, 0x0000 },
        { Address::Register40, 0x0000 },
        { Address::Register41, 0xFFFF },
        { Address::Register42, 0x1964 },
        { Address::Register43, 0x0000 },
        { Address::Register44, 0x0000 }
    };

    for (const auto& rv : defaults) {
        Register* reg = this->registryTable.getRegister(rv.address);

        if (reg) {
            reg->value = rv.value;
        }
    }
}