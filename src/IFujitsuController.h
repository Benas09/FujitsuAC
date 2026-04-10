/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <Arduino.h>
#include "RegistryTable.h"

namespace FujitsuAC {

    class IFujitsuController {
    	public:
    		IFujitsuController(Stream &uart):
    			uart(uart)
    		{}

    		virtual ~IFujitsuController() = default;
    		
			virtual void setup() = 0;
        	virtual void loop() = 0;

    		void setDebugCallback(std::function<void(const char* name, const char* message)> debugCallback) {
    			this->debugCallback = debugCallback;
    		}

    		void setOnRegisterChangeCallback(std::function<void(const RegistryTable::Register* reg)> onRegisterChangeCallback) {
		        this->onRegisterChangeCallback = onRegisterChangeCallback;
		    }

	    protected:
	    	Stream &uart;

	    	std::function<void(const char* name, const char* message)> debugCallback;
	    	std::function<void(const RegistryTable::Register *reg)> onRegisterChangeCallback;

	    	void debug(const char* name, const char* message) {
	    		if (this->debugCallback) {
	    			this->debugCallback(name, message);
	    		}
	    	}

	    	const char* toHexStr(uint8_t buffer[128], int size) {
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

	    private:
	    	virtual void initRegistryTable() = 0;
    };

}