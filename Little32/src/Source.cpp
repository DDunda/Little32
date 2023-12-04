#include <SDL.hpp>
#include <SDL_image.hpp>

#include "Little32.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace SDL;
using namespace Little32;

constexpr Colour neutral_colour(255, 106,   0, 255);
constexpr Colour hover_colour  (242,  96,   0, 255);
constexpr Colour click_colour  (216,  86,   0, 255);

constexpr Point scale { 4,4 };
constexpr Point text_size { 16,16 };
constexpr Point char_size { 8,8 };

constexpr Rect button_area
{
	{ 0, text_size.y * char_size.y * scale.y },
	{ text_size.x * char_size.x * scale.x, 80 }
};

constexpr word default_frame_delay = 16;
constexpr word default_clock_count = 1000;

void Program(int argc, char* argv[])
{
	bool running = true;
	bool manually_clocked = false;
	word frame_delay = default_frame_delay;
	word clock_count = default_clock_count;
	word clocks = 0;
	size_t selected_palette = 0;

	Window w;
	Renderer r;

	Point window_size = text_size * char_size * scale;
	window_size.h += 80;

	CreateWindowAndRenderer(window_size, w, r);

	const Uint32 wID = w.GetID();

	std::vector<std::array<Colour, 16>> palettes = {};
	LoadPalettes(palettes, "assets/palette.png");

	GUIButtonGroup buttons
	{
		{
			{
				Button::LEFT,
				wID,
				{
					button_area.pos,
					Point(lround((button_area.w * 1) / 5.f), button_area.h)
				},
				neutral_colour,
				hover_colour,
				click_colour
			}, // Reload
			{
				Button::LEFT,
				wID,
				{
					button_area.pos + Point(lround((button_area.w * 1) / 5.f), 0),
					Point(lround((button_area.w * 2) / 5.f) - lround((button_area.w * 1) / 5.f), button_area.h)
				},
				neutral_colour,
				hover_colour,
				click_colour
			}, // Pause / play
			{
				Button::LEFT,
				wID,
				{
					button_area.pos + Point(lround(( button_area.w * 2 ) / 5.f), 0),
					Point(lround(( button_area.w * 3 ) / 5.f) - lround(( button_area.w * 2 ) / 5.f), button_area.h)
				},
				neutral_colour,
				hover_colour,
				click_colour
			}, // Step
			{
				Button::LEFT,
				wID,
				{
					button_area.pos + Point(lround(( button_area.w * 3 ) / 5.f),0),
					Point(lround(( button_area.w * 4 ) / 5.f) - lround(( button_area.w * 3 ) / 5.f), button_area.h)
				},
				neutral_colour,
				hover_colour,
				click_colour
			}, // Palette
			{
				Button::LEFT,
				wID,
				{
					button_area.pos + Point(lround((button_area.w * 4) / 5.f), 0),
					Point(lround((button_area.w * 5) / 5.f) - lround((button_area.w * 4) / 5.f), button_area.h)
				},
				neutral_colour,
				hover_colour,
				click_colour
			}, // File
		}
	};

	Texture char_set = IMG::LoadTexture(r, "assets/char set.png");

	constexpr float multiple = 0.7f;

	std::map<const char* const, SDL::Texture> textures
	{
		{ "reload",  IMG::LoadTexture(r, "assets/buttons/reload.png" ) },
		{ "play",    IMG::LoadTexture(r, "assets/buttons/play.png"   ) },
		{ "pause",   IMG::LoadTexture(r, "assets/buttons/pause.png"  ) },
		{ "step",    IMG::LoadTexture(r, "assets/buttons/step.png"   ) },
		{ "palette", IMG::LoadTexture(r, "assets/buttons/palette.png") },
		{ "folder",  IMG::LoadTexture(r, "assets/buttons/folder.png" ) },
	};

	SpriteGroup sprites
	{
		{
			{
				textures["reload"],
				{
					buttons.buttons[0].area.x + (buttons.buttons[0].area.w - buttons.buttons[0].area.h * multiple) / 2.f,
					buttons.buttons[0].area.y + buttons.buttons[0].area.h * (1.f - multiple) / 2.f,
					buttons.buttons[0].area.h * multiple,
					buttons.buttons[0].area.h * multiple
				}
			}, // Reload
			{
				textures["play"],
				{
					buttons.buttons[1].area.x + (buttons.buttons[1].area.w - buttons.buttons[1].area.h * multiple) / 2.f,
					buttons.buttons[1].area.y + buttons.buttons[1].area.h * (1.f - multiple) / 2.f,
					buttons.buttons[1].area.h * multiple,
					buttons.buttons[1].area.h * multiple
				},
				false
			}, // Play
			{
				textures["pause"],
				{
					buttons.buttons[1].area.x + (buttons.buttons[1].area.w - buttons.buttons[1].area.h * multiple) / 2.f,
					buttons.buttons[1].area.y + buttons.buttons[1].area.h * (1.f - multiple) / 2.f,
					buttons.buttons[1].area.h * multiple,
					buttons.buttons[1].area.h * multiple
				}
			}, // Pause
			{
				textures["step"],
				{
					buttons.buttons[2].area.x + (buttons.buttons[2].area.w - buttons.buttons[2].area.h * multiple) / 2.f,
					buttons.buttons[2].area.y + buttons.buttons[2].area.h * (1.f - multiple) / 2.f,
					buttons.buttons[2].area.h * multiple,
					buttons.buttons[2].area.h * multiple
				}
			}, // Step
			{
				textures["palette"],
				{
					buttons.buttons[3].area.x + (buttons.buttons[3].area.w - buttons.buttons[3].area.h * multiple) / 2.f,
					buttons.buttons[3].area.y + buttons.buttons[3].area.h * (1.f - multiple) / 2.f,
					buttons.buttons[3].area.h * multiple,
					buttons.buttons[3].area.h * multiple
				}
			}, // Palette
			{
				textures["folder"],
				{
					buttons.buttons[4].area.x + (buttons.buttons[4].area.w - buttons.buttons[4].area.h * multiple) / 2.f,
					buttons.buttons[4].area.y + buttons.buttons[4].area.h * (1.f - multiple) / 2.f,
					buttons.buttons[4].area.h * multiple,
					buttons.buttons[4].area.h * multiple
				}
			} // Folder
		}
	};

	Computer computer;
	Little32Core core(computer);
	ComputerInfo info(computer);

	word address = 256;

	RAM ram(address, 4096);

	address += ram.GetRange();

	ColourCharDisplay cram
	(
		computer,
		r,
		char_set,
		palettes[selected_palette].data(),
		char_size,
		text_size.w,       // Characters per row of source texture
		text_size,
		scale,
		address,
		' ',
		0x0F  // White on black
	);

	address += cram.GetRange();

	KeyboardDevice keyboard
	(
		computer,
		address
	);

	address += keyboard.GetRange();

	computer.AddMapping(info);
	computer.AddMappedDevice(ram);
	computer.AddMappedDevice(cram);
	computer.AddMappedDevice(keyboard);

	Little32Assembler assembler;

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

	Listener<const Event&> quitter
	(
		[&](const Event& e) { running = false; },
		Input::GetTypedEventSubject(Event::Type::QUIT)
	);

	Listener<const Point, const Uint32> reloader
	(
		[&](const Point, const Uint32)
		{
			if (!file_provided) return;

			computer.HardReset();
			assembler.SetRAM(ram);
			assembler.FlushScopes();

			assembler.entry_point   = Little32Assembler::NULL_ADDRESS;
			assembler.program_start = Little32Assembler::NULL_ADDRESS;
			assembler.program_end   = Little32Assembler::NULL_ADDRESS;
			assembler.data_start    = Little32Assembler::NULL_ADDRESS;
			assembler.data_end      = Little32Assembler::NULL_ADDRESS;

			std::ifstream program;

			program.open(file_path);

			if (!program.is_open())
			{
				std::wcout << L"Could not open assembly file '" << file_path << "'\n" << std::endl;
				return;
			}

			try
			{
				assembler.Assemble(file_path, program);
				program.close();
			}
			catch (const Little32Assembler::FormatException& e)
			{
				std::string l { e.line };
				program.close();
				printf("%s\n", e.message.c_str());
				return;
			}

			if (assembler.entry_point != Little32Assembler::NULL_ADDRESS)
			{
				computer.start_PC = assembler.entry_point;
			}
			else if (assembler.program_start != Little32Assembler::NULL_ADDRESS)
			{
				computer.start_PC = assembler.program_start;
			}
			else
			{
				computer.start_PC = 0;
			}

			computer.SoftReset();

			printf("Program memory:\n");
			DisassembleMemory(computer, assembler.program_start, assembler.program_end);
			PrintMemory(computer, assembler.data_start, assembler.data_end, 0, true);
			printf("\n");

			Little32Assembler::TokenList var_out;

			if (assembler.GetVariable("clock_count", var_out) && var_out.size() > 0 && var_out.front().type == Little32Assembler::TokenType::INTEGER)
			{
				clock_count = stoul(var_out.front().token);
			}
			else
			{
				clock_count = default_clock_count;
			}

			if (assembler.GetVariable("frame_delay", var_out) && var_out.size() > 0 && var_out.front().type == Little32Assembler::TokenType::INTEGER)
			{
				frame_delay = stoul(var_out.front().token);
			}
			else
			{
				frame_delay = default_frame_delay;
			}
		},
		buttons.buttons[0]
	);

	Listener<const Point, const Uint32> pause_toggler
	(
		[&](const Point, const Uint32)
		{
			manually_clocked = !manually_clocked;
			clocks = 0;
			sprites.sprites[1].enabled = !sprites.sprites[1].enabled;
			sprites.sprites[2].enabled = !sprites.sprites[2].enabled;
		},
		buttons.buttons[1]
	);

	Listener<const Point, const Uint32> stepper
	(
		[&](const Point, const Uint32)
		{
			printf("PC: 0x%08X  SP: 0x%08X  LR: 0x%08X\n", core.PC, core.SP, core.LR);
			printf(" R0: % 10i  R1: % 10i  R2: % 10i  R3: % 10i\n", core.R0, core.R1, core.R2, core.R3);
			printf(" R4: % 10i  R5: % 10i  R6: % 10i  R7: % 10i\n", core.R4, core.R5, core.R6, core.R7);
			printf(" R8: % 10i  R9: % 10i R10: % 10i R11: % 10i\n", core.R8, core.R9, core.R10, core.R11);
			printf("R12: % 10i NZCV: %i%i%i%i \n", core.R12, core.N, core.Z, core.C, core.V);

			printf("0x%08X: %s\n\n", core.PC, core.Disassemble(computer.Read(core.PC)).c_str());

			//printf("0x%08X: ", core.PC);
			//word op = computer.Read(core.PC);
			//std::string diss = core.Disassemble(op);
			//printf("%s\n\n", diss.c_str());

			clocks++;
			clocks %= clock_count;
			computer.Clock(1);
			cram.Render(clocks == 0);
		},
		buttons.buttons[2]
	);

	Listener<const Point, const Uint32> palette_swapper
	(
		[&](const Point, const Uint32)
		{
			selected_palette = ( selected_palette + 1 ) % palettes.size();
			memcpy(cram.colours, palettes[selected_palette].data(), sizeof(SDL::Colour) * 16);
			cram.Render(false);
		},
		buttons.buttons[3]
	);

	Listener<const Point, const Uint32> file_picker
	(
		[&](const Point, const Uint32)
		{
			std::wstring file_name;
			if (FAILED(PickFile(file_name))) return;

			computer.HardReset();
			assembler.SetRAM(ram);
			assembler.FlushScopes();

			assembler.entry_point   = Little32Assembler::NULL_ADDRESS;
			assembler.program_start = Little32Assembler::NULL_ADDRESS;
			assembler.program_end   = Little32Assembler::NULL_ADDRESS;
			assembler.data_start    = Little32Assembler::NULL_ADDRESS;
			assembler.data_end      = Little32Assembler::NULL_ADDRESS;

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
				assembler.Assemble(file_path, program);
				program.close();
			}
			catch (const Little32Assembler::FormatException& e)
			{
				std::string l { e.line };
				program.close();
				printf("%s\n", e.message.c_str());
				return;
			}
			catch (const std::exception& e)
			{
				program.close();
				printf("%s\n", e.what());
				return;
			}

			Little32Assembler::TokenList var_out;

			if (assembler.GetVariable("clock_count", var_out) && var_out.size() > 0 && var_out.front().type == Little32Assembler::TokenType::INTEGER)
			{
				clock_count = stoul(var_out.front().token);
			}
			else
			{
				clock_count = default_clock_count;
			}

			if (assembler.GetVariable("frame_delay", var_out) && var_out.size() > 0 && var_out.front().type == Little32Assembler::TokenType::INTEGER)
			{
				frame_delay = stoul(var_out.front().token);
			}
			else
			{
				frame_delay = default_frame_delay;
			}

			if (assembler.entry_point != Little32Assembler::NULL_ADDRESS)
			{
				computer.start_PC = assembler.entry_point;
			}
			else if (assembler.program_start != Little32Assembler::NULL_ADDRESS)
			{
				computer.start_PC = assembler.program_start;
			}
			else
			{
				computer.start_PC = 0;
			}

			computer.SoftReset();

			printf("Program memory:\n");
			// Disassemble program
			DisassembleMemory(computer, assembler.program_start, assembler.program_end);

			// Print values in memory
			PrintMemory(computer, assembler.data_start, assembler.data_end, 0, true);
			printf("\n");

			file_provided = true;
		},
		buttons.buttons[4]
	);

	sprites.SetScaleMode(Texture::ScaleMode::Best);

	for (int frame = 0; running; frame++)
	{
		Input::Update();

		buttons.Render(r);
		sprites.Render();
		
		if (!manually_clocked)
		{
			computer.Clock(clock_count);
			cram.Render();

			r.Present();

			Delay(frame_delay);
		}
		else
		{
			r.Present();
			Delay(10);
		}
	}
}

int main(int argc, char* argv[])
{
	Init();
	IMG::Init(IMG::InitFlags::JPG | IMG::InitFlags::PNG);
	Input::Init();

	Program(argc, argv);

	Input::Quit();
	IMG::Quit();
	SDL::Quit();

	return 0;
}