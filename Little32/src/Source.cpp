#include <SDL.hpp>
#include <SDL_image.hpp>

#include "Little32.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace SDL;

namespace Little32
{
	struct Program
	{
		struct Settings
		{
			word start_address;
			word start_SP;

			bool ram_set = false;
			word ram_address;
			word ram_bytes;

			bool rom_set = false;
			word rom_address;
			word rom_bytes;

			std::vector<IDeviceSettings> components;

			Colour neutral_colour;
			Colour hover_colour;
			Colour click_colour;

			uint32_t frame_delay;
			uint32_t clocks_per_frame;

			SDL::Point viewport_size;

			std::vector<std::array<Colour, 16>> palettes = {};

			inline bool operator==(const Settings& other) const
			{
				if (!( start_address == other.start_address
					&& start_SP == other.start_SP
					&& ram_set == other.ram_set
					&& ram_address == other.ram_address
					&& ram_bytes == other.ram_bytes
					&& rom_set == other.rom_set
					&& rom_address == other.rom_address
					&& rom_bytes == other.rom_bytes
					&& components.size() == other.components.size()
					&& neutral_colour == other.neutral_colour
					&& hover_colour == other.hover_colour
					&& click_colour == other.click_colour
					&& frame_delay == other.frame_delay
					&& clocks_per_frame == other.clocks_per_frame
					&& viewport_size == other.viewport_size
					&& palettes.size() == other.palettes.size())) return false;

				for (size_t i = components.size(); i--;)
				{
					if (components[i] != other.components[i]) return false;
				}

				for (size_t i = palettes.size(); i--;)
				{
					if (!( palettes[i][ 0] == other.palettes[i][ 0]
						&& palettes[i][ 1] == other.palettes[i][ 1]
						&& palettes[i][ 2] == other.palettes[i][ 2]
						&& palettes[i][ 3] == other.palettes[i][ 3]
						&& palettes[i][ 4] == other.palettes[i][ 4]
						&& palettes[i][ 5] == other.palettes[i][ 5]
						&& palettes[i][ 6] == other.palettes[i][ 6]
						&& palettes[i][ 7] == other.palettes[i][ 7]
						&& palettes[i][ 8] == other.palettes[i][ 8]
						&& palettes[i][ 9] == other.palettes[i][ 9]
						&& palettes[i][10] == other.palettes[i][10]
						&& palettes[i][11] == other.palettes[i][11]
						&& palettes[i][12] == other.palettes[i][12]
						&& palettes[i][13] == other.palettes[i][13]
						&& palettes[i][14] == other.palettes[i][14]
						&& palettes[i][15] == other.palettes[i][15]
						)) return false;
				}

				return true;
			}

			inline bool operator!=(const Settings& other) const
			{
				return !(*this == other);
			}
		};

		Little32Assembler assembler;

		Settings default_settings =
		{
			0, // start_address
			0, // start_SP

			true, // ram_set
			0,    // ram_address
			8192, // ram_bytes

			true, // rom_set
			8192, // rom_address
			8192, // rom_bytes

			{ // components
				IDeviceSettings{
					0,
					"ROM",
					{ { "size_words", VarValue(2048) } },
					{},
					{}
				},
				IDeviceSettings{
					1,
					"RAM",
					{ { "size_words", VarValue(2048) } },
					{},
					{}
				},
				IDeviceSettings{
					2,
					"Colour Character Display",
					{
						{ "texture_file", VarValue("./assets/char set.png") },
						{ "texture_position", VarValue(SDL::Point(0,0)) },
						{ "texture_char_size", VarValue(SDL::Point(8,8)) },
						{ "texture_columns", VarValue(16) },

						{ "text_size", VarValue(SDL::Point(16,16)) },

						{ "pixel_position", VarValue(SDL::Point(0,0)) },
						{ "pixel_scale", VarValue(SDL::Point(4,4)) },

						// Modes:
						// 0 - Locked to cpu cycles
						// 1 - Locked to FPS
						// 2 - Locked to time per frame
						// 3 - VSYNC ?
						{ "framerate_mode", VarValue(0) },

						// 0 - Cycles per frame
						// 1 - Target FPS
						// 2 - ms per frame
						// 3 - Ignored
						{ "framerate_lock", VarValue(3000) }
					},
					{
						{ "COLOUR_MEM", { "colour_position" } },
						{ "RENDER_INT", { "interrupt_position" } }
					},
					{
						{ "CHAR_MEM", 0 }
					}
				},
				{
					3,
					"Keyboard",
					{},
					{},
					{
						{ "KEYBOARD", 0 }
					}
				}
			},

			{ 255, 106,   0, 255 }, // neutral_colour
			{ 242,  96,   0, 255 }, // hover_colour
			{ 216,  86,   0, 255 }, // click_colour

			16, // frame_delay
			1000, // clocks_per_frame

			{ 512, 512 }, // viewport_size

			{
				{{
					{  0,  0,  0}, {127,  0,  0}, {127, 51,  0}, {127,106,  0}, {  0,127,  0}, {  0,  0,127}, { 87,  0,127}, {127,127,127},
					{ 64, 64, 64}, {255,  0,  0}, {255,106,  0}, {255,216,  0}, {  0,255,  0}, {  0,  0,255}, {178,  0,255}, {255,255,255}
				}},
				{{
					{  0,  0,  0}, { 17, 17, 17}, { 34, 34, 34}, { 51, 51, 51}, { 68, 68, 68}, { 85, 85, 85}, {102,102,102}, {119,119,119},
					{136,136,136}, {153,153,153}, {170,170,170}, {187,187,187}, {204,204,204}, {221,221,221}, {238,238,238}, {255,255,255}
				}}
			} // palettes
		};
		Settings settings;

