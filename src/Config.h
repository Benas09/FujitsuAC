/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <Preferences.h>
#include <WiFi.h>

namespace FujitsuAC {

    class Config {
    	public:
    		Config(
                const char *version, 
                uart_port_t uartPort,
                int rxPin, 
                int txPin, 
                int ledWPin, 
                int ledRPin, 
                int resetButtonPin
            );

    		~Config();

            void load();
    		void clear();
    		bool isEmpty();
            void initIO();

            void toggleWLed(bool status);
            void toggleRLed(bool status);

    		String getUniqueId() { return _uniqueId; }

    		void setValue(const char* key, String value);

    		const char* getVersion() { return _version; }

            bool isLedsOn();
            void setLedsStatus(bool status);

            void setWifiSleepEnabled(bool status);
            bool isWifiSleepEnabled();

            void setLowCpuSpeedEnabled(bool status);
            bool isLowCpuSpeedEnabled();

            uart_port_t getUartPort() { return _uartPort; }
            int getRxPin() { return _rxPin; }
            int getTxPin() { return _txPin; }
            int getResetButtonPin() { return _resetButtonPin; }

    		int getLedWPin() { return _ledWPin; }
    		int getLedRPin() { return _ledRPin; }

    		String getWifiSsid() { return _wifiSsid; }
    		String getWifiPw() { return _wifiPw; }
    		const String& getMqttIp() { return _mqttIp; }
    		String getMqttPort() { return _mqttPort; }
    		String getMqttUser() { return _mqttUser; }
    		String getMqttPw() { return _mqttPw; }
    		String getDeviceName() { return _deviceName; }
    		String getOtaPw() { return _otaPw; }
    		String getProtocol() { return _protocol; }

        private:
            Preferences _preferences;
            
            String _uniqueId;
            const char *_version;
            
            uart_port_t _uartPort;
            int _rxPin;
            int _txPin;

            int _ledWPin;
            int _ledRPin;
            int _resetButtonPin;

            String _wifiSsid;
            String _wifiPw;
            String _mqttIp;
            String _mqttPort;
            String _mqttUser;
            String _mqttPw;
            String _deviceName;
            String _otaPw;
            String _protocol;
            
            bool _ledsOn = true;
            bool _wifiSleepEnabled = true;
            bool _lowCpuSpeedEnabled = true;

            void generateUniqueId();
    };

}