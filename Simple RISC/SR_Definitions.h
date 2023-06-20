#pragma once
#include<cstdint>

typedef uint32_t word;
typedef uint8_t byte;

#define R0 registers[0]
#define R1 registers[1]
#define R2 registers[2]
#define R3 registers[3]
#define R4 registers[4]
#define R5 registers[5]
#define R6 registers[6]
#define R7 registers[7]
#define R8 registers[8]
#define R9 registers[9]
#define R10 registers[10]
#define R11 registers[11]
#define R12 registers[12]
#define SP registers[13]
#define LR registers[14]
#define PC registers[15]

enum class Device_ID {
	Null = 0,
	ComputerInfo = 1,
	ROM = 2,
	RAM = 3,
	CharDisplay = 4
};