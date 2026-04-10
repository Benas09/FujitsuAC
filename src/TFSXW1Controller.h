/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#pragma once

#include <IFujitsuController.h>
#include <stdint.h>
#include "Buffer.h"
#include "Enums.h"

namespace FujitsuAC {

    class TFSXW1Controller: public IFujitsuController {

        public:
            enum Address: uint16_t {
                Initial0 = 0x0001,
                Initial1 = 0x0101,

                Initial2 = 0x0110,
                Initial3 = 0x0111,  
                Initial4 = 0x0112,  
                Initial5 = 0x0113,  
                Initial6 = 0x0114,  
                Initial7 = 0x0115,  
                Initial8 = 0x0117,  
                Initial9 = 0x011A,  
                Initial10 = 0x011D,  
                Initial11 = 0x0120,  
                VerticalAirflowDirectionCount = 0x0130,
                VerticalSwingSupported = 0x0131,  
                HorizontalAirflowDirectionCount = 0x0142,  
                HorizontalSwingSupported = 0x0143,  

                EconomyModeSupported = 0x0150,
                MinimumHeatSupported = 0x0151,  
                HumanSensorSupported = 0x0152,
                EnergySavingFanSupported = 0x0153,  
                Initial20 = 0x0154,  
                Initial21 = 0x0155,  
                Initial22 = 0x0156,  
                PowerfulSupported = 0x0170,  
                OutdoorUnitLowNoiseSupported = 0x0171,  
                CoilDrySupported = 0x0193,  

                Power = 0x1000,
                Mode      = 0x1001,
                SetpointTemp = 0x1002,
                FanSpeed = 0x1003,  
                VerticalAirflowSetterRegistry = 0x1010, 
                VerticalSwing = 0x1011,  
                VerticalAirflow  = 0x10A0,
                HorizontalAirflowSetterRegistry = 0x1022,  
                HorizontalSwing = 0x1023,
                HorizontalAirflow = 0x10A9,
                Register11 = 0x1031,  
                ActualTemp = 0x1033,  
                Register13 = 0x1034,  

                EconomyMode = 0x1100,  
                MinimumHeat = 0x1101,  
                HumanSensor = 0x1102,
                Register17 = 0x1103,  
                Register18 = 0x1104,  
                Register19 = 0x1105,  
                Register20 = 0x1106,  
                Register21 = 0x1107,  
                EnergySavingFan = 0x1108,  
                Register23 = 0x1109,  
                Powerful = 0x1120,  
                OutdoorUnitLowNoise = 0x1121,  
                CoilDry = 0x1144,  
                Register27 = 0x1200,  
                Register28 = 0x1201,  
                Register29 = 0x1202,  
                Register30 = 0x1203,  
                Register31 = 0x1204,  
                Register32 = 0x1141, 

                Register33 = 0x1400,  
                Register34 = 0x1401,  
                Register35 = 0x1402,  
                Register36 = 0x1403,  
                Register37 = 0x1404,  
                Register38 = 0x1405,  
                Register39 = 0x1406,  
                Register40 = 0x140E,  
                Register41 = 0x2000,  
                OutdoorTemp = 0x2020,  
                Register43 = 0x2021,  
                Register44 = 0xF001,
            };
            
            TFSXW1Controller(Stream &uart);

            void setup() override;
            void loop() override;

            void setPower(Enums::Power power);
            void setMinimumHeat(Enums::MinimumHeat minimumHeat);
            void setMode(Enums::Mode mode);
            void setFanSpeed(Enums::FanSpeed fanSpeed);
            void setVerticalAirflow(Enums::VerticalAirflow verticalAirflow);
            void setVerticalSwing(Enums::VerticalSwing verticalSwing);
            void setHorizontalAirflow(Enums::HorizontalAirflow horizontalAirflow);
            void setHorizontalSwing(Enums::HorizontalSwing horizontalSwing);
            void setPowerful(Enums::Powerful powerful);
            void setEconomy(Enums::EconomyMode economy);
            void setEnergySavingFan(Enums::EnergySavingFan energySavingFan);
            void setOutdoorUnitLowNoise(Enums::OutdoorUnitLowNoise outdoorUnitLowNoise);
            void setCoilDry(Enums::CoilDry coilDry);
            void setHumanSensor(Enums::HumanSensor humanSensor);
            void setTemp(const char *temp);

