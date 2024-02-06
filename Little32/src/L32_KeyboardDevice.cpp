#include "L32_KeyboardDevice.h"

#include "L32_Computer.h"
#include "L32_ICore.h"
#include "L32_IDeviceSettings.h"
#include "L32_IMappedDevice.h"

#include <events.hpp>

namespace Little32
{
	KeyboardDevice::KeyboardDevice(Computer& computer, word address)
		: computer(computer), address_start(address)
	{
		SDL::Input::RegisterEventType(SDL::Event::Type::KEYUP, *this);
		SDL::Input::RegisterEventType(SDL::Event::Type::KEYDOWN, *this);
	}

	void KeyboardDeviceFactory::CreateFromSettings(Computer& computer, word& start_address, const IDeviceSettings& settings, std::unordered_map<std::string, word>& labels, std::filesystem::path path) const
	{
		KeyboardDevice* device = new KeyboardDevice(computer, start_address);
		start_address += device->GetRange();
		computer.AddMappedDevice(*device);
	}

	void KeyboardDeviceFactory::VerifySettings(const IDeviceSettings& settings, std::filesystem::path path) const
	{
		if (!settings.named_labels.empty())
		{
			throw std::runtime_error("Unknown named label: '" + settings.named_labels.begin()->first + "'");
		}
	}

	void KeyboardDevice::PushKeyDown(word key)
	{
		if (keydown_interrupt == 0) return;

		if (down_count < BUFFER_SIZE) ++down_count;

		++down_head;
		down_head %= BUFFER_SIZE;

		keys_down[down_head] = key;

		computer.core->Interrupt(keydown_interrupt);
	}

	void KeyboardDevice::PushKeyUp(word key)
	{
		if (keyup_interrupt == 0) return;

		if (up_count < BUFFER_SIZE) ++up_count;

		++up_head;
		up_head %= BUFFER_SIZE;

		keys_up[up_head] = key;

		computer.core->Interrupt(keyup_interrupt);
	}

	word KeyboardDevice::PopKeyDown()
	{
		word val = keys_down[down_head];
		down_head += BUFFER_SIZE - 1;
		down_head %= BUFFER_SIZE;
		return val;
	}

	word KeyboardDevice::PopKeyUp()
	{
		word val = keys_up[up_head];
		up_head += BUFFER_SIZE - 1;
		up_head %= BUFFER_SIZE;
		return val;
	}

	void KeyboardDevice::Write(word address, word value)
	{
		if (address == 0)
		{
			keydown_interrupt = value;
		}
		else if (address == sizeof(word))
		{
			keyup_interrupt = value;
		}
	}

	void KeyboardDevice::WriteByte(word address, byte value)
	{
		word x = (address % sizeof(word)) * 8;

		if (address < sizeof(word))
		{
			value ^= keydown_interrupt >> x;
			keydown_interrupt ^= value << x;
		}
		else if (address < 2 * sizeof(word))
		{
			value ^= keyup_interrupt >> x;
			keyup_interrupt ^= value << x;
		}
	}

	void KeyboardDevice::WriteForced(word address, word value)
	{
		Write(address, value);
	}

	void KeyboardDevice::WriteByteForced(word address, byte value)
	{
		WriteByte(address, value);
	}

	word KeyboardDevice::Read(word address)
	{
		if (address % sizeof(word) != 0) return 0;

		address /= sizeof(word);

		switch (address)
		{
		case 0: return keydown_interrupt;
		case 1: return keyup_interrupt;
		case 2: return PopKeyDown();
		case 3: return PopKeyUp();
		case 4: return down_count;
		case 5: return up_count;
		default: return 0;
		}
	}

	byte KeyboardDevice::ReadByte(word address)
	{
		if (address >= 6 * sizeof(word)) return 0;

		word x = (address % sizeof(word)) * 8;
		
		return Read(address & ~3) >> x;
	}

	void KeyboardDevice::Notify(const SDL::Event& input_event)
	{
		if (input_event.type == SDL::Event::Type::KEYDOWN)
		{
			PushKeyDown(input_event.key.keysym.scancode);
		}
		else if (input_event.type == SDL::Event::Type::KEYUP)
		{
			PushKeyUp(input_event.key.keysym.scancode);
		}
	}
}