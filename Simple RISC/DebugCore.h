#pragma once
#include "Core.h"

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