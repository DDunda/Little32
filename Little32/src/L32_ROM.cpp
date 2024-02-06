#include "L32_ROM.h"

#include "L32_BigInt.h"
#include "L32_Computer.h"
#include "L32_IDeviceSettings.h"
#include "L32_String.h"
#include "L32_VarValue.h"

namespace Little32
{
	void ROM::_WriteByteUnsafe(word address, byte value)
	{
		const word x = (address % sizeof(word)) * 8;

		value ^= memory[address / sizeof(word)] >> x;
		memory[address / sizeof(word)] ^= value << x;
	}
	void ROM::_WriteWordUnsafe(word address, word value)
	{
		if (address % sizeof(word) == 0)
		{
			memory[address / sizeof(word)] = value;
		}
		else
		{
			const word x = (address % sizeof(word)) * 8;

			memory[(address / sizeof(word)) + 0] &= (1 << x) - 1;
			memory[(address / sizeof(word)) + 0] |= value << x;

			memory[(address / sizeof(word)) + 1] &= (1 >> (32 - x)) - 1;
			memory[(address / sizeof(word)) + 1] |= value >> (32 - x);
		}
	}

	word ROM::_ReadWordUnsafe(word address) { return memory[address / sizeof(word)] ; }
	byte ROM::_ReadByteUnsafe(word address) { return memory[address / sizeof(word)] >> ((address % sizeof(word)) * 8); }

	ROM::ROM(word address, word size, std::shared_ptr<word[]>& memory) :
		address_start(address),
		address_size(size * sizeof(word)),
		memory(memory)
	{
		if (this->memory) return;
		this->memory = std::shared_ptr<word[]>(new word[size]{ 0 });
	}

	ROM::ROM(word address, word size, char default_byte) :
		address_start(address),
		address_size(size * sizeof(word)),
		memory(new word[size](0)) {
		memset(memory.get(), default_byte, size * sizeof(word));
	}

	word ROM::Read(word address)
	{
		if (address >= address_size) return 0;

		return address % sizeof(word) ? _ReadByteUnsafe(address) : _ReadWordUnsafe(address);
	}

	byte ROM::ReadByte(word address)
	{
		if (address >= address_size) return 0;

		return _ReadByteUnsafe(address);
	}

	void ROM::WriteForced(word address, word value)
	{
		if (address + 3 >= address_size) return;

		_WriteWordUnsafe(address, value);
	}

	void ROM::WriteByteForced(word address, byte value)
	{
		if (address >= address_size) return;

		_WriteByteUnsafe(address, value);
	}

	void ROMFactory::CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path) const
	{
		assert(settings.Contains("size_words"));
		assert(settings["size_words"].GetType() == INTEGER_VAR);
		assert(!settings["size_words"].GetIntegerValue().negative);
		assert(settings["size_words"].GetIntegerValue().bits.size() == 1);
		assert(settings["size_words"].GetIntegerValue().bits[0] <= 0xFFFFFFFF);

		const uint32_t words = static_cast<uint32_t>
			(
				settings["size_words"]
				.GetIntegerValue()
				.bits[0]
				);

		uint8_t default_byte = 0;
		if (settings.Contains("default_byte"))
		{
			assert(settings["default_byte"].GetType() == INTEGER_VAR);
			if (settings["default_byte"].GetIntegerValue().bits.empty())
			{
				default_byte = 0;
			}
			else
			{
				assert(!settings["default_byte"].GetIntegerValue().negative);
				assert(settings["default_byte"].GetIntegerValue().bits.size() == 1);
				assert(settings["default_byte"].GetIntegerValue().bits[0] <= 255);

				default_byte = static_cast<uint8_t>(
					settings["default_byte"]
					.GetIntegerValue()
					.bits[0]
					);
			}
		}

		ROM* device;

		if (!settings.Contains("ROM_data"))
		{
			device = new ROM(start_address, words, default_byte);
		}
		else
		{
			assert(settings["ROM_data"].GetType() == STRING_VAR);
			assert(settings["ROM_data"].GetStringValue().size() <= words * sizeof(word));
			assert(settings["ROM_data"].GetStringValue().size() / sizeof(word) <= ~word(0) / sizeof(word));

			word* const arr = new word[words];
			std::shared_ptr<word[]> memory(arr);

			size_t i = 0;

			memset(arr, default_byte, words * sizeof(word));

			for (const char& c : settings["ROM_data"].GetStringValue())
			{
				arr[i / sizeof(word)] |= c * (i % sizeof(word)) * 8;
				++i;
			}

			device = new ROM(start_address, words, memory);
		}

		assert(settings.named_labels.empty());

		start_address += words * sizeof(word);

		computer.AddMapping(*device);
	}

	void ROMFactory::VerifySettings(const IDeviceSettings& settings, std::filesystem::path cur_path) const
	{
		if (!settings.Contains("size_words")) throw std::exception("ROM must have a size");

		MatchUIntRange<0x00000001, 0xFFFFFFFF>(settings["size_words"], "size_words");

		const uint32_t words = static_cast<uint32_t>
			(
				settings["size_words"]
				.GetIntegerValue()
				.bits[0]
				);

		uint8_t default_byte = 0;
		if (settings.Contains("default_byte"))
		{
			MatchUIntRange<0x00, 0xFF>(settings["default_byte"], "default_byte");
			default_byte = !settings["default_byte"].GetIntegerValue().bits.empty()
				? static_cast<uint8_t>(
					settings["default_byte"]
					.GetIntegerValue()
					.bits[0]
					)
				: 0;
		}

		if (settings.Contains("ROM_data"))
		{
			MatchType(settings["ROM_data"], STRING_VAR, "ROM_data");
			const std::string& string_mem = settings["ROM_data"].GetStringValue();

			// Too big for size of ROM
			if (string_mem.size() > words * sizeof(word)) throw std::exception("Data string for ROM is too large");

			// This would be a >4GB string...
			if (string_mem.size() / sizeof(word) > ~word(0) / sizeof(word)) throw std::exception("Data string for ROM is larger than addressable range?");
		}

		if (!settings.named_labels.empty())
		{
			throw std::runtime_error("Unknown named label: '" + settings.named_labels.begin()->first + "'");
		}
	}
}