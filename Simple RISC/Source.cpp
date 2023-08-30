#include <SDL.hpp>
#include <iostream>
#include <filesystem>

#include <bit>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <unordered_map>

#include "Computer.h"
#include "Device.h"
#include "MemoryMapped.h"
#include "MappedDevice.h"
#include "SR_String.h"

#include "ComputerInfo.h"
#include "CharDisplay.h"
#include "ColourCharDisplay.h"
#include "KeyboardDevice.h"
#include "RAM.h"
#include "ROM.h"

#include "Assembler.h"
#include "RISCCore.h"

#include "GUIButton.h"
#include "PickFile.h"
#include "Sprite.h"

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

const Colour neutral_colour(255, 106, 0);
const Colour hover_colour(242, 96, 0);
const Colour click_colour(216, 86, 0);

int main(int argc, char* argv[]) {
	Init();
	IMG::Init(IMG_INIT_PNG);
	Input::Init();

	const Point scale { 4,4 };
	const Point text_size { 16,16 };
	const Point char_size { 8,8 };

	int frame_delay = 16;
	int clock_count = 1000;
	bool manually_clocked = false;
	int selected_palette = 0;
	int clocks = 0;

	Rect button_area = { { 0, text_size.y * char_size.y * scale.y }, { text_size.x * char_size.x * scale.x, 80 } };

	Window w;
	Renderer r;

	Point window_size = text_size * char_size * scale;
	window_size.h += 80;

	CreateWindowAndRenderer(window_size, w, r);

	const Uint32 wID = w.GetID();

	/*Listener<const Event&> window_resizer(
		[&](const Event& e)
		{
			if (e.window.event != SDL_WINDOWEVENT_RESIZED) return;
			if (e.window.windowID != wID) return;
			
			window_size = { e.window.data1, e.window.data2 };
		}	
	);

	Input::RegisterEventType(Event::Type::WINDOWEVENT, window_resizer);*/

	GUIButton reload_button(
		Button::LEFT,
		wID,
		Rect(
			button_area.pos,
			Point(lround(( button_area.w * 1 ) / 5.f), button_area.h)
		),
		neutral_colour,
		hover_colour,
		click_colour
	);

	GUIButton pause_button(
		Button::LEFT,
		wID,
		Rect(
			button_area.pos + Point(lround(( button_area.w * 1 ) / 5.f), 0),
			Point(lround(( button_area.w * 2 ) / 5.f) - lround(( button_area.w * 1 ) / 5.f), button_area.h)
		),
		neutral_colour,
		hover_colour,
		click_colour
	);

	GUIButton step_button(
		Button::LEFT,
		wID,
		Rect(
			button_area.pos + Point(lround(( button_area.w * 2 ) / 5.f), 0),
			Point(lround(( button_area.w * 3 ) / 5.f) - lround(( button_area.w * 2 ) / 5.f), button_area.h)
		),
		neutral_colour,
		hover_colour,
		click_colour
	);

	GUIButton palette_button(
		Button::LEFT,
		wID,
		Rect(
			button_area.pos + Point(lround(( button_area.w * 3 ) / 5.f),0),
			Point(lround(( button_area.w * 4 ) / 5.f) - lround(( button_area.w * 3 ) / 5.f), button_area.h)
		),
		neutral_colour,
		hover_colour,
		click_colour
	);

	GUIButton file_button(
		Button::LEFT,
		wID,
		Rect(
			button_area.pos + Point(lround(( button_area.w * 4 ) / 5.f), 0),
			Point(lround(( button_area.w * 5 ) / 5.f) - lround(( button_area.w * 4 ) / 5.f), button_area.h)
		),
		neutral_colour,
		hover_colour,
		click_colour
	);

	Texture char_set = IMG::LoadTexture(r, "Char set.png");

	float multiple = 0.7;

	Sprite reload {
		IMG::LoadTexture(r, "reload.png"),
		{
			reload_button.area.x + lround((reload_button.area.w - reload_button.area.h * multiple ) / 2.f),
			reload_button.area.y + lround(reload_button.area.h * (1.f - multiple) / 2.f),
			lround(reload_button.area.h * multiple),
			lround(reload_button.area.h * multiple)
		}
	};
	Sprite play {
		IMG::LoadTexture(r, "play.png"),
		{
			pause_button.area.x + lround(( pause_button.area.w - pause_button.area.h * multiple ) / 2.f),
			pause_button.area.y + lround(pause_button.area.h * (1.f - multiple) / 2.f),
			lround(pause_button.area.h * multiple),
			lround(pause_button.area.h * multiple)
		}
	};
	Sprite pause {
		IMG::LoadTexture(r, "pause.png"),
		play.shape
	};
	Sprite step {
		IMG::LoadTexture(r, "step.png"),
		{
			step_button.area.x + lround(( step_button.area.w - step_button.area.h * multiple ) / 2.f),
			step_button.area.y + lround(step_button.area.h * ( 1.f - multiple ) / 2.f),
			lround(step_button.area.h* multiple),
			lround(step_button.area.h * multiple)
		}
	};
	Sprite palette {
		IMG::LoadTexture(r, "p_button.png"),
		{
			palette_button.area.x + lround(( palette_button.area.w - palette_button.area.h * multiple ) / 2.f),
			palette_button.area.y + lround(palette_button.area.h * ( 1.f - multiple ) / 2.f),
			lround(palette_button.area.h* multiple),
			lround(palette_button.area.h * multiple)
		}
	};
	Sprite folder {
		IMG::LoadTexture(r, "folder.png"),
		{
			file_button.area.x + lround(( file_button.area.w - file_button.area.h * multiple ) / 2.f),
			file_button.area.y + lround(file_button.area.h * ( 1.f - multiple ) / 2.f),
			lround(file_button.area.h* multiple),
			lround(file_button.area.h * multiple)
		}
	};

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

	Input::RegisterUntyped(keyboard);

	address += cram.GetRange();

	computer.AddMapping(info);
	computer.AddMappedDevice(ram);
	computer.AddMappedDevice(cram);
	computer.AddMappedDevice(keyboard);

	Assembler assembler;

	std::filesystem::path file_path;
	bool file_provided = false;

	assembler.AddLabel("CHAR_MEM", cram.address_start);
	assembler.AddLabel("COLOUR_MEM", cram.address_start + cram.colour_position);
	assembler.AddLabel("RENDER_INT", cram.address_start + cram.interrupt_position);
	assembler.AddLabel("KEYBOARD", keyboard.address_start);
	assembler.SetRAM(ram);

	ram.Write(0, 0b00001010000000000000000000000000); // HALT

	computer.core = &core;
	computer.start_PC = ram.address_start;
	computer.start_SP = ram.address_start + ram.address_size;
	computer.SoftReset();

	Listener<const Point, const Uint32> reloader(
		[&](const Point, const Uint32)
		{
			if (!file_provided) return;

			computer.HardReset();
			assembler.SetRAM(ram);
			assembler.FlushScopes();

			std::ifstream program;

			program.open(file_path);

			if (!program.is_open())
			{
				std::wcout << L"Could not open assembly file '" << file_path << "'\n" << std::endl;
				return;
			}

			try
			{
				assembler.Assemble(program);
				program.close();
			}
			catch (const Assembler::FormatException& e)
			{
				std::string l { e.line };
				program.close();
				printf("%s\n", e.message.c_str());
				return;
			}

			computer.start_PC = assembler.entry_point;
			computer.SoftReset();

			printf("Program memory:\n");
			DisassembleMemory(computer, assembler.program_start, assembler.program_end);
			PrintMemory(computer, assembler.data_start, assembler.data_end, 0, true);
			printf("\n");

			Assembler::token_list var_out;

			if (assembler.GetVariable("clock_count", var_out) && var_out.size() > 0 && var_out.front().type == Assembler::TokenType::INTEGER)
			{
				clock_count = stoul(var_out.front().token);
			}

			if (assembler.GetVariable("frame_delay", var_out) && var_out.size() > 0 && var_out.front().type == Assembler::TokenType::INTEGER)
			{
				frame_delay = stoul(var_out.front().token);
			}
		}
	);

	Listener<const Point, const Uint32> pause_toggler(
		[&](const Point, const Uint32)
		{
			manually_clocked = !manually_clocked;
			clocks = 0;
		}
	);

	Listener<const Point, const Uint32> stepper(
		[&](const Point, const Uint32)
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
		}
	);

	Listener<const Point, const Uint32> palette_swapper(
		[&](const Point, const Uint32)
		{
			selected_palette = ( selected_palette + 1 ) % palette_count;
			memcpy(cram.colours, palettes[selected_palette], sizeof(SDL::Colour) * 16);
			cram.Render(false);
		}
	);

	Listener<const Point, const Uint32> file_picker(
		[&](const Point, const Uint32)
		{
			computer.HardReset();
			assembler.SetRAM(ram);
			assembler.FlushScopes();

			std::wstring file_name;
			if (FAILED(PickFile(file_name))) return;

			std::ifstream program;
			auto new_file_path = std::filesystem::path(file_name);
			program.open(new_file_path);

			if (!program.is_open())
			{
				std::wcout << L"Could not open assembly file '" << file_name << "'\n" << std::endl;
				return;
			}

			file_path = new_file_path;
			assembler.FlushScopes();

			try
			{
				assembler.Assemble(program);
				program.close();
			}
			catch (const Assembler::FormatException& e)
			{
				std::string l { e.line };
				program.close();
				printf("%s\n", e.message.c_str());
				return;
			}

			Assembler::token_list var_out;

			if (assembler.GetVariable("clock_count", var_out) && var_out.size() > 0 && var_out.front().type == Assembler::TokenType::INTEGER)
			{
				clock_count = stoul(var_out.front().token);
			}

			if (assembler.GetVariable("frame_delay", var_out) && var_out.size() > 0 && var_out.front().type == Assembler::TokenType::INTEGER)
			{
				frame_delay = stoul(var_out.front().token);
			}

			computer.start_PC = assembler.entry_point;
			computer.SoftReset();

			printf("Program memory:\n");
			// Disassemble program
			DisassembleMemory(computer, assembler.program_start, assembler.program_end);

			// Print values in memory
			PrintMemory(computer, assembler.data_start, assembler.data_end, 0, true);
			printf("\n");

			file_provided = true;
		}
	);

	reload_button.Register(reloader);
	pause_button.Register(pause_toggler);
	step_button.Register(stepper);
	palette_button.Register(palette_swapper);
	file_button.Register(file_picker);

	for (int frame = 0; Input::running; frame++)
	{
		Input::Update();

		reload_button.Render(r);
		pause_button.Render(r);
		step_button.Render(r);
		palette_button.Render(r);
		file_button.Render(r);

		reload.Render(r);
		step.Render(r);
		palette.Render(r);
		folder.Render(r);
		
		if (!manually_clocked)
		{
			pause.Render(r);

			computer.Clock(clock_count);
			cram.Render();

			r.Present();

			Delay(frame_delay);
		}
		else
		{
			play.Render(r);

			r.Present();
			Delay(10);
		}
	}

	Input::Quit();
	IMG::Quit();
	Quit();

	return 0;
}