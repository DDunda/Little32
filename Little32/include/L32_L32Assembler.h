#ifndef L32_L32Assembler_h_
#define L32_L32Assembler_h_
#pragma once

#include <istream>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>

#include "L32_RAM.h"
#include "L32_ROM.h"
#include "L32_Types.h"

namespace Little32
{
	struct Little32Assembler
	{
		enum class TokenType
		{
			TEXT,

			MINUS,
			PLUS,
			INTEGER,

			STRING,

			MARKER_PREPROCESSOR,
			MARKER_FUNCTION,
			MARKER_VARIABLE,

			SCOPE_FUNCTION_OPEN,
			SCOPE_FUNCTION_CLOSE,
			SCOPE_VARIABLE_OPEN,
			SCOPE_VARIABLE_CLOSE,
			SCOPE_CONDITION_OPEN,
			SCOPE_CONDITION_CLOSE,
			SCOPE_LABEL_OPEN,
			SCOPE_LABEL_CLOSE,

			FLOAT,

			COMMA,
			VARGS,

			LSHIFT,
			RSHIFT,
			LPAREN,
			RPAREN,
			LBRACKET,
			RBRACKET,
			LBRACE,
			RBRACE,

			MARKER_CONDITION,
			MARKER_LABEL,
			MARKER_RELATIVE,

			INVALID,
			END_FILE, // Renamed from EOF because its defined in std
			END_LINE
		};

		enum class PackType
		{
			None,
			BranchOffset,
			Reg3,
			Flex3,
			Flex3i,
			Flex2,
			Flex2i,
			Reg2,
			Reg2ns,
			RegList
		};

		struct Instruction
		{
			word code = 0;
			PackType packing = PackType::None;
			bool allow_N = false;
			bool allow_S = false;
			bool allow_shift = true;
		};

		struct RawLine {
			size_t line_no;
			std::string_view line;
		};

		struct Token
		{
			TokenType type = TokenType::INVALID;
			std::string_view raw_token = {};
			std::string token = "";

			RawLine line = { 0, {} };
			size_t index = 0;
		};

		using TokenList = std::list<Token>;

