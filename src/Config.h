/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <Preferences.h>

namespace FujitsuAC {

    class Config {
    	public:
    		Config(const char *version, int ledWPin, int ledRPin);
    		~Config();

            void load();
    		void clear();
    		bool isEmpty();

    		String getUniqueId() { return _uniqueId; }

    		void setValue(const char* key, String value);

    		const char* getVersion() { return _version; }
    		int getLedWPin() { return _ledWPin; }
    		int getLedRPin() { return _ledRPin; }

    		String getWifiSsid() { return _wifiSsid; }
    		String getWifiPw() { return _wifiPw; }
    		String getMqttIp() { return _mqttIp; }
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
            int _ledWPin;
            int _ledRPin;
            
            String _wifiSsid;
            String _wifiPw;
            String _mqttIp;
            String _mqttPort;
            String _mqttUser;
            String _mqttPw;
            String _deviceName;
            String _otaPw;
            String _protocol;

            void generateUniqueId();
    };

}