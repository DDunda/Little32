#include <bit>

#include "Computer.h"
#include "RISCCore.h"
#include "SR_String.h"

namespace SimpleRISC {
	RISCCore::RISCCore(Computer& computer) : Core(computer) {}

	void RISCCore::Clock() {
		word instruction = computer.Read(registers[PC]);

		word c = (instruction >> 28) & 0xF; // The condition is stored in the upper 4 bits

		registers[PC] += sizeof(word); // Moves to the next instruction in case the condition fails
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
		registers[PC] -= sizeof(word); // Moves back to current instruction


		immediate = (instruction >> 20) & 1;  // Causes the last operand to be a constant value in most cases
		inv = 1 - ((instruction >> 26) & 2); // Used for inverting values with *
		neg = 1 + ~((instruction >> 27) & 1); // Used for inverting values with ^
		shift = (instruction & 0xF) << 1;     // The lower 4 bits are used as a barrel shift for flexible operands
		reg1 = (instruction >> 16) & 0xF;     // Reg1 is always in the same place (not used for branches or RFE)

		if (instruction & 0x04000000) { // Arithmetic
			word opcode = (instruction >> 22) & 0xF;
			set_status = (instruction >> 21) & 1;
			opcodes[opcode].func[immediate](*this, instruction);
		}
		else if (instruction & 0x02000000) { // B
			word diff = (instruction & 0x00FFFFFF) * sizeof(word);
			if (diff == 0 && neg) {
				if (instruction & 0x01000000) { // RET
					registers[PC] = registers[LR];
				}
				else { // RFE
					registers[PC] = Pop(registers[SP]);
					word status = Pop(registers[SP]);
					N = (status & N_flag) == N_flag;
					Z = (status & Z_flag) == Z_flag;
					C = (status & C_flag) == C_flag;
					V = (status & V_flag) == V_flag;
				}
			}
			else {
				if (instruction & 0x01000000) registers[LR] = registers[PC] + sizeof(word); // BL
				registers[PC] += diff * inv;
			}
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
						if (list & 0x8000) Push(registers[reg1], registers[i] * inv);
					}
				}
				else { // SRR
					for (word i = 0; list; i++, list >>= 1) {
						if (list & 1) registers[i] = Pop(registers[reg1]) * inv;
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
		else if (instruction & 0x00800000) { // FPU
			word& rdn = registers[reg1];
			word reg2 = registers[(instruction >> 12) & 0xF];
			word reg3 = registers[(instruction >> 8) & 0xF];
			word reg2s = std::rotl(reg2, shift);
			word reg3s = std::rotl(reg3, shift);
			float f;
			switch ((instruction) >> 20 & 0x7) {
			case 0: // ADDF
				f = (*(float*)(&reg2) + *(float*)(&reg3s)) * inv;
				rdn = *(word*)(&f);
				break;
			case 1: // SUBF
				f = (*(float*)(&reg2) - *(float*)(&reg3s)) * inv;
				rdn = *(word*)(&f);
				break;
			case 2: // MULF
				f = (*(float*)(&reg2) * *(float*)(&reg3s)) * inv;
				rdn = *(word*)(&f);
				break;
			case 3: // DIVF
				f = (*(float*)(&reg2) / *(float*)(&reg3s)) * inv;
				rdn = *(word*)(&f);
				break;
			case 4: // ITOF
				f = (float)(*(int32_t*)(&reg2s) * inv);
				rdn = *(word*)(&f);
				break;
			case 5: // FTOI
			{	int32_t i = (int32_t)(*(float*)(&reg2s) * inv);
				rdn = *(word*)(&i);
			}	break;
			case 6: // CMPF
			{	float a = *(float*)(&rdn);
				float b = *(float*)(&reg2s);

				float cmp = (a - b) * inv;
				N = cmp < 0;
				Z = cmp == 0;
				V = (a < 0.0) != (b < 0.0) && std::abs(b) > std::numeric_limits<double>::max() - std::abs(a);
				C = false;
			}	break;
			case 7: // CMPFI
			{	float a = *(float*)(&rdn);
				int32_t b = *(int32_t*)(&reg2s);

				float cmp = (a - b) * inv;
				N = cmp < 0;
				Z = cmp == 0;
				V = (a < 0) != (b < 0.0) && std::abs(b) > std::numeric_limits<double>::max() - std::abs(a);
				C = false;
			}	break;
			}
		}
		// Else NOP

		registers[PC] += sizeof(word); // Moves to the next word
	}

	const std::string RISCCore::Disassemble(word instruction) const {
		using namespace std;
		const string cond = condNamesRegular[(instruction >> 28) & 0xF] == "AL" ? "" : condNamesRegular[(instruction >> 28) & 0xF];
		const string cond2 = cond == "" ? "" : (" ?" + cond);

		bool i = (instruction >> 20) & 1;

		bool n = (instruction >> 27) & 1;
		string nstr = n ? "N" : "";
		word sh = (instruction & 0xF) << 1;
		string shstr = sh == 0 ? "" : ("<<" + std::to_string(sh));
		string r1 = regNames[(instruction >> 16) & 0xF];
		string r2 = regNames[(instruction >> 12) & 0xF];
		string r3 = regNames[(instruction >> 8) & 0xF];
		word im8 = rotl((instruction >> 4) & 0xFF, sh);
		word im12 = rotl((instruction >> 4) & 0xFFF, sh);
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
				cond2,
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

		if (instruction & 0x02000000) {
			if (n && (im24 == 0)) {
				if (instruction & 0x01000000) return "RET" + cond2; // RET
				else                          return "RFE" + cond2; // RFE
			}
			else {
				string diff = to_string(im24);
				if (instruction & 0x01000000) return "BL" + cond + " " + sign + diff; // BL
				else                          return "B" + cond + " " + sign + diff; // B
			}
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
				string addr = (r2 == "PC" ? "" : r2) + diff;

				static const string regMoveNames[4]{ "RRW", "RWW", "RRB", "RWB" };

				string name = regMoveNames[(instruction >> 21) & 0b11];

				return nstr + name + " " + r1 + ", [" + addr + "]" + cond2;
			}
			else if (instruction & 0x00400000) { // Register list <-> stack
				return nstr + ((instruction & 0x00200000) ? "SWR " : "SRR ") + r1 + ", " + RegListToString(im16) + cond2;
			}
			else if (instruction & 0x002000000) { // MVM
				return nstr + "MVM" + " " + r1 + ", " + RegListToString(im16) + cond2;
			}
			else { // SWP	
				return nstr + "SWP" + " " + r1 + ", " + r2 + shstr + cond2;
			}
		}
		else if (instruction & 0x00800000) { // FPU
			switch ((instruction) >> 20 & 0x7) {
			case 0: return nstr + "ADDF " + r1 + ", " + r2 + ", " + r3 + cond2;
			case 1: return nstr + "SUBF " + r1 + ", " + r2 + ", " + r3 + cond2;
			case 2: return nstr + "MULF " + r1 + ", " + r2 + ", " + r3 + cond2;
			case 3: return nstr + "DIVF " + r1 + ", " + r2 + ", " + r3 + cond2;
			case 4: return nstr + "ITOF " + r1 + ", " + r2 + cond2;
			case 5: return nstr + "FTOI " + r1 + ", " + r2 + cond2;
			case 6: return nstr + "CMPF " + r1 + ", " + r2 + cond2;
			case 7: return nstr + "CMPFI " + r1 + ", " + r2 + cond2;
			}
		}

		return "";
	}