		inline static constexpr int button_height = 80;
		inline static constexpr float button_padding = 0.15f;

		size_t selected_palette = 0;

		GUIButtonGroup buttons;
		SpriteGroup sprites;

		Uint32 wID;

		Window w;
		Renderer r;

		Point window_size;

		Computer computer;
		Little32Core core;

		Program() : assembler(), computer(), core(computer)
		{
			assembler.SetComputer(computer);
			computer.core = &core;
		}

		const std::unordered_map<std::string, const IDeviceFactory* const> device_type_factories =
		{
			{ "Empty", new EmptyDeviceFactory() },
			{ "RAM", new RAMFactory() },
			{ "ROM", new ROMFactory() },
			{ "Colour Character Display", new ColourCharDisplayFactory() },
			{ "Keyboard", new KeyboardDeviceFactory() }
		};

		// Assumes that incoming data is valid. Make sure it is.
		void ApplySettings(const Settings& new_settings, std::filesystem::path config_path)
		{
			if (settings == new_settings) return;

			settings.frame_delay = new_settings.frame_delay;
			settings.clocks_per_frame = new_settings.clocks_per_frame;

			settings.ram_set = new_settings.ram_set;
			settings.rom_set = new_settings.rom_set;

			if (settings.start_address != new_settings.start_address)
			{
				settings.start_address = new_settings.start_address;
				computer.start_PC = settings.start_address;
			}

			if (settings.start_SP != new_settings.start_SP)
			{
				settings.start_SP = new_settings.start_SP;
				computer.start_SP = settings.start_SP;
			}

			if (settings.ram_set)
			{
				settings.ram_address = new_settings.ram_address;
				settings.ram_bytes = new_settings.ram_bytes;
				assembler.SetRAM(settings.ram_address, settings.ram_bytes);
			}
			else
			{
				assembler.ClearRAM();
			}

			if (settings.rom_set)
			{
				settings.rom_address = new_settings.rom_address;
				settings.rom_bytes = new_settings.rom_bytes;
				assembler.SetROM(settings.rom_address, settings.rom_bytes);
			}
			else
			{
				assembler.ClearROM();
			}
			
			assert(!new_settings.palettes.empty());

			if (new_settings.palettes.size() == settings.palettes.size())
			{
				// If the palettes are the same, keep the same selected palette
				// Otherwise, select the first palette of the new palettes
				for (size_t i = settings.palettes.size(); i--;)
				{
					for (size_t c = 16; c--;)
					{
						if (settings.palettes[i][c] != new_settings.palettes[i][c]) goto mismatch;
					}
				}
			}
			else
			{
				mismatch:

				selected_palette = 0;
				settings.palettes = new_settings.palettes;

				ColourCharDisplay::colours[ 0] = settings.palettes[0][ 0];
				ColourCharDisplay::colours[ 1] = settings.palettes[0][ 1];
				ColourCharDisplay::colours[ 2] = settings.palettes[0][ 2];
				ColourCharDisplay::colours[ 3] = settings.palettes[0][ 3];
				ColourCharDisplay::colours[ 4] = settings.palettes[0][ 4];
				ColourCharDisplay::colours[ 5] = settings.palettes[0][ 5];
				ColourCharDisplay::colours[ 6] = settings.palettes[0][ 6];
				ColourCharDisplay::colours[ 7] = settings.palettes[0][ 7];
				ColourCharDisplay::colours[ 8] = settings.palettes[0][ 8];
				ColourCharDisplay::colours[ 9] = settings.palettes[0][ 9];
				ColourCharDisplay::colours[10] = settings.palettes[0][10];
				ColourCharDisplay::colours[11] = settings.palettes[0][11];
				ColourCharDisplay::colours[12] = settings.palettes[0][12];
				ColourCharDisplay::colours[13] = settings.palettes[0][13];
				ColourCharDisplay::colours[14] = settings.palettes[0][14];
				ColourCharDisplay::colours[15] = settings.palettes[0][15];
			}

			if (new_settings.neutral_colour != settings.neutral_colour)
			{
				settings.neutral_colour = new_settings.neutral_colour;

				for (auto& button : buttons.buttons)
				{
					button.neutral_colour = settings.neutral_colour;
				}
			}

			if (new_settings.hover_colour != settings.hover_colour)
			{
				settings.hover_colour = new_settings.hover_colour;

				for (auto& button : buttons.buttons)
				{
					button.hover_colour = settings.hover_colour;
				}
			}

			if (new_settings.click_colour != settings.click_colour)
			{
				settings.click_colour = new_settings.click_colour;

				for (auto& button : buttons.buttons)
				{
					button.click_colour = settings.click_colour;
				}
			}

			if (new_settings.viewport_size != settings.viewport_size)
			{
				assert(new_settings.viewport_size.w > 0);
				assert(new_settings.viewport_size.h > 0);

				settings.viewport_size = new_settings.viewport_size;

				sprites.viewport = { {0,settings.viewport_size.h}, {settings.viewport_size.w, button_height} };

				w.SetSize({ settings.viewport_size.w, settings.viewport_size.h + button_height });

				for (size_t i = 0; i < 5; i++)
				{
					buttons.buttons[i].area =
					{
						lround((settings.viewport_size.w * i) / 5.f),
						settings.viewport_size.h,
						lround((settings.viewport_size.w * (i + 1)) / 5.f) -
						lround((settings.viewport_size.w * (i + 0)) / 5.f),
						button_height
					};
				}

				const size_t sprite_buttons[6] = { 0,1,1,2,3,4 };

				const float button_size = std::min(button_height * 1.f, settings.viewport_size.w / 5.0f) * (1.f - button_padding);

				for (size_t i = 0; i < 6; i++)
				{
					auto& b = buttons.buttons[sprite_buttons[i]].area;
					sprites.sprites[i].shape =
					{
						b.x + (b.w - button_size) / 2.f,
						(b.h - button_size) / 2.f,
						button_size,
						button_size,
					};
				}
			}

			if (settings.components.size() == new_settings.components.size())
			{
				for (size_t i = settings.components.size(); i--;)
				{
					if (settings.components[i] != new_settings.components[i])
					{
						goto components_unequal;
					}
				}
			}
			else
			{
				components_unequal:
				settings.components = new_settings.components;

				for (auto*& d : computer.devices)
				{
					delete d;
				}
				for (auto*& md : computer.mapped_devices)
				{
					delete md;
				}
				for (auto*& m : computer.mappings)
				{
					delete m;
				}

				computer.devices.clear();
				computer.mapped_devices.clear();
				computer.mappings.clear();

				assembler.ClearLabels();

				word address = settings.start_address;

				std::unordered_map<std::string, word> labels;

				for (const auto& component : settings.components)
				{
					for (const auto& [label, offset] : component.relative_labels)
					{
						assert(!labels.contains(label));
						labels[label] = address + offset;
					}

					assert(device_type_factories.contains(component.component_type));

					device_type_factories.at(component.component_type)->CreateFromSettings(computer, address, component, labels, config_path);
				}

				assembler.AddLabels(labels);
			}
		}

