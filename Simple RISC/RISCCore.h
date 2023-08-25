#pragma once

#ifndef SR_RISCCore_h_
#define SR_RISCCore_h_

#include "Core.h"

#define N_flag 0b1000
#define Z_flag 0b0100
#define C_flag 0b0010
#define V_flag 0b0001

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8
#define R9 9
#define R10 10
#define R11 11
#define R12 12
#define SP 13
#define LR 14
#define PC 15

#define AL 0b0000
#define GT 0b0001
#define GE 0b0010
#define HI 0b0011
#define CS 0b0100
#define ZS 0b0101
#define NS 0b0110
#define VS 0b0111
#define VC 0b1000
#define NC 0b1001
#define ZC 0b1010
#define CC 0b1011
#define LS 0b1100
#define LT 0b1101
#define LE 0b1110

#define HS CS
#define EQ ZS
#define MI NS
#define PL NC
#define NE ZC
#define LO CC

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