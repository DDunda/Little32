#ifndef L32_ComputerInfo_h_
#define L32_ComputerInfo_h_
#pragma once

#include "L32_IMemoryMapped.h"

namespace Little32
{
	struct Computer;

	/// <summary>Read-Only memory listing the devices installed on the system in an array.</summary>
	class ComputerInfo : public IMemoryMapped
	{
	private:
		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);
	public:
		Computer& computer;

		constexpr ComputerInfo(Computer& computer) : computer(computer) {}

		word Read(word address);
		byte ReadByte(word address);

		inline word GetAddress() const { return 0; }
		word GetRange() const;
		constexpr const Device_ID GetID() const { return Device_ID::ComputerInfo; }
	};
}

#endif