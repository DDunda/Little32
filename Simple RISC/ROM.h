#pragma once

#include <memory>
#include "MemoryMapped.h"

namespace SimpleRISC {
	/// <summary>Read-Only memory device</summary>
	class ROM : public MemoryMapped {
	private:
		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);

	public:
		/// <summary> The start address of this ROM </summary>
		word address_start = 0;
		/// <summary> The size of this ROM in bytes </summary>
		word address_size = 0;
		std::shared_ptr<word[]> memory;

		ROM(word address, word size, std::shared_ptr<word[]>& memory);
		ROM(word address, word size);

		word Read(word address);
		byte ReadByte(word address);
		word GetAddress() const;
		word GetRange() const;
		const Device_ID GetID() const;
	};
}