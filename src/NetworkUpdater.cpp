/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/
#pragma once

#include "NetworkUpdater.h"

namespace FujitsuAC {

	NetworkUpdater::NetworkUpdater() {}

	void NetworkUpdater::setup() {
		this->setClock();
	}

	void NetworkUpdater::loop() {
		this->requestVersion();
	}

	void NetworkUpdater::setClock() {
		this->debug("info", "NetworkUpdater: Updating clock");

		uint32_t startMillis = millis();

        configTime(0, 0, "pool.ntp.org");

		time_t nowSecs = time(nullptr);
		
		while (nowSecs < 8 * 3600 * 2) {
			if ((millis() - startMillis) >= 10000) {
				sntp_stop();

				this->debug("info", "NetworkUpdater: Terminate updating clock");

		        return;
		    }

		    this->debug("info", "NetworkUpdater: Waiting for time");
			delay(500);
			nowSecs = time(nullptr);
		}

		this->debug("info", "NetworkUpdater: Clock updated");
		this->versionCheckerState = VersionCheckerState::TIME_OBTAINED;
	}

	void NetworkUpdater::requestVersion() {
		if (VersionCheckerState::NO_TIME == this->versionCheckerState) {
			return;
		}

		if (millis() - this->lastVersionCheckInitiatedAtMillis >= 86400000) {
			// repeat checking
			this->versionCheckerState = VersionCheckerState::TIME_OBTAINED;
		}

		if (VersionCheckerState::TIME_OBTAINED == this->versionCheckerState) {
			this->debug("info", "NetworkUpdater: Check last version start");

			this->lastVersionCheckInitiatedAtMillis = millis();

			client = new WiFiClientSecure();
			client->setCACert(rootCACertificate);

			if (!client->connect("raw.githubusercontent.com", 443)) {
				client->stop();
			    client = nullptr;
				
				this->debug("info", "NetworkUpdater: Check last version ERR1");

				this->versionCheckerState = VersionCheckerState::ERROR;

			    return;
			}

			client->print(
			  "GET /Benas09/FujitsuAC/refs/heads/master/library.properties HTTP/1.1\r\n"
			  "Host: raw.githubusercontent.com\r\n"
			  "Connection: close\r\n\r\n"
			);

			if (client->connected()) {
				this->debug("info", "NetworkUpdater: Check last version connected");
				this->versionCheckerState = VersionCheckerState::HTTPS_DOWNLOADING;
			} else {
				client->stop();
			    client = nullptr;

			    this->debug("info", "NetworkUpdater: Check last version ERR2");
				this->versionCheckerState = VersionCheckerState::ERROR;
			}

			return;
		}

		if (VersionCheckerState::HTTPS_DOWNLOADING == this->versionCheckerState) {
			if (client->connected() || client->available()) {
				if (client->available()) {
					String line = client->readStringUntil('\n');

					if (line.startsWith("version=")) {
					    client->stop();
					    client = nullptr;

					    this->versionCheckerState = VersionCheckerState::VERSION_CHECKED;
					    this->onVersionReceivedCallback(line.substring(8).c_str());
					}
				}

				return;
			}

			client->stop();
		    client = nullptr;

		    this->debug("info", "NetworkUpdater: Check last version ERR3");
			this->versionCheckerState = VersionCheckerState::ERROR;

			return;
		}
	}

	void NetworkUpdater::updateFirmware() {
		char msg[128];

		this->debug("info", "NetworkUpdater: Starting OTA update");
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
			default:
				chip = ESP.getChipModel();
				chip.toLowerCase();
				chip.replace("-", "");

				break;
		}

		snprintf(msg, sizeof(msg), "NetworkUpdater: %s", chip.c_str());
		this->debug("info", msg);

		String path = "/Benas09/FujitsuAC/refs/heads/master/fw/" + chip + ".bin";
		t_httpUpdate_return ret = httpUpdate.update(networkClient, "raw.githubusercontent.com", 443, path.c_str());

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
}