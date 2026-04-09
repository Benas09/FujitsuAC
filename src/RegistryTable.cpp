/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.
  
  Project home: https://github.com/Benas09/FujitsuAC
*/

#include "RegistryTable.h"
#include <cstddef>
#include <algorithm>

namespace FujitsuAC {

	RegistryTable::RegistryTable(size_t size, Register *registerTable):
		_size(size),
		_registerTable(registerTable)
	{
		std::sort(
			_registerTable, 
			_registerTable + _size,
			[](const Register& a, const Register& b) {
				return a.address < b.address;
			}
		);
	};

	RegistryTable::Register* RegistryTable::getRegister(uint16_t address) {
		auto it = std::lower_bound(
			_registerTable, 
			_registerTable + _size,
			address,
			[](const Register& reg, uint16_t val) {
				return reg.address < val;
			}
		);

		if (
			it != _registerTable + _size 
			&& it->address == address
		) {
			return it;
		}

		return nullptr;
	}

	const RegistryTable::Register* RegistryTable::getAllRegisters(size_t &outSize) const {
		outSize = _size;

		return _registerTable;
	}

}