		// Assumes that incoming data is valid. Make sure it is.
		void OverwriteSettings(const Settings& new_settings, std::filesystem::path config_path)
		{
			settings = new_settings;

			computer.start_PC = settings.start_address;
			computer.start_SP = settings.start_SP;

			if (settings.ram_set)
			{
				assembler.SetRAM(settings.ram_address, settings.ram_bytes);
			}
			else
			{
				assembler.ClearRAM();
			}

			if (settings.rom_set)
			{
				assembler.SetROM(settings.rom_address, settings.rom_bytes);
			}
			else
			{
				assembler.ClearROM();
			}

			assert(!settings.palettes.empty());

			selected_palette = 0;

			ColourCharDisplay::colours[ 0] = settings.palettes[0][ 0];
			ColourCharDisplay::colours[ 1] = settings.palettes[0][ 1];
			ColourCharDisplay::colours[ 2] = settings.palettes[0][ 2];
			ColourCharDisplay::colours[ 3] = settings.palettes[0][ 3];
			ColourCharDisplay::colours[ 4] = settings.palettes[0][ 4];
			ColourCharDisplay::colours[ 5] = settings.palettes[0][ 5];
			ColourCharDisplay::colours[ 6] = settings.palettes[0][ 6];
			ColourCharDisplay::colours[ 7] = settings.palettes[0][ 7];
			ColourCharDisplay::colours[ 8] = settings.palettes[0][ 8];
			ColourCharDisplay::colours[ 9] = settings.palettes[0][ 9];
			ColourCharDisplay::colours[10] = settings.palettes[0][10];
			ColourCharDisplay::colours[11] = settings.palettes[0][11];
			ColourCharDisplay::colours[12] = settings.palettes[0][12];
			ColourCharDisplay::colours[13] = settings.palettes[0][13];
			ColourCharDisplay::colours[14] = settings.palettes[0][14];
			ColourCharDisplay::colours[15] = settings.palettes[0][15];

			for (auto& button : buttons.buttons)
			{
				button.neutral_colour = settings.neutral_colour;
				button.hover_colour = settings.hover_colour;
				button.click_colour = settings.click_colour;
			}

			assert(settings.viewport_size.w > 0);
			assert(settings.viewport_size.h > 0);

			sprites.viewport = { {0,settings.viewport_size.h}, {settings.viewport_size.w, button_height} };

			w.SetSize({ settings.viewport_size.w, settings.viewport_size.h + button_height });

			for (size_t i = 0; i < 5; i++)
			{
				buttons.buttons[i].area =
				{
					lround((settings.viewport_size.w * i) / 5.f),
					settings.viewport_size.h,
					lround((settings.viewport_size.w * (i + 1)) / 5.f) -
					lround((settings.viewport_size.w * (i + 0)) / 5.f),
					button_height
				};
			}

			const size_t sprite_buttons[6] = { 0,1,1,2,3,4 };

			const float button_size = std::min(button_height * 1.f, settings.viewport_size.w / 5.0f) * (1.f - button_padding);

			for (size_t i = 0; i < 6; i++)
			{
				auto& b = buttons.buttons[sprite_buttons[i]].area;
				sprites.sprites[i].shape =
				{
					b.x + (b.w - button_size) / 2.f,
					(b.h - button_size) / 2.f,
					button_size,
					button_size,
				};
			}

			for (auto*& d : computer.devices)
			{
				delete d;
			}
			for (auto*& md : computer.mapped_devices)
			{
				delete md;
			}
			for (auto*& m : computer.mappings)
			{
				delete m;
			}

			computer.devices.clear();
			computer.mapped_devices.clear();
			computer.mappings.clear();

			assembler.ClearLabels();

			word address = settings.start_address;

			std::unordered_map<std::string, word> labels;

			for (const auto& component : settings.components)
			{
				for (const auto& [label, offset] : component.relative_labels)
				{
					assert(!labels.contains(label));
					labels[label] = address + offset;
				}

				assert(device_type_factories.contains(component.component_type));

				device_type_factories.at(component.component_type)->CreateFromSettings(computer, address, component, labels, config_path);
			}

			assembler.AddLabels(labels);
		}

