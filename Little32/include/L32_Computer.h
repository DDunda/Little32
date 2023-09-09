#ifndef L32_Computer_h_
#define L32_Computer_h_
#pragma once

#include <vector>

#include "L32_Types.h"

namespace Little32 
{
	struct ICore;
	struct IDevice;
	struct IMemoryMapped;
	struct IMappedDevice;

	struct Computer
	{
		ICore* core = nullptr;
		std::vector<IDevice*> devices;
		std::vector<IMemoryMapped*> mappings;

		word start_PC = 0;
		word start_SP = 0;

		constexpr Computer() : devices(), mappings() {}

		/// <summary> Clocks the computer a number of times </summary>
		/// <param name="clocks">Number of times to clock the computer</param>
		void Clock(unsigned clocks);

		/// <summary> Clocks the computer once </summary>
		void Clock();

		word Read(word addr);

		byte ReadByte(word addr);

		void Write(word addr, word value);

		void WriteByte(word addr, byte value);

		/// <summary> Puts the core back to where it started executing without resetting the whole computer </summary>
		void SoftReset();

		/// <summary> Resets the computer as if it were power cycled </summary>
		void HardReset();

		void AddDevice(IDevice& dev);

		void AddMapping(IMemoryMapped& map);

		void AddMappedDevice(IMappedDevice& dev);
	};
}

#endif