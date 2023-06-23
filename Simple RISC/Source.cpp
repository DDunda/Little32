#include<SDL.hpp>
#include<iostream>
#include<bit>

#include "Computer.h"
#include "Core.h"
#include "Device.h"
#include "MemoryMapped.h"
#include "MappedDevice.h"
#include "RAM.h"
#include "ROM.h"
#include "ComputerInfo.h"
#include "CharDisplay.h"
#include "PreProcAssember.h"

using namespace SDL;

std::string to_binary(word value, byte pad = 32) {
	std::string str = "";
	for (; pad > 0; pad--) {
		str += "0";
	}
	for (int i = str.length() - 1; i > 0 && value; value >>= 1, i--) {
		if (value & 1) str[i] = '1';
	}
	for (; value; value >>= 1) {
		if (value & 1) str = "1" + str;
		else str = "0" + str;
	}
	return "0b" + str;
}

class DebugCore : public Core {
public:
	word registers[16]{0};
	byte status;

	DebugCore(Computer& computer) : Core(computer) {}

	void Clock() {
		if (interrupt) {
			registers[SP] -= 4;
			computer.Write(registers[SP], status);
			registers[SP] -= 4;
			computer.Write(registers[SP], registers[PC]);

			status = 0;
			registers[PC] = interrupt_addr;

			interrupt = false;

			return;
		}

		word addr = computer.Read(7 * 4);

		std::string gradient = "   \xb0\xb0\xb0\xb1\xb1\xb1\xb1\xb2\xb2\xb2\xdb\xdb\xdb";

		std::string test = "...\xc9\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbb...\
...\xba        \xba...\
...\xba Hello, \xba...\
...\xba World! \xba...\
...\xba        \xba...\
...\xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc...";

		for (word y = 0; y < 16; y++) {
			word w = (byte)gradient[y];
			w |= w << 8;
			w |= w << 16;
			computer.Write(addr + y * 16, w);
			computer.Write(addr + y * 16 + 4, w);
			computer.Write(addr + y * 16 + 8, w);
			computer.Write(addr + y * 16 + 12, w);
		}

		for (word i = 0; i < test.length(); i++) {
			if (test[i] != '.') {
				computer.WriteByte(addr + i + 16 * 4, test[i]);
			}
		}
		return;
	}

	void Reset() {
		memset(registers, 0, sizeof(registers));
		interrupt = false;
		interrupt_addr = 0;
	}

	void SetPC(word value) { registers[PC] = value; }
	void SetSP(word value) { registers[SP] = value; }
};

// The readable names of registers by index
static const std::string regNames[16] { "R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","R12","SP","LR","PC" };
// The readable names of conditions by index
static const std::string condNames[16] { "","GT","GE","HI","HS","EQ","MI","VS","VC","PL","NE","LO","LS","LT","LE","" };

// Converts a list of registers encoded in bits into a human readable format (e.g. {R0-R7,R9,LR})
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

class RISCCore : public Core {
public:

	word registers[16]{ 0 };

	bool N = false, Z = false, C = false, V = false; // Status flags

	int32_t inv;
	uint32_t neg;
	word shift;
	word reg1;
	bool set_status;
	bool immediate;

	typedef void (*opFunc)(RISCCore& core, word instruction);
	typedef std::string(*opDisassemble) (
		word instruction,
		const std::string& sign,
		bool n,
		word sh,
		const std::string& nstr,
		const std::string& sstr,
		const std::string& shstr,
		const std::string& r1,
		const std::string& r2,
		const std::string& r3,
		word im8,
		word im12
	);

	struct Opcode {
		const opFunc func[2];        // Used to perform instructions (regular/immediate mode)
		const opDisassemble diss[2]; // Used to disassemble instructions (regular/immediate mode)
	};

	static const Opcode opcodes[16]; // Lookup table for the arithmetic/logic instructions (indexed by opcode)

	RISCCore(Computer& computer) : Core(computer) {}

