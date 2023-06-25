#pragma once
#include<cstdint>

namespace SimpleRISC {
	typedef uint32_t word;
	typedef uint8_t byte;

	enum class Device_ID {
		Null = 0,
		ComputerInfo = 1,
		ROM = 2,
		RAM = 3,
		CharDisplay = 4
	};
}