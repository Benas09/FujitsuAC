/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <IFujitsuController.h>
#include <stdint.h>

namespace FujitsuAC {

    class TFSXJ4Controller: public IFujitsuController {
        public:
            enum Address: uint16_t {
                Address01 = 0x0001,
                Address02 = 0x0004,

                Address03 = 0x0001,
                Address04 = 0x5200,
                Address05 = 0xF001,
                Address06 = 0x5206,
                Address07 = 0x114D,
                Address08 = 0x10B5,
                Address09 = 0x1099,

                Address10 = 0x1000,
                Address11 = 0x1001,
                Address12 = 0x1002,
                Address13 = 0x1003,
                Address14 = 0x1010,
                Address15 = 0x1011,
                Address16 = 0x1022,
                Address17 = 0x1023,
                Address18 = 0x1030,
                Address19 = 0x1031,
                Address20 = 0x1033,
                Address21 = 0x1400,
                Address22 = 0x1401,
                Address23 = 0x1402,
                Address24 = 0x1403,
                Address25 = 0x1404,
                Address26 = 0x1405,
                Address27 = 0x1406,
                Address28 = 0x140E,
                Address29 = 0x1116,

                Address30 = 0x1120,
                Address31 = 0x1100,
                Address32 = 0x1121,
                Address33 = 0x1108,
                Address34 = 0x1102,
                Address35 = 0x10B6,
                Address36 = 0x1109,
                Address37 = 0x1141,
                Address38 = 0x1004,
                Address39 = 0x1101,

                Address40 = 0x01E0,
                Address41 = 0x01E1,
                Address42 = 0x01E2,
                Address43 = 0x01E3,
                Address44 = 0x01E4,
                Address45 = 0x01E5,
                Address46 = 0x01E6,
                Address47 = 0x01E7,
                Address48 = 0x01E8,
                Address49 = 0x01E9,
                Address50 = 0x01EA,
                Address51 = 0x01EB,
                Address52 = 0x01EC,
                Address53 = 0x01ED,
                Address54 = 0x01EE,
                Address55 = 0x01EF,
                Address56 = 0x1142,
                Address57 = 0x0142,
                Address58 = 0x0130,
                Address59 = 0x2020,

                Address60 = 0x0110,
                Address61 = 0x0111,
                Address62 = 0x0112,
                Address63 = 0x0113,
                Address64 = 0x0114,
                Address65 = 0x0115,
                Address66 = 0x0117,
                Address67 = 0x011A,
                Address68 = 0x011D,
                Address69 = 0x0120,
                Address70 = 0x0131,
                Address71 = 0x0143,
                Address72 = 0x0150,
                Address73 = 0x0151,
                Address74 = 0x0153,
                Address75 = 0x0170,
                Address76 = 0x0171,
                Address77 = 0x0193,
                Address78 = 0x0190,
            };
            
            struct RequestFrame {
                size_t size;
                Address registries[20];
            };

            TFSXJ4Controller(Stream &uart);

            void setup() override;
            void loop() override;

        private:
            enum State: int {
                None = 0,
                Init1 = 1,
                Init2 = 2,
                RequestA = 3,
                RequestB = 4,
                RequestC = 5,
                RequestD = 6,
                RequestE = 7,
                RequestF = 8,
            };

            State _currentState = State::None;

            uint32_t _lastRequestMillis = 0;
            uint32_t _lastResponseMillis = 0;
            bool _lastResponseReceived = true;
            bool _terminated = false;

            void sendRequest();
            void requestRegistries(TFSXJ4Controller::RequestFrame frame);
            void onFrame(uint8_t buffer[128], int size, bool isValid);

