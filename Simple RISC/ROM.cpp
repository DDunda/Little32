#include "ROM.h"

word ROM::_ReadWordUnsafe(word address) { return memory[address / sizeof(word)]; }
byte ROM::_ReadByteUnsafe(word address) { return memory[address / sizeof(word)] >> ((sizeof(word) - 1 - (address % sizeof(word))) * 8); }

ROM::ROM(word address, word size, std::shared_ptr<word[]>& memory) :
	address_start(address),
	address_size(size * sizeof(word)),
	memory(memory) {
	if (this->memory) return;
	this->memory = std::shared_ptr<word[]>(new word[size]{0});
}

ROM::ROM(word address, word size) :
	address_start(address),
	address_size(size * sizeof(word)),
	memory(new word[size]{ 0 }) {}

word ROM::Read(word address) {
	if (address >= address_size) return 0;

	return address % sizeof(word) ? _ReadByteUnsafe(address) : _ReadWordUnsafe(address);
}
byte ROM::ReadByte(word address) {
	if (address >= address_size) return 0;

	return _ReadByteUnsafe(address);
}

word ROM::GetAddress() const { return address_start; }
word ROM::GetRange() const { return address_size; }

const Device_ID ROM::GetID() const { return Device_ID::ROM; }