		size_t LoadSettings(Settings& settings, const ConfigObject& new_settings, std::filesystem::path config_path, const bool throw_errors = true) const
		{
			uint32_t tmp_uint;
			BigInt tmp_bint;
			std::string tmp_str;
			const ConfigObject* tmp_obj;
			const std::vector<VarValue>* tmp_list;
			SDL::Point tmp_vec;

			size_t exceptions = 0;

			if (new_settings.TryFindUInt32("ram_bytes", tmp_uint))
			{
				if (new_settings.TryFindUInt32("ram_address", settings.ram_address))
				{
					settings.ram_bytes = tmp_uint;
					settings.ram_set = true;
				}
				else if (tmp_uint == 0)
				{
					settings.ram_set = false;
				}
				else
				{
					if (throw_errors) throw std::exception("RAM size set, but not address ('ram_address')");
					std::cout << "RAM size set, but not address ('ram_bytes')" << std::endl;
					++exceptions;
				}
			}
			else if (new_settings.TryFindUInt32("ram_address", tmp_uint))
			{
				if (throw_errors) throw std::exception("RAM address set, but not size ('ram_bytes')");
				std::cout << "RAM address set, but not size ('ram_bytes')" << std::endl;
				++exceptions;
			}

			if (new_settings.TryFindUInt32("rom_bytes", tmp_uint))
			{
				if (new_settings.TryFindUInt32("rom_address", settings.rom_address))
				{
					settings.rom_bytes = tmp_uint;
					settings.rom_set = true;
				}
				else if (tmp_uint == 0)
				{
					settings.rom_set = false;
				}
				else
				{
					if (throw_errors) throw std::exception("ROM size set, but not address ('rom_address')");
					std::cout << "ROM size set, but not address ('rom_bytes')" << std::endl;
					++exceptions;
				}
			}
			else if (new_settings.TryFindUInt32("rom_address", tmp_uint))
			{
				if (throw_errors) throw std::exception("ROM address set, but not size ('rom_bytes')");
				std::cout << "ROM address set, but not size ('rom_bytes')" << std::endl;
				++exceptions;
			}

			new_settings.TryFindUInt32("start_address", settings.start_address);
			new_settings.TryFindUInt32("stack_address", settings.start_SP);

			if (new_settings.TryFindString("palettes", tmp_str))
			{
				auto relative_to_program = std::filesystem::current_path() / tmp_str;
				auto relative_to_config = config_path / tmp_str;

				SDL::Surface surf = IMG::Load(relative_to_config.string().c_str());
				if (surf.surface.get() == NULL)
				{
					surf = IMG::Load(relative_to_program.string().c_str());
				}

				if (surf.surface.get() == NULL)
				{
					if (throw_errors) throw std::runtime_error("Could not open palette image at '" + tmp_str + "'");
					std::cout << "Could not open palette image at '" << tmp_str << '\'' << std::endl;
					++exceptions;
				}
				else if (surf.surface->w * surf.surface->h == 0)
				{
					if (throw_errors) throw std::runtime_error("Palette image has no pixels? (" + std::to_string(surf.surface->w) + 'x' + std::to_string(surf.surface->h) + " from '" + tmp_str + "')");
					std::cout << "Palette image has no pixels? (" << surf.surface->w << 'x' << surf.surface->h << " from '" << tmp_str << "')" << std::endl;
					++exceptions;
				}
				else if ((surf.surface->w * surf.surface->h) % 16 != 0)
				{
					if (throw_errors) throw std::runtime_error("Area of palette image is not multiple of 16 (" + std::to_string(surf.surface->w * surf.surface->h) + " pixels from '" + tmp_str + "')");
					std::cout << "Area of palette image is not multiple of 16 (" << (surf.surface->w * surf.surface->h) << " pixels from '" << tmp_str << "')";
					++exceptions;
				}
				else LoadPalettes(settings.palettes, surf);
			}

			if (new_settings.TryFindInteger("frame_delay", tmp_bint))
			{
				if (tmp_bint.NumBits() > 32)
				{
					if (throw_errors) throw std::runtime_error("Frame delay must fit into 32 bits (" + tmp_bint.ToStringCheap() + ')');
					std::cout << "Frame delay must fit into 32 bits (" << tmp_bint.ToStringCheap() << ')' << std::endl;
					++exceptions;
				}
				else if (tmp_bint.negative || tmp_bint.bits.empty())
				{
					if (throw_errors) throw std::runtime_error("Frame delay must be greater than zero (" + tmp_bint.ToStringCheap() + ')');
					std::cout << "Frame delay must be greater than zero (" << tmp_bint.ToStringCheap() << ')' << std::endl;
					++exceptions;
				}
				else
				{
					settings.frame_delay = static_cast<uint32_t>(tmp_bint.bits[0]);
				}
			}

			if (new_settings.TryFindInteger("clocks_per_frame", tmp_bint))
			{
				if (tmp_bint.NumBits() > 32)
				{
					if (throw_errors) throw std::runtime_error("Clocks per frame must fit into 32 bits (" + tmp_bint.ToStringCheap() + ')');
					std::cout << "Clocks per frame must fit into 32 bits (" << tmp_bint.ToStringCheap() << ')' << std::endl;
					++exceptions;
				}
				else if (tmp_bint.negative || tmp_bint.bits.empty())
				{
					if (throw_errors) throw std::runtime_error("Clocks per frame must be greater than zero (" + tmp_bint.ToStringCheap() + ')');
					std::cout << "Clocks per frame must be greater than zero (" << tmp_bint.ToStringCheap() << ')' << std::endl;
					++exceptions;
				}
				else
				{
					settings.clocks_per_frame = static_cast<uint32_t>(tmp_bint.bits[0]);
				}
			}

			if (new_settings.TryFindVector("viewport_size", tmp_vec))
			{
				if (tmp_vec.x <= 0 || tmp_vec.y <= 0)
				{
					if (throw_errors)
						throw std::runtime_error
						(
							"Viewport dimensions must be greater than zero (" +
								std::to_string(tmp_vec.x) +
							'x' +
								std::to_string(tmp_vec.y) +
							')'
						);
					std::cout << "Viewport dimensions must be greater than zero (" << tmp_vec.x << 'x' << tmp_vec.y << ')' << std::endl;
					++exceptions;
				}
				else
				{
					settings.viewport_size = tmp_vec;
					settings.viewport_size.w = std::max(settings.viewport_size.w, 200);
				}
			}

			if (new_settings.TryFindObject("gui_colours", tmp_obj))
			{
				tmp_obj->TryFindColour("neutral", settings.neutral_colour);
				tmp_obj->TryFindColour("hover", settings.hover_colour);
				tmp_obj->TryFindColour("click", settings.click_colour);
			}

			if (new_settings.TryFindList("components", tmp_list))
			{
				settings.components.clear();

				size_t component_num = 0;

				std::set<std::string> all_labels = {};

				for (const auto& c : *tmp_list)
				{
					++component_num;
					try
					{
						settings.components.push_back(IDeviceSettings(component_num - 1, new_settings, c, all_labels));
					}
					catch (const std::exception& e)
					{
						const std::string err_msg = "Parsing error in " + ToOrdinal(component_num) + " component: " + std::string(e.what());
						if (throw_errors) throw std::runtime_error(err_msg);
						std::cout << err_msg << std::endl;
						++exceptions;
						continue;
					}

					const IDeviceSettings& ids = settings.components.back();

					if (!device_type_factories.contains(ids.component_type))
					{
						const std::string err_msg = "Unknown device type in " + ToOrdinal(component_num) + " component: '" + ids.component_type + "'";
						if (throw_errors) throw std::runtime_error(err_msg);
						std::cout << err_msg << std::endl;
						settings.components.pop_back(); // Unusable device
						++exceptions;
						continue;
					}
						
					// Verify that these settings are well formed!
					try
					{
						device_type_factories.at(
							ids.component_type
						)->VerifySettings(ids, config_path);
					}
					catch (const std::exception& e)
					{
						const std::string err_msg = "Verification error in " + ToOrdinal(component_num) + " component: " + e.what();
						if (throw_errors) throw std::runtime_error(err_msg);
						std::cout << err_msg << std::endl;
						settings.components.pop_back(); // Remove settings; they are bad
						++exceptions;
						continue;
					}
				}
			}

			return exceptions;
		}