		inline static const std::unordered_map<std::string, Instruction> instructions
		{
			//          N1ppppSi   ,   .   ,   .   ,
			{"ADD",  {0b0100000000000000000000000000, PackType::Flex3i,  true,  true          }},
			{"SUB",  {0b0100010000000000000000000000, PackType::Flex3i,  true,  true          }},
			{"ADC",  {0b0100100000000000000000000000, PackType::Flex3,   true,  true          }},
			{"SBB",  {0b0100110000000000000000000000, PackType::Flex3,   true,  true          }},
			{"ASL",  {0b0101000000000000000000000000, PackType::Flex3,   true,  true          }},
			{"ASR",  {0b0101010000000000000000000000, PackType::Flex3,   true,  true          }},
			{"CMP",  {0b0101100000000000000000000000, PackType::Flex2i,  true,  false         }},
			{"CMN",  {0b0101110000000000000000000000, PackType::Flex2i,  true,  false         }},

			{"ORR",  {0b0110000000000000000000000000, PackType::Flex3,   true,  true          }},
			{"AND",  {0b0110010000000000000000000000, PackType::Flex3,   true,  true          }},
			{"XOR",  {0b0110100000000000000000000000, PackType::Flex3,   true,  true          }},
			{"TST",  {0b0110110000000000000000000000, PackType::Flex2,   true                 }},
			{"LSL",  {0b0111000000000000000000000000, PackType::Flex3,   true,  true          }},
			{"LSR",  {0b0111010000000000000000000000, PackType::Flex3,   true,  true          }},
			{"MOV",  {0b0111100000000000000000000000, PackType::Flex2i,  true,  true          }},
			{"INV",  {0b0111110000000000000000000000, PackType::Flex2i,  true,  true          }},
			//          N01L   .   ,   .   ,   .   ,
			{"B"  ,  {0b0010000000000000000000000000, PackType::BranchOffset                  }},
			{"BL" ,  {0b0011000000000000000000000000, PackType::BranchOffset                  }},
			{"RFE",  {0b1010000000000000000000000000, PackType::None                          }},
			{"RET",  {0b1011000000000000000000000000, PackType::None                          }},
			//          N0011BWx   ,   .   ,   .   ,
			{"RRW",  {0b0001100000000000000000000000, PackType::Flex3,   true                 }},
			{"RWW",  {0b0001101000000000000000000000, PackType::Flex3,   true                 }},
			{"RRB",  {0b0001110000000000000000000000, PackType::Flex3,   true                 }},
			{"RWB",  {0b0001111000000000000000000000, PackType::Flex3,   true                 }},
			//          N001010W   ,   .   ,   .   ,
			{"SRR",  {0b0001010000000000000000000000, PackType::RegList, true                 }},
			{"SWR",  {0b0001010100000000000000000000, PackType::RegList, true                 }},
			//          N001011p   ,   .   ,   .   ,
			{"MVM",  {0b0001011000000000000000000000, PackType::RegList, true                 }},
			{"SWP",  {0b0001011100000000000000000000, PackType::Reg2,    true                 }},
			//          N0001ppp   ,   .   ,   .   ,
			{"ADDF", {0b0000100000000000000000000000, PackType::Reg3,    true,  false, false  }},
			{"SUBF", {0b0000100100000000000000000000, PackType::Reg3,    true,  false, false  }},
			{"MULF", {0b0000101000000000000000000000, PackType::Reg3,    true,  false, false  }},
			{"DIVF", {0b0000101100000000000000000000, PackType::Reg3,    true,  false, false  }},
			{"ITOF", {0b0000110000000000000000000000, PackType::Reg2,    true,  false, false  }},
			{"FTOI", {0b0000110100000000000000000000, PackType::Reg2,    true,  false, false  }},
			{"CMPF", {0b0000111000000000000000000000, PackType::Reg2,    true,  false, false  }},
			{"CMPFI",{0b0000111100000000000000000000, PackType::Reg2,    true,  false, false  }},
		};

		Little32Assembler()
		{
			variable_scopes = { {} };
			label_scopes = { {} };
			func_scopes = { const_replace };
		}

		class FormatException : public std::exception {
		public:
			const size_t line_no;
			const std::string line;
			const std::string message;
			const std::string inner_message;
			FormatException(const Token& token, const std::string_view message);
		};

		struct AssemblyLine {
			const RawLine rline;
			word addr;
			word* mem;
			Token code;
			std::list<TokenList> args;

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

		static constexpr word NULL_ADDRESS = -1;

		word entry_point = NULL_ADDRESS;
		word program_start = NULL_ADDRESS;
		word program_end = NULL_ADDRESS;
		word data_start = NULL_ADDRESS;
		word data_end = NULL_ADDRESS;

		static constexpr const char valid_text_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

		void SetRAM(const RAM& ram);
		constexpr void SetRAM(word* memory, word start_address, word range)
		{
			ram = memory;
			ram_start = start_address;
			ram_range = range;
			ram_current_address = 0;
		}

		void SetROM(const ROM& rom);
		constexpr void SetROM(word* memory, word start_address, word range)
		{
			rom = memory;
			rom_start = start_address;
			rom_range = range;
			rom_current_address = 0;
		}

		void AddLabel(const std::string& label, word address);
		void AddLabels(const std::unordered_map<std::string, word>& addresses);
		
		bool GetLabel(const std::string& label, std::string& label_out) const;
		bool GetVariable(const std::string& variable, TokenList& var_out) const;

		constexpr void ClearRAM() noexcept {
			ram = nullptr;
			ram_start = 0;
			ram_range = 0;
			ram_current_address = 0;
		}

		constexpr void ClearROM() noexcept
		{
			rom = nullptr;
			rom_start = 0;
			rom_range = 0;
			rom_current_address = 0;
		}

		void ClearLabels() noexcept;

