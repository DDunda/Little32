#pragma once

#ifndef SR_String_h_
#define SR_String_h_

#include <string>
#include <unordered_map>
#include "SR_Types.h"

namespace SimpleRISC {
	class Computer;

	// The readable names of registers by index
	static const std::string regNames[16] { "R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","R12","SP","LR","PC" };
	// The readable names of conditions by index
	static const std::string condNamesRegular[16]{ "AL","GT","GE","HI","HS","EQ","MI","VS","VC","PL","NE","LO","LS","LT","LE","NV" };
	// The variant names of conditions by index
	static const std::string condNamesVariants[16]{ "","","","","CS","ZS","NS","","","NC","ZC","CC","","","","" };
	// All condition names
	static const std::unordered_map<std::string, byte> condNames{
		{"AL",0b0000},
		{"GT",0b0001},
		{"GE",0b0010},
		{"HI",0b0011},
		{"CS",0b0100},{"HS",0b0100},
		{"ZS",0b0101},{"EQ",0b0101},
		{"NS",0b0110},{"MI",0b0110},
		{"VS",0b0111},
		{"VC",0b1000},
		{"NC",0b1001},{"PL",0b1001},
		{"ZC",0b1010},{"NE",0b1010},
		{"CC",0b1011},{"LO",0b1011},
		{"LS",0b1100},
		{"LT",0b1101},
		{"LE",0b1110},
		{"NV",0b1111}
	};

	// Converts a word to its binary representation to an optionally specified number of bits
	std::string ToBinary(word value, byte pad = 32);

	// Converts a list of registers encoded in bits into a human readable format (e.g. {R0-R7,R9,LR})
	std::string RegListToString(word list);

	void DisassembleMemory(Computer& computer, word start, word end, word offset = 0, bool print_NOP = false);

	void PrintMemory(Computer& computer, word start, word end, word offset = 0, bool print_null = false);

	bool IsChar(const std::string& str, const char c, size_t off = 0);

	bool IsChars(const std::string& str, const char* const chars, size_t off = 0);

	bool IsNumeric(const std::string& str);
}

#endif