#pragma once

#ifndef L32_ROM_h_
#define L32_ROM_h_

#include <memory>

#include "L32_IMemoryMapped.h"

namespace Little32
{
	/// <summary>Read-Only memory device</summary>
	class ROM : public IMemoryMapped
	{
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
		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return address_size; }
		constexpr const Device_ID GetID() const { return Device_ID::ROM; }
	};
}

#endif