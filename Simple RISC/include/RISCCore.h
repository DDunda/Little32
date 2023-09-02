#pragma once

#ifndef SR_RISCCore_h_
#define SR_RISCCore_h_

#include "Core.h"

const char N_flag = 0b1000;
const char Z_flag = 0b0100;
const char C_flag = 0b0010;
const char V_flag = 0b0001;

const char R0 = 0;
const char R1 = 1;
const char R2 = 2;
const char R3 = 3;
const char R4 = 4;
const char R5 = 5;
const char R6 = 6;
const char R7 = 7;
const char R8 = 8;
const char R9 = 9;
const char R10 = 10;
const char R11 = 11;
const char R12 = 12;
const char SP = 13;
const char LR = 14;
const char PC = 15;

const char AL = 0b0000;
const char GT = 0b0001;
const char GE = 0b0010;
const char HI = 0b0011;
const char CS = 0b0100;
const char ZS = 0b0101;
const char NS = 0b0110;
const char VS = 0b0111;
const char VC = 0b1000;
const char NC = 0b1001;
const char ZC = 0b1010;
const char CC = 0b1011;
const char LS = 0b1100;
const char LT = 0b1101;
const char LE = 0b1110;

const char HS = CS;
const char EQ = ZS;
const char MI = NS;
const char PL = NC;
const char NE = ZC;
const char LO = CC;

namespace SimpleRISC {
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
			const std::string& cond,
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

		RISCCore(Computer& computer);

		void Clock();
		void Interrupt(word address);
		void Reset();

		const std::string Disassemble(word instruction) const;

		void Push(word& ptr, word val);
		word Pop(word& ptr);

		void SetPC(word value);
		void SetSP(word value);
	};
}

#endif