            bool isPoweredOn();
            bool isFeatureSupported(Address address);
            
            int getVerticalAirflowDirectionCount();
            int getHorizontalAirflowDirectionCount();

            const RegistryTable::Register* getAllRegisters(size_t &outSize) const;
            RegistryTable::Register* getRegister(Address address);

        private:
            RegistryTable *registryTable;
            Buffer buffer;
            uint32_t lastRequestMillis = 0;
            bool lastResponseReceived = true;
            bool noResponseNotified = false;
            bool initialized = false;
            bool terminated = false;

            bool humanSensorSupported = false;

            enum class FrameType: int {
                None = -1,
                Init1 = 0,
                Init2 = 1,
                InitialRegistries1 = 2,
                InitialRegistries2 = 3,
                InitialRegistries3 = 4,
                FrameA = 5,
                FrameB = 6,
                FrameC = 7,
                SendRegistries = 8,
                CheckRegistries = 9,
            };

            FrameType lastFrameSent = FrameType::None;

            struct Frame {
                FrameType type;
                size_t size;
                Address registries[19];
            };

            struct FrameSendRegistries {
                FrameType type;
                size_t size;
                Address registries[19];
                uint16_t values[19];
            };

            Frame initialRegistries1 = {FrameType::InitialRegistries1, 2, {
                  Address::Initial0,
                  Address::Initial1,
                }
            };

            Frame initialRegistries2 = {FrameType::InitialRegistries2, 14, {
                  Address::Initial2,
                  Address::Initial3,  
                  Address::Initial4,  
                  Address::Initial5,  
                  Address::Initial6,  
                  Address::Initial7,  
                  Address::Initial8,  
                  Address::Initial9,  
                  Address::Initial10,  
                  Address::Initial11,  
                  Address::VerticalAirflowDirectionCount,  
                  Address::VerticalSwingSupported,  
                  Address::HorizontalAirflowDirectionCount,  
                  Address::HorizontalSwingSupported,
                }
            };

            Frame initialRegistries3 = {FrameType::InitialRegistries3, 10, {
                  Address::EconomyModeSupported,
                  Address::MinimumHeatSupported,  
                  Address::HumanSensorSupported,  
                  Address::EnergySavingFanSupported,  
                  Address::Initial20,  
                  Address::Initial21,  
                  Address::Initial22,  
                  Address::PowerfulSupported,  
                  Address::OutdoorUnitLowNoiseSupported,  
                  Address::CoilDrySupported,
                }
            };

            Frame frameA = {FrameType::FrameA, 13, {
                  Address::Power,
                  Address::Mode,
                  Address::SetpointTemp,
                  Address::FanSpeed,
                  Address::VerticalAirflowSetterRegistry,
                  Address::VerticalSwing,
                  Address::VerticalAirflow,
                  Address::HorizontalAirflowSetterRegistry,
                  Address::HorizontalSwing,
                  Address::HorizontalAirflow,
                  Address::Register11,
                  Address::ActualTemp,
                  Address::Register13,
                }
            };

            Frame frameB = {FrameType::FrameB, 19, {
                  Address::EconomyMode,  
                  Address::MinimumHeat,  
                  Address::HumanSensor,  
                  Address::Register17,  
                  Address::Register18,  
                  Address::Register19,  
                  Address::Register20,  
                  Address::Register21,  
                  Address::EnergySavingFan,  
                  Address::Register23,  
                  Address::Powerful,  
                  Address::OutdoorUnitLowNoise,  
                  Address::CoilDry,  
                  Address::Register27,  
                  Address::Register28,  
                  Address::Register29,  
                  Address::Register30,  
                  Address::Register31,  
                  Address::Register32,
                }
            };

            Frame frameC = {FrameType::FrameC, 12, {
                  Address::Register33,  
                  Address::Register34,  
                  Address::Register35,  
                  Address::Register36,  
                  Address::Register37,  
                  Address::Register38,  
                  Address::Register39,  
                  Address::Register40,  
                  Address::Register41,  
                  Address::OutdoorTemp,  
                  Address::Register43,  
                  Address::Register44,
                }
            };

