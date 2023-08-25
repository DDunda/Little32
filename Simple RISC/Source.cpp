#include <SDL.hpp>
#include <iostream>
#include <fstream>
#include <bit>

#include "Computer.h"
#include "Device.h"
#include "MemoryMapped.h"
#include "MappedDevice.h"

#include "RAM.h"
#include "ROM.h"
#include "ComputerInfo.h"
#include "CharDisplay.h"
#include "ColourCharDisplay.h"
#include "KeyboardDevice.h"

#include "RISCCore.h"

#include "SR_String.h"
#include "PreProcAssember.h"
#include "Assembler.h"

using namespace SDL;
using namespace SimpleRISC;

const int palette_count = 2;

const SDL::Colour palettes[palette_count][16] =
{
	{
		{0, 0, 0 }, {127,0,0}, {127,51, 0}, {127,106,0}, {0,127,0}, {0,0,127}, {87, 0,127}, {127,127,127},
		{64,64,64}, {255,0,0}, {255,106,0}, {255,216,0}, {0,255,0}, {0,0,255}, {178,0,255}, {255,255,255}
	},
	{
		{12,18, 6 }, {15,22, 7 }, {18, 27, 9 }, {23, 34, 11}, {29, 43, 14}, {36, 53, 17 }, {45, 67, 21 }, {56, 84, 27 },
		{70,104,33}, {88,131,41}, {109,163,52}, {137,204,65}, {171,255,81}, {195,255,119}, {223,255,174}, {255,255,255}
	}
};

int main(int argc, char* argv[]) {
	Init();
	IMG::Init(IMG_INIT_PNG);

	bool manually_clocked = false;
	int selected_palette = 0;
	int clocks = 0;

	Input input;

	const Point scale { 4,4 };
	const Point text_size { 16,16 };
	const Point char_size { 8,8 };
	int frame_delay = 16;
	int clock_count = 1000;

	Window w;
	Renderer r;
	Point& window_size = input.windowSize = text_size * char_size * scale;

	CreateWindowAndRenderer(window_size, w, r);

	Texture char_set = IMG::LoadTexture(r, "Char set.png");

	Computer computer;
	RISCCore core(computer);
	ComputerInfo info(computer);

	word address = 256;

	RAM ram(address, 512);

	address += ram.GetRange();

	ColourCharDisplay cram(
		computer,
		r,
		char_set,
		palettes[selected_palette],
		char_size,
		16,       // Characters per row of source texture
		text_size,
		scale,
		address,
		' ',
		0x0F  // White on black
	);

	address += cram.GetRange();

	KeyboardDevice keyboard(
		computer,
		address
	);

	input.Register(keyboard);

	address += cram.GetRange();

	computer.AddMapping(info);
	computer.AddMappedDevice(ram);
	computer.AddMappedDevice(cram);
	computer.AddMappedDevice(keyboard);

	Assembler assembler;

	assembler.AddLabel("CHAR_MEM", cram.address_start);
	assembler.AddLabel("COLOUR_MEM", cram.address_start + cram.address_size);
	assembler.AddLabel("KEYBOARD", keyboard.address_start);
	assembler.SetRAM(ram);

	std::string file_name = "";

	if (file_name == "")
	{
		printf("Please enter the program to assemble: (defaults to program.asm)\n");
		std::getline(std::cin, file_name);
		printf("\n");

		if (file_name == "") file_name = "program.asm";
	}

	if (file_name.find('.') == std::string::npos) file_name += ".asm";

	if (file_name == "program.asm" || file_name == "blue_wave.asm")
	{
		clock_count = 2611; //2099;
		frame_delay = 50;
	}
	else if (file_name == "bounce.asm") {
		clock_count = 26;
		frame_delay = 16;
	}

	std::fstream program;

	program.open(file_name);

	if (!program.is_open()) {
		printf("Could not open assembly file '%s'\n", file_name.c_str());
		return 1;
	}

	try {
		assembler.Assemble(program);
		program.close();
	}
	catch (const Assembler::FormatException& e)
	{
		program.close();
		printf("%s\n%s", e.message.c_str(), e.line.c_str());
		return 1;
	}

	computer.core = &core;
	computer.start_PC = assembler.program_start;
	computer.start_SP = ram.address_start + ram.address_size;
	computer.SoftReset();

	printf("Program memory:\n");
	// Disassemble program
	DisassembleMemory(computer, assembler.program_start, assembler.program_end);

	// Print values in memory
	PrintMemory(computer, assembler.data_start, assembler.data_end, 0, true);
	printf("\n");

	for (int frame = 0; input.running; frame++)
	{
		input.Update();

		if (input.scancodeDown(Scancode::ESCAPE)) break;

		if (input.scancodeDown(Scancode::BACKSPACE))
		{
			manually_clocked = !manually_clocked;
			clocks = 0;
		}

		if (input.scancodeDown(Scancode::P))
		{
			selected_palette = (selected_palette + 1) % palette_count;
			memcpy(cram.colours, palettes[selected_palette], sizeof(SDL::Colour) * 16);
		}

		if (input.scancodeDown(Scancode::R))
		{
			computer.HardReset();
			assembler.SetRAM(ram);

			program.open(file_name);

			if (!program.is_open())
			{
				printf("Could not open assembly file '%s'\n", file_name.c_str());
				return 1;
			}

			try
			{
				assembler.Assemble(program);
				program.close();
			}
			catch (const Assembler::FormatException& e)
			{
				program.close();
				printf("%s\n%s", e.message.c_str(), e.line.c_str());
				return 1;
			}

			printf("Program memory:\n");
			DisassembleMemory(computer, assembler.program_start, assembler.program_end);
			PrintMemory(computer, assembler.data_start, assembler.data_end, 0, true);
			printf("\n");
		}
		
		if (!manually_clocked)
		{
			computer.Clock(clock_count);
			cram.Render();

			r.Present();

			Delay(frame_delay);
		}
		else
		{
			if (input.scancodeDown(Scancode::SPACE) || input.buttonDown(Button::LEFT))
			{
				printf("PC: 0x%08X  SP: 0x%08X  LR: 0x%08X\n", core.registers[PC], core.registers[SP], core.registers[LR]);
				printf(" R0: % 10i  R1: % 10i  R2: % 10i  R3: % 10i\n", core.registers[R0], core.registers[R1], core.registers[R2], core.registers[R3]);
				printf(" R4: % 10i  R5: % 10i  R6: % 10i  R7: % 10i\n", core.registers[R4], core.registers[R5], core.registers[R6], core.registers[R7]);
				printf(" R8: % 10i  R9: % 10i R10: % 10i R11: % 10i\n", core.registers[R8], core.registers[R9], core.registers[R10], core.registers[R11]);
				printf("R12: % 10i NZCV: %i%i%i%i \n", core.registers[R12], core.N, core.Z, core.C, core.V);

				printf("0x%08X: %s\n\n", core.registers[PC], core.Disassemble(computer.Read(core.registers[PC])).c_str());

				//printf("0x%08X: ", core.registers[PC]);
				//word op = computer.Read(core.registers[PC]);
				//std::string diss = core.Disassemble(op);
				//printf("%s\n\n", diss.c_str());

				clocks++;
				clocks %= clock_count;
				computer.Clock(1);
				cram.Render(clocks == 0);
				r.Present();
			}
			Delay(10);
		}
	}

	IMG::Quit();
	Quit();

	return 0;
}