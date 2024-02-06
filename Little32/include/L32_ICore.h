#pragma once

#ifndef L32_ICore_h_
#define L32_ICore_h_

#include <string>

#include "L32_Types.h"

namespace Little32
{
	struct Computer;

	struct ICore
	{
		virtual void Clock() = 0;
		virtual void Reset() = 0;
		virtual void Interrupt(word address) = 0;

		virtual const std::string Disassemble(word instruction) const = 0;

		virtual void SetPC(word value) = 0;
		virtual void SetSP(word value) = 0;

		inline constexpr ~ICore() {}
	};
}

#endif