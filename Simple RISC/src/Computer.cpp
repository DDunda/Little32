#include "Core.h"
#include "Device.h"
#include "MemoryMapped.h"
#include "MappedDevice.h"
#include "Computer.h"

namespace SimpleRISC {
	void Computer::Clock(unsigned clocks) {
		for (; clocks > 0; clocks--) {
			for (int i = 0; i < devices.size(); i++) {
				devices[i]->Clock();
			}
			core->Clock();
		}
	}

	void Computer::Clock() {
		for (int i = 0; i < devices.size(); i++) {
			devices[i]->Clock();
		}
		core->Clock();
	}

	word Computer::Read(word addr) {
		word value = 0;
		for (int i = 0; i < mappings.size(); i++) {
			word start = mappings[i]->GetAddress();
			if (addr < start) continue;
			start = addr - start;
			if (start >= mappings[i]->GetRange()) continue;
			value |= mappings[i]->Read(start);
		}
		return value;
	}

	byte Computer::ReadByte(word addr) {
		byte value = 0;
		for (int i = 0; i < mappings.size(); i++) {
			word start = mappings[i]->GetAddress();
			if (addr < start) continue;
			start = addr - start;
			if (start >= mappings[i]->GetRange()) continue;
			value |= mappings[i]->ReadByte(start);
		}
		return value;
	}

	void Computer::Write(word addr, word value) {
		for (int i = 0; i < mappings.size(); i++) {
			word start = mappings[i]->GetAddress();
			if (addr < start) continue;
			start = addr - start;
			if (start >= mappings[i]->GetRange()) continue;
			mappings[i]->Write(start, value);
		}
	}

	void Computer::WriteByte(word addr, byte value) {
		for (int i = 0; i < mappings.size(); i++) {
			word start = mappings[i]->GetAddress();
			if (addr < start) continue;
			start = addr - start;
			if (start >= mappings[i]->GetRange()) continue;
			mappings[i]->WriteByte(start, value);
		}
	}

	void Computer::SoftReset() {
		core->SetPC(start_PC);
		core->SetSP(start_SP);
	}

	void Computer::HardReset() {
		for (int i = 0; i < devices.size(); i++) {
			devices[i]->Reset();
		}
		core->Reset();
		SoftReset();
	}

	void Computer::AddDevice(Device& dev) {
		devices.push_back(&dev);
	}

	void Computer::AddMapping(MemoryMapped& map) {
		mappings.push_back(&map);
	}

	void Computer::AddMappedDevice(MappedDevice& dev) {
		devices.push_back(&dev);
		mappings.push_back(&dev);
	}
}