#pragma once

#ifndef L32_ROM_h_
#define L32_ROM_h_

#include "L32_IDeviceFactory.h"
#include "L32_IMemoryMapped.h"

#include <memory>

namespace Little32
{
	/// <summary>Read-Only memory device</summary>
	class ROM : public IMemoryMapped
	{
	private:
		void _WriteWordUnsafe(word address, word value);
		void _WriteByteUnsafe(word address, byte value);

		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);

	public:
		/// <summary> The start address of this ROM </summary>
		word address_start = 0;
		/// <summary> The size of this ROM in bytes </summary>
		word address_size = 0;
		std::shared_ptr<word[]> memory;

		ROM(word address, word size, std::shared_ptr<word[]>& memory);
		ROM(word address, word size, char default_byte = 0);

		word Read(word address);
		byte ReadByte(word address);
		void WriteForced(word address, word value);
		void WriteByteForced(word address, byte value);
		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return address_size; }
		constexpr const Device_ID GetID() const { return ROM_DEVICE; }
	};

	struct ROMFactory : IDeviceFactory
	{
		void CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path cur_path) const;
		void VerifySettings(const IDeviceSettings& settings, std::filesystem::path cur_path) const;
	};
}

#endif