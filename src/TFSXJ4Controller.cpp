/*
/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "TFSXJ4Controller.h"

namespace FujitsuAC {

    TFSXJ4Controller::TFSXJ4Controller(Stream &uart): IFujitsuController(uart) {}

    void TFSXJ4Controller::setup() {
        this->initRegistryTable();

        _lastRequestMillis = millis();
    }

    void TFSXJ4Controller::loop() {
    	this->sendRequest();

    	this->buffer.loop([this](uint8_t buffer[128], int size, bool isValid) {
            this->onFrame(buffer, size, isValid);
        });
    }

    void TFSXJ4Controller::sendRequest() {
    	if (_terminated) {
    		return;
    	}

    	uint32_t now = millis();

        if (now - _lastRequestMillis >= 200) {
            uint8_t payload[] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFB};
	        this->debug("status", "Init1 Send");
	        this->debug("send", this->toHexStr(payload, sizeof(payload)));

	        uart.write(payload, sizeof(payload));

	        _lastRequestMillis = now;
        }
    }

    void TFSXJ4Controller::onFrame(uint8_t buffer[128], int size, bool isValid) {
    	this->debug("received", this->toHexStr(buffer, size));
		this->debug("status", "Terminated");

        _terminated = true;
    }
}