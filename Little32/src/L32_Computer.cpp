#include "L32_Computer.h"

#include "L32_ICore.h"
#include "L32_IDevice.h"
#include "L32_IMappedDevice.h"
#include "L32_IMemoryMapped.h"

namespace Little32
{
	void Computer::Clock(unsigned clocks)
	{
		for (; clocks > 0; clocks--)
		{
			CheckIntervals();

			for (size_t i = 0; i < devices.size(); i++)
			{
				devices[i]->Clock();
			}
			for (size_t i = 0; i < mapped_devices.size(); i++)
			{
				mapped_devices[i]->Clock();
			}
			core->Clock();
			cur_cycle++;
		}
	}

	void Computer::Clock()
	{
		CheckIntervals();

		for (size_t i = 0; i < devices.size(); i++)
		{
			devices[i]->Clock();
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			mapped_devices[i]->Clock();
		}
		core->Clock();
		cur_cycle++;
	}

	word Computer::Read(word addr)
	{
		word value = 0;
		for (size_t i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			value |= mappings[i]->Read(addr - start);
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			const word start = mapped_devices[i]->GetAddress();

			if (addr < start || addr >= start + mapped_devices[i]->GetRange()) continue;

			value |= mapped_devices[i]->Read(addr - start);
		}
		return value;
	}

	byte Computer::ReadByte(word addr)
	{
		byte value = 0;
		for (size_t i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			value |= mappings[i]->ReadByte(addr - start);
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			const word start = mapped_devices[i]->GetAddress();

			if (addr < start || addr >= start + mapped_devices[i]->GetRange()) continue;

			value |= mapped_devices[i]->ReadByte(addr - start);
		}
		return value;
	}

	void Computer::Write(word addr, word value)
	{
		for (size_t i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			mappings[i]->Write(addr - start, value);
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			const word start = mapped_devices[i]->GetAddress();

			if (addr < start || addr >= start + mapped_devices[i]->GetRange()) continue;

			mapped_devices[i]->Write(addr - start, value);
		}
	}

	void Computer::WriteByte(word addr, byte value)
	{
		for (size_t i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			mappings[i]->WriteByte(addr - start, value);
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			const word start = mapped_devices[i]->GetAddress();

			if (addr < start || addr >= start + mapped_devices[i]->GetRange()) continue;

			mapped_devices[i]->WriteByte(addr - start, value);
		}
	}

	void Computer::WriteForced(word addr, word value)
	{
		for (size_t i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			mappings[i]->WriteForced(addr - start, value);
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			const word start = mapped_devices[i]->GetAddress();

			if (addr < start || addr >= start + mapped_devices[i]->GetRange()) continue;

			mapped_devices[i]->WriteForced(addr - start, value);
		}
	}

	void Computer::WriteByteForced(word addr, byte value)
	{
		for (size_t i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			mappings[i]->WriteByteForced(addr - start, value);
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			const word start = mapped_devices[i]->GetAddress();

			if (addr < start || addr >= start + mapped_devices[i]->GetRange()) continue;

			mapped_devices[i]->WriteByteForced(addr - start, value);
		}
	}

	void Computer::SoftReset()
	{
		core->SetPC(start_PC);
		core->SetSP(start_SP);
	}

	void Computer::HardReset()
	{
		for (size_t i = 0; i < devices.size(); i++)
		{
			devices[i]->Reset();
		}
		for (size_t i = 0; i < mapped_devices.size(); i++)
		{
			mapped_devices[i]->Reset();
		}
		core->Reset();
		SoftReset();
	}

	void Computer::AddDevice(IDevice& dev)
	{
		devices.push_back(&dev);
	}

	void Computer::AddMapping(IMemoryMapped& map)
	{
		mappings.push_back(&map);
	}

	void Computer::AddMappedDevice(IMappedDevice& dev)
	{
		mapped_devices.push_back(&dev);
	}
}