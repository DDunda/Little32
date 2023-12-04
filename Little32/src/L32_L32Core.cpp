#include "L32_L32Core.h"

#include "L32_Computer.h"
#include "L32_String.h"

#include <bit>

namespace Little32
{
	void Little32Core::Clock()
	{
		using namespace std;

		const word instruction = computer.Read(PC);

		// The condition is stored in the upper 4 bits
		const byte cond = (instruction & cond_bits) >> 28;

		PC += sizeof(word); // Moves to the next instruction in case the condition fails

		switch (cond)
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

		PC -= sizeof(word); // Moves back to current instruction

		const bool negative   = (instruction & negative_bit ) != 0;
		const bool immediate  = (instruction & immediate_bit) != 0;  // Causes the last operand to be a constant value in most cases
		const bool set_status = (instruction & status_bit   ) != 0;
		const word shift      = (instruction & shift_bits) * 2; // These bits are used as a barrel shift for flexible operands (multiplied by 2 to extend from a range of 16 to 32 shifts)
		const word imm8       = rotl((instruction & imm8_bits) >> 4, shift);
		const word imm12      = rotl((instruction & imm12_bits) >> 4, shift);

		const int32_t inv = negative ? -1 : 1;    // 1 or -1:      Used for inverting values with *
		const word neg = negative ? ~(word)0 : 0; // all 0s or 1s: Used for inverting values with ^
		
		word& reg1 = registers[(instruction & reg1_bits) >> 16];
		word& reg2 = registers[(instruction & reg2_bits) >> 12];
		word& reg3 = registers[(instruction & reg3_bits) >> 8 ];

		const word reg2s = rotl(reg2, shift);
		const word reg3s = rotl(reg3, shift);
		      int32_t& reg1_int  = reinterpret_cast<      int32_t&>(reg1 );
		const int32_t& reg2_int  = reinterpret_cast<const int32_t&>(reg2 );
		const int32_t& reg2s_int = reinterpret_cast<const int32_t&>(reg2s);
		const int32_t& reg3_int  = reinterpret_cast<const int32_t&>(reg3s);


		if (instruction & arithmetic_bit) // Arithmetic
		{
			const word val2 = immediate ? imm12 : reg2s;
			const word val3 = immediate ? imm8  : reg3s;

			const int32_t val2_int = immediate ? reinterpret_cast<const int32_t&>(imm12) : reg2s_int;
			const int32_t val3_int = immediate ? reinterpret_cast<const int32_t&>(imm8 ) : reg3_int;

			int64_t long_val;
			int32_t val;

			const byte op = (instruction & opcode_bits) >> 22;

			switch (op)
			{
				case 0b0000: // ADD          Add
					reg1_int = long_val = ((int64_t)reg2_int + (int64_t)val3_int) * inv;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg1_int != long_val;
						V = (long_val < 0) != (reg1_int < 0);
					}
					break;

				case 0b0001: // SUB          Sub
					reg1_int = long_val = ((int64_t)reg2_int - (int64_t)val3_int) * inv;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg1_int != long_val;
						V = (long_val < 0) != (reg1_int < 0);
					}
					break;

				case 0b0010: // ADC          Add with carry
					reg1_int = long_val = ((int64_t)reg2_int + (int64_t)val3_int + C * (1 - 2 * (int64_t)(N != V))) * inv;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg1_int != long_val;
						V = (long_val < 0) != (reg1_int < 0);
					}
					break;

