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
			for (int i = 0; i < devices.size(); i++)
			{
				devices[i]->Clock();
			}
			core->Clock();
		}
	}

	void Computer::Clock()
	{
		for (int i = 0; i < devices.size(); i++)
		{
			devices[i]->Clock();
		}
		core->Clock();
	}

	word Computer::Read(word addr)
	{
		word value = 0;
		for (int i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			value |= mappings[i]->Read(addr - start);
		}
		return value;
	}

	byte Computer::ReadByte(word addr)
	{
		byte value = 0;
		for (int i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			value |= mappings[i]->ReadByte(addr - start);
		}
		return value;
	}

	void Computer::Write(word addr, word value)
	{
		for (int i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			mappings[i]->Write(addr - start, value);
		}
	}

	void Computer::WriteByte(word addr, byte value)
	{
		for (int i = 0; i < mappings.size(); i++)
		{
			const word start = mappings[i]->GetAddress();

			if (addr < start || addr >= start + mappings[i]->GetRange()) continue;

			mappings[i]->WriteByte(addr - start, value);
		}
	}

	void Computer::SoftReset()
	{
		core->SetPC(start_PC);
		core->SetSP(start_SP);
	}

	void Computer::HardReset()
	{
		for (int i = 0; i < devices.size(); i++)
		{
			devices[i]->Reset();
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
		devices.push_back(&dev);
		mappings.push_back(&dev);
	}
}