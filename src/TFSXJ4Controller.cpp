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

        _lastResponseReceived = true;
        _lastRequestMillis = millis();
        _lastResponseMillis = _lastRequestMillis;
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

    	if (!_lastResponseReceived) {
    		if (now - _lastRequestMillis >= 200) {
    			if (State::Init1 == _currentState) {
    				// Communication not established yet. Initial request will be repeated
	    			_currentState = State::None;
	    			_lastResponseReceived = true;
	    			_lastRequestMillis = now;
	    			_lastResponseMillis = _lastRequestMillis;

	    			return;
    			}

    			this->debug("error", "No response for 200 ms");
            	this->debug("status", "No response for 200 ms");

            	_terminated = true;
    		}

    		return;
    	} else if (now - _lastResponseMillis < 260) {
    		return;
    	}

    	if (State::None == _currentState) {
    		_currentState = State::Init1;

    		uint8_t payload[] = {0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFB};
	        this->debug("status", "Init1 Send");
	        this->debug("send", this->toHexStr(payload, sizeof(payload)));

	        uart.write(payload, sizeof(payload));

	        _lastResponseReceived = false;
	        _lastRequestMillis = now;

	        return;
    	}

    	if (State::Init1 == _currentState) {
    		_currentState = State::Init2;

    		uint8_t payload[] = {0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0xFF, 0xF0};
	        this->debug("status", "Init2 Send");
	        this->debug("send", this->toHexStr(payload, sizeof(payload)));

	        uart.write(payload, sizeof(payload));

			_lastResponseReceived = false;
	        _lastRequestMillis = now;

	        return;
    	}

    	if (State::Init2 == _currentState || State::RequestF == _currentState) {
    		_currentState = State::RequestA;

    		this->requestRegistries(_requestA);

    		return;
    	}

    	if (State::RequestA == _currentState) {
    		_currentState = State::RequestB;

    		this->requestRegistries(_requestB);

    		return;
    	}

    	if (State::RequestB == _currentState) {
    		_currentState = State::RequestC;

    		this->requestRegistries(_requestC);

    		return;
    	}

    	if (State::RequestC == _currentState) {
    		_currentState = State::RequestD;

    		this->requestRegistries(_requestD);

    		return;
    	}

    	if (State::RequestD == _currentState) {
    		_currentState = State::RequestE;

    		this->requestRegistries(_requestE);

    		return;
    	}

    	if (State::RequestE == _currentState) {
    		_currentState = State::RequestF;

    		this->requestRegistries(_requestF);

    		return;
    	}
    }

    void TFSXJ4Controller::requestRegistries(TFSXJ4Controller::RequestFrame frame) {
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

        _lastResponseReceived = false;
        _lastRequestMillis = millis();
    }

    void TFSXJ4Controller::onFrame(uint8_t buffer[128], int size, bool isValid) {
    	this->debug("received", this->toHexStr(buffer, size));

    	_lastResponseReceived = true;
    	_lastResponseMillis = millis();

    	if (State::Init1 == _currentState) {
    		uint8_t expectedResponse[8] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFD};

            if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) > 0) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Unexpected response. Terminate");
                this->debug("status", "Terminated Init1");

                _terminated = true;
            }

            return;
    	}

    	if (State::Init2 == _currentState) {
    		uint8_t expectedResponse[8] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFC};

            if (size != sizeof(expectedResponse) || memcmp(buffer, expectedResponse, sizeof(expectedResponse)) > 0) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Unexpected response. Terminate");
                this->debug("status", "Terminated Init2");

                _terminated = true;
            }

            return;
    	}

    	if (0x03 == buffer[0]) {
            if (0x01 != buffer[5]) {
                this->debug("received", this->toHexStr(buffer, size));
                this->debug("error", "Invalid status");
                this->debug("status", "Terminated Invalid status");

                _terminated = true;
            }

            _lastResponseReceived = true;

            if (0x01 == buffer[5]) {
                this->updateRegistries(buffer, size);
            }

            return;
        }
    }
}