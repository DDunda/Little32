#include <vector>
#include "Core.h"
#include "Computer.h"
#include "SR_String.h"

namespace SimpleRISC {
	std::string ToBinary(word value, byte pad) {
		std::string str = "";
		for (; pad > 0; pad--) {
			str += "0";
		}
		for (int i = str.length(); i > 0 && value; value >>= 1, i--) {
			if (value & 1) str[i - 1] = '1';
		}
		for (; value; value >>= 1) {
			if (value & 1) str = "1" + str;
			else str = "0" + str;
		}
		return str;
	}

	std::string RegListToString(word list) {
		using namespace std;

		list &= 0x0000FFFF;

		if (list == 0) return "{}";

		vector<word> regs;
		for (word i = 0; list; i++, list >>= 1) {
			if (list & 1) regs.push_back(i);
		}

		word rsize = regs.size();
		string str;

		for (word i = 0;;) {
			word start = regs[i];
			word end = start;

			str += regNames[start];

			if ((i + 2 < rsize) && regs[i + 2] == end + 2) {
				end += 2;
				i += 2;

				while ((i + 1 < rsize) && (regs[i + 1] == end + 1)) {
					end++;
					i++;
				}

				str += "-" + regNames[end];
			}

			if (++i == rsize) return "{" + str + "}";

			str += ", ";
		}
	}

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

	bool IsChar(const std::string& str, const char c, size_t off)
	{
		return str.find_first_not_of(c, off) == std::string::npos;
	}

	bool IsChars(const std::string& str, const char* const chars, size_t off)
	{
		return chars != nullptr && str.find_first_not_of(chars, off) == std::string::npos;
	}

	bool IsNumeric(const std::string& str)
	{
		for (auto c : str)
		{
			if (c < '0' || c > '9') return false;
		}

		return true;
	}
}