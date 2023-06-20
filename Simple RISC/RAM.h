#pragma once

#include<memory>
#include "MappedDevice.h"

/// <summary>Read-Write memory device</summary>
class RAM : public MappedDevice {
public:
	void _WriteWordUnsafe(word address, word value);
	void _WriteByteUnsafe(word address, byte value);

	word _ReadWordUnsafe(word address);
	byte _ReadByteUnsafe(word address);

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

	word GetAddress() const;
	word GetRange() const;
	const Device_ID GetID() const;

	void Reset();
};