	void RISCCore::Reset() {
		memset(registers, 0, sizeof(registers));
		N = Z = C = V = false;
	}

	void RISCCore::Interrupt(word address) {
		word status = (N * N_flag) | (Z * Z_flag) | (C * C_flag) | (V * V_flag);
		Push(registers[SP], status);
		Push(registers[SP], registers[PC]);
		registers[PC] = address;
		N = Z = C = V = false;
	}

	void RISCCore::Push(word& ptr, word val) {
		ptr -= sizeof(word);
		computer.Write(ptr, val);
	}

	word RISCCore::Pop(word& ptr) {
		ptr += sizeof(word);
		return computer.Read(ptr - sizeof(word));
	}

	void RISCCore::SetPC(word value) { registers[PC] = value; }
	void RISCCore::SetSP(word value) { registers[SP] = value; }

#define doshift(a) std::rotl(a,core.shift)

#define op [](RISCCore& core, word instruction)
#define dop [](word instruction,const std::string& sign,bool n,word sh,const std::string& cond,const std::string& nstr,const std::string& sstr,const std::string& shstr,const std::string& r1,const std::string& r2,const std::string& r3,word im8,word im12)->std::string

#define op3i word &reg1=core.registers[core.reg1], &reg2=core.registers[(instruction >> 12)&0xF],imm=doshift((instruction >> 4) & 0xFF)
#define op3r word &reg1=core.registers[core.reg1], &reg2=core.registers[(instruction >> 12)&0xF],&reg3=core.registers[(instruction >> 8)&0xF]

#define op2i word &reg =core.registers[core.reg1],   imm=doshift((instruction >> 4) & 0xFFF)
#define op2r word &reg1=core.registers[core.reg1], &reg2=core.registers[(instruction >> 12)&0xF]