		size_t LoadSettings(std::filesystem::path config_path, const Settings& default_settings, const bool throw_errors, const bool force_overwrite)
		{
			config_path = std::filesystem::weakly_canonical(config_path);

			std::ifstream file_stream;
			file_stream.open(config_path);

			if (!file_stream.is_open())
			{
				std::cout << "Failed to open config file" << std::endl << "Using default settings" << std::endl << std::endl;
				if(force_overwrite) OverwriteSettings(default_settings, config_path);
				else ApplySettings(default_settings, config_path);
				return 1;
			}

			ConfigObject* obj = nullptr;
			VarValue* new_settings = nullptr;

			try
			{
				obj = ConfigParser::ParseFile(file_stream);
			}
			catch (const std::exception& e)
			{
				std::cout << e.what() << std::endl << "Using default settings" << std::endl << std::endl;
				if (force_overwrite) OverwriteSettings(default_settings, config_path);
				else ApplySettings(default_settings, config_path);
				return 1;
			}

			Settings settings_out = default_settings;

			try
			{
				size_t exceptions = LoadSettings(settings_out, *obj, config_path, throw_errors);

				new_settings = new VarValue(obj);

				std::cout << "Loaded settings with " << exceptions << " error/s:" << std::endl << new_settings->ToString(*obj, true, false, "") << std::endl << std::endl;

				delete new_settings; // deletes obj
				new_settings = nullptr;
				obj = nullptr;

				if (force_overwrite) OverwriteSettings(settings_out, config_path);
				else ApplySettings(settings_out, config_path);
				return exceptions;
			}
			catch (const std::exception& e)
			{
				if (new_settings != nullptr) delete new_settings;
				else if (obj != nullptr) delete obj;
				std::cout << e.what() << std::endl << "Using default settings" << std::endl << std::endl;
				if (force_overwrite) OverwriteSettings(default_settings, config_path);
				else ApplySettings(default_settings, config_path);
				return 1;
			}
		}

