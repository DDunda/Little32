#include "L32_DebugCore.h"

#include "L32_Computer.h"

namespace Little32
{
	DebugCore::DebugCore(Computer& computer)
		: computer(computer) {}

	void DebugCore::Clock()
	{
		word addr = computer.Read(7 * 4);
		std::string gradient = "   \xb0\xb0\xb0\xb1\xb1\xb1\xb1\xb2\xb2\xb2\xdb\xdb\xdb";

		std::string test = "...\xc9\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbb...\
...\xba        \xba...\
...\xba Hello, \xba...\
...\xba World! \xba...\
...\xba        \xba...\
...\xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc...";

		for (word y = 0; y < 16; y++)
		{
			word w = (byte)gradient[y];
			w |= w << 8;
			w |= w << 16;
			computer.Write(addr + y * 16, w);
			computer.Write(addr + y * 16 + 4, w);
			computer.Write(addr + y * 16 + 8, w);
			computer.Write(addr + y * 16 + 12, w);
		}

		for (word i = 0; i < test.length(); i++)
		{
			if (test[i] != '.')
			{
				computer.WriteByte(addr + i + 16 * 4, test[i]);
			}
		}
		return;
	}

	const std::string DebugCore::Disassemble(word instruction) const
		{ return "NOP"; }

	void DebugCore::Reset()
	{
		memset(registers, 0, sizeof(registers));
		status = 0;
	}

	void DebugCore::Interrupt(word addr)
	{
		SP -= 4;
		computer.Write(SP, status);
		SP -= 4;
		computer.Write(SP, PC);

		status = 0;
		PC = addr;
	}

	void DebugCore::SetPC(word value) { PC = value; }
	void DebugCore::SetSP(word value) { SP = value; }
}