#include "L32_RAM.h"

#include "L32_BigInt.h"
#include "L32_Computer.h"
#include "L32_IDeviceSettings.h"
#include "L32_String.h"
#include "L32_VarValue.h"

namespace Little32
{
	void RAM::_WriteWordUnsafe(word address, word value)
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

	void RAM::_WriteByteUnsafe(word address, byte value)
	{
		word x = (address % sizeof(word)) * 8;

		value ^= memory[address / sizeof(word)] >> x;
		memory[address / sizeof(word)] ^= value << x;
	}

	word RAM::_ReadWordUnsafe(word address) { return memory[address / sizeof(word)]; }

	byte RAM::_ReadByteUnsafe(word address) { return memory[address / sizeof(word)] >> ((address % sizeof(word)) * 8); }

	RAM::RAM(word address, word size, std::shared_ptr<word[]>& memory) :
		address_start(address),
		address_size(size * sizeof(word)),
		default_memory(memory),
		memory(new word[size](0))
	{
		if (!default_memory) return;
		memcpy(this->memory.get(), default_memory.get(), address_size);
	}

	RAM::RAM(word address, word size, char default_byte) :
		address_start(address),
		address_size(size * sizeof(word)),
		memory(new word[size](0)) {
		memset(memory.get(), default_byte, size * sizeof(word));
	}

	void RAM::Write(word address, word value)
	{
		if (address + 3 >= address_size) return;

		return address % sizeof(word) ? _WriteByteUnsafe(address, value) : _WriteWordUnsafe(address, value);
	}

	void RAM::WriteByte(word address, byte value)
	{
		if (address >= address_size) return;

		return _WriteByteUnsafe(address, value);
	}

	void RAM::WriteForced(word address, word value)
	{
		Write(address, value);
	}

	void RAM::WriteByteForced(word address, byte value)
	{
		WriteByte(address, value);
	}

	word RAM::Read(word address)
	{
		if (address >= address_size) return 0;

		return address % sizeof(word) ? _ReadByteUnsafe(address) : _ReadWordUnsafe(address);
	}

	byte RAM::ReadByte(word address)
	{
		if (address >= address_size) return 0;

		return _ReadByteUnsafe(address);
	}

	void RAM::Reset()
	{
		if (default_memory) memcpy(memory.get(), default_memory.get(), address_size);
		else memset(memory.get(), 0, address_size);
	};

	void RAMFactory::CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path) const
	{
		assert( settings.Contains("size_words"));
		assert( settings["size_words"].GetType() == INTEGER_VAR);
		assert(!settings["size_words"].GetIntegerValue().negative);
		assert( settings["size_words"].GetIntegerValue().bits.size() == 1);
		assert( settings["size_words"].GetIntegerValue().bits[0] <= 0xFFFFFFFF);

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

		RAM* device;

		if (!settings.Contains("RAM_data"))
		{
			device = new RAM(start_address, words, default_byte);
		}
		else
		{
			assert(settings["RAM_data"].GetType() == STRING_VAR);
			assert(settings["RAM_data"].GetStringValue().size() <= words * sizeof(word));
			assert(settings["RAM_data"].GetStringValue().size() / sizeof(word) <= ~word(0) / sizeof(word));

			word* const arr = new word[words];
			std::shared_ptr<word[]> memory(arr);

			size_t i = 0;

			memset(arr, default_byte, words * sizeof(word));

			for (const char& c : settings["RAM_data"].GetStringValue())
			{
				arr[i / sizeof(word)] |= c * (i % sizeof(word)) * 8;
				++i;
			}

			device = new RAM(start_address, words, memory);
		}

		start_address += words * sizeof(word);

		computer.AddMappedDevice(*device);
	}

	void RAMFactory::VerifySettings(const IDeviceSettings& settings, std::filesystem::path cur_path) const
	{
		if (!settings.Contains("size_words")) throw std::exception("RAM must have a size");

		MatchUIntRange<0x00000001,0xFFFFFFFF>(settings["size_words"], "size_words");

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

		if (settings.Contains("RAM_data"))
		{
			MatchType(settings["RAM_data"], STRING_VAR, "RAM_data");
			const std::string& string_mem = settings["RAM_data"].GetStringValue();

			// Too big for size of RAM
			if (string_mem.size() > words * sizeof(word)) throw std::exception("Data string for RAM is too large");

			// This would be a >4GB string...
			if (string_mem.size() / sizeof(word) > ~word(0) / sizeof(word)) throw std::exception("Data string for RAM is larger than addressable range?");
		}

		if (!settings.named_labels.empty())
		{
			throw std::runtime_error("Unknown named label: '" + settings.named_labels.begin()->first + "'");
		}
	}
}