		void Run(int argc, char* argv[])
		{
			bool running = true;
			bool manually_clocked = false;
			word clocks = 0;

			settings = Settings();

			buttons =
			{
				{
					{
						Button::LEFT,
						wID,
						{
							{0,settings.viewport_size.h},
							Point(lround((settings.viewport_size.w * 1) / 5.f), button_height)
						},
						settings.neutral_colour,
						settings.hover_colour,
						settings.click_colour
					}, // Reload
					{
						Button::LEFT,
						wID,
						{
							Point(lround((settings.viewport_size.w * 1) / 5.f), settings.viewport_size.h),
							Point(lround((settings.viewport_size.w * 2) / 5.f) - lround((settings.viewport_size.w * 1) / 5.f), button_height)
						},
						settings.neutral_colour,
						settings.hover_colour,
						settings.click_colour
					}, // Pause / play
					{
						Button::LEFT,
						wID,
						{
							Point(lround((settings.viewport_size.w * 2) / 5.f), settings.viewport_size.h),
							Point(lround((settings.viewport_size.w * 3) / 5.f) - lround((settings.viewport_size.w * 2) / 5.f), button_height)
						},
						settings.neutral_colour,
						settings.hover_colour,
						settings.click_colour
					}, // Step
					{
						Button::LEFT,
						wID,
						{
							Point(lround((settings.viewport_size.w * 3) / 5.f), settings.viewport_size.h),
							Point(lround((settings.viewport_size.w * 4) / 5.f) - lround((settings.viewport_size.w * 3) / 5.f), button_height)
						},
						settings.neutral_colour,
						settings.hover_colour,
						settings.click_colour
					}, // Palette
					{
						Button::LEFT,
						wID,
						{
							Point(lround((settings.viewport_size.w * 4) / 5.f), settings.viewport_size.h),
							Point(lround((settings.viewport_size.w * 5) / 5.f) - lround((settings.viewport_size.w * 4) / 5.f), button_height)
						},
						settings.neutral_colour,
						settings.hover_colour,
						settings.click_colour
					}, // File
				},
			};

			std::unordered_map<std::string, SDL::Texture> textures
			{
				{ "reload",  ImageLoader::GetImage("assets/buttons/reload.png") },
				{ "play",    ImageLoader::GetImage("assets/buttons/play.png") },
				{ "pause",   ImageLoader::GetImage("assets/buttons/pause.png") },
				{ "step",    ImageLoader::GetImage("assets/buttons/step.png") },
				{ "palette", ImageLoader::GetImage("assets/buttons/palette.png") },
				{ "folder",  ImageLoader::GetImage("assets/buttons/folder.png") },
			};

			sprites =
			{
				{
					{
						textures["reload"],
						{
							buttons.buttons[0].area.x + (buttons.buttons[0].area.w - buttons.buttons[0].area.h * (1.f - 2.f * button_padding)) / 2.f,
							0 + buttons.buttons[0].area.h * button_padding,
							buttons.buttons[0].area.h * (1.f - 2.f * button_padding),
							buttons.buttons[0].area.h * (1.f - 2.f * button_padding)
						}
					}, // Reload
					{
						textures["play"],
						{
							buttons.buttons[1].area.x + (buttons.buttons[1].area.w - buttons.buttons[1].area.h * (1.f - 2.f * button_padding)) / 2.f,
							0 + buttons.buttons[1].area.h * button_padding,
							buttons.buttons[1].area.h * (1.f - 2.f * button_padding),
							buttons.buttons[1].area.h * (1.f - 2.f * button_padding)
						},
						false
					}, // Play
					{
						textures["pause"],
						{
							buttons.buttons[1].area.x + (buttons.buttons[1].area.w - buttons.buttons[1].area.h * (1.f - 2.f * button_padding)) / 2.f,
							0 + buttons.buttons[1].area.h * button_padding,
							buttons.buttons[1].area.h * (1.f - 2.f * button_padding),
							buttons.buttons[1].area.h * (1.f - 2.f * button_padding)
						}
					}, // Pause
					{
						textures["step"],
						{
							buttons.buttons[2].area.x + (buttons.buttons[2].area.w - buttons.buttons[2].area.h * (1.f - 2.f * button_padding)) / 2.f,
							0 + buttons.buttons[2].area.h * button_padding,
							buttons.buttons[2].area.h * (1.f - 2.f * button_padding),
							buttons.buttons[2].area.h * (1.f - 2.f * button_padding)
						}
					}, // Step
					{
						textures["palette"],
						{
							buttons.buttons[3].area.x + (buttons.buttons[3].area.w - buttons.buttons[3].area.h * (1.f - 2.f * button_padding)) / 2.f,
							0 + buttons.buttons[3].area.h * button_padding,
							buttons.buttons[3].area.h * (1.f - 2.f * button_padding),
							buttons.buttons[3].area.h * (1.f - 2.f * button_padding)
						}
					}, // Palette
					{
						textures["folder"],
						{
							buttons.buttons[4].area.x + (buttons.buttons[4].area.w - buttons.buttons[4].area.h * (1.f - 2.f * button_padding)) / 2.f,
							0 + buttons.buttons[4].area.h * button_padding,
							buttons.buttons[4].area.h * (1.f - 2.f * button_padding),
							buttons.buttons[4].area.h * (1.f - 2.f * button_padding)
						}
					} // Folder
				},
				{ { 0, settings.viewport_size.h }, { settings.viewport_size.w, button_height } }
			};

			LoadSettings(std::filesystem::current_path() / "computer.cfg", default_settings, false, true);
			default_settings = settings;

			ColourCharDisplay::colours[ 0] = settings.palettes[0][ 0];
			ColourCharDisplay::colours[ 1] = settings.palettes[0][ 1];
			ColourCharDisplay::colours[ 2] = settings.palettes[0][ 2];
			ColourCharDisplay::colours[ 3] = settings.palettes[0][ 3];
			ColourCharDisplay::colours[ 4] = settings.palettes[0][ 4];
			ColourCharDisplay::colours[ 5] = settings.palettes[0][ 5];
			ColourCharDisplay::colours[ 6] = settings.palettes[0][ 6];
			ColourCharDisplay::colours[ 7] = settings.palettes[0][ 7];
			ColourCharDisplay::colours[ 8] = settings.palettes[0][ 8];
			ColourCharDisplay::colours[ 9] = settings.palettes[0][ 9];
			ColourCharDisplay::colours[10] = settings.palettes[0][10];
			ColourCharDisplay::colours[11] = settings.palettes[0][11];
			ColourCharDisplay::colours[12] = settings.palettes[0][12];
			ColourCharDisplay::colours[13] = settings.palettes[0][13];
			ColourCharDisplay::colours[14] = settings.palettes[0][14];
			ColourCharDisplay::colours[15] = settings.palettes[0][15];

			w.SetSize(window_size);
			w.Show();
			w.SetInputFocus();

			std::filesystem::path file_path;
			bool file_provided = false;

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
					assembler.FlushScopes();
					assembler.ResetMemory();

					auto cfg_path = file_path;
					LoadSettings(cfg_path.replace_extension(".cfg"), default_settings, true, false);

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
					computer.Clock(1);
				},
				buttons.buttons[2]
			);