	void Clock() {
		word instruction = computer.Read(registers[PC]);

		bool c = (instruction >> 28) & 0xF; // The condition is stored in the upper 4 bits

		switch (c)
		{
			case AL: break;                             // Always
			case GT: if ((N == V) && !Z) break; return; // >
			case GE: if (N == V) break; return;         // >=
			case HI: if (C && !Z) break; return;        // > (unsigned)
			case CS: if (C) break; return;              // >= (unsigned) / Carry set
			case ZS: if (Z) break; return;              // == / Zero set
			case NS: if (N) break; return;              // < 0 / Negative set
			case VS: if (V) break; return;              // oVerflow set
			case VC: if (!V) break; return;             // oVerflow not set
			case NC: if (!N) break; return;             // >= 0 / Negative not set
			case ZC: if (!Z) break; return;             // != / Zero not set
			case CC: if (!C) break; return;             // < (unsigned) / Carry not set
			case LS: if (!C || Z) break; return;        // <= (unsigned)
			case LT: if (N != V) break; return;         // <
			case LE: if ((N != V) || Z) break; return;  // <=
			default: return;                            // Never
		}

		immediate = (instruction >> 20) & 1;  // Causes the last operand to be a constant value in most cases
		inv = 1 -  ((instruction >> 26) & 2); // Used for inverting values with *
		neg = 1 + ~((instruction >> 27) & 1); // Used for inverting values with ^
		shift = (instruction & 0xF) << 1;     // The lower 4 bits are used as a barrel shift for flexible operands
		reg1 = (instruction >> 16) & 0xF;     // Reg1 is always in the same place (not used for branches or RFE)

		if (instruction & 0x04000000) { // Arithmetic
			word opcode = (instruction >> 22) & 0xF;
			set_status = (instruction >> 21) & 1;
			opcodes[opcode].func[immediate](*this, instruction);
		}
		else if(instruction & 0x02000000) { // B
			if (instruction & 0x01000000) registers[LR] = registers[PC]; // BL
			word diff = (instruction & 0x00FFFFFF) * sizeof(word);
			registers[PC] += diff * inv;
			return; // Don't change the PC again
		}
		else if (instruction & 0x01000000) {
			if (instruction & 0x00800000) {
				word addr = registers[(instruction >> 12) & 0xF];
				word off = std::rotl(immediate ? (instruction >> 4) & 0xFF : registers[(instruction >> 8) & 0xF], shift);
				off *= inv;
				addr += off;

				if (instruction & 0x00400000) { // Register <-> Memory (Byte)
					if (instruction & 0x00200000) { // RWB
						computer.WriteByte(addr, registers[reg1]);
					}
					else { // RRB
						registers[reg1] = computer.ReadByte(addr);
					}
				}
				else { // Register <-> Memory (Word)
					if (instruction & 0x00200000) { // RWW
						computer.Write(addr, registers[reg1]);
					}
					else { // RRW
						registers[reg1] = computer.Read(addr);
					}
				}
			}
			else if (instruction & 0x00400000) {
				word list = instruction & 0x0000FFFF;
				if (instruction & 0x00200000) { // SWR
					for (word i = 15; list & 0xFFFF; i--, list <<= 1) {
						if (list & 0x80) push(registers[reg1], registers[i] * inv);
					}
				}
				else { // SRR
					for (word i = 0; list; i++, list >>= 1) {
						if (list & 1) registers[i] = pop(registers[reg1]) * inv;
					}
				}
			}
			else if (instruction & 0x002000000) { // MVM
				word list = instruction & 0x0000FFFF;
				word v = registers[reg1] * inv;
				for (word i = 15; list & 0xFFFF; i--, list <<= 1) {
					if (list & 0x80) registers[i] = v;
				}
			}
			else { // SWP
				word reg2 = (instruction >> 12) & 0xF;
				word tmp = registers[reg1];
				registers[reg1] = std::rotl(registers[reg2], shift) * inv;
				registers[reg2] = tmp * inv;
			}
		}
		else if (instruction & 0x008000000) { // RFE
			registers[PC] = pop(registers[SP]);
			word status = pop(registers[SP]);
			N = (status & N_flag) == N_flag;
			Z = (status & Z_flag) == Z_flag;
			C = (status & C_flag) == C_flag;
			V = (status & V_flag) == V_flag;
		}
		// Else NOP

		registers[PC] += sizeof(word); // Moves to the next word
	}