				case 0b0011: // SBB          Sub with borrow
					reg1_int = long_val = ((int64_t)reg2_int - (int64_t)val3_int + C * (1 - 2 * (int64_t)(N != V))) * inv;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg1_int != long_val;
						V = (long_val < 0) != (reg1_int < 0);
					}
					break;

				case 0b0100: // ASL          Arithmetic shift left
					reg1 = (reg2 << val3) * inv;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg2 != ((reg1 * inv) >> val3);
						V = (reg2_int < 0) != (reg1_int < 0);
					}
					break;

				case 0b0101: // ASR          Arithmetic shift right
					reg1 = ((reg2 >> val3) | ~(~(word)0 >> val3)) * inv;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg2 != ((reg1 * inv) << val3);
						V = (reg2_int < 0) != (reg1_int < 0);
					}
					break;

				case 0b0110: // CMP          Compare two values with -
					val = long_val = ((int64_t)reg1_int - (int64_t)val2_int) * inv;

					N = val < 0;
					Z = val == 0;
					C = val != long_val;
					V = (long_val < 0) != (val < 0);
					break;

				case 0b0111: // CMN          Compare two values with +
					val = long_val = ((int64_t)reg1 + (int64_t)val2_int) * inv;

					N = val < 0;
					Z = val == 0;
					C = ((long_val >> 32) & 1) != 0;
					V = (long_val < 0) != (val < 0);
					break;

				case 0b1000: // ORR          A | B
					reg1 = (reg2 | val3) ^ neg;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = V = false;
					}
					break;

				case 0b1001: // AND          A & B
					reg1 = (reg2 & val3) ^ neg;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = V = false;
					}
					break;

				case 0b1010: // XOR          A ^ B
					reg1 = (reg2 ^ val3) ^ neg;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = V = false;
					}
					break;

				case 0b1011: // TST          Test bits with &
					val = reg1 & (val2 ^ neg);

					N = val < 0;
					Z = val == 0;
					C = V = false;
					break;

				case 0b1100: // LSL          Logical shift left
					reg1 = (reg2 << val3) ^ neg;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg2 != ((reg1 ^ neg) >> val3);
						V = (reg1_int < 0) != (reg2_int < 0);
					}
					break;

				case 0b1101: // LSR          Logical shift right
					reg1 = (reg2 >> val3) ^ neg;

					if (set_status)
					{
						N = reg1_int < 0;
						Z = reg1 == 0;
						C = reg2 != ((reg1 ^ neg) << val3);
						V = (reg1_int < 0) != (reg2_int < 0);
					}
					break;

				case 0b1110: // MOV / MVN    Copy value to register
					val = reg1_int;
					reg1 = val2 ^ neg;

					if (set_status)
					{
						N = reg1 < 0;
						Z = reg1 == 0;
						C = false;
						V = (val < 0) != ( reg1_int < 0);
					}
					break;

				case 0b1111: // INV          -A
					val = reg1_int;
					reg1 = (~val2 + 1) ^ neg;
					
					if (set_status)
					{
						N = reg1 < 0;
						Z = reg1 == 0;
						C = false;
						V = (val < 0) != (reg1_int < 0);
					}
					break;
			}
		}
		else if (instruction & branch_bit) // B / BL
		{
			const word offset = (instruction & offset_bits) * sizeof(word) * inv;
			const bool link_back = (instruction & link_bit) != 0;

			if (offset == 0 && negative)
			{
				if (link_back) // RET
				{
					PC = LR;
				}
				else { // RFE
					PC = Pop(SP);
					const word status = Pop(SP);
					N = (status & N_flag) == N_flag;
					Z = (status & Z_flag) == Z_flag;
					C = (status & C_flag) == C_flag;
					V = (status & V_flag) == V_flag;
				}
			}
			else
			{
				if (link_back) LR = PC + sizeof(word); // BL
				PC += offset;
			}
			return; // Don't change the PC again
		}
		else if (instruction & extended_bit)
		{
			const int32_t off  = rotl(reg3, shift) * inv;
			const int32_t offi = imm8 * inv;
			const word addr = reg2 + off;
			const word addri = reg2 + offi;
			const word list = instruction & reglist_bits;

			switch ((instruction & extended_op_bits) >> 20)
			{
				case 0b1000: reg1 = computer.Read(addr ); break; // RRW
				case 0b1001: reg1 = computer.Read(addri); break;
				case 0b1010: computer.Write(addr,  reg1); break; // RWW
				case 0b1011: computer.Write(addri, reg1); break;
				case 0b1100: reg1 = computer.ReadByte(addr ); break; // RRB
				case 0b1101: reg1 = computer.ReadByte(addri); break;
				case 0b1110: computer.WriteByte(addr,  reg1); break; // RWB
				case 0b1111: computer.WriteByte(addri, reg1); break;
				case 0b0100: // SRR
					if (list & 0x8000) registers[15] = Pop(reg1) * inv;
					if (list & 0x4000) registers[14] = Pop(reg1) * inv;
					if (list & 0x2000) registers[13] = Pop(reg1) * inv;
					if (list & 0x1000) registers[12] = Pop(reg1) * inv;
					if (list & 0x0800) registers[11] = Pop(reg1) * inv;
					if (list & 0x0400) registers[10] = Pop(reg1) * inv;
					if (list & 0x0200) registers[9 ] = Pop(reg1) * inv;
					if (list & 0x0100) registers[8 ] = Pop(reg1) * inv;
					if (list & 0x0080) registers[7 ] = Pop(reg1) * inv;
					if (list & 0x0040) registers[6 ] = Pop(reg1) * inv;
					if (list & 0x0020) registers[5 ] = Pop(reg1) * inv;
					if (list & 0x0010) registers[4 ] = Pop(reg1) * inv;
					if (list & 0x0008) registers[3 ] = Pop(reg1) * inv;
					if (list & 0x0004) registers[2 ] = Pop(reg1) * inv;
					if (list & 0x0002) registers[1 ] = Pop(reg1) * inv;
					if (list & 0x0001) registers[0 ] = Pop(reg1) * inv;
					break;
				case 0b0101: // SWR
					if (list & 0x0001) Push(reg1, registers[0 ] * inv);
					if (list & 0x0002) Push(reg1, registers[1 ] * inv);
					if (list & 0x0004) Push(reg1, registers[2 ] * inv);
					if (list & 0x0008) Push(reg1, registers[3 ] * inv);
					if (list & 0x0010) Push(reg1, registers[4 ] * inv);
					if (list & 0x0020) Push(reg1, registers[5 ] * inv);
					if (list & 0x0040) Push(reg1, registers[6 ] * inv);
					if (list & 0x0080) Push(reg1, registers[7 ] * inv);
					if (list & 0x0100) Push(reg1, registers[8 ] * inv);
					if (list & 0x0200) Push(reg1, registers[9 ] * inv);
					if (list & 0x0400) Push(reg1, registers[10] * inv);
					if (list & 0x0800) Push(reg1, registers[11] * inv);
					if (list & 0x1000) Push(reg1, registers[12] * inv);
					if (list & 0x2000) Push(reg1, registers[13] * inv);
					if (list & 0x4000) Push(reg1, registers[14] * inv);
					if (list & 0x8000) Push(reg1, registers[15] * inv);
					break;
				case 0b0110: // MVM
				{	const word v = reg1 * inv;
					if (list & 0x0001) registers[0 ] = v;
					if (list & 0x0002) registers[1 ] = v;
					if (list & 0x0004) registers[2 ] = v;
					if (list & 0x0008) registers[3 ] = v;
					if (list & 0x0010) registers[4 ] = v;
					if (list & 0x0020) registers[5 ] = v;
					if (list & 0x0040) registers[6 ] = v;
					if (list & 0x0080) registers[7 ] = v;
					if (list & 0x0100) registers[8 ] = v;
					if (list & 0x0200) registers[9 ] = v;
					if (list & 0x0400) registers[10] = v;
					if (list & 0x0800) registers[11] = v;
					if (list & 0x1000) registers[12] = v;
					if (list & 0x2000) registers[13] = v;
					if (list & 0x4000) registers[14] = v;
					if (list & 0x8000) registers[15] = v;
				}	break;
				case 0b0111: // SWP
					reg2 = reg2s * inv;
					swap(reg1, reg2);
					break;
				case 0b0010: // Room for more instructions?
				case 0b0011:
				case 0b0001:
				case 0b0000:
					break;
			}
		}
		else if (instruction & float_bit) // FPU
		{
			      float& reg1f  = reinterpret_cast<      float&>(reg1);
			const float& reg2f  = reinterpret_cast<const float&>(reg2 );
			const float& reg2sf = reinterpret_cast<const float&>(reg2s);
			const float& reg3f  = reinterpret_cast<const float&>(reg3s);

			switch ((instruction & float_op_bits) >> 20)
			{
			case 0b000: // ADDF
				reg1f = (reg2f + reg3f) * inv;
				break;
			case 0b001: // SUBF
				reg1f = (reg2f - reg3f) * inv;
				break;
			case 0b010: // MULF
				reg1f = (reg2f * reg3f) * inv;
				break;
			case 0b011: // DIVF
				reg1f = (reg2f / reg3f) * inv;
				break;
			case 0b100: // ITOF
				reg1f = (float)(reg2s_int * inv);
				break;
			case 0b101: // FTOI
				reg1_int = (int32_t)(reg2sf * inv);
				break;
			case 0b110: // CMPF
			{
				const float cmp = (reg1f - reg2sf) * inv;
				N = cmp < 0.f;
				Z = cmp == 0.f;
				V = (reg1f < 0.f) != (reg2sf < 0.f) && std::abs(reg2sf) > std::numeric_limits<float>::max() - std::abs(reg1f);
				C = false;
			}	break;
			case 0b111: // CMPFI
			{
				const float cmp = (reg1f - reg2s_int) * inv;
				N = cmp < 0.f;
				Z = cmp == 0.f;
				V = (reg1f < 0.f) != (reg2s_int < 0.f) && std::abs(reg2s_int) > std::numeric_limits<float>::max() - std::abs(reg1f);
				C = false;
			}	break;
			}
		}
		// Else NOP

		PC += sizeof(word); // Moves to the next word
	}

	const std::string Little32Core::Disassemble(word instruction) const
	{
		using namespace std;

		const byte c = ( instruction & cond_bits ) >> 28;
		const string cond = c == 0 ? " " : string(cond_names_regular[c]) + " ";
		const string cond2 = c == 0 ? "" : " ?" + string(cond_names_regular[c]);

		const bool negative = ( instruction & negative_bit ) != 0;
		const bool immediate = ( instruction & immediate_bit ) != 0;
		const bool set_status = ( instruction & status_bit ) != 0;
		const bool link = ( instruction & link_bit ) != 0;

		const char* const sign = negative ? "-" : "+";
		const string nstr = negative ? "N" : "";
		const string sstr = set_status ? "S" : "";

		const byte shift = ( instruction & shift_bits ) * 2;
		const string shstr = shift == 0 ? "" : ( " << " + to_string(shift) );

		const word   imm8_v = rotl(( instruction & imm8_bits ) >> 4, shift);
		const word   imm12_v = rotl(( instruction & imm12_bits ) >> 4, shift);
		const string imm8 = to_string(imm8_v);
		const string imm12 = to_string(imm12_v);
		const string reg_list = RegListToString(instruction & reglist_bits);
		const word   b_off_v = ( instruction & offset_bits ) * sizeof(word);
		const string b_off = sign + to_string(b_off_v);

		const string r1 = reg_names[( instruction & reg1_bits ) >> 16];
		const string r2 = reg_names[( instruction & reg2_bits ) >> 12];
		const string r3 = reg_names[( instruction & reg3_bits ) >> 8];

		if (instruction & arithmetic_bit) // Arithmetic
		{
			const string& val2 = immediate ? imm12 : r2;
			const string& val3 = immediate ? imm8 : r3;

			const string& val2b = immediate ? "0b" + ToBinary(imm12_v, 0) : r2;
			const string& val3b = immediate ? "0b" + ToBinary(imm8_v, 0) : r3;

			switch (( instruction & opcode_bits ) >> 22)
			{
			case 0b0000: return nstr + "ADD" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b0001: return nstr + "SUB" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b0010: return nstr + "ADC" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b0011: return nstr + "SBB" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b0100: return nstr + "ASL" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b0101: return nstr + "ASR" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b0110: return nstr + "CMP " + r1 + ", " + val2 + cond2;
			case 0b0111: return nstr + "CMN " + r1 + ", " + val2 + cond2;
			case 0b1000: return ( negative ? "NOR" : "ORR" ) + sstr + " " + r1 + ", " + r2 + ", " + val3b + cond2;
			case 0b1001: return nstr + "AND" + sstr + " " + r1 + ", " + r2 + ", " + val3b + cond2;
			case 0b1010: return "X" + nstr + "OR" + sstr + " " + r1 + ", " + r2 + ", " + val3b + cond2;
			case 0b1011: return nstr + "TST " + r1 + ", " + val2b + cond2;
			case 0b1100: return nstr + "LSL" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b1101: return nstr + "LSR" + sstr + " " + r1 + ", " + r2 + ", " + val3 + cond2;
			case 0b1110: return ( negative ? "MVN" : "MOV" ) + sstr + " " + r1 + ", " + val2 + cond2;
			case 0b1111: return nstr + "INV" + sstr + " " + r1 + ", " + val2 + cond2;
			}
		}

		if (instruction & branch_bit)
		{
			if (negative && b_off_v == 0)
			{
				if (link) return "RET" + cond2; // RET
				else      return "RFE" + cond2; // RFE
			}
			else
			{
				if (link) return "BL " + b_off + cond2; // BL
				else if (b_off_v == 0) return "HALT" + cond2;
				else      return "B" + cond + b_off; // B
			}
		}
		else if (instruction & extended_bit)
		{

			const string addr = r2 == "PC" ? sign : r2 + " " + sign + " ";

			switch (( instruction & extended_op_bits ) >> 20)
			{
			case 0b1000: return nstr + "RRW " + r1 + ", [" + addr + r3 + shstr + "]" + cond2;
			case 0b1001: return nstr + "RRW " + r1 + ", [" + addr + imm8 + "]" + cond2;
			case 0b1010: return nstr + "RWW " + r1 + ", [" + addr + r3 + shstr + "]" + cond2;
			case 0b1011: return nstr + "RWW " + r1 + ", [" + addr + imm8 + "]" + cond2;
			case 0b1100: return nstr + "RRB " + r1 + ", [" + addr + r3 + shstr + "]" + cond2;
			case 0b1101: return nstr + "RRB " + r1 + ", [" + addr + imm8 + "]" + cond2;
			case 0b1110: return nstr + "RWB " + r1 + ", [" + addr + r3 + shstr + "]" + cond2;
			case 0b1111: return nstr + "RWB " + r1 + ", [" + addr + imm8 + "]" + cond2;
			case 0b0100: return nstr + "SRR " + r1 + ", " + reg_list + cond2;
			case 0b0101: return nstr + "SWR " + r1 + ", " + reg_list + cond2;
			case 0b0110: return nstr + "MVM " + r1 + ", " + reg_list + cond2;
			case 0b0111: return nstr + "SWP " + r1 + ", " + r2 + shstr + cond2;
			case 0b0010: // Room for more instructions?
			case 0b0011:
			case 0b0001:
			case 0b0000:
				return "";
			}
		}
		else if (instruction & float_bit)
		{ // FPU
			switch (( instruction & float_op_bits ) >> 20)
			{
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

	void Little32Core::Reset()
	{
		memset(registers, 0, sizeof(registers));
		N = Z = C = V = false;
	}

	void Little32Core::Interrupt(word address)
	{
		word status = (N * N_flag) | (Z * Z_flag) | (C * C_flag) | (V * V_flag);
		Push(SP, status);
		Push(SP, PC);
		PC = address;
		N = Z = C = V = false;
	}

	void Little32Core::Push(word& ptr, word val)
	{
		ptr -= sizeof(word);
		computer.Write(ptr, val);
	}

	word Little32Core::Pop(word& ptr)
	{
		ptr += sizeof(word);
		return computer.Read(ptr - sizeof(word));
	}
}