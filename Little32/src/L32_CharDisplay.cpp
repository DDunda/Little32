#include "L32_CharDisplay.h"

#include "L32_Computer.h"
#include "L32_ICore.h"

#include <render.hpp>

namespace Little32
{
	void CharDisplay::_WriteWordUnsafe(word address, word value)
	{
		for (word i = 0; i < sizeof(word); i++)
		{
			memory[address++] = value;
			value >>= 8;
		}
	}
	void CharDisplay::_WriteByteUnsafe(word address, byte value){ memory[address] = value; }

	word CharDisplay::_ReadWordUnsafe(word address)
	{
		word value = memory[address++];
		for (word i = sizeof(word) - 1; i--;)
		{
			value <<= 8;
			value |= memory[address++];
		}
		return value;
	}
	byte CharDisplay::_ReadByteUnsafe(word address) { return memory[address]; }

	CharDisplay::CharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Point& charSize, word span, SDL::Point textSize, SDL::Point scale, word address, std::shared_ptr<byte[]>& memory) :
		computer(computer),
		r(r),
		txt(txt),
		charSize(charSize),
		span(span),
		dstCharSize(charSize* scale),
		textSize(textSize),
		address_start(address),
		address_size(textSize.w * textSize.h),
		interrupt_address(0),
		defaultMemory(memory),
		memory(new byte[address_size](0))
	{
		if (!defaultMemory) return;
		memcpy(this->memory.get(), defaultMemory.get(), address_size);
	}

	CharDisplay::CharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Point& charSize, word span, SDL::Point textSize, SDL::Point scale, word address) :
		computer(computer),
		r(r),
		txt(txt),
		charSize(charSize),
		span(span),
		dstCharSize(charSize* scale),
		textSize(textSize),
		address_start(address),
		address_size(textSize.w * textSize.h),
		interrupt_address(0),
		memory(new byte[address_size](0)) {}

	void CharDisplay::Write(word address, word value)
	{
		if (address > address_size) return;

		if (address == address_size)
		{
			interrupt_address = value;
		}
		else if ((address % sizeof(word) == 0) && (address_size - address >= sizeof(word)))
		{
			_WriteWordUnsafe(address, value);
		}
		else
		{
			_WriteByteUnsafe(address, value);
		}
	}
	void CharDisplay::WriteByte(word address, byte value)
	{
		if (address >= address_size + 4) return;

		if (address >= address_size)
		{
			word x = (address - address_size) * 8;

			value ^= interrupt_address >> x;
			interrupt_address ^= value << x;
		}
		else
		{
			_WriteByteUnsafe(address, value);
		}
	}

	word CharDisplay::Read(word address)
	{
		if (address > address_size) return 0;

		if (address == address_size)
		{
			return interrupt_address;
		}
		else if ((address % sizeof(word) == 0) && (address_size - address >= sizeof(word)))
		{
			return _ReadWordUnsafe(address);
		}
		else
		{
			return _ReadByteUnsafe(address);
		}

	}
	byte CharDisplay::ReadByte(word address)
	{
		if (address >= address_size + 4) return 0;
		if (address >= address_size)
		{
			return interrupt_address >> ((sizeof(word) - 1 - (address - address_size)) * 8);
		}
		else {
			return _ReadByteUnsafe(address);
		}
	}

	void CharDisplay::Render(bool do_interrupt)
	{
		if (address_size == 0) return;
		for (int i = 0, y = 0; y < textSize.h; y++)
		{
			for (int x = 0; x < textSize.w; x++)
			{
				byte c = _ReadByteUnsafe(i++);
				txt.Copy({ SDL::Point(c % span, c / span) * charSize, charSize }, { SDL::Point(x, y) * dstCharSize, dstCharSize });
			}
		}
		if (interrupt_address != 0 && do_interrupt)
		{
			computer.core->Interrupt(interrupt_address);
		}
	}

	void CharDisplay::Reset()
	{
		if (defaultMemory) memcpy(memory.get(), defaultMemory.get(), address_size);
		else memset(memory.get(), 0, address_size);
		interrupt_address = 0;
	};
}