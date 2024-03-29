#pragma once

#ifndef L32_L32Assembler_h_
#define L32_L32Assembler_h_

#include <filesystem>
#include <istream>
#include <list>
#include <stack>
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

			LPAREN,   // ( x
			RELATIVE_MARKER, // . x
			NOT,      // ~ x
			MINUS,    // x - x or - x
			PLUS,     // x + x or + x
			MULTIPLY, // x * x
			DIVIDE,   // x / x
			MODULO,   // x % x
			OR,       // x | x
			AND,      // x & x
			XOR,      // x ^ x
			LSHIFT,   // x << x
			RSHIFT,   // x >> x
			RPAREN,   // x )

			ROTL,   // x ROTL x
			ROTR,   // x ROTR x

			INTEGER,
			REGISTER,

			STRING,

			FLOAT,

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

			ASSIGNMENT,

			COMMA,
			VARGS,
			ARG_NUM,
			LBRACKET,
			RBRACKET,
			LBRACE,
			RBRACE,

			MARKER_CONDITION,
			MARKER_LABEL,

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

		word* cur_start = nullptr;
		word* cur_end = nullptr;

		static constexpr const char valid_text_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

		void SetRAM(const RAM& ram);
		constexpr void SetRAM(word start_address, word range)
		{
			ram_start = start_address;
			ram_range = range;
			ram_current_address = 0;
		}

		void SetROM(const ROM& rom);
		constexpr void SetROM(word start_address, word range)
		{
			rom_start = start_address;
			rom_range = range;
			rom_current_address = 0;
		}

		constexpr void SetComputer(Computer& computer)
		{
			this->computer = &computer;
		}

		void AddLabel(const std::string& label, word address);
		void AddLabels(const std::unordered_map<std::string, word>& addresses);
		
		bool GetLabel(const std::string& label, std::string& label_out) const;
		bool GetVariable(const std::string& variable, TokenList& var_out) const;

		constexpr void ClearRAM() noexcept
		{
			ram_start = NULL_ADDRESS;
			ram_range = 0;
			ram_current_address = 0;
		}

		constexpr void ClearROM() noexcept
		{
			rom_start = NULL_ADDRESS;
			rom_range = 0;
			rom_current_address = 0;
		}

		void ClearLabels() noexcept;

		void FlushScopes() noexcept;

		void ResetMemory() noexcept;
		
		void Assemble(const std::filesystem::path file_path, std::string_view file_contents, bool print_intermediate = false);
		void Assemble(const std::filesystem::path file_path, std::istream& code, bool print_intermediate = false);

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

		struct CondScope
		{
			bool has_cond = false;
			word cond = 0;
		};

		Computer* computer;

		std::list<std::unordered_map<std::string, TokenList>> variable_scopes = {}; // Currently active variable scopes (a variable is not visible outside its scope)
		std::list<std::unordered_map<std::string, word>> label_scopes = {}; // Currently active label scopes (a label is not visible outside its scope)
		std::list<std::list<OpReplace>> func_scopes = {}; // Currently active function scopes (a function is not visible outside its scope)
		std::list<CondScope> cond_scopes = { {false, 0} }; // Currently active condition scopes (sets the condition for everything inside)
		std::list<std::filesystem::path> file_stack = {}; // Files currently open for assembly. Used to prevent infinite recursion

		TokenList cond_scope_openings = {};
		TokenList variable_scope_openings = {};
		TokenList func_scope_openings = {};
		TokenList label_scope_openings = {};

		struct MemoryExpression
		{
			word address;
			bool is_bool;
			TokenList expression;
			size_t num_labels;
		};

		struct MemoryLabel
		{
			std::list<MemoryExpression>::iterator expression;
			Token* token;
		};

		std::list<std::list<Token*>> pending_labels = { {} };
		std::list<std::list<MemoryLabel>> pending_memory_labels = { {} };
		std::list<MemoryExpression> pending_expressions = { };

		word ram_start = NULL_ADDRESS;
		word ram_range = 0;
		word ram_current_address = 0;

		word rom_start = NULL_ADDRESS;
		word rom_range = 0;
		word rom_current_address = 0;

		word* memory_start = nullptr;
		word* memory_range = nullptr;
		word* current_address = nullptr;

		std::unordered_map<std::string, word> constant_addresses = {};

		const std::list<OpReplace> const_replace =
		{
			{"HALT","B",   0,{{TokenType::INTEGER,        {},"0"  }                                                                                                                                                                                                                  }            },
			{"STR", "RWW",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"LDR", "RRW",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"STRB","RWB",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"LDRB","RRB",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"PUSH","SWR",-1,{{TokenType::REGISTER,       {},"SP" },{TokenType::COMMA,  {},","},{TokenType::VARGS,{},"..."}                                                                                                                                                          }            },
			{"POP", "SRR",-1,{{TokenType::REGISTER,       {},"SP" },{TokenType::COMMA,  {},","},{TokenType::VARGS,{},"..."}                                                                                                                                                          }            },
			{"OR",  "ORR",-1,{{TokenType::VARGS,          {},"..."}                                                                                                                                                                                                                  }            },
			{"LSL", "LSL", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"LSR", "LSR", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"ADD", "ADD", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"SUB", "SUB", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"}}            },
			{"INC", "ADD", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"},{TokenType::COMMA,{},","},{TokenType::INTEGER,{},"1"}                                    }            },
			{"DEC", "SUB", 2,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"1"},{TokenType::COMMA,{},","},{TokenType::INTEGER,{},"1"}                                    }            },
			{"INC", "ADD", 1,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::INTEGER,{},"1"}                                    }            },
			{"DEC", "SUB", 1,{{TokenType::MARKER_FUNCTION,{},"@"  },{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","  },{TokenType::MARKER_FUNCTION,{},"@"},{TokenType::INTEGER,{},"0"},{TokenType::COMMA,{},","},{TokenType::INTEGER,{},"1"}                                    }            },
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

		inline std::list<AssemblyLine> ParseTokens(const std::filesystem::path working_dir, TokenList& tokens, bool print_intermediate);
		inline void ParseFunction(TokenList& tokens);
		inline void ParseVariable(TokenList& tokens);
		inline void ParseInstruction(TokenList& tokens, std::list<AssemblyLine>& assembly_lines, std::list<std::list<Token*>>& pending_labels);
		inline void ParsePreprocessor(const std::filesystem::path working_dir, TokenList& tokens, bool& terminate_mode, bool& byte_mode, bool print_intermediate);
		TokenList SolveExpression(const TokenList& tokens, const word address);
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