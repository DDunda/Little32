#ifndef L32_L32Core_h_
#define L32_L32Core_h_
#pragma once

#include "L32_ICore.h"

namespace Little32
{
	struct Little32Core : public ICore
	{
		Computer& computer;

		union {
			struct
			{
				word R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, SP, LR, PC;
			};
			word registers[16] { 0 };
		};

		bool N = false, Z = false, C = false, V = false; // Status flags

		static constexpr const char N_flag = 0b1000;
		static constexpr const char Z_flag = 0b0100;
		static constexpr const char C_flag = 0b0010;
		static constexpr const char V_flag = 0b0001;

		static constexpr const char AL = 0b0000;
		static constexpr const char GT = 0b0001;
		static constexpr const char GE = 0b0010;
		static constexpr const char HI = 0b0011;
		static constexpr const char CS = 0b0100;
		static constexpr const char ZS = 0b0101;
		static constexpr const char NS = 0b0110;
		static constexpr const char VS = 0b0111;
		static constexpr const char VC = 0b1000;
		static constexpr const char NC = 0b1001;
		static constexpr const char ZC = 0b1010;
		static constexpr const char CC = 0b1011;
		static constexpr const char LS = 0b1100;
		static constexpr const char LT = 0b1101;
		static constexpr const char LE = 0b1110;

		static constexpr const char HS = CS;
		static constexpr const char EQ = ZS;
		static constexpr const char MI = NS;
		static constexpr const char PL = NC;
		static constexpr const char NE = ZC;
		static constexpr const char LO = CC;

		//                                         CCCCNxxxxxxi111122223333xxxxSSSS
		static constexpr word cond_bits        = 0b11110000000000000000000000000000;
		static constexpr word negative_bit     = 0b00001000000000000000000000000000;
		static constexpr word immediate_bit    = 0b00000000000100000000000000000000;
		static constexpr word reg1_bits        = 0b00000000000011110000000000000000;
		static constexpr word reg2_bits        = 0b00000000000000001111000000000000;
		static constexpr word reg3_bits        = 0b00000000000000000000111100000000;
		static constexpr word imm12_bits       = 0b00000000000000001111111111110000;
		static constexpr word imm8_bits        = 0b00000000000000000000111111110000;
		static constexpr word shift_bits       = 0b00000000000000000000000000001111;

		//                                         CCCCN0001pppxxxxxxxxxxxxxxxxSSSS
		static constexpr word float_bit        = 0b00000000100000000000000000000000;
		static constexpr word float_op_bits    = 0b00000000011100000000000000000000;

		//                                         CCCCN1ppppSixxxxxxxxxxxxxxxxSSSS
		static constexpr word arithmetic_bit   = 0b00000100000000000000000000000000;
		static constexpr word opcode_bits      = 0b00000011110000000000000000000000;
		static constexpr word status_bit       = 0b00000000001000000000000000000000;

		//                                         CCCCN01Lbbbbbbbbbbbbbbbbbbbbbbbb
		static constexpr word branch_bit       = 0b00000010000000000000000000000000;
		static constexpr word link_bit         = 0b00000001000000000000000000000000;
		static constexpr word offset_bits      = 0b00000000111111111111111111111111;

		//                                         CCCCN001ppppxxxxxxxxxxxxxxxxSSSS
		static constexpr word extended_bit     = 0b00000001000000000000000000000000;
		static constexpr word extended_op_bits = 0b00000000111100000000000000000000;
		static constexpr word reglist_bits     = 0b00000000000000001111111111111111;

		constexpr Little32Core(Computer& computer) : computer(computer) {}

		void Clock();
		void Interrupt(word address);
		void Reset();

		const std::string Disassemble(word instruction) const;

		void Push(word& ptr, word val);
		word Pop(word& ptr);

		constexpr void SetPC(word value) { PC = value; }
		constexpr void SetSP(word value) { SP = value; }
	};
}

#endif