            FrameSendRegistries frameSendRegistries = {FrameType::SendRegistries, 0, {}, {}};

            bool isMinimumHeatEnabled();
            bool isCoilDryEnabled();

            void sendRequest();
            void requestRegistries(Frame frame);
            void sendRegistries();
            void onFrame(uint8_t buffer[128], int size, bool isValid);
            void updateRegistries(uint8_t buffer[128], int size);

            void initRegistryTable() override {
                static RegistryTable::Register registries[] = {
                    {Address::Initial0, 0x0000},
                    {Address::Initial1, 0x0000},
                    
                    {Address::Initial2, 0x0000},
                    {Address::Initial3, 0x0000},
                    {Address::Initial4, 0x0000},
                    {Address::Initial5, 0x0000},
                    {Address::Initial6, 0x0000},
                    {Address::Initial7, 0x0000},
                    {Address::Initial8, 0x0000},
                    {Address::Initial9, 0x0000},
                    {Address::Initial10, 0x0000},
                    {Address::Initial11, 0x0000},
                    {Address::VerticalAirflowDirectionCount, 0x0000},
                    {Address::VerticalSwingSupported, 0x0000},
                    {Address::HorizontalAirflowDirectionCount, 0x0000},
                    {Address::HorizontalSwingSupported, 0x0000},
                    
                    {Address::EconomyModeSupported, 0x0000},
                    {Address::MinimumHeatSupported, 0x0000},
                    {Address::HumanSensorSupported, 0x0000},
                    {Address::EnergySavingFanSupported, 0x0000},
                    {Address::Initial20, 0x0000},
                    {Address::Initial21, 0x0000},
                    {Address::Initial22, 0x0000},
                    {Address::PowerfulSupported, 0x0000},
                    {Address::OutdoorUnitLowNoiseSupported, 0x0000},
                    {Address::CoilDrySupported, 0x0000},
                    
                    {Address::Power, 0x0000},
                    {Address::Mode, 0x0000},
                    {Address::SetpointTemp, 0x0000},
                    {Address::FanSpeed, 0x0000},
                    {Address::VerticalAirflowSetterRegistry, 0x0000},
                    {Address::VerticalSwing, 0x0000},
                    {Address::VerticalAirflow, 0x0000},
                    {Address::HorizontalAirflowSetterRegistry, 0x0000},
                    {Address::HorizontalSwing, 0x0000},
                    {Address::HorizontalAirflow, 0x0000},
                    {Address::Register11, 0x0000},
                    {Address::ActualTemp, 0x0000},
                    {Address::Register13, 0x0000},
                    
                    {Address::EconomyMode, 0x0000},
                    {Address::MinimumHeat, 0x0000},
                    {Address::HumanSensor, 0x0000},
                    {Address::Register17, 0x0000},
                    {Address::Register18, 0x0000},
                    {Address::Register19, 0x0000},
                    {Address::Register20, 0x0000},
                    {Address::Register21, 0x0000},
                    {Address::EnergySavingFan, 0x0000},
                    {Address::Register23, 0x0000},
                    {Address::Powerful, 0x0000},
                    {Address::OutdoorUnitLowNoise, 0x0000},
                    {Address::CoilDry, 0x0000},
                    {Address::Register27, 0x0000},
                    {Address::Register28, 0x0000},
                    {Address::Register29, 0x0000},
                    {Address::Register30, 0x0000},
                    {Address::Register31, 0x0000},
                    {Address::Register32, 0x0000},
                    
                    {Address::Register33, 0x0000},
                    {Address::Register34, 0x0000},
                    {Address::Register35, 0x0000},
                    {Address::Register36, 0x0000},
                    {Address::Register37, 0x0000},
                    {Address::Register38, 0x0000},
                    {Address::Register39, 0x0000},
                    {Address::Register40, 0x0000},
                    {Address::Register41, 0x0000},
                    {Address::OutdoorTemp, 0x0000},
                    {Address::Register43, 0x0000},
                    {Address::Register44, 0x0000},
                };

                this->registryTable = new RegistryTable(70, registries);
            }
    };

}