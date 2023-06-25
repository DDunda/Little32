#pragma once

#include <rect.hpp>
#include "MappedDevice.h"

namespace SDL {
	struct Renderer;
	struct Texture;
}

namespace SimpleRISC {
	class Computer;

	class CharDisplay : public MappedDevice {
	public:
		void _WriteWordUnsafe(word address, word value);
		void _WriteByteUnsafe(word address, byte value);

		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);

		/// <summary> The start address of this RAM </summary>
		word address_start = 0;
		/// <summary> The size of this RAM in bytes (calculated from area of textSize) </summary>
		const word address_size = 0;
		/// <summary> The data used to fill memory when the device is reset </summary>
		std::shared_ptr<byte[]> defaultMemory = nullptr;
		std::shared_ptr<byte[]> memory;

		word interrupt_address = 0;

		Computer& computer;
		SDL::Renderer& r;
		SDL::Texture& txt;
		/// <summary> The pixel size of a character from the source texture </summary>
		const SDL::Point charSize;
		/// <summary> The number of characters per row from the source texture </summary>
		const word span;
		/// <summary> The pixel size of a character rendered to the screen (calculated as charSize*scale) </summary>
		const SDL::Point dstCharSize;
		/// <summary> The of the display in characters </summary>
		const SDL::Point textSize;

		CharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Point& charSize, word span, SDL::Point textSize, SDL::Point scale, word address, std::shared_ptr<byte[]>& memory);
		CharDisplay(Computer& computer, SDL::Renderer& r, SDL::Texture& txt, const SDL::Point& charSize, word span, SDL::Point textSize, SDL::Point scale, word address);

		void Write(word address, word value);
		void WriteByte(word address, byte value);

		word Read(word address);
		byte ReadByte(word address);

		word GetAddress() const;
		word GetRange() const;
		const Device_ID GetID() const;

		void Render();

		void Reset();
	};
}