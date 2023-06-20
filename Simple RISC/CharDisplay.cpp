#include "CharDisplay.h"
#include <render.hpp>

void CharDisplay::_WriteWordUnsafe(word address, word value) {
	address += sizeof(word) - 1;
	memory[address--] = value;
	for (word i = 0; i < sizeof(word) - 1; i++) {
		value >>= 8;
		memory[address--] = value;
	}
}
void CharDisplay::_WriteByteUnsafe(word address, byte value) { memory[address] = value; }

word CharDisplay::_ReadWordUnsafe(word address) {
	word value = memory[address++];
	for (word i = 0; i < sizeof(word) - 1; i++) {
		value <<= 8;
		value |= memory[address++];
	}
	return value;
}
byte CharDisplay::_ReadByteUnsafe(word address) { return memory[address]; }

CharDisplay::CharDisplay(SDL::Renderer& r, SDL::Texture& txt, const SDL::Point& charSize, word span, SDL::Point textSize, SDL::Point scale, word address, std::shared_ptr<byte[]>& memory) :
	r(r),
	txt(txt),
	charSize(charSize),
	span(span),
	dstCharSize(charSize* scale),
	textSize(textSize),
	address_start(address),
	address_size(textSize.w* textSize.h),
	defaultMemory(memory),
	memory(new byte[address_size]{0}) {
	if (!defaultMemory) return;
	memcpy(this->memory.get(), defaultMemory.get(), address_size);
}

CharDisplay::CharDisplay(SDL::Renderer& r, SDL::Texture& txt, const SDL::Point& charSize, word span, SDL::Point textSize, SDL::Point scale, word address) :
	r(r),
	txt(txt),
	charSize(charSize),
	span(span),
	dstCharSize(charSize* scale),
	textSize(textSize),
	address_start(address),
	address_size(textSize.w* textSize.h),
	memory(new byte[address_size]{0}) {}

void CharDisplay::Write(word address, word value) {
	if (address >= address_size) return;

	return (address % sizeof(word) == 0) && (address_size - address >= sizeof(word)) ? _WriteWordUnsafe(address, value) : _WriteByteUnsafe(address, value);
}
void CharDisplay::WriteByte(word address, byte value) {
	if (address >= address_size) return;

	return _WriteByteUnsafe(address, value);
}

word CharDisplay::Read(word address) {
	if (address >= address_size) return 0;

	return address % sizeof(word) ? _ReadByteUnsafe(address) : _ReadWordUnsafe(address);
}
byte CharDisplay::ReadByte(word address) {
	if (address >= address_size) return 0;

	return _ReadByteUnsafe(address);
}

word CharDisplay::GetAddress() const { return address_start; }
word CharDisplay::GetRange() const { return address_size; }
const Device_ID CharDisplay::GetID() const { return Device_ID::CharDisplay; }

void CharDisplay::Render() {
	if (address_size == 0) return;
	for (int i = 0, y = 0; y < textSize.h; y++) {
		for (int x = 0; x < textSize.w; x++) {
			byte c = _ReadByteUnsafe(i++);
			r.Copy(txt, { SDL::Point(c % span, c / span) * charSize, charSize }, { SDL::Point(x, y) * dstCharSize, dstCharSize });
		}
	}
}

void CharDisplay::Reset() {
	if (defaultMemory) memcpy(memory.get(), defaultMemory.get(), address_size);
	else memset(memory.get(), 0, address_size);
};