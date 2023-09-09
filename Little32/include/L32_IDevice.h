#ifndef L32_IDevice_h_
#define L32_IDevice_h_
#pragma once

namespace Little32
{
	struct IDevice
	{
		/// <summary> Clocks the device</summary>
		virtual void Clock() {}

		/// <summary> Reset the state of the device</summary>
		virtual void Reset() {};
	};
}

#endif