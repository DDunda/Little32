#pragma once

#ifndef SR_NullDevice_h_
#define SR_NullDevice_h_

#include "MemoryMapped.h"

namespace SimpleRISC {
	class NullDevice : public MemoryMapped {
		word GetAddress() const { return 0; }
		word GetRange() const { return 0; }
		const Device_ID GetID() const { return Device_ID::Null; }
	};
}

#endif