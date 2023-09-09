#pragma once

#ifndef L32_Types_h_
#define L32_Types_h_

#include<cstdint>

namespace Little32 {
	typedef uint32_t word;
	typedef uint8_t byte;

	enum class Device_ID {
		Null = 0,
		ComputerInfo = 1,
		ROM = 2,
		RAM = 3,
		CharDisplay = 4,
		ColourCharDisplay = 5,
		Keyboard = 6
	};
}

#endif