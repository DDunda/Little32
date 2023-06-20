#pragma once
#include "Core.h"
#include "Computer.h"

Core::Core(Computer& computer) : computer(computer) {}

bool Core::Interrupt(word address) {
	if (interrupt) return false;
	interrupt_addr = address;
	interrupt = true;
	return true;
}