/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "RegistryTable.h"
#include <cstddef>
#include <algorithm>

RegistryTable::RegistryTable() {
	std::sort(
		this->registerTable, 
		this->registerTable + sizeof(this->registerTable) / sizeof(this->registerTable[0]),
		[](const Register& a, const Register& b) {
			return a.address < b.address;
		}
	);
};

Register* RegistryTable::getRegister(Address address) {
	size_t size = sizeof(this->registerTable) / sizeof(this->registerTable[0]);

	auto it = std::lower_bound(
		this->registerTable, 
		this->registerTable + size,
		address,
		[](const Register& reg, Address val) {
			return reg.address < val;
		}
	);

	if (
		it != registerTable + sizeof(registerTable) / sizeof(registerTable[0]) 
		&& it->address == address
	) {
		return it;
	}

	return nullptr;
}

const Register* RegistryTable::getAllRegisters(size_t &outSize) const {
	outSize = sizeof(registerTable) / sizeof(registerTable[0]);

	return registerTable;
}
