#pragma once

#ifndef L32_RAM_h_
#define L32_RAM_h_

#include "L32_IDeviceFactory.h"
#include "L32_IMappedDevice.h"

#include <memory>

namespace Little32
{
	struct Computer;

	/// <summary>Read-Write memory device</summary>
	class RAM : public IMappedDevice
	{
	private:
		void _WriteWordUnsafe(word address, word value);
		void _WriteByteUnsafe(word address, byte value);

		word _ReadWordUnsafe(word address);
		byte _ReadByteUnsafe(word address);

	public:
		/// <summary> The start address of this RAM </summary>
		word address_start = 0;
		/// <summary> The size of this RAM in bytes </summary>
		word address_size = 0;
		/// <summary> The data used to fill memory when the device is reset </summary>
		std::shared_ptr<word[]> default_memory;
		std::shared_ptr<word[]> memory;

		RAM(word address, word size, std::shared_ptr<word[]>& memory);
		RAM(word address, word size, char default_byte = 0);

		void Write(word address, word value);
		void WriteByte(word address, byte value);

		void WriteForced(word address, word value);
		void WriteByteForced(word address, byte value);

		word Read(word address);
		byte ReadByte(word address);

		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return address_size; }

		constexpr const Device_ID GetID() const { return RAM_DEVICE; }

		void Reset();
	};

	struct RAMFactory : IDeviceFactory
	{
		void CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path cur_path) const;
		void VerifySettings(const IDeviceSettings& settings, std::filesystem::path cur_path) const;
	};
}

#endif