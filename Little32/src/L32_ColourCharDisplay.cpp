#include "L32_ColourCharDisplay.h"

#include <unordered_set>

#include "L32_BigInt.h"
#include "L32_Computer.h"
#include "L32_ConfigParser.h"
#include "L32_ICore.h"
#include "L32_IDeviceSettings.h"
#include "L32_ImageLoader.h"
#include "L32_VarValue.h"

namespace Little32
{
	void ColourCharDisplay::_WriteTextWordUnsafe(word address, word value)
	{
		text_memory[address + 0] = value >> 0;
		text_memory[address + 1] = value >> 8;
		text_memory[address + 2] = value >> 16;
		text_memory[address + 3] = value >> 24;
	}

	void ColourCharDisplay::_WriteTextByteUnsafe(word address, byte value)
	{
		text_memory[address] = value;
	}

	void ColourCharDisplay::_WriteColourWordUnsafe(word address, word value)
	{
		colour_memory[address + 0] = value >> 0;
		colour_memory[address + 1] = value >> 8;
		colour_memory[address + 2] = value >> 16;
		colour_memory[address + 3] = value >> 24;
	}

	void ColourCharDisplay::_WriteColourByteUnsafe(word address, byte value)
	{
		colour_memory[address] = value;
	}

	word ColourCharDisplay::_ReadTextWordUnsafe(word address)
	{
		return
			(static_cast<word>(text_memory[address + 0]) <<  0) |
			(static_cast<word>(text_memory[address + 1]) <<  8) | 
			(static_cast<word>(text_memory[address + 2]) << 16) | 
			(static_cast<word>(text_memory[address + 3]) << 24);
	}

	byte ColourCharDisplay::_ReadTextByteUnsafe(word address)
	{
		return text_memory[address];
	}

	word ColourCharDisplay::_ReadColourWordUnsafe(word address)
	{
		return
			(static_cast<word>(colour_memory[address + 0]) << 0) |
			(static_cast<word>(colour_memory[address + 1]) << 8) |
			(static_cast<word>(colour_memory[address + 2]) << 16) |
			(static_cast<word>(colour_memory[address + 3]) << 24);
	}

	byte ColourCharDisplay::_ReadColourByteUnsafe(word address)
	{
		return colour_memory[address];
	}

	ColourCharDisplay::ColourCharDisplay(Computer& computer, SDL::Renderer r, SDL::Texture txt, SDL::Point texture_position, SDL::Point texture_char_size, word texture_columns, SDL::Point text_size, SDL::Point pixel_position, SDL::Point pixel_scale, word address, std::shared_ptr<byte[]>& text_memory, std::shared_ptr<byte[]>& colour_memory) :
		computer(computer),
		r(r),
		txt(txt),
		texture_position(texture_position),
		texture_char_size(texture_char_size),
		texture_columns(texture_columns),
		dst_char_size(texture_char_size * pixel_scale),
		text_size(text_size),
		dst_corner(pixel_position - texture_char_size * text_size * SDL::Point(pixel_scale.w < 0 ? pixel_scale.w : 0, pixel_scale.h < 0 ? pixel_scale.h : 0)),
		address_start(address),
		pixel_area(text_size.w * text_size.h),
		colour_position(text_size.w * text_size.h + ((sizeof(word) - ((text_size.w * text_size.h) % sizeof(word))) % sizeof(word))),
		interrupt_position(2 * colour_position),
		address_size(2 * (text_size.w * text_size.h + ((sizeof(word) - ((text_size.w * text_size.h) % sizeof(word))) % sizeof(word))) + sizeof(word)),
		interrupt_address(0),
		default_text_memory(text_memory),
		default_colour_memory(colour_memory),
		text_memory(new byte[pixel_area](0)),
		colour_memory(new byte[pixel_area](0))
	{
		if (default_text_memory) memcpy(this->text_memory.get(), default_text_memory.get(), pixel_area);
		if (default_colour_memory) memcpy(this->colour_memory.get(), default_colour_memory.get(), pixel_area);
		else memset(colour_memory.get(), 0x0F, pixel_area);
	}

