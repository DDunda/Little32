#pragma once

#ifndef SR_ComputerInfo_h_
#define SR_ComputerInfo_h_

#include "MemoryMapped.h"

namespace SimpleRISC {
	class Computer;

	/// <summary>Read-Only memory listing the devices installed on the system in an array.</summary>
	class ComputerInfo : public MemoryMapped {
	private:
		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);
	public:
		Computer& computer;

		ComputerInfo(Computer& computer);

		word Read(word address);
		byte ReadByte(word address);

		word GetAddress() const;
		word GetRange() const;
		const Device_ID GetID() const;
	};
}

#endif