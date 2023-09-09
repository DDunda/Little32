#ifndef L32_KeyboardDevice_h_
#define L32_KeyboardDevice_h_
#pragma once

#include <input.hpp>

#include "L32_IMappedDevice.h"

namespace Little32
{
	struct Computer;

	struct KeyboardDevice : public IMappedDevice, public SDL::InputObserver
	{
		static constexpr word BUFFER_SIZE = 256;

		/// <summary> The start address of this RAM </summary>
		word address_start = 0;
		
		word keydown_interrupt = 0;
		word keyup_interrupt = 0;

		word down_head = BUFFER_SIZE - 1;
		word up_head = BUFFER_SIZE - 1;

		word down_count = 0;
		word up_count = 0;

		word keys_down[BUFFER_SIZE] = {};
		word keys_up[BUFFER_SIZE] = {};

		Computer& computer;

		KeyboardDevice(Computer& computer, word address);

		void PushKeyDown(word key);
		void PushKeyUp(word key);

		word PopKeyDown();
		word PopKeyUp();

		void Write(word address, word value);
		void WriteByte(word address, byte value);

		word Read(word address);
		byte ReadByte(word address);

		inline word GetAddress() const { return address_start; }
		inline word GetRange() const { return 6 * sizeof(word); }

		constexpr const Device_ID GetID() const { return Device_ID::Keyboard; }

		void Notify(const SDL::Event& input_event);

		inline void Reset()
		{
			down_count = 0;
			down_head = BUFFER_SIZE - 1;

			up_count = 0;
			up_head = BUFFER_SIZE - 1;

			keydown_interrupt = 0;
			keyup_interrupt = 0;
		}
	};
}

#endif