#pragma once
#include "Device.h"
#include "MemoryMapped.h"

class MappedDevice : public Device, public MemoryMapped {};