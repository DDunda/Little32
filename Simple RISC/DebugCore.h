#pragma once

#ifndef SR_DebugCore_h_
#define SR_DebugCore_h_

#include "Core.h"

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8
#define R9 9
#define R10 10
#define R11 11
#define R12 12
#define SP 13
#define LR 14
#define PC 15

namespace SimpleRISC {
	class DebugCore : public Core {
	public:
		word registers[16]{ 0 };
		byte status;

		DebugCore(Computer& computer);

		void Clock();

		const std::string Disassemble(word instruction) const;

		void Reset();

		void Interrupt(word addr);

		void SetPC(word value);
		void SetSP(word value);
	};
}

#endif