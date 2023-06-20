#pragma once
#include "SR_Definitions.h"

class Computer;

class Core {
protected:
	bool interrupt = false;
	word interrupt_addr = 0;

public:
	Computer& computer;

	Core(Computer& computer);

	virtual void Clock() = 0;
	virtual void Reset() = 0;
	bool Interrupt(word address);

	virtual void SetPC(word value) = 0;
	virtual void SetSP(word value) = 0;
};