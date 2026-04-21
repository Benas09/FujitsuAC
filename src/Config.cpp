/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "Config.h"

namespace FujitsuAC {

    Config::Config(const char *version, int rxPin, int txPin, int ledWPin, int ledRPin, int resetButtonPin) : 
    	_preferences(),
    	_version(version),
        _rxPin(rxPin),
        _txPin(txPin),
    	_ledWPin(ledWPin),
    	_ledRPin(ledRPin),
        _resetButtonPin(resetButtonPin)
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

    void Config::initIO() {
        if (_ledRPin > 0) {
            ledcAttach(_ledRPin, 12000, 10);
            this->toggleRLed(true);
        }

        if (_ledWPin > 0) {
            ledcAttach(_ledWPin, 12000, 10);
            this->toggleWLed(false);
        }

        pinMode(_rxPin, INPUT_PULLUP);
        pinMode(_txPin, OUTPUT);
        digitalWrite(_txPin, LOW);

        if (_resetButtonPin > 0) {
            pinMode(_resetButtonPin, INPUT_PULLUP);
        }
    }

    void Config::toggleWLed(bool status) {
        if (_ledWPin > 0) {
            ledcWrite(_ledWPin, status ? 1020 : 1023);
            _ledWStatus = status;
        }
    }

    void Config::toggleRLed(bool status) {
        if (_ledRPin > 0) {
            ledcWrite(_ledRPin, status ? 1020 : 1023);
            _ledRStatus = status;
        }
    }

    void Config::toggleLeds(bool status) {
        this->toggleWLed(status);
        this->toggleRLed(status);
    }

    bool Config::isLedsOn() {
        return _ledWStatus || _ledRStatus;
    }
}