	const std::string Disassemble(word instruction) const {
		using namespace std;
		const string cond = condNames[(instruction >> 28) & 0xF];

		bool i = (instruction >> 20) & 1;

		bool n = (instruction >> 27) & 1;
		string nstr = n ? "N" : "";
		word sh = (instruction & 0xF) << 1;
		string shstr = sh == 0 ? "" : ("<<" + std::to_string(sh));
		string r1 = regNames[(instruction >> 16) & 0xF];
		string r2 = regNames[(instruction >> 12) & 0xF];
		string r3 = regNames[(instruction >> 8) & 0xF];
		word im8 = rotl((instruction >> 4) & 0xFF,sh);
		word im12 = rotl((instruction >> 4) & 0xFFF,sh);
		string sign = n ? "-" : "+";

		if (instruction & 0x04000000) { // Arithmetic
			word opcode = (instruction >> 22) & 0xF;
			bool s = (instruction >> 21) & 1;
			string sstr = s ? "S" : "";
			return opcodes[opcode].diss[i](
				instruction,
				sign,
				n,
				sh,
				nstr,
				sstr,
				shstr,
				r1,
				r2,
				r3,
				im8,
				im12
			);
		}

		word im16 = instruction & 0xFFFF;
		word im24 = (instruction & 0xFFFFFF) * sizeof(word);

		if (instruction & 0x02000000) { // B
			string diff = to_string(im24);
			if (instruction & 0x01000000) return "BL" + cond + " ." + sign + diff; // BL
			else                          return "B"  + cond + " ." + sign + diff;
		}
		else if (instruction & 0x01000000) {
			if (instruction & 0x00800000) { // Register <-> memory
				string diff;
				if (i) {
					diff = (im8 == 0 ? (sh == 0 ? "" : "0") : (sign + to_string(im8)));
				}
				else {
					diff = sign + r3 + shstr;
				}
				string addr = (r2 == "PC" ? "." : r2) + diff;

				static const string regMoveNames[4] { "RRW", "RWW", "RRB", "RWB" };

				string name = regMoveNames[(instruction >> 21) & 0b11];

				return nstr + name + cond + " " + r1 + ", [" + addr + "]";
			}
			else if (instruction & 0x00400000) { // Register list <-> stack
				return nstr + ((instruction & 0x00200000) ? "SWR " : "SRR ") + r1 + ", " + RegListToString(im16) + shstr;
			}
			else if (instruction & 0x002000000) { // MVM
				return nstr + "MVM" + cond + " " + r1 + ", " + RegListToString(im16) + shstr;
			}
			else { // SWP	
				return nstr + "SWP" + cond + " " + r1 + ", " + r2 + shstr;
			}
		}
		else if (instruction & 0x008000000) { // RFE
			return "RFE";
		}
		else {
			return "NOP";
		}
	}

	void Reset() {
		memset(registers, 0, sizeof(registers));
		interrupt = false;
		interrupt_addr = 0;
		N = Z = C = V = false;
	}

	bool Interrupt(word address) {
		word status = (N * N_flag) | (Z * Z_flag) | (C * C_flag) | (V * V_flag);
		push(registers[SP], status);
		push(registers[SP], registers[PC]);
		registers[PC] = address;
		N = Z = C = V = false;
	}

	void push(word& ptr, word val) {
		ptr -= sizeof(word);
		computer.Write(ptr, val);
	}

	word pop(word& ptr) {
		ptr += sizeof(word);
		return computer.Read(ptr - sizeof(word));
	}

	void SetPC(word value) { registers[PC] = value; }
	void SetSP(word value) { registers[SP] = value; }
};

#define doshift(a) std::rotl(a,core.shift)

#define op [](RISCCore& core, word instruction)
#define dop [](word instruction,const std::string& sign,bool n,word sh,const std::string& nstr,const std::string& sstr,const std::string& shstr,const std::string& r1,const std::string& r2,const std::string& r3,word im8,word im12)->std::string

#define op3i word &reg1=core.registers[core.reg1], &reg2=core.registers[(instruction >> 12)&0xF],imm=doshift((instruction >> 4) & 0xFF)
#define op3r word &reg1=core.registers[core.reg1], &reg2=core.registers[(instruction >> 12)&0xF],&reg3=core.registers[(instruction >> 8)&0xF]

#define op2i word &reg =core.registers[core.reg1],   imm=doshift((instruction >> 4) & 0xFFF)
#define op2r word &reg1=core.registers[core.reg1], &reg2=core.registers[(instruction >> 12)&0xF]

