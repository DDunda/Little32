#include <vector>
#include "Core.h"
#include "Computer.h"
#include "SR_String.h"

namespace SimpleRISC
{
	void DisassembleMemory(Computer& computer, word start, word end, word offset, bool print_NOP) {
		for (word laddr = start, addr = start; addr < end; addr += 4) {
			word v = computer.Read(addr + offset);
			std::string instruction = computer.core->Disassemble(v);
			if (instruction == "NOP") continue;
			if (addr - laddr > 12) printf("...\n");
			else if(print_NOP) {
				if (addr - laddr > 4) printf("0x%08X: NOP\n", laddr + 4 + offset);
				if (addr - laddr > 8) printf("0x%08X: NOP\n", laddr + 8 + offset);
			} else if(addr - laddr > 4) printf("...\n");

			printf("0x%08X: %s\n", addr + offset, instruction.c_str());
			laddr = addr;
		}
	}

	void PrintMemory(Computer& computer, word start, word end, word offset, bool print_null) {
		for (word laddr = start, addr = start; addr < end; addr += 4) {
			int32_t v = computer.Read(addr + offset);
			if (v == 0 && !print_null) continue;
			if (addr - laddr > 12) printf("...                         .   ,   .   ,   .   ,   .   ,\n");
			else {
				if (addr - laddr > 4) printf("0x%08X: 0x%08X 0b%s % 10u %+ 11i \n", laddr + 4 + offset, 0, ToBinary(0).c_str(), 0, 0);
				if (addr - laddr > 8) printf("0x%08X: 0x%08X 0b%s % 10u %+ 11i \n", laddr + 8 + offset, 0, ToBinary(0).c_str(), 0, 0);
			}
			printf("0x%08X: 0x%08X 0b%s % 10u %+ 11i \n", addr + offset, v, ToBinary(v).c_str(), v, v);
			laddr = addr;
		}
	}
}