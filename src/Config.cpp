/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include "Config.h"

namespace FujitsuAC {

    Config::Config(const char *version, int ledWPin, int ledRPin) : 
    	_preferences(),
    	_version(version),
    	_ledWPin(ledWPin),
    	_ledRPin(ledRPin)
	{
    }

	Config::~Config() {
	    _preferences.end();
	}

    void Config::generateUniqueId() {
        char buf[13];
        snprintf(buf, sizeof(buf), "%012llX", ESP.getEfuseMac());

        String uniqueId = buf;
        uniqueId.toLowerCase();

        _uniqueId = uniqueId;
    }

    void Config::load() {
        this->generateUniqueId();
        
        _preferences.begin("fujitsu_ac", false);

        _wifiSsid = _preferences.getString("wifi-ssid", "");
        _wifiPw = _preferences.getString("wifi-pw", "");
        _mqttIp = _preferences.getString("mqtt-ip", "");
        _mqttPort = _preferences.getString("mqtt-port", "");
        _mqttUser = _preferences.getString("mqtt-user", "");
        _mqttPw = _preferences.getString("mqtt-pw", "");
        _deviceName = _preferences.getString("device-name", "");
        _otaPw = _preferences.getString("ota-pw", "");
        _protocol = _preferences.getString("protocol", "");
    }

    void Config::clear() {
	    _preferences.clear(); 
	}

	bool Config::isEmpty() {
		return _wifiSsid == "";
	}

	void Config::setValue(const char* key, String value) {
		_preferences.putString(key, value);
	}

    void Config::initLeds() {
        if (_ledRPin > 0) {
            ledcAttach(_ledRPin, 12000, 10);
            ledcWrite(_ledRPin, 1020);
        }

        if (_ledWPin > 0) {
            ledcAttach(_ledWPin, 12000, 10);
            ledcWrite(_ledWPin, 1023);
        }
    }

    void Config::toggleWLed(bool on) {
        if (_ledWPin > 0) {
            ledcWrite(_ledWPin, on ? 1020 : 1023);
        }
    }

    void Config::toggleRLed(bool on) {
        if (_ledWPin > 0) {
            ledcWrite(_ledRPin, on ? 1020 : 1023);
        }
    }
}