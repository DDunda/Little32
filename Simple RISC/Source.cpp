#include<SDL.hpp>
#include<iostream>

#include "Computer.h"
#include "Core.h"
#include "Device.h"
#include "MemoryMapped.h"
#include "MappedDevice.h"
#include "RAM.h"
#include "ROM.h"
#include "ComputerInfo.h"
#include "CharDisplay.h"

using namespace SDL;

class DebugCore : public Core {
public:
	word registers[16]{0};
	byte status;

	DebugCore(Computer& computer) : Core(computer) {}

	void Clock() {
		if (interrupt) {
			computer.Write(SP, status);
			SP -= 4;
			computer.Write(SP, PC);
			SP -= 4;

			status = 0;
			PC = interrupt_addr;

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

	void SetPC(word value) { PC = value; }
	void SetSP(word value) { SP = value; }
};

class RISCCore : public Core {
public:
	word registers[16]{ 0 };
	byte status;

	RISCCore(Computer& computer) : Core(computer) {}

	void Clock() {

	}

	void Reset() {
		memset(registers, 0, sizeof(registers));
		interrupt = false;
		interrupt_addr = 0;
	}

	void SetPC(word value) { PC = value; }
	void SetSP(word value) { SP = value; }
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
	DebugCore core(computer);
	ComputerInfo info(computer);
	RAM ram(512, 256);
	CharDisplay cram(r, charset, { 8,8 }, 16, {16,16}, { 4,4 }, ram.address_start + ram.address_size);

	computer.AddMapping(info);
	computer.AddMappedDevice(ram);
	computer.AddMappedDevice(cram);

	computer.core = &core;
	computer.start_PC = ram.GetAddress();
	computer.start_SP = computer.start_PC + ram.GetRange() - 4;
	computer.SoftReset();

	for (int frame = 0; input.running; frame++) {
		input.Update();
		
		computer.Clock();
		cram.Render();

		r.Present();

		Delay(500);
	}

	Quit();

	return 0;
}