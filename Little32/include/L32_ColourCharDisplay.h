#ifndef L32_ColourCharDisplay_h_
#define L32_ColourCharDisplay_h_
#pragma once

#include <pixels.hpp>
#include <rect.hpp>

#include "L32_IMappedDevice.h"

namespace SDL
{
	struct Renderer;
	struct Texture;
}

namespace Little32
{
	struct Computer;

	class ColourCharDisplay : public IMappedDevice
	{
	private:
		void _WriteTextWordUnsafe(word address, word value);
		void _WriteTextByteUnsafe(word address, byte value);
		void _WriteColourWordUnsafe(word address, word value);
		void _WriteColourByteUnsafe(word address, byte value);

		word _ReadTextWordUnsafe(word address);
		byte _ReadTextByteUnsafe(word address);
		word _ReadColourWordUnsafe(word address);
		byte _ReadColourByteUnsafe(word address);

	public:
		/// <summary> The start address of this RAM </summary>
		word address_start = 0;
		/// <summary> The size of this RAM in bytes (calculated from area of textSize) </summary>
		const word address_size = 0;
		const word pixel_area = 0;

		const word colour_position = 0;
		const word interrupt_position = 0;

		/// <summary> The data used to fill text memory when the device is reset </summary>
		std::shared_ptr<byte[]> default_text_memory = nullptr;
		std::shared_ptr<byte[]> text_memory;
		/// <summary> The data used to fill colour memory when the device is reset </summary>
		std::shared_ptr<byte[]> default_colour_memory = nullptr;
		std::shared_ptr<byte[]> colour_memory;

		SDL::Colour colours[16];

		word interrupt_address = 0;

		Computer& computer;
		SDL::Renderer& r;
		SDL::Texture& txt;
		/// <summary> The pixel size of a character from the source texture </summary>
		const SDL::Point char_size;
		/// <summary> The number of characters per row from the source texture </summary>
		const word span;
		/// <summary> The pixel size of a character rendered to the screen (calculated as charSize*scale) </summary>
		const SDL::Point dst_char_size;
		/// <summary> The of the display in characters </summary>
		const SDL::Point text_size;

		ColourCharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Colour colours[16], const SDL::Point& char_size, word span, SDL::Point text_size, SDL::Point scale, word address, std::shared_ptr<byte[]>& text_memory, std::shared_ptr<byte[]>& colour_memory);
		ColourCharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Colour colours[16], const SDL::Point& char_size, word span, SDL::Point text_size, SDL::Point scale, word address, byte default_char, byte default_colour);
		ColourCharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Colour colours[16], const SDL::Point& char_size, word span, SDL::Point text_size, SDL::Point scale, word address);

		void Write(word address, word value);
		void WriteByte(word address, byte value);

		word Read(word address);
		byte ReadByte(word address);

		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return address_size; }

		constexpr const Device_ID GetID() const { return Device_ID::ColourCharDisplay; }

		void Render(bool do_interrupt = true);

		void Reset();
	};
}

#endif