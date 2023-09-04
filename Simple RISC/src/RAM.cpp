#include <cstring>
#include "RAM.h"

namespace SimpleRISC {
	void RAM::_WriteWordUnsafe(word address, word value) { memory[address / sizeof(word)] = value; }

	void RAM::_WriteByteUnsafe(word address, byte value) {
		word x = (address % sizeof(word)) * 8;

		value ^= memory[address / sizeof(word)] >> x;
		memory[address / sizeof(word)] ^= value << x;
	}

	word RAM::_ReadWordUnsafe(word address) { return memory[address / sizeof(word)]; }

	byte RAM::_ReadByteUnsafe(word address) { return _ReadWordUnsafe(address) >> ((address % sizeof(word)) * 8); }

	RAM::RAM(word address, word size, std::shared_ptr<word[]>& memory) :
		address_start(address),
		address_size(size * sizeof(word)),
		defaultMemory(memory),
		memory(new word[size](0)) {
		if (!defaultMemory) return;
		memcpy(this->memory.get(), defaultMemory.get(), address_size);
	}

	RAM::RAM(word address, word size) :
		address_start(address),
		address_size(size * sizeof(word)),
		memory(new word[size](0)) {}

	void RAM::Write(word address, word value) {
		if (address >= address_size) return;

		return address % sizeof(word) ? _WriteByteUnsafe(address, value) : _WriteWordUnsafe(address, value);
	}

	void RAM::WriteByte(word address, byte value) {
		if (address >= address_size) return;

		return _WriteByteUnsafe(address, value);
	}

	word RAM::Read(word address) {
		if (address >= address_size) return 0;

		return address % sizeof(word) ? _ReadByteUnsafe(address) : _ReadWordUnsafe(address);
	}

	byte RAM::ReadByte(word address) {
		if (address >= address_size) return 0;

		return _ReadByteUnsafe(address);
	}


	void RAM::Reset() {
		if (defaultMemory) memcpy(memory.get(), defaultMemory.get(), address_size);
		else memset(memory.get(), 0, address_size);
	};
}