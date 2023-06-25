#pragma once
#include "Core.h"

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