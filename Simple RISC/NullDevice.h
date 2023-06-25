#pragma once

#include "MemoryMapped.h"

namespace SimpleRISC {
	class NullDevice : public MemoryMapped {
		word GetAddress() const { return 0; }
		word GetRange() const { return 0; }
		const Device_ID GetID() const { return Device_ID::Null; }
	};
}