			Listener<const Point, const Uint32> palette_swapper
			(
				[&](const Point, const Uint32)
				{
					selected_palette = (selected_palette + 1) % settings.palettes.size();
					memcpy(ColourCharDisplay::colours, settings.palettes[selected_palette].data(), sizeof(SDL::Colour) * 16);
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
					assembler.FlushScopes();
					assembler.ResetMemory();

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
					assembler.ResetMemory();

					auto cfg_path = file_path;
					LoadSettings(cfg_path.replace_extension(".cfg"), default_settings, false, false);

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
					computer.Clock(settings.clocks_per_frame);

					r.Present();

					Delay(settings.frame_delay);
				}
				else
				{
					r.Present();
					Delay(10);
				}
			}
		}
	};
};

int main(int argc, char* argv[])
{
	SDL::Init();

	{
		Little32::Program prog;

		if (!CreateWindowAndRenderer({ 100, 100 }, prog.w, prog.r, WindowFlags::HIDDEN))
		{
			SDL::Quit();
			return -1;
		}

		IMG::Init(IMG::InitFlags::JPG | IMG::InitFlags::PNG);
		Input::Init();
		Little32::ImageLoader::Init(prog.r.renderer);

		prog.wID = prog.w.GetID();

		prog.Run(argc, argv);
	} // Destroy prog before quitting resources

	Little32::ImageLoader::Quit();
	Input::Quit();
	IMG::Quit();
	SDL::Quit();

	return 0;
}