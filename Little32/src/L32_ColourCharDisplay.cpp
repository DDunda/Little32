#include "L32_ColourCharDisplay.h"

#include "L32_Computer.h"
#include "L32_ICore.h"

#include <render.hpp>

namespace Little32
{
	void ColourCharDisplay::_WriteTextWordUnsafe(word address, word value)
	{
		for (word i = 0; address < pixel_area && i < sizeof(word); i++)
		{
			text_memory[address++] = value;
			value >>= 8;
		}
	}

	void ColourCharDisplay::_WriteTextByteUnsafe(word address, byte value)
		{ text_memory[address] = value; }

	void ColourCharDisplay::_WriteColourWordUnsafe(word address, word value)
	{
		for (word i = 0; address < pixel_area && i < sizeof(word); i++)
		{
			colour_memory[address++] = value;
			value >>= 8;
		}
	}

	void ColourCharDisplay::_WriteColourByteUnsafe(word address, byte value)
		{ colour_memory[address] = value; }

	word ColourCharDisplay::_ReadTextWordUnsafe(word address)
	{
		word value = text_memory[address++];
		for (word i = 1; i < sizeof(word); i++, address++)
		{
			value <<= 8;
			if (address < pixel_area) value |= text_memory[address];
		}
		return value;
	}

	byte ColourCharDisplay::_ReadTextByteUnsafe(word address)
		{ return text_memory[address]; }

	word ColourCharDisplay::_ReadColourWordUnsafe(word address)
	{
		word value = colour_memory[address++];
		for (word i = 1; i < sizeof(word); i++, address++)
		{
			value <<= 8;
			if (address < pixel_area) value |= colour_memory[address];
		}
		return value;
	}

	byte ColourCharDisplay::_ReadColourByteUnsafe(word address)
		{ return colour_memory[address]; }

