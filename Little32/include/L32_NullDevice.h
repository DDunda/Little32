#pragma once

#ifndef L32_NullDevice_h_
#define L32_NullDevice_h_

#include "L32_IMemoryMapped.h"

namespace Little32
{
	struct NullDevice : public IMemoryMapped
	{
		inline word GetAddress() const { return 0; }
		inline word GetRange() const { return 0; }
		constexpr const Device_ID GetID() const { return NULL_DEVICE; }
	};
}

#endif