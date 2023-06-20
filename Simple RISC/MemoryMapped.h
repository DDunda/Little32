#pragma once

#include "SR_Definitions.h"

class MemoryMapped {
public:
	/// <summary>Attempts to write a word to this device</summary>
	/// <param name="address">The address relative to <c>address_start</c></param>
	/// <param name="value">The word to write</param>
	virtual void Write(word address, word value) {}

	/// <summary>Attempts to write a byte to this device</summary>
	/// <param name="address">The address relative to <c>address_start</c></param>
	/// <param name="value">The byte to write</param>
	virtual void WriteByte(word address, byte value) {}

	/// <summary>Attempts to read a word from this device</summary>
	/// <param name="address">The address relative to <c>address_start</c></param>
	virtual word Read(word address) { return 0; }

	/// <summary>Attempts to read a byte from this device</summary>
	/// <param name="address">The address relative to <c>address_start</c></param>
	virtual byte ReadByte(word address) { return 0; }

	/// <summary>Returns the start of the address space</summary>
	virtual word GetAddress() const = 0;

	/// <summary>Returns the addressable range of this device in bytes</summary>
	virtual word GetRange() const = 0;

	/// <summary> Returns an type identifier for this device </summary>
	virtual const Device_ID GetID() const = 0;
};