#pragma once

#ifndef SR_MappedDevice_h_
#define SR_MappedDevice_h_

#include "Device.h"
#include "MemoryMapped.h"

namespace SimpleRISC {
	class MappedDevice : public Device, public MemoryMapped {};
}

#endif