/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "RegistryTable.h"
#include "Buffer.h"

class DummyUnit {
    public:
        DummyUnit(Stream &uart);

        bool setup();
        bool loop();

    private:
        Stream &uart;
        RegistryTable registryTable;
        Buffer buffer;

        void onFrame(uint8_t buffer[128], int size, bool isValid);

        bool setRegistryValues(uint8_t buffer[128], int size);
        bool sendRegistryValues(uint8_t buffer[128], int size);

        void createDefaultRegistryValues();
};
