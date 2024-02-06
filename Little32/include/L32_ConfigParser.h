#pragma once

#ifndef L32_ConfigFile_h_
#define L32_ConfigFile_h_

#include "L32_IO.h"
#include "L32_String.h"

#include <pixels.hpp>
#include <rect.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <string>
#include <set>

namespace Little32
{
	struct ConfigObject;

	struct VarValue;
	struct VarReference;
	
	struct ConfigParser
	{
	public:
		inline static uint8_t tab_width = 4;

		enum TokenType
		{
			VARNAME, // [a-zA-Z_][a-zA-Z_\d]*

			ASSIGN, // =

			// Basic types
			INTEGER,
			FLOAT,
			STRING,
			COLOUR,
			
			// References
			LREF,
			RREF,
			DOUBLE_SLASH,
			DOUBLE_DOT,
			SLASH,

			// Vectors
			LPAREN, // (
			RPAREN, // )
			COMMA,  // ,

			// Objects
			LBRACKET, // {
			RBRACKET, // }

			// Lists
			LBRACE, // [
			RBRACE, // ]

			INVALID
		};

		inline static const std::map<TokenType, const char*> type_names
		{
			{ VARNAME, "variable name" },
			{ ASSIGN, "assignment tag" },
			{ INTEGER, "integer value" },
			{ FLOAT, "float value" },
			{ STRING, "string value" },
			{ COLOUR, "colour value" },
			{ LREF, "opening reference tag" },
			{ RREF, "closing reference tag" },
			{ DOUBLE_SLASH, "root reference tag" },
			{ DOUBLE_DOT, "upwards reference tag" },
			{ SLASH, "divider reference tag" },
			{ LPAREN, "opening vector tag" },
			{ RPAREN, "closing vector tag" },
			{ COMMA, "comma" },
			{ LBRACKET, "opening object tag" },
			{ RBRACKET, "closing object tag" },
			{ LBRACE, "opening list tag" },
			{ RBRACE, "closing list tag" },
			{ INVALID, "unknown token" }
		};

		struct RawLine
		{
			size_t line_no;
			std::string_view line;
		};

		struct Token
		{
			TokenType type = INVALID;
			std::string_view raw_token = {};
			std::string token = "";

			size_t file_index = 0;
			const RawLine* line = nullptr;
			size_t index = 0;
		};

		typedef std::list<Token> TokenList;

		class FormatException : public std::exception
		{
		public:
			size_t line_no;
			std::string line;
			std::string message;
			std::string inner_message;
			FormatException(const Token& token, const std::string_view message, uint8_t tab_width) :
				line_no(token.line->line_no),
				line(token.line->line),
				inner_message(message),
				exception("Improper format")
			{
				std::string tmp_message = std::string("Improper format: ") + std::string(message) + ": (line " + (token.line->line_no == 0 ? "??" : std::to_string(token.line->line_no)) + ")\n\n";
				size_t depth = token.index;
				size_t width = token.raw_token.size();

				size_t end = std::min(depth, token.line->line.size());

				size_t i = 0;

				for (; i < end; ++i)
				{
					switch (token.line->line[i])
					{
					case '\t':
						depth += tab_width - 1;
						break; 

					case '\n':
					case '\v':
					case '\f':
					case '\r':
					case '\\':
						depth++;
						break;
					}
				}

				end = std::min(token.index + width, token.line->line.size());

				for (; i < end; ++i)
				{
					switch (token.line->line[i])
					{
					case '\t':
						width += tab_width - 1;
						break;

					case '\n':
					case '\v':
					case '\f':
					case '\r':
					case '\\':
						width++;
					}
				}

				std::string tab = "";
				for (i = tab_width; i > 0; --i)
					tab += ' ';

				for (const char& c : std::string(token.line->line))
				{
					switch (c)
					{
					case '\t':
						tmp_message += tab;
						break;

					case '\n':
						tmp_message += "\n";
						break;

					case '\v':
						tmp_message += "\v";
						break;

					case '\f':
						tmp_message += "\f";
						break;

					case '\r':
						tmp_message += "\r";
						break;

					case '\\':
						tmp_message += "\\\\";
						break;

					default:
						tmp_message += c;
					}
				}

				tmp_message += '\n';
				for (i = depth; i > 0; --i)
					tmp_message += ' ';

				if (width == 0) tmp_message += '^';
				else
				{
					for (i = width; i > 0; --i)
						tmp_message += '^';
				}

				this->message = tmp_message;
				//	std::string(token.line->line) + "\n" +
				//	std::string(token.index, ' ') + std::string(token.raw_token.size() > 0 ? token.raw_token.size() : 1, '^');
			}
		};

	private:
		static VarValue* GetValue(TokenList& tokens, ConfigObject& root, VarValue* parent);

	public:
		static size_t GetTokens(std::string_view file, std::list<RawLine>::const_iterator& line, size_t& file_pos, size_t& pos, TokenList& tokens);
		
		static ConfigObject* ParseFile(std::string_view file_contents);

		static ConfigObject* ParseFile(std::istream& file_stream);
	};
};

#endif