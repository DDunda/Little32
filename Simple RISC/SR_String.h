#pragma once

#include <string>
#include "SR_Types.h"

namespace SimpleRISC {
	class Computer;

	// The readable names of registers by index
	static const std::string regNames[16] { "R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","R12","SP","LR","PC" };
	// The readable names of conditions by index
	static const std::string condNames[16] { "","GT","GE","HI","HS","EQ","MI","VS","VC","PL","NE","LO","LS","LT","LE","" };

	// Converts a word to its binary representation to an optionally specified number of bits
	std::string ToBinary(word value, byte pad = 32);

	// Converts a list of registers encoded in bits into a human readable format (e.g. {R0-R7,R9,LR})
	std::string RegListToString(word list);

	void DisassembleMemory(Computer& computer, word start, word end, word offset = 0, bool tolerateNOP = false);

	void PrintMemory(Computer& computer, word start, word end, word offset = 0);
}