	const RISCCore::Opcode RISCCore::opcodes[16] = {
		{ // ADD
			{
				op { op3r;
					uint64_t val = ((int64_t)(int32_t)reg2 + (int64_t)(int32_t)doshift(reg3)) * core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				},
				op { op3i;
					uint64_t val = ((int64_t)(int32_t)reg2 + (int64_t)(int32_t)imm) * core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				}
			},
			{
				dop {
					return nstr + "ADD" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "ADD" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
				}
			}
		}, // 0000 ADD          Add
		{ // SUB
			{
				op { op3r;
					uint64_t val = ((int64_t)(int32_t)reg2 - (int64_t)(int32_t)doshift(reg3)) * (int64_t)core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				},
				op { op3i;
					uint64_t val = ((int64_t)(int32_t)reg2 - (int64_t)(int32_t)imm) * (int64_t)core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				}
			},
			{
				dop {
					return nstr + "SUB" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "SUB" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
				}
			}
		}, // 0001 SUB          Sub
		{ // ADC
			{
				op { op3r;
					int64_t c = core.C;
					c *= 1 - (int64_t)(core.N ^ core.V) * 2;
					uint64_t val = ((int64_t)(int32_t)reg2 + (int64_t)(int32_t)doshift(reg3) + c) * core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				},
				op { op3i;
					int64_t c = core.C;
					c *= 1 - (int64_t)(core.N ^ core.V) * 2;
					uint64_t val = ((int64_t)(int32_t)reg2 + (int64_t)(int32_t)imm + c) * core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				}
			},
			{
				dop {
					return nstr + "ADC" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "ADC" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
				}
			}
		}, // 0010 ADC          Add with carry
		{ // SBB
			{
				op { op3r;
					int64_t c = core.C;
					c *= 1 - (int64_t)(core.N ^ core.V) * 2;
					uint64_t val = ((int64_t)(int32_t)reg2 - (int64_t)(int32_t)doshift(reg3) + c) * core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				},
				op { op3i;
					int64_t c = core.C;
					c *= 1 - (int64_t)(core.N ^ core.V) * 2;
					uint64_t val = ((int64_t)(int32_t)reg2 - (int64_t)(int32_t)imm + c) * core.inv;
					reg1 = (word)val;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				}
			},
			{
				dop {
					return nstr + "SBB" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "SBB" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
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
					core.V = (reg1 >> 31) != (reg2 >> 31);
				},
				op { op3i;
					reg1 = (reg2 << imm) * core.inv;

					if (!core.set_status) return;

					core.N = reg1 >> 31;
					core.Z = reg1 == 0;
					core.C = ((reg1 * core.inv) >> imm) != reg2;
					core.V = (reg1 >> 31) != (reg2 >> 31);
				}
			},
			{
				dop {
					return nstr + "ASL" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "ASL" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
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
					core.V = (reg1 >> 31) != (reg2 >> 31);
				},
				op { op3i;
					reg1 = ((reg2 >> imm) | ~(~((word)0) << imm)) * core.inv;

					if (!core.set_status) return;

					core.N = reg1 >> 31;
					core.Z = reg1 == 0;
					core.C = ((reg1 * core.inv) << imm) != reg2;
					core.V = (reg1 >> 31) != (reg2 >> 31);
				}
			},
			{
				dop {
					return nstr + "ASR" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "ASR" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
				}
			}
		}, // 0101 ASR          Arithmetic shift right
		{ // CMP
			{
				op { op3r;
					uint64_t val = ((int64_t)(int32_t)reg2 - (int64_t)(int32_t)doshift(reg3)) * core.inv;
					uint32_t v2 = (uint32_t)val;

					core.N = v2 >> 31 == 1;
					core.Z = v2 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (reg1 >> 31);
				},
				op { op2i;
					uint64_t val = ((int64_t)(int32_t)reg - (int64_t)(int32_t)imm) * core.inv;
					uint32_t v2 = (uint32_t)val;

					core.N = v2 >> 31 == 1;
					core.Z = v2 == 0;
					core.C = ((val >> 32) & 1) != 0;
					core.V = (val >> 63) != (v2 >> 31);
				}
			},
			{
				dop {
					return nstr + "CMP" + " " + r1 + ", " + r2 + shstr + cond;
				},
				dop {
					return nstr + "CMP" + " " + r1 + ", " + std::to_string(im12) + cond;
				}
			}
		}, // 0110 CMP          Compare two values with -
		{ // CMN
			{
				op { op3r;
					uint64_t val = ((int64_t)(int32_t)reg2 + (int64_t)(int32_t)doshift(reg3)) * core.inv;
					uint32_t v2 = (uint32_t)val;

					core.N = v2 >> 31 == 1;
					core.Z = v2 == 0;
					core.C = val > v2;
					core.V = (val >> 63) != (v2 >> 31);
				},
				op { op2i;
					uint64_t val = ((int64_t)(int32_t)reg + (int64_t)(int32_t)imm) * core.inv;
					uint32_t v2 = (uint32_t)val;

					core.N = v2 >> 31 == 1;
					core.Z = v2 == 0;
					core.C = val > v2;
					core.V = (val >> 63) != (v2 >> 31);
				}
			},
			{
				dop {
					return nstr + "CMN" + " " + r1 + ", " + r2 + shstr + cond;
				},
				dop {
					return nstr + "CMN" + " " + r1 + ", " + std::to_string(im12) + cond;
				}
			}
		}, // 0111 CMN          Compare two values with +
		{ // ORR
			{
				op { op3r;
					reg1 = (reg2 | doshift(reg3)) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = core.V = false;
				},
				op { op3i;
					reg1 = (reg2 | doshift(imm)) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = core.V = false;
				}
			},
			{
				dop {
					return (n ? "NOR" : "ORR") + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return (n ? "NOR" : "ORR") + sstr + " " + r1 + ", " + r2 + ", 0b" + ToBinary(im8,0) + cond;
				}
			}
		}, // 1000 ORR          A | B
		{ // AND
			{
				op { op3r;
					reg1 = (reg2 & doshift(reg3)) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = core.V = false;
				},
				op { op3i;
					reg1 = (reg2 & imm) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = core.V = false;
				}
			},
			{
				dop {
					return nstr + "AND" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "AND" + sstr + " " + r1 + ", " + r2 + ", 0b" + ToBinary(im8,0) + cond;
				}
			}
		}, // 1001 AND          A & B
		{ // XOR
			{
				op { op3r;
					reg1 = (reg2 ^ doshift(reg3)) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = core.V = false;
				},
				op { op3i;
					reg1 = (reg2 ^ imm) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = core.V = false;
				}
			},
			{
				dop {
					return "X" + nstr + "OR" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return "X" + nstr + "OR" + sstr + " " + r1 + ", " + r2 + ", 0b" + ToBinary(im8,0) + cond;
				}
			}
		}, // 1010 XOR          A ^ B
		{ // TST
			{
				op { op3r;
					word val = reg1 & (doshift(reg2) ^ core.neg);

					core.N = val >> 31 == 1;
					core.Z = val == 0;
					core.C = false;
					core.V = false;
				},
				op { op2i;
					word val = reg & (imm ^ core.neg);

					core.N = val >> 31 == 1;
					core.Z = val == 0;
					core.C = false;
					core.V = false;
				}
			},
			{
				dop {
					return nstr + "TST " + r1 + ", " + r2 + shstr + cond;
				},
				dop {
					return nstr + "TST " + r1 + ", 0b" + ToBinary(im12 ^ (n ? -1 : 0),0) + cond;
				}
			}
		}, // 1011 TST          Test bits with &
		{ // LSL
			{
				op { op3r;
					reg1 = (reg2 << doshift(reg3)) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((reg1 * core.inv) >> doshift(reg3)) != reg2;
					core.V = (reg1 >> 31) != (reg2 >> 31);
				},
				op { op3i;
					reg1 = (reg2 << imm) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((reg1 * core.inv) >> imm) != reg2;
					core.V = (reg1 >> 31) != (reg2 >> 31);
				}
			},
			{
				dop {
					return nstr + "LSL" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "LSL" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
				}
			}
		}, // 1100 LSL          Logical shift left
		{ // LSR
			{
				op { op3r;
					reg1 = (reg2 >> doshift(reg3)) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((reg1 * core.inv) << doshift(reg3)) != reg2;
					core.V = (reg1 >> 31) != (reg2 >> 31);
				},
				op { op3i;
					reg1 = (reg2 >> imm) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31 == 1;
					core.Z = reg1 == 0;
					core.C = ((reg1 * core.inv) << imm) != reg2;
					core.V = (reg1 >> 31) != (reg2 >> 31);
				}
			},
			{
				dop {
					return nstr + "LSR" + sstr + " " + r1 + ", " + r2 + ", " + r3 + shstr + cond;
				},
				dop {
					return nstr + "LSR" + sstr + " " + r1 + ", " + r2 + ", " + std::to_string(im8) + cond;
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
					core.V = (r1 >> 31) != (reg2 >> 31);
				},
				op { op2i;
					word r1 = reg;
					reg = imm ^ core.neg;

					if (!core.set_status) return;

					core.N = reg >> 31;
					core.Z = reg == 0;
					core.C = false;
					core.V = (r1 >> 31) != (reg >> 31);
				}
			},
			{
				dop {
					if (!n && sh == 0 && r1 == "LR" && r2 == "PC") return "RET" + sstr + cond;
					return (n ? "MVN" : "MOV") + sstr + " " + r1 + ", " + r2 + shstr + cond;
				},
				dop {
					return (n ? "MVN" : "MOV") + sstr + " " + r1 + ", " + std::to_string(im12) + cond;
				}
			}
		}, // 1110 MOV / MVN    Copy value to register
		{ // INV
			{
				op { op2r;
					word r1 = reg1;
					reg1 = (~doshift(reg2) + 1) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg1 >> 31;
					core.Z = reg1 == 0;
					core.C = false;
					core.V = (r1 >> 31) != (reg1 >> 31);
				},
				op { op2i;
					word r1 = reg;
					reg = (~imm + 1) ^ core.neg;

					if (!core.set_status) return;

					core.N = reg >> 31;
					core.Z = reg == 0;
					core.C = false;
					core.V = (r1 >> 31) != (reg >> 31);
				}
			},
			{
				dop {
					return nstr + "INV" + sstr + " " + r1 + ", " + r2 + shstr + cond;
				},
				dop {
					return nstr + "INV" + sstr + " " + r1 + ", " + std::to_string(im12) + cond;
				}
			}
		}, // 1111 INV          -A
	};
}