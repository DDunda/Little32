#pragma once

#ifndef SR_Core_h_
#define SR_Core_h_

#include <string>
#include "SR_Types.h"

namespace SimpleRISC {
	class Computer;

	class Core {
	public:
		Computer& computer;

		constexpr Core(Computer& computer) : computer(computer) {}

		virtual void Clock() = 0;
		virtual void Reset() = 0;
		virtual void Interrupt(word address) = 0;

		virtual const std::string Disassemble(word instruction) const = 0;

		virtual void SetPC(word value) = 0;
		virtual void SetSP(word value) = 0;
	};
}

#endif