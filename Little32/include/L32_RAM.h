#pragma once

#ifndef L32_RAM_h_
#define L32_RAM_h_

#include "L32_IMappedDevice.h"

#include <memory>

namespace Little32
{
	/// <summary>Read-Write memory device</summary>
	class RAM : public IMappedDevice
	{
	private:
		void _WriteWordUnsafe(word address, word value);
		void _WriteByteUnsafe(word address, byte value);

		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);

	public:
		/// <summary> The start address of this RAM </summary>
		word address_start = 0;
		/// <summary> The size of this RAM in bytes </summary>
		word address_size = 0;
		/// <summary> The data used to fill memory when the device is reset </summary>
		std::shared_ptr<word[]> defaultMemory;
		std::shared_ptr<word[]> memory;

		RAM(word address, word size, std::shared_ptr<word[]>& memory);
		RAM(word address, word size);

		void Write(word address, word value);
		void WriteByte(word address, byte value);

		word Read(word address);
		byte ReadByte(word address);

		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return address_size; }

		constexpr const Device_ID GetID() const { return Device_ID::RAM; }

		void Reset();
	};
}

#endif