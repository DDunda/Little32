#pragma once

#include <vector>
#include "SR_Types.h"

namespace SimpleRISC {
	class Core;
	class Device;
	class MemoryMapped;
	class MappedDevice;

	class Computer {
	public:
		Core* core;
		std::vector<Device*> devices;
		std::vector<MemoryMapped*> mappings;

		word start_PC;
		word start_SP;

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

		void AddDevice(Device& dev);

		void AddMapping(MemoryMapped& map);

		void AddMappedDevice(MappedDevice& dev);
	};
}