	ColourCharDisplay::ColourCharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Colour colours[16], const SDL::Point& char_size, word span, SDL::Point text_size, SDL::Point scale, word address, std::shared_ptr<byte[]>& text_memory, std::shared_ptr<byte[]>& colour_memory) :
		computer(computer),
		r(r),
		txt(txt),
		char_size(char_size),
		span(span),
		dst_char_size(char_size * scale),
		text_size(text_size),
		address_start(address),
		pixel_area(text_size.w * text_size.h),
		colour_position(text_size.w * text_size.h + ((sizeof(word) - (( text_size.w * text_size.h ) % sizeof(word))) % sizeof(word))),
		interrupt_position(2 * colour_position),
		address_size(2 * (text_size.w * text_size.h + ((sizeof(word) - (( text_size.w * text_size.h ) % sizeof(word))) % sizeof(word)))+sizeof(word)),
		interrupt_address(0),
		default_text_memory(text_memory),
		default_colour_memory(colour_memory),
		text_memory(new byte[pixel_area](0)),
		colour_memory(new byte[pixel_area](0))
	{
		if (default_text_memory) memcpy(this->text_memory.get(), default_text_memory.get(), pixel_area);
		if (default_colour_memory) memcpy(this->colour_memory.get(), default_colour_memory.get(), pixel_area);
		else memset(colour_memory.get(), 0x0F, pixel_area);

		memcpy(this->colours, colours, sizeof(SDL::Colour) * 16);
	}

	ColourCharDisplay::ColourCharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Colour colours[16], const SDL::Point& char_size, word span, SDL::Point text_size, SDL::Point scale, word address, byte default_char, byte default_colour) :
		computer(computer),
		r(r),
		txt(txt),
		char_size(char_size),
		span(span),
		dst_char_size(char_size * scale),
		text_size(text_size),
		address_start(address),
		pixel_area(text_size.w * text_size.h),
		colour_position(text_size.w * text_size.h + ((sizeof(word) - (( text_size.w * text_size.h ) % sizeof(word))) % sizeof(word))),
		interrupt_position(2 * colour_position),
		address_size(2 * (text_size.w * text_size.h + ((sizeof(word) - (( text_size.w * text_size.h ) % sizeof(word))) % sizeof(word)))+sizeof(word)),
		interrupt_address(0),
		default_text_memory(new byte[pixel_area](0)),
		default_colour_memory(new byte[pixel_area](0)),
		text_memory(new byte[pixel_area](0)),
		colour_memory(new byte[pixel_area](0))
	{
		memset(default_text_memory.get(), default_char, pixel_area);
		memset(text_memory.get(), default_char, pixel_area);
		memset(default_colour_memory.get(), default_colour, pixel_area);
		memset(colour_memory.get(), default_colour, pixel_area);

		memcpy(this->colours, colours, sizeof(SDL::Colour) * 16);
	}

	ColourCharDisplay::ColourCharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Colour colours[16], const SDL::Point& char_size, word span, SDL::Point text_size, SDL::Point scale, word address) :
		computer(computer),
		r(r),
		txt(txt),
		char_size(char_size),
		span(span),
		dst_char_size(char_size * scale),
		text_size(text_size),
		address_start(address),
		pixel_area(text_size.w * text_size.h),
		colour_position(text_size.w * text_size.h + ((sizeof(word) - (( text_size.w * text_size.h ) % sizeof(word))) % sizeof(word))),
		interrupt_position(2 * colour_position),
		address_size(2 * (text_size.w * text_size.h + ((sizeof(word) - (( text_size.w * text_size.h ) % sizeof(word))) % sizeof(word)))+sizeof(word)),
		interrupt_address(0),
		text_memory(new byte[pixel_area](0)),
		colour_memory(new byte[pixel_area](0))
	{
		memset(colour_memory.get(), 0x0F, pixel_area);

		memcpy(this->colours, colours, sizeof(SDL::Colour) * 16);
	}

	void ColourCharDisplay::Write(word address, word value)
	{
		if (address >= address_size) return;
		if (address == interrupt_position)
		{
			interrupt_address = value;
			return;
		}
		if (address >= colour_position)
		{
			if (( address - colour_position ) % sizeof(word) != 0) return;
			return _WriteColourWordUnsafe(address - colour_position, value);
		}
		else
		{
			if ( address % sizeof(word) != 0) return;
			return _WriteTextWordUnsafe(address, value);
		}
	}

	void ColourCharDisplay::WriteByte(word address, byte value)
	{
		if (address >= address_size) return;
		if (address >= interrupt_position)
		{
			word x = (address - interrupt_position) * 8;

			value ^= interrupt_address >> x;
			interrupt_address ^= value << x;
			return;
		}
		
		if (address >= colour_position)
		{
			if (address - colour_position >= pixel_area) return;
			_WriteColourByteUnsafe(address - colour_position, value);
		}
		else
		{
			if (address >= pixel_area) return;
			_WriteTextByteUnsafe(address, value);
		}
	}

	word ColourCharDisplay::Read(word address)
	{
		if (address > address_size) return 0;
		if (address == interrupt_position) return interrupt_address;

		if (address >= colour_position)
		{
			if (( address - colour_position ) % sizeof(word) != 0) return 0;
			return _ReadColourWordUnsafe(address - colour_position);
		}
		else
		{
			if (address % sizeof(word) != 0) return 0;
			return _ReadTextWordUnsafe(address);
		}
	}

	byte ColourCharDisplay::ReadByte(word address)
	{
		if (address >= address_size) return 0;
		if (address >= interrupt_position) return interrupt_address >> ((sizeof(word) - 1 - (address - interrupt_position)) * 8);

		if (address >= colour_position)
		{
			if (address - colour_position >= pixel_area) return 0;
			return _ReadColourByteUnsafe(address - colour_position);
		}
		else
		{
			if (address >= pixel_area) return 0;
			return _ReadTextByteUnsafe(address);
		}
	}

	void ColourCharDisplay::Render(bool doInterrupt)
	{
		if (address_size == 0) return;

		for (int i = 0, y = 0; y < text_size.h; y++)
		{
			for (int x = 0; x < text_size.w; x++, i++)
			{
				byte c = text_memory[i];
				SDL::Color fg = colours[colour_memory[i] & 0xF];
				SDL::Color bg = colours[(colour_memory[i] >> 4) & 0xF];

				SDL::Rect target = { SDL::Point(x, y) * dst_char_size, dst_char_size };

				r.SetDrawColour(bg).FillRect(target);

				txt.SetColourMod(fg.r, fg.g, fg.b);
				r.Copy(txt, { SDL::Point(c % span, c / span) * char_size, char_size }, target);
			}
		}

		if (interrupt_address != 0 && doInterrupt)
		{
			computer.core->Interrupt(interrupt_address);
		}
	}

	void ColourCharDisplay::Reset()
	{
		if (default_text_memory) memcpy(text_memory.get(), default_text_memory.get(), pixel_area);
		else memset(text_memory.get(), 0, pixel_area);

		if (default_colour_memory) memcpy(colour_memory.get(), default_colour_memory.get(), pixel_area);
		else memset(colour_memory.get(), 0x0F, pixel_area);

		interrupt_address = 0;
	};
}