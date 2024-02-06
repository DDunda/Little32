#pragma once

#ifndef L32_DebugCore_h_
#define L32_DebugCore_h_

#include "L32_ICore.h"

namespace Little32
{
	struct DebugCore : public ICore
	{
		Computer& computer;

		union {
			struct
			{
				word R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC;
			};
			word registers[16]{ 0 };
		};
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