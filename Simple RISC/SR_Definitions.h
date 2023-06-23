#pragma once
#include<cstdint>

typedef uint32_t word;
typedef uint8_t byte;

#define N_flag 0b1000
#define Z_flag 0b0100
#define C_flag 0b0010
#define V_flag 0b0001

enum class Device_ID {
	Null = 0,
	ComputerInfo = 1,
	ROM = 2,
	RAM = 3,
	CharDisplay = 4
};