		void FlushScopes() noexcept;
		
		void Assemble(const std::string_view code, bool print_intermediate = false);
		void Assemble(std::istream& code, bool print_intermediate = false);

	protected:
		struct OpReplace
		{
			std::string op = "";
			std::string newop = "";
			int requiredArgs = -1;
			TokenList tokens = {};

			bool has_cond = false;
			byte new_cond = 0;
			bool has_shift = false;
			byte new_shift = 0;

			bool newN = false;
			bool newS = false;
		};

		struct cond_scope
		{
			bool has_cond = false;
			word cond = 0;
		};

		std::list<std::unordered_map<std::string, TokenList>> variable_scopes;
		std::list<std::unordered_map<std::string, word>> label_scopes;
		std::list<std::list<OpReplace>> func_scopes;
		std::list<cond_scope> cond_scopes;

		word* ram = nullptr;
		word ram_start = 0;
		word ram_range = 0;
		word ram_current_address = 0;

		word* rom = nullptr;
		word rom_start = 0;
		word rom_range = 0;
		word rom_current_address = 0;

		word* memory = nullptr;
		word memory_start = 0;
		word memory_range = 0;
		word* current_address = 0;

		std::unordered_map<std::string, word> constant_addresses = {};

		const std::list<OpReplace> const_replace = {
			{"HALT","B",   0,{{TokenType::INTEGER,        {},"0"  }                                                                                                                                                                                                                  }            },
			{"STR", "RWW",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"LDR", "RRW",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"STRB","RWB",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"LDRB","RRB",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"PUSH","SWR",-1,{{TokenType::TEXT,           {},"SP" },{TokenType::COMMA,  {},","},{TokenType::VARGS,{},"..."}                                                                                                                                                          }            },
			{"POP", "SRR",-1,{{TokenType::TEXT,           {},"SP" },{TokenType::COMMA,  {},","},{TokenType::VARGS,{},"..."}                                                                                                                                                          }            },
			{"OR",  "ORR",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"ADD", "ADD", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"SUB", "SUB", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"INC", "ADD", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"DEC", "SUB", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"INC", "ADD", 1,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"DEC", "SUB", 1,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"BAL", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0000},
			{"BGT", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0001},
			{"BGE", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0010},
			{"BHI", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0011},
			{"BCS", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0100},
			{"BHS", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0100},
			{"BZS", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0101},
			{"BEQ", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0101},
			{"BNS", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0110},
			{"BMI", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0110},
			{"BVS", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b0111},
			{"BVC", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1000},
			{"BNC", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1001},
			{"BPL", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1001},
			{"BZC", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1010},
			{"BNE", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1010},
			{"BCC", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1011},
			{"BLO", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1011},
			{"BLS", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1100},
			{"BLT", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1101},
			{"BLE", "B",  -1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  },true,0b1110}
		};

		void ThrowException(const std::string_view msg, const Token& token) const;
		bool GetCond(TokenList& l, byte& cond) const;
		bool GetShift(TokenList& l, byte& shift) const;
		word GetBranchOffset(TokenList& l, const word shift, bool& isNegative) const;

		inline std::list<AssemblyLine> ParseTokens(TokenList& tokens);
		inline void ParseFunction(TokenList& tokens);
		inline void ParseVariable(TokenList& tokens);
		inline void ParseInstruction(TokenList& tokens, std::list<AssemblyLine>& assembly_lines, std::list<std::list<Token*>>& pending_labels);
		inline void ParsePreprocessor(TokenList& tokens, bool& terminate_mode, bool& byte_mode);
		size_t ConvertNumbers(TokenList& tokens) const;
		size_t SplitSquareBrackets(TokenList& l) const;
		size_t ResolveVariables(TokenList& l) const;
		size_t ResolveRegLists(TokenList& l) const;
		size_t ResolveRelatives(AssemblyLine& l) const;

		uint64_t xToI(Token& t, word base, uint64_t max);
		word ToReg(Token& t) const;
	};
}

#endif