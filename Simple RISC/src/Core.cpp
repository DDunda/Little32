#pragma once
#include "Computer.h"
#include "Core.h"

namespace SimpleRISC {
	Core::Core(Computer& computer) : computer(computer) {}
}