#pragma once
#include "Device.h"
#include "MemoryMapped.h"

namespace SimpleRISC {
	class MappedDevice : public Device, public MemoryMapped {};
}