const RISCCore::Opcode RISCCore::opcodes[16] = {
	{ // ADD
		{
			op { op3r;
				uint64_t val = ((int64_t)reg2 + (int64_t)doshift(reg3)) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			},
			op { op3i;
				uint64_t val = ((int64_t)reg2 + (int64_t)imm) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			}
		},
		{
			dop {
				return nstr + "ADD" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "ADD" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 0000 ADD          Add
	{ // SUB
		{
			op { op3r;
				uint64_t val = ((int64_t)reg2 - (int64_t)doshift(reg3)) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			},
			op { op3i;
				uint64_t val = ((int64_t)reg2 - (int64_t)imm) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			}
		},
		{
			dop {
				return nstr + "SUB" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "SUB" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 0001 SUB          Sub
	{ // ADC
		{
			op { op3r;
				int64_t c = core.C;
				c *= 1 - (int64_t)(core.N ^ core.V) * 2;
				uint64_t val = ((int64_t)reg2 + (int64_t)doshift(reg3) + c) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			},
			op { op3i;
				int64_t c = core.C;
				c *= 1 - (int64_t)(core.N ^ core.V) * 2;
				uint64_t val = ((int64_t)reg2 + (int64_t)imm + c) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			}
		},
		{
			dop {
				return nstr + "ADC" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "ADC" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 0010 ADC          Add with carry
	{ // SBB
		{
			op { op3r;
				int64_t c = core.C;
				c *= 1 - (int64_t)(core.N ^ core.V) * 2;
				uint64_t val = ((int64_t)reg2 - (int64_t)doshift(reg3) + c) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			},
			op { op3i;
				int64_t c = core.C;
				c *= 1 - (int64_t)(core.N ^ core.V) * 2;
				uint64_t val = ((int64_t)reg2 - (int64_t)imm + c) * core.inv;
				reg1 = val;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = val > reg1;
				core.V = (val >> 63) ^ (reg1 >> 31);
			}
		},
		{
			dop {
				return nstr + "SBB" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "SBB" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 0011 SBB          Sub with borrow
	{ // ASL
		{
			op { op3r;
				reg1 = (reg2 << doshift(reg3)) * core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) >> doshift(reg3)) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			},
			op { op3i;
				reg1 = (reg2 << imm) * core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) >> imm) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			}
		},
		{
			dop {
				return nstr + "ASL" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "ASL" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 0100 ASL          Arithmetic shift left
	{ // ASR
		{
			op { op3r;
				reg1 = ((reg2 >> doshift(reg3)) | ~(~((word)0) >> doshift(reg3))) * core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) << doshift(reg3)) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			},
			op { op3i;
				reg1 = ((reg2 >> imm) | ~(~((word)0) << imm)) * core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) << imm) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			}
		},
		{
			dop {
				return nstr + "ASR" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "ASR" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 0101 ASR          Arithmetic shift right
	{ // CMP
		{
			op { op3r;
				uint64_t val = ((int64_t)reg2 - (int64_t)doshift(reg3)) * core.inv;
				uint32_t v2 = val;

				core.N = v2 >> 31;
				core.Z = v2 == 0;
				core.C = val > v2;
				core.V = (val >> 63) ^ (v2 >> 31);
			},
			op { op2i;
				uint64_t val = ((int64_t)reg - (int64_t)imm) * core.inv;
				uint32_t v2 = val;

				core.N = v2 >> 31;
				core.Z = v2 == 0;
				core.C = val > v2;
				core.V = (val >> 63) ^ (v2 >> 31);
			}
		},
		{
			dop {
				return nstr + "CMP" + sstr + " " + r1 + ", " + r2 + shstr;
			},
			dop {
				return nstr + "CMP" + sstr + " " + r1 + ", " + std::to_string(im12);
			}
		}
	}, // 0110 CMP          Compare two values with -
	{ // CMN
		{
			op { op3r;
				uint64_t val = ((int64_t)reg2 + (int64_t)doshift(reg3)) * core.inv;
				uint32_t v2 = val;

				core.N = v2 >> 31;
				core.Z = v2 == 0;
				core.C = val > v2;
				core.V = (val >> 63) ^ (v2 >> 31);
			},
			op { op2i;
				uint64_t val = ((int64_t)reg + (int64_t)imm) * core.inv;
				uint32_t v2 = val;

				core.N = v2 >> 31;
				core.Z = v2 == 0;
				core.C = val > v2;
				core.V = (val >> 63) ^ (v2 >> 31);
			}
		},
		{
			dop {
				return nstr + "CMN" + sstr + " " + r1 + ", " + r2 + shstr;
			},
			dop {
				return nstr + "CMN" + sstr + " " + r1 + ", " + std::to_string(im12);
			}
		}
	}, // 0111 CMN          Compare two values with +
	{ // ORR
		{
			op { op3r;
				reg1 = (reg2 | reg3) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = core.V = false;
			},
			op { op3i;
				reg1 = (reg2 | imm) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = core.V = false;
			}
		},
		{
			dop {
				return (n ? "NOR" : "ORR") + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return (n ? "NOR" : "ORR") + sstr + " " + r1 + ", " + r2 + ", " + to_binary(im8,0);
			}
		}
	}, // 1000 ORR          A | B
	{ // AND
		{
			op { op3r;
				reg1 = (reg2 & reg3) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = core.V = false;
			},
			op { op3i;
				reg1 = (reg2 & imm) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = core.V = false;
			}
		},
		{
			dop {
				return nstr + "AND" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "AND" + sstr + " " + r1 + ", " + r2 + ", " + to_binary(im8,0);
			}
		}
	}, // 1001 AND          A & B
	{ // XOR
		{
			op { op3r;
				reg1 = (reg2 ^ reg3) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = core.V = false;
			},
			op { op3i;
				reg1 = (reg2 ^ imm) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = core.V = false;
			}
		},
		{
			dop {
				return "X" + nstr + "OR" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return "X" + nstr + "OR" + sstr + " " + r1 + ", " + r2 + ", " + to_binary(im8,0);
			}
		}
	}, // 1010 XOR          A ^ B
	{ // CMX
		{
			op { op3r;
				uint64_t val = (int64_t)reg2 ^ (int64_t)doshift(reg3) ^ core.inv;
				uint32_t v2 = val;

				core.N = v2 >> 31;
				core.Z = v2 == 0;
				core.C = val > v2;
				core.V = (val >> 63) ^ (v2 >> 31);
			},
			op { op2i;
				uint64_t val = (int64_t)reg ^ (int64_t)imm ^ core.inv;
				uint32_t v2 = val;

				core.N = v2 >> 31;
				core.Z = v2 == 0;
				core.C = val > v2;
				core.V = (val >> 63) ^ (v2 >> 31);
			}
		},
		{
			dop {
				return nstr + "CMX" + sstr + " " + r1 + ", " + r2 + shstr;
			},
			dop {
				return nstr + "CMX" + sstr + " " + r1 + to_binary(im12,0);
			}
		}
	}, // 1011 CMX          Compare two values with ^
	{ // LSL
		{
			op { op3r;
				reg1 = (reg2 << doshift(reg3)) ^ core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) >> doshift(reg3)) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			},
			op { op3i;
				reg1 = (reg2 << imm) ^ core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) >> imm) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			}
		},
		{
			dop {
				return nstr + "LSL" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "LSL" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 1100 LSL          Logical shift left
	{ // LSR
		{
			op { op3r;
				reg1 = (reg2 >> doshift(reg3)) ^ core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) << doshift(reg3)) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			},
			op { op3i;
				reg1 = (reg2 >> imm) ^ core.inv;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = ((reg1 * core.inv) << imm) != reg2;
				core.V = (reg1 ^ reg2) >> 31;
			}
		},
		{
			dop {
				return nstr + "LSR" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr;
			},
			dop {
				return nstr + "LSR" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8);
			}
		}
	}, // 1101 LSR          Logical shift right
	{ // MOV / MVN
		{
			op { op2r;
				word r1 = reg1;
				reg1 = doshift(reg2) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = false;
				core.V = (r1 ^ reg1) >> 31;
			},
			op { op2i;
				word r1 = reg;
				reg = imm ^ core.neg;

				if (!core.set_status) return;

				core.N = reg >> 31;
				core.Z = reg == 0;
				core.C = false;
				core.V = (r1 ^ reg) >> 31;
			}
		},
		{
			dop {
				return (n ? "MVN" : "MOV") + sstr + " " + r1 + ", " + r2 + shstr;
			},
			dop {
				return (n ? "MVN" : "MOV") + sstr + " " + r1 + ", " + std::to_string(im12);
			}
		}
	}, // 1110 MOV / MVN    Copy value to register
	{ // INV
		{
			op { op2r;
				word r1 = reg1;
				reg1 = (~reg2 + 1) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg1 >> 31;
				core.Z = reg1 == 0;
				core.C = false;
				core.V = (r1 ^ reg1) >> 31;
			},
			op { op2i;
				word r1 = reg;
				reg = (~imm + 1) ^ core.neg;

				if (!core.set_status) return;

				core.N = reg >> 31;
				core.Z = reg == 0;
				core.C = false;
				core.V = (r1 ^ reg) >> 31;
			}
		},
		{
			dop {
				return nstr + "INV" + sstr + " " + r1 + ", " + r2 + shstr;
			},
			dop {
				return nstr + "INV" + sstr + " " + r1 + ", " + std::to_string(im12);
			}
		}
	}, // 1111 INV          -A
};

int main(int argc, char* argv[]) {
	Init();
	IMG::Init(IMG_INIT_PNG);

	Input input;

	Window w;
	Renderer r;
	Point& windowSize = input.windowSize = { 512, 512 };

	CreateWindowAndRenderer(windowSize, w, r);

	Texture charset = IMG::LoadTexture(r, "Char set.png");

	Computer computer;
	RISCCore core(computer);
	ComputerInfo info(computer);

	word CHAR_MEM = ram.address_start + ram.address_size;
	word start_num = 100;

	RAM ram(512, 256);
	CharDisplay cram(r, charset, { 8,8 }, 16, {16,16}, { 4,4 }, CHAR_MEM);
	
	word pc = 0;
	
	RRWi(AL, R0, PC, start_num - pc, 1) // RRW R0, [.start_num]
	MOVi(AL, R1, CHAR_MEM >> 2, 1)      // MOV R1, CHAR_MEM
	ADDi(AL, R0, R0, 3, 0)              // ADD R0, 3
	RWWi(AL, R0, R1, 0, 0)              // RWW R0, [R1]
	B(AL, -2)                           // B .-8

	ram.memory[start_num] = 0x20202020; // start_num: #word 0x20202020
	
	/*
	BL(AL,3)
	ADDSi(AL,R0,R0,1,0)
	B(AL,-2)
	MOV(AL,PC,LR,0)
	*/

	/*
	//                   CCCCN1ccccSi   ,   .   ,   .   ,
	//                   CCCCN01L   .   ,   .   ,   .   ,
	ram.memory[pc++] = 0b00000011000000000000000000000011; // BL .+12
	ram.memory[pc++] = 0b00000100001100000000000000010000; // ADDS R0, R0, 1
	ram.memory[pc++] = 0b00001010000000000000000000000010; // B .-8
	ram.memory[pc++] = 0b00000111100011111110000000000000; // MOV PC, LR
	*/

	computer.AddMapping(info);
	computer.AddMappedDevice(ram);
	computer.AddMappedDevice(cram);

	computer.core = &core;
	computer.start_PC = ram.GetAddress();
	computer.start_SP = computer.start_PC + ram.GetRange();
	computer.SoftReset();

	// Disassemble program
	for(word addr = 0; addr < pc; addr++) {
		word v = ram.memory[addr];
		std::string instruction = core.Disassemble(v);
		printf("0x%08X: %s\n", addr * 4, instruction.c_str());
	}

	// Print values in memory
	for (word laddr = pc, addr = pc; addr < ram.address_size / 4; addr++) {
		int32_t v = ram.memory[addr];
		if (v == 0) continue;
		if (addr - laddr > 1) printf("...\n");
		printf("0x%08X: 0x%08X 0b%s % 10u %+ 11i \n", addr * 4, v, to_binary(v).c_str(), v, v);
		laddr = addr;
	}

	for (int frame = 0; input.running; frame++) {
		input.Update();
		
		//printf("PC: 0x%08X  SP: 0x%08X  LR: 0x%08X\n", core.registers[PC], core.registers[SP], core.registers[LR]);
		//printf("\n\nR0: % 10u R1: % 10u R2: % 10u R3: % 10u", core.registers[R0], core.registers[R1], core.registers[R2], core.registers[R3]);
		//printf("\n0x%08X: %s", core.registers[PC], core.Disassemble(computer.Read(core.registers[PC])).c_str());
		//printf("frame\n");
		
		computer.Clock();
		cram.Render();

		r.Present();

		Delay(16);
	}

	Quit();

	return 0;
}

#undef R0
#undef R1
#undef R2
#undef R3
#undef R4
#undef R5
#undef R6
#undef R7
#undef R8
#undef R9
#undef R10
#undef R11
#undef R12
#undef SP
#undef LR
#undef PC