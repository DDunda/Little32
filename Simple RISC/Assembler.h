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
			std::string message;
			const std::string line;
			const char* const inner_message;
			FormatException(size_t line_no, std::string line, const char* const message);
		};

		struct RawLine {
			size_t line_no;
			std::string* line;
		};

		struct AssemblyLine {
			const RawLine rline;
			word addr;
			std::string code;
			std::string cond = "";
			std::list<token_list> args;
			bool N = false;
			bool S = false;
		};

		size_t total_variables_defined = 0;
		size_t total_variables_replaced = 0;
		size_t total_ops_defined = 0;
		size_t total_ops_replaced = 0;

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
			std::string newcond = "";
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
			{"HALT","B",   0,{"0"                            }     },
			{"STR", "RWW",-1,{"..."                          }     },
			{"LDR", "RRW",-1,{"..."                          }     },
			{"STRB","RWB",-1,{"..."                          }     },
			{"LDRB","RRB",-1,{"..."                          }     },
			{"PUSH","SWR",-1,{"SP",   ",","..."              }     },
			{"POP", "SRR",-1,{"SP",   ",","..."              }     },
			{"OR",  "ORR",-1,{"..."                          }     },
			{"RET", "MOV",-1,{"PC",   ",","LR"               }     },
			{"ADD", "ADD", 2,{"@","0",",","@","0",",","@","1"}     },
			{"SUB", "SUB", 2,{"@","0",",","@","0",",","@","1"}     },
			{"INC", "ADD", 2,{"@","0",",","@","1",",","#","1"}     },
			{"DEC", "SUB", 2,{"@","0",",","@","1",",","#","1"}     },
			{"INC", "ADD", 1,{"@","0",",","@","0",",","#","1"}     },
			{"DEC", "SUB", 1,{"@","0",",","@","0",",","#","1"}     },
			{"BGT", "B",  -1,{"..."                          },"GT"},
			{"BGE", "B",  -1,{"..."                          },"GE"},
			{"BHI", "B",  -1,{"..."                          },"HI"},
			{"BCS", "B",  -1,{"..."                          },"CS"},
			{"BZS", "B",  -1,{"..."                          },"ZS"},
			{"BNS", "B",  -1,{"..."                          },"NS"},
			{"BVS", "B",  -1,{"..."                          },"VS"},
			{"BVC", "B",  -1,{"..."                          },"VC"},
			{"BNC", "B",  -1,{"..."                          },"NC"},
			{"BZC", "B",  -1,{"..."                          },"ZC"},
			{"BCC", "B",  -1,{"..."                          },"CC"},
			{"BLS", "B",  -1,{"..."                          },"LS"},
			{"BLT", "B",  -1,{"..."                          },"LT"},
			{"BLE", "B",  -1,{"..."                          },"LE"},
			{"BHS", "B",  -1,{"..."                          },"HS"},
			{"BEQ", "B",  -1,{"..."                          },"EQ"},
			{"BMI", "B",  -1,{"..."                          },"MI"},
			{"BPL", "B",  -1,{"..."                          },"PL"},
			{"BNE", "B",  -1,{"..."                          },"NE"},
			{"BLO", "B",  -1,{"..."                          },"LO"}
		};

		void ThrowException(const char* const msg) const;
		std::string GetCond(token_list& l) const;

		size_t ConvertNumbers(token_list& tokens) const;
		size_t SplitSquareBrackets(token_list& l) const;
		size_t ResolveVariables(token_list& l) const;
		size_t ResolveRegLists(token_list& l) const;

		uint64_t xToI(const RawLine& rline, std::string str, word base, uint64_t max);
	};
}