/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/
#pragma once

#include "NetworkUpdater.h"

namespace FujitsuAC {

	NetworkUpdater::NetworkUpdater() {}

	void NetworkUpdater::loop() {
		this->setClock();
		this->requestVersion();
	}

	void NetworkUpdater::setClock() {
		if (TimeState::WAITING_INITIALIZATION == _timeState) {
			timeval tv = {0, 0};
			settimeofday(&tv, nullptr);
			configTime(0, 0, "pool.ntp.org");

			_timeCheckInitiatedAtMillis = millis();
			_timeState = TimeState::INITIATED;

			this->debug("info", "NetworkUpdater: Updating clock");

			return;
		}

		if (TimeState::INITIATED == _timeState) {
			time_t nowSecs = time(nullptr);

			if (nowSecs > 1609459200) {
				_timeState = TimeState::SYNCED;
				_versionCheckerState = VersionCheckerState::TIME_OBTAINED;

				this->debugTime(nowSecs);
				this->debug("info", "NetworkUpdater: Clock updated");

				return;
			}

			if ((millis() - _timeCheckInitiatedAtMillis) >= 10000) {
				this->debug("info", "NetworkUpdater: Terminate updating clock");
				_timeState = TimeState::ERROR;

		        return;
		    }
		}
	}

	void NetworkUpdater::requestVersion() {
		if (VersionCheckerState::WAITING_TIME == _versionCheckerState) {
			return;
		}

		if (millis() - _lastVersionCheckInitiatedAtMillis >= 86400000) {
			// repeat checking
			_versionCheckerState = VersionCheckerState::TIME_OBTAINED;
		}

		if (VersionCheckerState::TIME_OBTAINED == _versionCheckerState) {
			this->debug("info", "NetworkUpdater: Check last version start");

			_lastVersionCheckInitiatedAtMillis = millis();

			_client = new WiFiClientSecure();
			_client->setCACert(rootCACertificate);

			if (!_client->connect("raw.githubusercontent.com", 443)) {
				_client->stop();
			    _client = nullptr;
				
				this->debug("info", "NetworkUpdater: Check last version ERR1");

				_versionCheckerState = VersionCheckerState::ERROR;

			    return;
			}

			_client->print(
			  "GET /Benas09/FujitsuAC/refs/heads/master/library.properties HTTP/1.1\r\n"
			  "Host: raw.githubusercontent.com\r\n"
			  "Connection: close\r\n\r\n"
			);

			if (_client->connected()) {
				this->debug("info", "NetworkUpdater: Check last version connected");
				_versionCheckerState = VersionCheckerState::HTTPS_DOWNLOADING;
			} else {
				_client->stop();
			    _client = nullptr;

			    this->debug("info", "NetworkUpdater: Check last version ERR2");
				_versionCheckerState = VersionCheckerState::ERROR;
			}

			return;
		}

		if (VersionCheckerState::HTTPS_DOWNLOADING == _versionCheckerState) {
			if (_client->connected() || _client->available()) {
				if (_client->available()) {
					String line = _client->readStringUntil('\n');

					if (line.startsWith("version=")) {
					    _client->stop();
					    _client = nullptr;

					    _versionCheckerState = VersionCheckerState::VERSION_CHECKED;
					    this->onVersionReceivedCallback(line.substring(8).c_str());
					}
				}

				return;
			}

			_client->stop();
		    _client = nullptr;

		    this->debug("info", "NetworkUpdater: Check last version ERR3");
			_versionCheckerState = VersionCheckerState::ERROR;

			return;
		}
	}

	void NetworkUpdater::updateFirmware(const char *branch) {
		char msg[128];
		snprintf(msg, sizeof(msg), "NetworkUpdater: Starting OTA update from %s", branch);

		this->debug("info", msg);
		this->debug("status", "Updating");

		NetworkClientSecure networkClient;
		networkClient.setCACert(rootCACertificate);
		networkClient.setTimeout(12000);

		esp_chip_info_t info;
		esp_chip_info(&info);

		String chip;

		switch (info.model) {
			case CHIP_ESP32:
				chip = "esp32";   
				break;
			case CHIP_ESP32S3:
				chip = "esp32s3";
				break;
			case CHIP_ESP32C3:
				chip = "esp32c3";
				break;
			case CHIP_ESP32C6:
				chip = "esp32c6";
				break;
			default:
				chip = ESP.getChipModel();
				chip.toLowerCase();
				chip.replace("-", "");

				break;
		}

		snprintf(msg, sizeof(msg), "NetworkUpdater: %s", chip.c_str());
		this->debug("info", msg);

		char path[128];
		snprintf(path, sizeof(path), "/Benas09/FujitsuAC/refs/heads/%s/fw/%s.bin", branch, chip);

		t_httpUpdate_return ret = httpUpdate.update(networkClient, "raw.githubusercontent.com", 443, path);

		switch (ret) {
			case HTTP_UPDATE_FAILED: 
				snprintf(msg, sizeof(msg), "NetworkUpdater: Error (%d): %s",
					httpUpdate.getLastError(),
					httpUpdate.getLastErrorString().c_str()
				);

				this->debug("info", msg);
				this->debug("status", msg);
			
				break;
			case HTTP_UPDATE_NO_UPDATES: 
				this->debug("info", "NetworkUpdater: No updates");

				break;
			case HTTP_UPDATE_OK: 
				this->debug("info", "NetworkUpdater: Update OK");

				break;
		}

		this->debug("info", "NetworkUpdater: Finished");
	}

	void NetworkUpdater::debug(const char* name, const char* message) {
        this->debugCallback(name, message);
    }

    void NetworkUpdater::debugTime(time_t nowSecs)
    {
    	struct tm timeinfo;
		localtime_r(&nowSecs, &timeinfo);

    	char timeStr[32];
		strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

		char msgBuf[128];
		snprintf(msgBuf, sizeof(msgBuf), "NetworkUpdater: Current time: %s", timeStr);

		this->debug("info", msgBuf);
    }
}