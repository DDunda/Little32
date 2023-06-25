#include "ComputerInfo.h"
#include "Computer.h"

namespace SimpleRISC {
	word ComputerInfo::_ReadWordUnsafe(word address) {
		int i = address / (3 * sizeof(word));

		word devinfo[3] = {
			(word)computer.mappings[i]->GetID(),
			computer.mappings[i]->GetAddress(),
			computer.mappings[i]->GetRange()
		};

		return devinfo[(address % (3 * sizeof(word))) / sizeof(word)];
	}
	byte ComputerInfo::_ReadByteUnsafe(word address) { return _ReadWordUnsafe(address) >> ((sizeof(word) - 1 - (address % sizeof(word))) * 8); }

	ComputerInfo::ComputerInfo(Computer& computer) : computer(computer) {}

	word ComputerInfo::Read(word address) {
		if (address >= GetRange()) return 0;

		return address % sizeof(word) ? _ReadByteUnsafe(address) : _ReadWordUnsafe(address);
	}
	byte ComputerInfo::ReadByte(word address) {
		if (address >= GetRange()) return 0;

		return _ReadByteUnsafe(address);
	}

	word ComputerInfo::GetAddress() const { return 0; }
	word ComputerInfo::GetRange() const { return computer.mappings.size() * 3 * sizeof(word); }
	const Device_ID ComputerInfo::GetID() const { return Device_ID::ComputerInfo; }
}