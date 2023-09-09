#pragma once

#ifndef L32_IMappedDevice_h_
#define L32_IMappedDevice_h_

#include "L32_IDevice.h"
#include "L32_IMemoryMapped.h"

namespace Little32
{
	struct IMappedDevice : public IDevice, public IMemoryMapped {};
}

#endif