	ColourCharDisplay::ColourCharDisplay(Computer& computer, SDL::Renderer r, SDL::Texture txt, SDL::Point texture_position, SDL::Point texture_char_size, word texture_columns, SDL::Point text_size, SDL::Point pixel_position, SDL::Point pixel_scale, word address, byte default_char, byte default_colour) :
		computer(computer),
		r(r),
		txt(txt),
		texture_position(texture_position),
		texture_char_size(texture_char_size),
		texture_columns(texture_columns),
		dst_char_size(texture_char_size * pixel_scale),
		text_size(text_size),
		dst_corner(pixel_position - texture_char_size * text_size * SDL::Point(pixel_scale.w < 0 ? pixel_scale.w : 0, pixel_scale.h < 0 ? pixel_scale.h : 0)),
		address_start(address),
		pixel_area(text_size.w * text_size.h),
		colour_position(text_size.w * text_size.h + ((sizeof(word) - ((text_size.w * text_size.h) % sizeof(word))) % sizeof(word))),
		interrupt_position(2 * colour_position),
		address_size(2 * (text_size.w * text_size.h + ((sizeof(word) - ((text_size.w * text_size.h) % sizeof(word))) % sizeof(word))) + sizeof(word)),
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
	}

	ColourCharDisplay::ColourCharDisplay(Computer& computer, SDL::Renderer r, SDL::Texture txt, SDL::Point texture_position, SDL::Point texture_char_size, word texture_columns, SDL::Point text_size, SDL::Point pixel_position, SDL::Point pixel_scale, word address) :
		computer(computer),
		r(r),
		txt(txt),
		texture_position(texture_position),
		texture_char_size(texture_char_size),
		texture_columns(texture_columns),
		dst_char_size(texture_char_size * pixel_scale),
		text_size(text_size),
		dst_corner(pixel_position - texture_char_size * text_size * SDL::Point(pixel_scale.w < 0 ? pixel_scale.w : 0, pixel_scale.h < 0 ? pixel_scale.h : 0)),
		address_start(address),
		pixel_area(text_size.w * text_size.h),
		colour_position(text_size.w * text_size.h + ((sizeof(word) - ((text_size.w * text_size.h) % sizeof(word))) % sizeof(word))),
		interrupt_position(2 * colour_position),
		address_size(2 * (text_size.w * text_size.h + ((sizeof(word) - ((text_size.w * text_size.h) % sizeof(word))) % sizeof(word))) + sizeof(word)),
		interrupt_address(0),
		text_memory(new byte[pixel_area](0)),
		colour_memory(new byte[pixel_area](0))
	{
		memset(colour_memory.get(), 0x0F, pixel_area);
	}

	ColourCharDisplay::~ColourCharDisplay()
	{
		if (refresh_interval == nullptr) return;

		computer.RemoveInterval(refresh_interval);
	}

	void ColourCharDisplayFactory::CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path path) const
	{
		std::string texture_file = "./assets/char set.png";

		if (settings.Contains("texture_file"))
		{
			assert(settings["texture_file"].GetType() == STRING_VAR);
			texture_file = settings["texture_file"].GetStringValue();
		}

		const auto relative_to_program = (std::filesystem::current_path() / texture_file).lexically_normal();
		const auto relative_to_config = (path.parent_path() / texture_file).lexically_normal();

		SDL::Texture texture = ImageLoader::GetImage(relative_to_config);
		if (texture.texture.get() == NULL)
		{
			texture = ImageLoader::GetImage(relative_to_program);
			if (texture.texture.get() == NULL)
				throw std::runtime_error("Could not open character texture at '" + texture_file + "'");
		}

		// Where the character set is placed in the texture
		SDL::Point texture_position = { 0, 0 };
		// The width and height of characters on the texture
		SDL::Point texture_char_size = { 8, 8 };
		// The number of columns on the texture
		uint32_t texture_columns = 16;

		// The size of the text output in characters
		SDL::Point text_size = { 16, 16 };

		// Where this display is placed in the viewport
		SDL::Point pixel_position = { 0, 0 };
		// The scale of the texts displayed, in multiples of base characters
		SDL::Point pixel_scale = { 4, 4 };

		// Modes:
		// 0 - Locked to cpu cycles
		// 1 - Locked to FPS
		// 2 - Locked to time per frame
		// 3 - VSYNC ?
		uint8_t framerate_mode = 0;

		// Meaning per mode:
		// 0 - Cycles per frame
		// 1 - Target FPS
		// 2 - ms per frame
		// 3 - Ignored
		uint32_t framerate_lock = 1000; // todo

		byte default_char = ' ';
		byte default_colour = 0x0F; // White on black
		
		if (settings.Contains("texture_position"))
		{
			texture_position = settings["texture_position"].GetVectorValue();
			assert(texture_position.x >= 0 && texture_position.y >= 0);
		}

		if (settings.Contains("texture_char_size"))
		{
			texture_char_size = settings["texture_char_size"].GetVectorValue();
			assert(texture_char_size.w > 0);
			assert(texture_char_size.h > 0);
		}

		if (settings.Contains("texture_columns"))
		{
			assert(!settings["texture_columns"].GetIntegerValue().negative);
			assert(!settings["texture_columns"].GetIntegerValue().bits.empty());
			assert(settings["texture_columns"].GetIntegerValue().NumBits() <= 8);
			texture_columns = settings["texture_columns"].GetIntegerValue().bits[0];
		}

		SDL::Point txt_size = {0,0};

		if (!texture.QuerySize(txt_size))
			throw std::exception("Could not retrieve texture size");

		if (texture_position.x >= txt_size.w ||
			texture_position.y >= txt_size.h ||
			texture_position.x + texture_char_size.w > txt_size.w ||
			texture_position.y + texture_char_size.h > txt_size.h
			)
			throw std::runtime_error(
				"Character set (" +
					std::to_string(texture_char_size.w)
				+ "x" +
					std::to_string(texture_char_size.h)
				+ " at " +
					(std::string)texture_position
				+ ") does not fit inside of texture (" +
					std::to_string(txt_size.w)
				+ "x" +
					std::to_string(txt_size.h)
				+ ")"
			);

		if (settings.Contains("text_size"))
		{
			text_size = settings["text_size"].GetVectorValue();
			assert(text_size.w > 0);
			assert(text_size.h > 0);
		}

		// This position can be anything, so checking is unecessary (allows negatives, zeroes)
		if (settings.Contains("pixel_position"))
		{
			pixel_position = settings["pixel_position"].GetVectorValue();
		}

		if (settings.Contains("pixel_scale"))
		{
			pixel_scale = settings["pixel_scale"].GetVectorValue();
			assert(pixel_scale.x != 0);
			assert(pixel_scale.y != 0);
		}

		if (settings.Contains("framerate_mode"))
		{
			const BigInt& val = settings["framerate_mode"].GetIntegerValue();
			assert(!val.negative);
			assert(val.bits.size() < 2);
			assert(val.bits.empty()
				|| val.bits[0] < 3);
			//if (framerate_mode > 3)
			//	throw std::runtime_error("Unknown framerate mode selected (" + std::to_string(framerate_mode) + ", must be 0-3)");

			framerate_mode = val.bits.empty() ? 0 : static_cast<uint8_t>(val.bits[0]);

			// Reasonable defaults per mode
			switch (framerate_mode)
			{
			case 0:
				framerate_lock = 1000; // Cycles per frame
				break;
			case 1:
				framerate_lock = 24; // FPS
				break;
			case 2: // ms per frame
				framerate_lock = 42;
				break;
			}
		}

		if (settings.Contains("framerate_lock"))
		{
			const BigInt& val = settings["framerate_lock"].GetIntegerValue();
			assert(!val.negative);
			assert(val.NumBits() <= 32);
			framerate_lock = val.bits.empty() ? 0 : static_cast<uint32_t>(val.bits[0]);
		}

		SDL::Renderer r = SDL::Renderer(texture.renderer);

		ColourCharDisplay* ccd = new ColourCharDisplay(
			computer,
			SDL::Renderer(texture.renderer),
			texture,
			texture_position,
			texture_char_size,
			static_cast<word>(texture_columns),
			text_size,
			pixel_position,
			pixel_scale,
			start_address,
			default_char,
			default_colour
		);

		const ConfigObject* labels_obj;

		const std::unordered_map<std::string, word> named_labels
			= {
				{ "text_position", start_address + 0 },
				{ "colour_position", start_address + ccd->colour_position },
				{ "interrupt_position", start_address + ccd->interrupt_position }
		};

		for (const auto& [name, vec] : settings.named_labels)
		{
			assert(named_labels.contains(name));

			const word& addr = named_labels.at(name);

			for (auto& label : vec)
			{
				labels[label] = addr;
			}
		}

		computer.AddMappedDevice(*ccd);
		start_address += ccd->GetRange();

		switch (framerate_mode)
		{
		case 0:
			ccd->refresh_interval = computer.AddInterval(framerate_lock, [ccd](Computer& computer)->void
				{
					ccd->Render(true);
				}
			);
			break;
		case 1:
			break;
		case 2:
			break;
		}
	}

	void ColourCharDisplayFactory::VerifySettings(const IDeviceSettings& settings, std::filesystem::path path) const
	{
		std::string texture_file = "./assets/char set.png";

		if (settings.Contains("texture_file"))
		{
			MatchType(settings["texture_file"], STRING_VAR, "texture_file");
			texture_file = settings["texture_file"].GetStringValue();
		}

		const auto relative_to_program = (std::filesystem::current_path() / texture_file).lexically_normal();
		const auto relative_to_config = (path.parent_path() / texture_file).lexically_normal();

		SDL::Texture texture = ImageLoader::GetImage(relative_to_config);
		if (texture.texture.get() == NULL)
		{
			texture = ImageLoader::GetImage(relative_to_program);
			if (texture.texture.get() == NULL) throw std::runtime_error("Could not open character set at '" + texture_file + "'");
		}

		// Where the character set is placed in the texture
		SDL::Point texture_position = { 0, 0 };
		// The width and height of characters on the texture
		SDL::Point texture_char_size = { 8, 8 };
		// The number of columns on the texture
		uint8_t texture_columns = 16;

		// The size of the text output in characters
		SDL::Point text_size = { 16, 16 };

		// Where this display is placed in the viewport
		SDL::Point pixel_position = { 0, 0 };
		// The scale of the texts displayed, in multiples of base characters
		SDL::Point pixel_scale = { 4, 4 };

		// Modes:
		// 0 - Locked to cpu cycles
		// 1 - Locked to FPS
		// 2 - Locked to time per frame
		// 3 - VSYNC ?
		uint8_t framerate_mode = 0;

		// Meaning per mode:
		// 0 - Cycles per frame
		// 1 - Target FPS
		// 2 - ms per frame
		// 3 - Ignored
		uint32_t framerate_lock = 1000; // todo

		byte default_char = ' ';
		byte default_colour = 0x0F; // White on black

		if (settings.Contains("texture_position"))
		{
			MatchType(settings["texture_position"], VECTOR_VAR, "texture_position");
			texture_position = settings["texture_position"].GetVectorValue();
			if (texture_position.x < 0 || texture_position.y < 0)
				throw std::runtime_error("Texture position cannot be negative " + (std::string)texture_position);
		}

		if (settings.Contains("texture_char_size"))
		{
			MatchType(settings["texture_char_size"], VECTOR_VAR, "texture_char_size");
			texture_char_size = settings["texture_char_size"].GetVectorValue();
			if (texture_char_size.w < 0)
				throw std::runtime_error("Character width cannot be negative " + (std::string)texture_char_size);
			if (texture_char_size.h < 0)
				throw std::runtime_error("Character height cannot be negative " + (std::string)texture_char_size);
			if (pixel_scale.w == 0)
				throw std::runtime_error("Character width cannot be zero " + (std::string)texture_char_size);
			if (pixel_scale.h == 0)
				throw std::runtime_error("Character height cannot be zero " + (std::string)texture_char_size);
		}

		if (settings.Contains("texture_columns"))
		{
			MatchType(settings["texture_columns"], INTEGER_VAR, "texture_columns");
			const BigInt& val = settings["texture_columns"].GetIntegerValue();
			if (val.bits.empty()) throw std::runtime_error("Character set cannot be 0 characters wide");
			if (val.NumBits() > 8) throw std::runtime_error("Character set cannot be over 256 characters wide (" + val.ToStringCheap() + ")");
			if (val.negative)  throw std::runtime_error("Character set cannot be negative characters wide (" + val.ToStringCheap() + ")");
			texture_columns = val.bits[0];
		}

		SDL::Point txt_size = { 0,0 };

		if (!texture.QuerySize(txt_size)) throw std::exception("Could not retrieve texture size");

		if (texture_position.x >= txt_size.w ||
			texture_position.y >= txt_size.h ||
			texture_position.x + texture_char_size.w > txt_size.w ||
			texture_position.y + texture_char_size.h > txt_size.h
			)
			throw std::runtime_error(
				"Character set (" +
					std::to_string(texture_char_size.w)
				+ "x" +
					std::to_string(texture_char_size.h)
				+ " at " +
					(std::string)texture_position
				+ ") does not fit inside of texture (" +
					std::to_string(txt_size.w)
				+ "x" +
					std::to_string(txt_size.h)
				+ ")"
			);

		if (settings.Contains("text_size"))
		{
			MatchType(settings["text_size"], VECTOR_VAR, "text_size");
			text_size = settings["text_size"].GetVectorValue();
			if (text_size.x  < 0) throw std::runtime_error("Text width cannot be negative " + (std::string)text_size);
			if (text_size.y  < 0) throw std::runtime_error("Text height cannot be negative " + (std::string)text_size);
			if (text_size.x == 0) throw std::runtime_error("Text width cannot be zero " + (std::string)text_size);
			if (text_size.y == 0) throw std::runtime_error("Text height cannot be zero " + (std::string)text_size);
		}

		// This position can be anything, so checking is unecessary (allows negatives, zeroes)
		if (settings.Contains("pixel_position"))
		{
			MatchType(settings["pixel_position"], VECTOR_VAR, "pixel_position");
			pixel_position = settings["pixel_position"].GetVectorValue();
		}

		if (settings.Contains("pixel_scale"))
		{
			MatchType(settings["pixel_scale"], VECTOR_VAR, "pixel_scale");
			pixel_scale = settings["pixel_scale"].GetVectorValue();
			if (pixel_scale.x == 0) throw std::runtime_error("Text X scale cannot be zero " + (std::string)pixel_scale);
			if (pixel_scale.y == 0) throw std::runtime_error("Text Y scale cannot be zero " + (std::string)pixel_scale);
		}

		if (settings.Contains("framerate_mode"))
		{
			MatchType(settings["framerate_mode"], INTEGER_VAR, "framerate_mode");
			const BigInt& val = settings["framerate_mode"].GetIntegerValue();
			if (val.negative || val.bits.size() > 1 || (!val.bits.empty() && val.bits[0] > 2))
				throw std::runtime_error("Unknown framerate mode selected (" + val.ToStringCheap() + ", must be 0-3)");

			framerate_mode = val.bits.empty() ? 0 : static_cast<uint8_t>(val.bits[0]);

			// Reasonable defaults per mode
			switch (framerate_mode)
			{
			case 0:
				framerate_lock = 1000; // Cycles per frame
				break;
			case 1:
				framerate_lock = 24; // FPS
				break;
			case 2: // ms per frame
				framerate_lock = 42;
				break;
			}
		}

		if (settings.Contains("framerate_lock"))
		{
			MatchType(settings["framerate_lock"], INTEGER_VAR, "framerate_lock");
			const BigInt& val = settings["framerate_lock"].GetIntegerValue();
			if (val.negative ||val.NumBits() > 32)
				throw std::runtime_error("Framerate lock must be unsigned and within 32 bits (" + val.ToStringCheap() + ")");
			framerate_lock = val.bits.empty() ? 0 : static_cast<uint32_t>(val.bits[0]);
		}

		SDL::Renderer r = SDL::Renderer(texture.renderer);

		const ConfigObject* labels_obj;

		const std::unordered_set<std::string> label_names
			= { "text_position", "colour_position", "interrupt_position" };

		for (const auto& [name, vec] : settings.named_labels)
		{
			if (!label_names.contains(name))
				throw std::runtime_error("Unknown named label: '" + name + "'");
		}

		switch (framerate_mode)
		{
		case 0: // Cycles per frame
			if (framerate_lock == 0) throw std::exception("Cycles per frame must be at least 1");
			break;
		case 1: // FPS
			break;
		case 2: // ms per frame
			if (framerate_lock == 0) return throw std::exception("Milliseconds per frame must be at least 1ms");
			break;
		}
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
			if ((address - colour_position) % sizeof(word) != 0) return;
			return _WriteColourWordUnsafe(address - colour_position, value);
		}
		else
		{
			if (address % sizeof(word) != 0) return;
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

	void ColourCharDisplay::WriteForced(word address, word value)
	{
		Write(address, value);
	}

	void ColourCharDisplay::WriteByteForced(word address, byte value)
	{
		WriteByte(address, value);
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

		// Original viewport before entering function
		const SDL::Rect vp = r.GetViewport();
		r.SetViewport({ vp.pos + position, text_size * dst_char_size });

		for (int i = 0, y = 0; y < text_size.h; y++)
		{
			for (int x = 0; x < text_size.w; x++, i++)
			{
				byte c = text_memory[i];
				SDL::Color fg = colours[colour_memory[i] & 0xF];
				SDL::Color bg = colours[(colour_memory[i] >> 4) & 0xF];

				SDL::Rect target = { dst_corner + SDL::Point(x, y) * dst_char_size, dst_char_size };

				r.SetDrawColour(bg);
				r.FillRect(target);

				txt.SetColourMod(fg.r, fg.g, fg.b);
				txt.Copy({ SDL::Point(c % texture_columns, c / texture_columns) * texture_char_size + texture_position, texture_char_size }, target);
			
			}
		}

		r.ClearViewport();
		r.SetViewport(vp);

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