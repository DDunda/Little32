#pragma once

#ifndef SR_Device_h_
#define SR_Device_h_

namespace SimpleRISC {
	class Device {
	public:
		/// <summary> Clocks the device</summary>
		virtual void Clock() {}

		/// <summary> Reset the state of the device</summary>
		virtual void Reset() {};
	};
}

#endif