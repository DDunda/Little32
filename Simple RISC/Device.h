#pragma once

class Device {
public:
	/// <summary> Clocks the device</summary>
	virtual void Clock() {}

	/// <summary> Reset the state of the device</summary>
	virtual void Reset() {};
};
