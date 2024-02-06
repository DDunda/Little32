#pragma once

#ifndef L32_ColourCharDisplay_h_
#define L32_ColourCharDisplay_h_

#include <pixels.hpp>
#include <rect.hpp>
#include <render.hpp>

#include <filesystem>
#include <unordered_map>

#include "L32_Computer.h"
#include "L32_IMappedDevice.h"
#include "L32_IDeviceFactory.h"

namespace Little32
{
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

		inline static SDL::Colour colours[16] = {};

		word interrupt_address = 0;

		std::shared_ptr<Computer::Interval> refresh_interval = nullptr;

		Computer& computer;
		SDL::Renderer r;
		SDL::Texture txt;
		/// <summary> The pixel size of a character from the source texture </summary>
		const SDL::Point texture_char_size;
		const SDL::Point texture_position;
		/// <summary> Coordinates for the top-left of the display </summary>
		const SDL::Point position;
		/// <summary> The number of characters per row from the source texture </summary>
		const word texture_columns;
		/// <summary> The pixel size of a character rendered to the screen (calculated as charSize*scale) </summary>
		const SDL::Point dst_char_size;
		const SDL::Point dst_corner;
		/// <summary> The of the display in characters </summary>
		const SDL::Point text_size;

		ColourCharDisplay(Computer& computer, SDL::Renderer r, SDL::Texture txt, SDL::Point texture_position, SDL::Point texture_char_size, word texture_columns, SDL::Point text_size, SDL::Point pixel_position, SDL::Point pixel_scale, word address, std::shared_ptr<byte[]>& text_memory, std::shared_ptr<byte[]>& colour_memory);
		ColourCharDisplay(Computer& computer, SDL::Renderer r, SDL::Texture txt, SDL::Point texture_position, SDL::Point texture_char_size, word texture_columns, SDL::Point text_size, SDL::Point pixel_position, SDL::Point pixel_scale, word address, byte default_char, byte default_colour);
		ColourCharDisplay(Computer& computer, SDL::Renderer r, SDL::Texture txt, SDL::Point texture_position, SDL::Point texture_char_size, word texture_columns, SDL::Point text_size, SDL::Point pixel_position, SDL::Point pixel_scale, word address);

		~ColourCharDisplay();

		void Write(word address, word value);
		void WriteByte(word address, byte value);

		void WriteForced(word address, word value);
		void WriteByteForced(word address, byte value);

		word Read(word address);
		byte ReadByte(word address);

		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return address_size; }

		constexpr const Device_ID GetID() const { return COLOURCHARDISPLAY_DEVICE; }

		void Render(bool do_interrupt = true);

		void Reset();
	};

	struct ColourCharDisplayFactory : IDeviceFactory
	{
		void CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path cur_path) const;
		void VerifySettings(const IDeviceSettings& settings, std::filesystem::path path) const;
	};
}

#endif