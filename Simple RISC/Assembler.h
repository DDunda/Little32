#pragma once
#include <string>
#include <list>
#include <unordered_map>
#include <istream>
#include "RAM.h"
#include "ROM.h"
#include "SR_Types.h"

namespace SimpleRISC {

	class Assembler {
	public:
		using token_list = std::list<std::string>;

		Assembler() {
			variable_scopes = { {} };
			label_scopes = { {} };
			func_scopes = { const_replace };
		}

		class FormatException : public std::exception {
		public:
			const size_t line_no;
			const std::string message;
			const std::string line;
			const std::string inner_message;
			FormatException(size_t line_no, const std::string& line, const char* const message);
			FormatException(size_t line_no, const std::string& line, const std::string& message);
		};

		struct RawLine {
			size_t line_no;
			std::string* line;
		};

		struct AssemblyLine {
			const RawLine rline;
			word addr;
			word* mem;
			std::string code;
			std::list<token_list> args;

			bool has_cond = false;
			byte cond = 0;
			bool has_shift = false;
			byte shift = 0;

			bool N = false;
			bool S = false;
		};

		size_t total_variables_defined = 0;
		size_t total_variables_replaced = 0;
		size_t total_ops_defined = 0;
		size_t total_ops_replaced = 0;

		word entry_point = 0;
		word program_start = 0;
		word program_end = 0;
		word data_start = 0;
		word data_end = 0;

		static const char valid_text_chars[];

		void SetRAM(const RAM& ram);
		void SetRAM(word* memory, word start_address, word range);

		void SetROM(const ROM& rom);
		void SetROM(word* memory, word start_address, word range);

		void AddLabel(const std::string& label, word address);
		void AddLabels(const std::unordered_map<std::string, word>& addresses);
		
		bool GetLabel(const std::string& label, std::string& label_out) const;
		bool GetVariable(const std::string& variable, token_list& var_out) const;

		constexpr void ClearRAM() noexcept;

		constexpr void ClearROM() noexcept;

		void ClearLabels() noexcept;

		void FlushScopes() noexcept;

		void Assemble(std::istream& code, bool print_intermediate = false);

	protected:
		struct OpReplace {
			std::string op = "";
			std::string newop = "";
			int requiredArgs = -1;
			std::list<std::string> tokens{};

			bool has_cond = false;
			byte new_cond = 0;
			bool has_shift = false;
			byte new_shift = 0;

			bool newN = false;
			bool newS = false;
		};

		std::list<std::unordered_map<std::string, token_list>> variable_scopes;
		std::list<std::unordered_map<std::string, word>> label_scopes;
		std::list<std::list<OpReplace>> func_scopes;

		RawLine _current_line = {};

		word* ram = nullptr;
		word ram_start = 0;
		word ram_range = 0;
		word ram_current_address = 0;

		word* rom = nullptr;
		word rom_start = 0;
		word rom_range = 0;
		word rom_current_address = 0;

		std::unordered_map<std::string, word> constant_addresses = {};

		const std::list<OpReplace> const_replace = {
			{"HALT","B",   0,{"0"                            }            },
			{"STR", "RWW",-1,{"..."                          }            },
			{"LDR", "RRW",-1,{"..."                          }            },
			{"STRB","RWB",-1,{"..."                          }            },
			{"LDRB","RRB",-1,{"..."                          }            },
			{"PUSH","SWR",-1,{"SP",   ",","..."              }            },
			{"POP", "SRR",-1,{"SP",   ",","..."              }            },
			{"OR",  "ORR",-1,{"..."                          }            },
			{"ADD", "ADD", 2,{"@","0",",","@","0",",","@","1"}            },
			{"SUB", "SUB", 2,{"@","0",",","@","0",",","@","1"}            },
			{"INC", "ADD", 2,{"@","0",",","@","1",",","@","1"}            },
			{"DEC", "SUB", 2,{"@","0",",","@","1",",","@","1"}            },
			{"INC", "ADD", 1,{"@","0",",","@","0",",","@","1"}            },
			{"DEC", "SUB", 1,{"@","0",",","@","0",",","@","1"}            },
			{"BAL", "B",  -1,{"..."                          },true,0b0000},
			{"BGT", "B",  -1,{"..."                          },true,0b0001},
			{"BGE", "B",  -1,{"..."                          },true,0b0010},
			{"BHI", "B",  -1,{"..."                          },true,0b0011},
			{"BCS", "B",  -1,{"..."                          },true,0b0100},
			{"BHS", "B",  -1,{"..."                          },true,0b0100},
			{"BZS", "B",  -1,{"..."                          },true,0b0101},
			{"BEQ", "B",  -1,{"..."                          },true,0b0101},
			{"BNS", "B",  -1,{"..."                          },true,0b0110},
			{"BMI", "B",  -1,{"..."                          },true,0b0110},
			{"BVS", "B",  -1,{"..."                          },true,0b0111},
			{"BVC", "B",  -1,{"..."                          },true,0b1000},
			{"BNC", "B",  -1,{"..."                          },true,0b1001},
			{"BPL", "B",  -1,{"..."                          },true,0b1001},
			{"BZC", "B",  -1,{"..."                          },true,0b1010},
			{"BNE", "B",  -1,{"..."                          },true,0b1010},
			{"BCC", "B",  -1,{"..."                          },true,0b1011},
			{"BLO", "B",  -1,{"..."                          },true,0b1011},
			{"BLS", "B",  -1,{"..."                          },true,0b1100},
			{"BLT", "B",  -1,{"..."                          },true,0b1101},
			{"BLE", "B",  -1,{"..."                          },true,0b1110}
		};

		void ThrowException(const char* const msg) const;
		void ThrowException(const std::string& msg) const;
		bool GetCond(token_list& l, byte& cond) const;
		bool GetShift(token_list& l, byte& shift) const;
		word GetBranchOffset(token_list& l, byte shift, bool& isNegative) const;

		size_t ConvertNumbers(token_list& tokens) const;
		size_t SplitSquareBrackets(token_list& l) const;
		size_t ResolveVariables(token_list& l) const;
		size_t ResolveRegLists(token_list& l) const;
		size_t ResolveRelatives(AssemblyLine& l) const;

		uint64_t xToI(std::string str, word base, uint64_t max);
		word ToReg(const std::string& str) const;
	};
}