            void initRegistryTable() override {
                static RegistryTable::Register registries[] = {
                    {Address::Address01, 0x0000},
                    {Address::Address02, 0x0000},

                    {Address::Address03, 0x0000},
                    {Address::Address04, 0x0000},
                    {Address::Address05, 0x0000},
                    {Address::Address06, 0x0000},
                    {Address::Address07, 0x0000},
                    {Address::Address08, 0x0000},
                    {Address::Address09, 0x0000},

                    {Address::Address10, 0x0000},
                    {Address::Address11, 0x0000},
                    {Address::Address12, 0x0000},
                    {Address::Address13, 0x0000},
                    {Address::Address14, 0x0000},
                    {Address::Address15, 0x0000},
                    {Address::Address16, 0x0000},
                    {Address::Address17, 0x0000},
                    {Address::Address18, 0x0000},
                    {Address::Address19, 0x0000},
                    {Address::Address20, 0x0000},
                    {Address::Address21, 0x0000},
                    {Address::Address22, 0x0000},
                    {Address::Address23, 0x0000},
                    {Address::Address24, 0x0000},
                    {Address::Address25, 0x0000},
                    {Address::Address26, 0x0000},
                    {Address::Address27, 0x0000},
                    {Address::Address28, 0x0000},
                    {Address::Address29, 0x0000},

                    {Address::Address30, 0x0000},
                    {Address::Address31, 0x0000},
                    {Address::Address32, 0x0000},
                    {Address::Address33, 0x0000},
                    {Address::Address34, 0x0000},
                    {Address::Address35, 0x0000},
                    {Address::Address36, 0x0000},
                    {Address::Address37, 0x0000},
                    {Address::Address38, 0x0000},
                    {Address::Address39, 0x0000},

                    {Address::Address40, 0x0000},
                    {Address::Address41, 0x0000},
                    {Address::Address42, 0x0000},
                    {Address::Address43, 0x0000},
                    {Address::Address44, 0x0000},
                    {Address::Address45, 0x0000},
                    {Address::Address46, 0x0000},
                    {Address::Address47, 0x0000},
                    {Address::Address48, 0x0000},
                    {Address::Address49, 0x0000},
                    {Address::Address50, 0x0000},
                    {Address::Address51, 0x0000},
                    {Address::Address52, 0x0000},
                    {Address::Address53, 0x0000},
                    {Address::Address54, 0x0000},
                    {Address::Address55, 0x0000},
                    {Address::Address56, 0x0000},
                    {Address::Address57, 0x0000},
                    {Address::Address58, 0x0000},
                    {Address::Address59, 0x0000},

                    {Address::Address60, 0x0000},
                    {Address::Address61, 0x0000},
                    {Address::Address62, 0x0000},
                    {Address::Address63, 0x0000},
                    {Address::Address64, 0x0000},
                    {Address::Address65, 0x0000},
                    {Address::Address66, 0x0000},
                    {Address::Address67, 0x0000},
                    {Address::Address68, 0x0000},
                    {Address::Address69, 0x0000},
                    {Address::Address70, 0x0000},
                    {Address::Address71, 0x0000},
                    {Address::Address72, 0x0000},
                    {Address::Address73, 0x0000},
                    {Address::Address74, 0x0000},
                    {Address::Address75, 0x0000},
                    {Address::Address76, 0x0000},
                    {Address::Address77, 0x0000},
                    {Address::Address78, 0x0000},
                };

                this->registryTable = new RegistryTable(78, registries);
            }

            RequestFrame _requestA = {2, {
                Address::Address01,
                Address::Address02
            }};

            RequestFrame _requestB = {7, {
                Address::Address03,
                Address::Address04,
                Address::Address05,
                Address::Address06,
                Address::Address07,
                Address::Address08,
                Address::Address09,
            }};

            RequestFrame _requestC = {20, {
                Address::Address10,
                Address::Address11,
                Address::Address12,
                Address::Address13,
                Address::Address14,
                Address::Address15,
                Address::Address16,
                Address::Address17,
                Address::Address18,
                Address::Address19,
                Address::Address20,
                Address::Address21,
                Address::Address22,
                Address::Address23,
                Address::Address24,
                Address::Address25,
                Address::Address26,
                Address::Address27,
                Address::Address28,
                Address::Address29,
            }};

            RequestFrame _requestD = {10, {
                Address::Address30,
                Address::Address31,
                Address::Address32,
                Address::Address33,
                Address::Address34,
                Address::Address35,
                Address::Address36,
                Address::Address37,
                Address::Address38,
                Address::Address39,
            }};

            RequestFrame _requestE = {20, {
                Address::Address40,
                Address::Address41,
                Address::Address42,
                Address::Address43,
                Address::Address44,
                Address::Address45,
                Address::Address46,
                Address::Address47,
                Address::Address48,
                Address::Address49,
                Address::Address50,
                Address::Address51,
                Address::Address52,
                Address::Address53,
                Address::Address54,
                Address::Address55,
                Address::Address56,
                Address::Address57,
                Address::Address58,
                Address::Address59,
            }};

            RequestFrame _requestF = {20, {
                Address::Address60,
                Address::Address61,
                Address::Address62,
                Address::Address63,
                Address::Address64,
                Address::Address65,
                Address::Address66,
                Address::Address67,
                Address::Address68,
                Address::Address69,
                Address::Address70,
                Address::Address71,
                Address::Address72,
                Address::Address73,
                Address::Address74,
                Address::Address75,
                Address::Address76,
                Address::Address77,
                Address::Address78,
                Address::Address16,
            }};
    };

}