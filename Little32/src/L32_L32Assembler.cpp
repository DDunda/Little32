#include "L32_L32Assembler.h"

#include "L32_IO.h"
#include "L32_RAM.h"
#include "L32_ROM.h"
#include "L32_String.h"

#include <cassert>
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace Little32
{
	Little32Assembler::FormatException::FormatException(const Token& token, const std::string_view message) :
		line_no(token.line.line_no),
		line(token.line.line),
		inner_message(message),
		message(
			std::string("Improper format: ") + std::string(message) + ": (line " + (token.line.line_no == 0 ? "??" : std::to_string(token.line.line_no)) + ")\n\n" +
			std::string(token.line.line) + "\n" +
			std::string(token.index, ' ') + std::string(token.raw_token.size() > 0 ? token.raw_token.size() : 1, '^')
		),
		exception("Improper format") {}

	void Little32Assembler::ThrowException(const std::string_view msg, const Token& token) const { throw FormatException(token, msg); }

	bool TryGet(const Little32Assembler::TokenList& l, const Little32Assembler::TokenList::iterator& it, Little32Assembler::Token& out) noexcept
	{
		if (it == l.end()) return false;
		if (it->type >= Little32Assembler::TokenType::INVALID) return false;

		out = *it;

		return true;
	}

	bool TryConsume(Little32Assembler::TokenList& l, Little32Assembler::TokenList::iterator& it, Little32Assembler::Token& out) noexcept
	{
		if (it == l.end()) return false;
		if (it->type >= Little32Assembler::TokenType::INVALID) return false;

		out = *it;
		it = l.erase(it);

		return true;
	}

	bool TryGetFront(const Little32Assembler::TokenList& l, Little32Assembler::Token& out) noexcept
	{
		if (l.empty()) return false;
		if (l.front().type >= Little32Assembler::TokenType::INVALID) return false;

		out = l.front();

		return true;
	}

	bool TryConsumeFront(Little32Assembler::TokenList& l, Little32Assembler::Token& out) noexcept
	{
		if (l.empty()) return false;
		if (l.front().type >= Little32Assembler::TokenType::INVALID) return false;

		out = l.front();
		l.pop_front();

		return true;
	}

	constexpr uint64_t decToI(std::string_view str, uint64_t max)
	{
		if (!IsChars(str,"0123456789_")) return max;
		uint64_t v = 0;
		for (char c : str)
		{
			if (c == '_') continue;
			v *= 10;
			v += (uint64_t)c - '0';
			if (v >= max) return max;
		}
		return v;
	}

	constexpr uint64_t binToI(std::string_view str, uint64_t max)
	{
		if (str.size() > 2 && str[1] == 'b')
		{
			if (str[0] != '0') return max;
			str = str.substr(2);
		}

		if (!IsChars(str, "01_")) return max;

		uint64_t v = 0;

		for (char c : str)
		{
			if (c == '_') continue;
			v <<= 1;
			v += (uint64_t)c - '0';
			if (v >= max) return max;
		}

		return v;
	}

	constexpr uint64_t octToI(std::string_view str, uint64_t max)
	{
		if (!IsChars(str, "01234567_")) return max;

		uint64_t v = 0;

		for (char c : str)
		{
			if (c == '_') continue;
			v <<= 3;
			v += (uint64_t)c - '0';
			if (v >= max) return max;
		}

		return v;
	}

	constexpr uint64_t hexToI(std::string_view str, uint64_t max)
	{
		if (str.size() > 2 && str[1] == 'x')
		{
			if (str[0] != '0') return max;
			str = str.substr(2);
		}

		if (!IsChars(str, "0123456789abcdefABCDEF_")) return max;

		uint64_t v = 0;
		for (char c : str)
		{
			if (c == '_') continue;
			v <<= 4;
			if (c >= '0' && c <= '9') v += (uint64_t)c - '0';
			else if (c >= 'a' && c <= 'f') v += (uint64_t)c - 'a' + 10;
			else if (c >= 'A' && c <= 'F') v += (uint64_t)c - 'A' + 10;
			if (v >= max) return max;
		}

		return v;
	}

	uint64_t Little32Assembler::xToI(Token& t, word base, uint64_t max)
	{
		uint64_t val = max;

		switch (base)
		{
		case 16: val = hexToI(t.token, max); break;
		case 10: val = decToI(t.token, max); break;
		case 8:  val = octToI(t.token, max); break;
		case 2:  val = binToI(t.token, max); break;
		default: ThrowException("This number cannot be processed", t);
		}

		if (val == max) ThrowException("This number is too large", t);

		return val;
	}

	word Little32Assembler::ToReg(Token& t) const
	{
		for (int i = 0; i < 16; i++)
		{
			if (t.token == reg_names[i]) return i;
		}

		ThrowException("'" + std::string(t.token) + "' is not a register", t);
		return -1;
	}

	constexpr bool IsAlpha(const std::string_view str) noexcept
	{
		for (auto c : str)
		{
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) return false;
		}

		return true;
	}

	constexpr int IsNumber(const std::string_view str) noexcept
	{
		if (str.empty()) return 0;
		if (str.size() == 1) return 10 * (str[0] >= '0' && str[0] <= '9');
		if (IsChar(str, '_')) return 0;
		if (str[0] != '0') return 10 * IsChars(str, "0123456789_");
		if (str.size() == 2) return 8 * (str[1] >= '0' && str[1] <= '7');
		if (str[1] == 'x') return 16 * IsChars(str, "0123456789abcdefABCDEF_", 2);
		if (str[1] == 'b') return 2 * IsChars(str, "01_", 2);
		return 8 * IsChars(str, "01234567_");
	}

	constexpr bool IsFloat(const std::string_view str)
	{
		if (str.size() < 3) return false;
		if (std::count(str.begin(), str.end(), '.') != 1) return false;
		if (str[0] == '.' || str[str.size() - 1] == '.') return false;

		return IsChars(str, "0123456789.");
	}

	constexpr bool IsLower(const std::string_view str) noexcept
	{
		for (auto c : str)
		{
			if (c < 'a' || c > 'z') return false;
		}

		return true;
	}

	constexpr bool IsUpper(const std::string_view str) noexcept
	{
		for (auto c : str)
		{
			if (c < 'A' || c > 'Z') return false;
		}

		return true;
	}

	constexpr bool IsReg(const std::string_view str) noexcept
	{
		for (int i = 0; i < 16; i++)
		{
			if (str == reg_names[i]) return true;
		}

		return false;
	}

	bool IsCond(const std::string& str) noexcept
		{ return cond_names.contains(str); }

	constexpr void RemoveComments(std::string& str) noexcept
	{
		size_t i = str.find("//", 0, 2);
		if (i == str.npos) return;
		str.erase(i);
	}

	constexpr void PutByte(word* memory, word address, byte value)
	{
		word x = (address % sizeof(word)) * 8;

		value ^= memory[address / sizeof(word)] >> x;
		memory[address / sizeof(word)] ^= value << x;
	}

	size_t GetTokens(std::string_view file, Little32Assembler::RawLine& line, Little32Assembler::TokenList& tokens, size_t& pos)
	{
		using namespace std;

		size_t token_count = 0;

		if (file.empty()) return 0;
		
		while (string(" \t\r\n").find(file[0]) != string::npos)
		{
			if (file[0] == '\n')
			{
				tokens.push_back({ Little32Assembler::TokenType::END_LINE, file.substr(0,1), "\\n", line, pos });
				token_count++;
				line.line = file.substr(1, file.find_first_of('\n', 1) - 1);
				line.line_no++;
				pos = 0;
			}
			else {
				pos++;
			}
			file = file.substr(1);
			if (file.empty()) return 0;
		}

		size_t i = file.find('\"');
		size_t l_comment = file.find("//");
		size_t m_comment = file.find("/*");

		while (i != string::npos || l_comment != string::npos || m_comment != string::npos)
		{
			if (i < l_comment && i < m_comment) // "
			{
				token_count += GetTokens(file.substr(0, i), line, tokens, pos) + 1;
				file = file.substr(i);

				size_t end = 0;

				if (Contains(file.substr(1), '"', i) && i < file.find('\r') && i < file.find('\n'))
				{
					string str = "";

					for (size_t it = 1; ; it++)
					{
						char c = file[it];

						if (c == '\\')
						{
							switch (file[++it])
							{
							case 'a':
								str += '\a';
								break;
							case 'b':
								str += '\b';
								break;
							case 'f':
								str += '\f';
								break;
							case 'n':
								str += '\n';
								break;
							case 'r':
								str += '\r';
								break;
							case 't':
								str += '\t';
								break;
							case 'v':
								str += '\v';
								break;
							case '0':
								str += '\0';
								break;
							case 'x':
								for(;;)
								{
									char value = 0;

									c = ++it < file.size() ? file[it] : 0;

									if (c >= '0' && c <= '9')      value = ( c - '0' ) << 4;
									else if (c >= 'a' && c <= 'f') value = ( 10 + c - 'a' ) << 4;
									else if (c >= 'A' && c <= 'F') value = ( 10 + c - 'A' ) << 4;
									else
									{
										it--;
										if (file[it] == 'x') str += 'x';
										break;
									}

									c = ++it < file.size() ? file[it] : 0;

									if (c >= '0' && c <= '9')      value += c - '0';
									else if (c >= 'a' && c <= 'f') value += 10 + c - 'a';
									else if (c >= 'A' && c <= 'F') value += 10 + c - 'A';
									else
									{
										it -= 2;
										if (file[it] == 'x') str += 'x';
										break;
									}
									str += value;
								}
								break;
							case '\\':
								str += '\\';
								break;
							case '"':
								str += '"';
								break;
							default:
								str += file[it];
								break;
							}
						}
						else if (c != '"')
						{
							str += file[it];
						}
						else
						{
							end = it;
							break;
						}
					}

					tokens.push_back({ Little32Assembler::TokenType::STRING, file.substr(1,end - 1), str, line, pos + 1});
				}
				else
				{
					tokens.push_back({ Little32Assembler::TokenType::INVALID, file.substr(0,1), "\"", line, pos });
				}

				token_count++;

				file = file.substr(end + 1);
				pos += end + 1;
			}
			else if (l_comment < m_comment) // "//"
			{
				token_count += GetTokens(file.substr(0, l_comment), line, tokens, pos);
				file = file.substr(l_comment + 2);
				pos += 2;

				if (!Contains(file, '\n', i)) return token_count; // No newline after comment; means there is no more code

				file = file.substr(i);
				pos += i;

				tokens.push_back({ Little32Assembler::TokenType::END_LINE, file.substr(0,1), "\\n", line, pos });
				token_count++;
				file = file.substr(1);
				pos = 0;
				line.line = file.substr(0, file.find_first_of('\n'));
				line.line_no++;
			}
			else // /*
			{
				token_count += GetTokens(file.substr(0, m_comment), line, tokens, pos);
				file = file.substr(m_comment);
				pos += m_comment;

				size_t end;

				if (Contains(file, "*/", end))
				{
					file = file.substr(2);
					end -= 2;
					pos += 2;

					while (Contains(file.substr(0, end), '\n', i))
					{
						file = file.substr(i);
						end -= i;
						pos += i;

						tokens.push_back({ Little32Assembler::TokenType::END_LINE, file.substr(0,1), "\n", line, pos });
						token_count++;

						file = file.substr(1);
						end -= 1;
						pos = 0;

						line.line = file.substr(0, file.find_first_of('\n'));
						line.line_no++;
					}

					file = file.substr(end + 2);
					pos += end + 2; // Remaining characters in comment + "*/"
				}
				else
				{
					tokens.push_back({ Little32Assembler::TokenType::INVALID, file.substr(0,2), "/*", line, pos });
					token_count++;

					file = file.substr(2);
					pos += 2;
				}
			}

			if (file.empty()) return token_count;

			i = file.find('"');
			l_comment = file.find("//");
			m_comment = file.find("/*");
		}

		while ((i = file.find('<')) != string::npos)
		{
			token_count += GetTokens(file.substr(0, i), line, tokens, pos) + 1;
			file = file.substr(i);

			size_t end = 0;
			std::string_view num;

			if (Contains(file, '>', i) && i < file.find('\r') && i < file.find('\n') && (IsNumeric(num = TrimString(file.substr(1, i - 1)))  || num == "-1"))
			{
				tokens.push_back({ Little32Assembler::TokenType::ARG_NUM, num, std::string(num), line, pos });

				end = i;
			}
			else
			{
				tokens.push_back({ Little32Assembler::TokenType::INVALID, file.substr(0,1), "<", line, pos });
			}

			token_count++;

			file = file.substr(end + 1);
			pos += end + 1;
		}

#define _GetTokens(type,term,l)\
		while (Contains(file, term, i))\
		{\
			token_count += GetTokens(file.substr(0, i), line, tokens, pos) + 1;\
			tokens.push_back({ Little32Assembler::TokenType::type, file.substr(i,l), term, line, pos });\
			file = file.substr(i + l);\
			pos += l;\
			if (file.empty()) return token_count;\
		}

		_GetTokens(ROTL, "ROTL", 4);
		_GetTokens(ROTR, "ROTR", 4);
		_GetTokens(VARGS,"...",3);
		_GetTokens(SCOPE_FUNCTION_OPEN,"@{",2);
		_GetTokens(SCOPE_FUNCTION_CLOSE,"}@",2);
		_GetTokens(SCOPE_VARIABLE_OPEN,"${",2);
		_GetTokens(SCOPE_VARIABLE_CLOSE,"}$",2);
		_GetTokens(SCOPE_CONDITION_OPEN,"?{",2);
		_GetTokens(SCOPE_CONDITION_CLOSE,"}?",2);
		_GetTokens(SCOPE_LABEL_OPEN, ":{", 2);
		_GetTokens(SCOPE_LABEL_CLOSE,"}:",2);
		_GetTokens(LSHIFT,"<<",2);
		_GetTokens(RSHIFT,">>",2);
		_GetTokens(MARKER_PREPROCESSOR,"#", 1);
		_GetTokens(MARKER_FUNCTION,"@",1);
		_GetTokens(MARKER_VARIABLE,"$",1);
		_GetTokens(MARKER_CONDITION,"?",1);
		_GetTokens(MARKER_LABEL,":",1);
		_GetTokens(COMMA,",",1);
		_GetTokens(LBRACKET,"[",1);
		_GetTokens(RBRACKET,"]",1);
		_GetTokens(LBRACE,"{",1);
		_GetTokens(RBRACE,"}",1);
		_GetTokens(LPAREN,"(",1);
		_GetTokens(NOT,"~",1);
		_GetTokens(PLUS,"+",1);
		_GetTokens(MINUS,"-",1);
		_GetTokens(MULTIPLY,"*",1);
		_GetTokens(DIVIDE,"/",1);
		_GetTokens(MODULO,"%",1);
		_GetTokens(OR,"|",1);
		_GetTokens(AND,"&",1);
		_GetTokens(XOR,"^",1);
		_GetTokens(RPAREN,")",1);
		_GetTokens(ASSIGNMENT,"=",1);
#undef _GetTokens

		while (!file.empty())
		{
			while (!Contains(Little32Assembler::valid_text_chars, file[0]))
			{
				if (file[0] == '\n')
				{
					tokens.push_back({ Little32Assembler::TokenType::END_LINE, file.substr(0,1), "\n", line, pos });
					token_count++;

					file = file.substr(1);
					pos = 0;

					line.line = file.substr(0, file.find_first_of('\n'));
					line.line_no++;
				}
				else if (file[0] == '.')
				{
					Little32Assembler::TokenType t = Little32Assembler::TokenType::RELATIVE_MARKER;
					tokens.push_back({ t, file.substr(0,1), ".", line, pos });
					token_count++;

					file = file.substr(1);
					pos++;
				}
				else if (Contains(" \t\r", file[0]))
				{
					file = file.substr(1);
					pos++;
				}
				else
				{
					size_t last = file.find_first_of("\n\r\t .abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");

					if (last == string::npos)
					{
						tokens.push_back({ Little32Assembler::TokenType::INVALID, file, string(file), line, pos });
						return token_count;
					}

					tokens.push_back({ Little32Assembler::TokenType::INVALID, file.substr(0,last), string(file.substr(0,last)), line, pos });
					token_count++;

					file = file.substr(last);
					pos += last;
				}

				if (file.empty()) return token_count;
			}

			i = file.find_first_not_of(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
			string_view t = i == string::npos ? file : file.substr(0, i);
			file = i == string::npos ? string_view() : file.substr(i);

			Little32Assembler::Token tok { Little32Assembler::TokenType::INVALID, t, string(t), line, pos };

			pos += i == string::npos ? t.size() : i;

			if (IsNumber(t))
			{
				tok.type = Little32Assembler::TokenType::INTEGER;
			}
			else if (IsFloat(t))
			{
				tok.type = Little32Assembler::TokenType::FLOAT;
			}
			else if (IsReg(t))
			{
				tok.type = Little32Assembler::TokenType::REGISTER;
			}
			else if (IsChars(t, Little32Assembler::valid_text_chars))
			{
				tok.type = Little32Assembler::TokenType::TEXT;
			}

			tokens.push_back(tok);

			token_count++;
		}

		return token_count;
	}

	Little32Assembler::TokenList::iterator FindToken(Little32Assembler::TokenList& list, Little32Assembler::TokenType type, size_t offset)
	{
		auto it = std::next(list.begin(), offset);
		const auto end = list.end();

		for (; it != end; it++)
		{
			if (it->type == Little32Assembler::TokenType::END_LINE) return end;
			if (it->type == type) break;
		}

		return it;
	}

	Little32Assembler::TokenList::iterator FindToken(Little32Assembler::TokenList& list, Little32Assembler::TokenType type)
	{
		auto it = list.begin();
		const auto end = list.end();

		for (; it != end; it++)
		{
			if (it->type == Little32Assembler::TokenType::END_LINE) return end;
			if (it->type == type) break;
		}

		return it;
	}

	bool Little32Assembler::GetCond(TokenList& l, byte& cond) const
	{
		using namespace std;

		auto it = FindToken(l, Little32Assembler::TokenType::MARKER_CONDITION);

		if (it == l.end() || it->type == Little32Assembler::TokenType::END_LINE) return false;

		it = l.erase(it); // Erase '?'

		if (it == l.end() || it->type != Little32Assembler::TokenType::TEXT || !IsCond(it->token)) ThrowException("Expected condition for '?' statement", *it);

		cond = cond_names.at(it->token);
		it = l.erase(it); // Erase condition
		
		if (it->type != Little32Assembler::TokenType::END_LINE) ThrowException("Line does not end after '?' statement", *it);
		return true;
	}

	bool Little32Assembler::GetShift(TokenList& l, byte& shift) const
	{
		using namespace std;

		auto it = FindToken(l, TokenType::ROTL);
		it = (it == l.end() || it->type == TokenType::END_LINE) ? FindToken(l, Little32Assembler::TokenType::ROTR) : it;

		if (it == l.end() || it->type == TokenType::END_LINE) return false;

		const bool rshift = it->type == TokenType::ROTR;
		it = l.erase(it);

		if (it == l.end() || it->type != TokenType::INTEGER) ThrowException("Expected number after rotation shift", *it);

		shift = (byte)stoul(it->token);
		if (shift >= 32) ThrowException("Rotation shift is too large", *it);

		it = l.erase(it);

		if (it != l.end() && it->type != TokenType::END_LINE && it->type != TokenType::MARKER_CONDITION) ThrowException("Rotation shift must be at end of instruction", *it);

		if (rshift && shift) shift = 32 - shift;

		return true;
	}

	word Little32Assembler::GetBranchOffset(TokenList& l, const word shift, bool& is_negative) const
	{
		using namespace std;

		is_negative = false;
		if (l.front().type == Little32Assembler::TokenType::MINUS)
		{
			if (l.empty()) ThrowException("Expected offset after sign", l.front());
			l.pop_front();

			is_negative = true;
		}

		if (l.size() > 1) ThrowException("Expected one value for offset", *next(l.begin()));
		if (l.front().type != Little32Assembler::TokenType::INTEGER) ThrowException("Expected offset to be a number", l.front());

		const word offset = rotl(stoul(l.front().token), shift);

		if (offset & 3) ThrowException("Expected offset to be word aligned", l.front());
		if (offset > 0x03FFFFFC) ThrowException("Expected smaller offset", l.front());

		return offset >> 2;
	}

	size_t Little32Assembler::ConvertNumbers(TokenList& tokens) const
	{
		size_t numbers_replaced = 0;

		for (auto& t : tokens)
		{
			if (t.type == Little32Assembler::TokenType::INTEGER)
			{
				int numBase = IsNumber(t.token);

				if (numBase == 10)
				{
					uint64_t long_val = stoul(t.token);
					word word_Val = (word)long_val;
					if (long_val > word_Val) ThrowException("This number is too large for a word",t);
					continue;
				}
				numbers_replaced++;

				word _v = 0, v = 0;

				if (numBase == 2 || numBase == 16) t.token = t.token.substr(2);
				else if (numBase == 8) t.token = t.token.substr(1);

				for (auto c : t.token)
				{
					if (c == '_') continue;
					v *= numBase;
					if (c >= '0' && c <= '9') v += c - '0';
					else if (c >= 'a' && c <= 'f') v += 10 + c - 'a';
					else if (c >= 'A' && c <= 'F') v += 10 + c - 'A';

					if (_v > v) ThrowException("This number is too large for a word",t);
					_v = v;
				}
				t.token = std::to_string(v);
			}
			else if (t.type == Little32Assembler::TokenType::FLOAT)
			{
				float f = std::stof(t.token);
				word val = *(word*)&f;
				t.token = std::to_string(val);
				t.type = Little32Assembler::TokenType::INTEGER;
			}
		}

		return numbers_replaced;
	}

	size_t Little32Assembler::SplitSquareBrackets(TokenList& l) const
	{
		using namespace std;

		size_t brackets_replaced = 0;

		TokenList::iterator it = l.begin();
		Token token;

		while (TryGet(l, it, token))
		{
			if (token.type == TokenType::RBRACKET) ThrowException("Closing square bracket without opening",token);
			if (token.type != TokenType::LBRACKET)
			{
				it++;
				continue;
			}
			brackets_replaced++;
			it = l.erase(it); // Erase '['

			TokenList::iterator start = it;

			if (!TryGet(l, it, token)) ThrowException("Square brackets must be closed",token);
			if (token.type == TokenType::RBRACKET) ThrowException("Square brackets must contain contents",token);
			if (token.type == TokenType::LBRACKET) ThrowException("Square brackets cannot be nested",token);
			if (token.type == TokenType::COMMA) ThrowException("Square brackets cannot contain commas",token);

			if (token.type == TokenType::PLUS || token.type == TokenType::MINUS) // Bracket starts with a sign - Interpreted as single value relative to PC
			{
				l.insert(it, { TokenType::REGISTER, token.raw_token, "PC", token.line, token.index });
			}
			else {
				Token first_token = token;
				it++;
				while (TryGet(l, it, token) && token.type != TokenType::RBRACKET)
				{
					if (token.type == TokenType::LBRACKET) ThrowException("Square brackets cannot be nested",token);
					if (token.type == TokenType::COMMA) ThrowException("Square brackets cannot contain commas",token);
					if (token.type == TokenType::PLUS || token.type == TokenType::MINUS) break;
					it++;
				}

				if (it == l.end() || it->type == TokenType::END_LINE) ThrowException("Square brackets must be closed",token);

				if (token.type == TokenType::RBRACKET) // Occurs when no signs were encountered - Interpreted as single value relative to PC
				{
					if (first_token.type == TokenType::REGISTER) // User entered a register; consider contents as relative to it
					{
						l.insert(it, { TokenType::COMMA, token.raw_token, ",", token.line, token.index });
						l.insert(it, { TokenType::INTEGER, token.raw_token, "0", token.line, token.index });
					}
					else  // User entered a value; consider contents as relative to PC
					{
						l.insert(start, { TokenType::REGISTER, token.raw_token, "PC", token.line, token.index });
						l.insert(start, { TokenType::COMMA, token.raw_token, ",", token.line, token.index });
					}
					it = l.erase(it); // Erase ']'
					continue;
				}
			}

			if (token.type == TokenType::PLUS) *it = { TokenType::COMMA, token.raw_token, ",", token.line, token.index };
			else l.insert(it, { TokenType::COMMA, token.raw_token, ",", token.line, token.index });
			it++;

			if (!TryGet(l, it, token)) ThrowException("Square brackets must be closed",token);
			if (token.type == TokenType::RBRACKET) ThrowException("Sign in square brackets must be followed by offset",token);

			do
			{
				if (token.type == TokenType::LBRACKET) ThrowException("Square brackets cannot be nested",token);
				if (token.type == TokenType::COMMA) ThrowException("Square brackets cannot contain commas",token);
				if (token.type == TokenType::PLUS || token.type == TokenType::MINUS) ThrowException("Square brackets can only have one offset",token);
				it++;
			} while (TryGet(l, it, token) && token.type != TokenType::RBRACKET);

			if (token.type != TokenType::RBRACKET) ThrowException("Square brackets must be closed",token);
			it = l.erase(it); // Erase ']'
		}

		return brackets_replaced;
	}

	size_t Little32Assembler::ResolveVariables(TokenList& l) const
	{
		using namespace std;

		size_t vars_replaced = 0;
		auto it = l.begin();
		Token token;

		while (TryGet(l, it, token))
		{
			if (token.type != TokenType::MARKER_VARIABLE)
			{
				it++;
				continue;
			}
			vars_replaced++;

			it = l.erase(it);

			Token var_name;

			if (!TryConsume(l, it, var_name)) ThrowException("Variable name not provided", token);

			if (var_name.type != TokenType::TEXT) ThrowException("Invalid characters in variable name", var_name);

			auto scope = variable_scopes.rbegin(); // Start with the innermost scope

			while (!scope->contains(var_name.token))
			{
				if (++scope == variable_scopes.rend()) ThrowException("Variable is undefined", var_name);
			}

			const TokenList var = scope->at(var_name.token);
			TokenList var_copy;

			for (auto& tok : var)
			{
				var_copy.push_back({ tok.type, var_name.raw_token, tok.token, var_name.line, var_name.index });
			}

			it = l.insert(it, var_copy.begin(), var_copy.end());
		}
		return vars_replaced;
	}

	size_t Little32Assembler::ResolveRelatives(AssemblyLine& l) const
	{
		using namespace std;
		
		size_t relatives_replaced = 0;

		for (auto& a : l.args)
		{
			for (auto it = a.begin(); it != a.end(); it++)
			{
				if (it->type != TokenType::RELATIVE_MARKER) continue;

				it = a.erase(it);
				if (it->type != TokenType::INTEGER) ThrowException("Expected number after '.'", *it);

				long int addr = stoul(it->token) - l.addr;
				if (addr < 0)
				{
					addr *= -1;
					a.insert(it, { TokenType::MINUS, it->raw_token, "-", it->line, it->index });
				}

				relatives_replaced++;
				it->token = to_string(addr);
			}
		}

		return relatives_replaced;
	}
	
	size_t Little32Assembler::ResolveRegLists(TokenList& l) const
	{
		using namespace std;

		size_t lists_replaced = 0;
		auto it = l.begin();
		Token token;

		while (TryGet(l, it, token))
		{
			if (token.type != TokenType::LBRACE)
			{
				it++;
				continue;
			}
			lists_replaced++;

			word list = 0;
			it = l.erase(it); // Erase '{'

			while (TryConsume(l, it, token) && token.type != TokenType::RBRACE)
			{
				word reg1 = 0;
				for (; reg1 < 16; reg1++)
				{
					if (reg_names[reg1] == token.token) break;
				}
				if (reg1 == 16) ThrowException("Expected register in register list",token);

				if (!TryConsume(l, it, token)) ThrowException("Expected register list to continue",token);

				if (token.type == TokenType::COMMA || token.type == TokenType::RBRACE)
				{
					if (list & (1 << reg1)) ThrowException("Register list has duplicate registers",token);
					list |= 1 << reg1;
					if (token.type == TokenType::RBRACE) break;
				}
				else if (token.type == TokenType::MINUS)
				{
					if (!TryConsume(l, it, token)) ThrowException("Expected second register for range",token);
					word reg2 = 0;
					for (; reg2 < 16; reg2++)
					{
						if (reg_names[reg2] == token.token) break;
					}
					if (reg2 == 16) ThrowException("Expected register in register list",token);
					if (reg1 > reg2) ThrowException("Expected register range to be from min to max",token);
					if (reg1 == reg2) ThrowException("Expected register range to be greater than 1",token);
					word range = ((0xFFFF >> (reg1 + (15 - reg2))) << reg1) & 0xFFFF;

					if (list & range) ThrowException("Register list has duplicate registers",token);
					list |= range;

					if (TryGet(l, it, token) && token.type == TokenType::COMMA)
					{
						it = l.erase(it);
					}
				}
				else ThrowException("Unexpected value in register list",token);
			}

			if (token.type != TokenType::RBRACE) ThrowException("Register list must be closed",token);

			l.insert(it, { TokenType::INTEGER, it->raw_token, to_string(list), it->line, it->index });
		}

		return lists_replaced;
	}

	void Little32Assembler::AddLabel(const std::string& label, word address)
	{
		constant_addresses[label] = address;
		label_scopes.front()[label] = address;
	}

	void Little32Assembler::AddLabels(const std::unordered_map<std::string, word>& addresses)
	{
		for (auto& address : addresses)
		{
			constant_addresses[address.first] = address.second;
			label_scopes.front()[address.first] = address.second;
		}
	}

	bool Little32Assembler::GetLabel(const std::string& label, std::string& label_out) const
	{
		if (!label_scopes.front().contains(label)) return false;

		label_out = label_scopes.front().at(label);

		return true;
	}

	bool Little32Assembler::GetVariable(const std::string& variable, TokenList& var_out) const
	{
		if (!variable_scopes.front().contains(variable)) return false;

		var_out = variable_scopes.front().at(variable);

		return true;
	}

	void Little32Assembler::SetRAM(const RAM& ram)
	{
		this->ram = ram.memory.get();
		ram_start = ram.GetAddress();
		ram_range = ram.GetRange();
		ram_current_address = 0;
	}

	void Little32Assembler::SetROM(const ROM& rom)
	{
		this->rom = rom.memory.get();
		ram_start = rom.GetAddress();
		ram_range = rom.GetRange();
		ram_current_address = 0;
	}

	void Little32Assembler::ClearLabels() noexcept
		{ constant_addresses.clear(); }

	void Little32Assembler::FlushScopes() noexcept
	{
		variable_scopes = { {} };
		label_scopes = { constant_addresses };
		func_scopes = { const_replace };
		cond_scopes = { {false, 0} };
		file_stack = {};

		cond_scope_openings = {};
		variable_scope_openings = {};
		func_scope_openings = {};
		label_scope_openings = {};

		pending_labels = { {} };
		pending_memory_labels = { {} };
		pending_expressions = { };
	}

	inline std::list<Little32Assembler::AssemblyLine> Little32Assembler::ParseTokens(const std::filesystem::path working_dir, TokenList& tokens, bool print_intermediate)
	{
		using namespace std;

		list<AssemblyLine> assembly_lines = {};

		bool byte_mode = false;
		bool terminate_mode = true;

		while (!tokens.empty())
		{
			Token token = tokens.front();

			switch (token.type)
			{
			case TokenType::TEXT:
			{
				const Token& next = *std::next(tokens.begin());

				if (next.type == TokenType::MARKER_LABEL) // This is a label
				{
					if (label_scopes.back().contains(token.token)) ThrowException("Label already defined in scope", token);

					tokens.pop_front(); // Erase name
					tokens.pop_front(); // Erase ':'
					label_scopes.back()[token.token] = *current_address + memory_start;

					string num = to_string(label_scopes.back()[token.token]);

					auto lit = pending_labels.back().begin();
					while (lit != pending_labels.back().end())
					{
						if ((*lit)->token == token.token) // Resolve this label
						{
							(*lit)->token = num;
							(*lit)->type = TokenType::INTEGER;
							lit = pending_labels.back().erase(lit);
						}
						else // Needs another label
						{
							lit++;
						}
					}

					auto lmit = pending_memory_labels.back().begin();
					while (lmit != pending_memory_labels.back().end())
					{
						if (lmit->token->token == token.token) // Resolve this label
						{
							lmit->token->token = to_string(label_scopes.back()[token.token]);
							--lmit->expression->num_labels;

							// All the labels in the expression are resolved; proceed to assign memory
							if (lmit->expression->num_labels == 0)
							{
								TokenList answer = SolveExpression(lmit->expression->expression, lmit->expression->address);

								long long value = std::stoll(answer.back().token);

								if (answer.front().type == TokenType::MINUS) value = -value;

								if (lmit->expression->is_bool)
								{
									if (value < -255 || value > 255) ThrowException("Constant is too large for byte (" + std::to_string(value) + ")", answer.back());

									PutByte(lmit->expression->memory, lmit->expression->address, static_cast<byte>(value));
								}
								else
								{
									if (value < -4294967295 || value > 4294967295) ThrowException("Constant is too large for word (" + std::to_string(value) + ")", answer.back());

									lmit->expression->memory[lmit->expression->address >> 2] = static_cast<word>(value);
								}

								pending_expressions.erase(lmit->expression);
							}

							lmit = pending_memory_labels.back().erase(lmit);
						}
						else // Needs another label
						{
							lmit++;
						}
					}
					break;
				}
				else if (IsUpper(token.token) &&
					(
						//  Matches an instruction name
						find_if(
							instructions.begin(),
							instructions.end(),
							[&token](const pair<const string, Instruction>& def) -> bool
							{
								return def.first == token.token ||
									("N" + def.first == token.token && def.second.allow_N) ||
									(def.first + "S" == token.token && def.second.allow_S) ||
									("N" + def.first + "S" == token.token && def.second.allow_N && def.second.allow_S);
							}
						) != instructions.end()
						||
						// Matches a function name
						find_if(
							func_scopes.rbegin(),
							func_scopes.rend(),
							[&token](const list<OpReplace>& l) -> bool
							{
								return find_if(
									l.begin(),
									l.end(),
									[&token](const OpReplace& op) -> bool
									{
										return op.op == token.token || ("N" + op.op) == token.token || (op.op + "S") == token.token || ("N" + op.op + "S") == token.token;
									}
								) != l.end();
							}
						) != func_scopes.rend()
					)
				)
				{ // This is an instruction
					ParseInstruction(tokens, assembly_lines, pending_labels);
					break;
				}
				// Otherwise, fall through as a label used for a constant
			}

			case TokenType::LPAREN:
			case TokenType::RELATIVE_MARKER:
			case TokenType::NOT:
			case TokenType::MINUS:
			case TokenType::PLUS:
			case TokenType::INTEGER: // This is a constant value
			{
				tokens.pop_front();

				if (cur_start == nullptr)
				{
					cur_start = &data_start;
					cur_end = &data_end;
				}

				if (*cur_start == NULL_ADDRESS)
				{
					*cur_start = *current_address + memory_start;
					*cur_end = *cur_start;
				}

				if (byte_mode)
				{
					if (*current_address >= memory_range) ThrowException("Constant exceeds memory", token);
				}
				else
				{
					if (*current_address & 3) ThrowException("Word not aligned", token);
					if ((*current_address >> 2) >= (memory_range >> 2)) ThrowException("Constant exceeds memory", token);
				}

				TokenList expression = { token };

				Token next_token;

				size_t num_labels = token.type == TokenType::TEXT ? 1 : 0;

				while (TryGetFront(tokens, next_token) && next_token.type == TokenType::MARKER_VARIABLE)
				{
					tokens.pop_front();

					if(!TryConsumeFront(tokens, next_token)) ThrowException("Expected variable", next_token);

					auto scope = variable_scopes.rbegin(); // Start with the innermost scope

					while (!scope->contains(next_token.token))
					{
						if (++scope == variable_scopes.rend()) ThrowException("Variable is undefined", next_token);
					}

					const TokenList var = scope->at(next_token.token);
					TokenList var_copy;

					for (auto& tok : var)
					{
						var_copy.push_back({ tok.type, next_token.raw_token, tok.token, next_token.line, next_token.index });
					}

					tokens.insert(tokens.begin(), var_copy.begin(), var_copy.end());
				}

				if (TryGetFront(tokens, next_token) && ((token.type != TokenType::INTEGER && token.type != TokenType::TEXT) || (next_token.type >= TokenType::LPAREN && next_token.type <= TokenType::RPAREN)))
				{
					do
					{
						if (next_token.type == TokenType::MARKER_VARIABLE)
						{
							tokens.pop_front();

							if (!TryConsumeFront(tokens, next_token)) ThrowException("Expected variable", next_token);

							auto scope = variable_scopes.rbegin(); // Start with the innermost scope

							while (!scope->contains(next_token.token))
							{
								if (++scope == variable_scopes.rend()) ThrowException("Variable is undefined", next_token);
							}

							const TokenList var = scope->at(next_token.token);
							TokenList var_copy;

							for (auto& tok : var)
							{
								var_copy.push_back({ tok.type, next_token.raw_token, tok.token, next_token.line, next_token.index });
							}

							tokens.insert(tokens.begin(), var_copy.begin(), var_copy.end());

							continue;
						}

						if (token.type == TokenType::INTEGER || token.type == TokenType::TEXT || token.type == TokenType::RPAREN)
						{
							// Values can only be followed by operators
							if (next_token.type < TokenType::MINUS || next_token.type > TokenType::RPAREN)
							{
								break;
							}
						}
						else
						{
							// Operators can be followed by a value, unary operator, or parentheses
							if (next_token.type != TokenType::INTEGER && next_token.type != TokenType::TEXT
								&& (next_token.type < TokenType::LPAREN || next_token.type > TokenType::PLUS))
							{
								ThrowException("Expected value after operator", next_token);
							}
						}

						tokens.pop_front();
						expression.push_back(next_token);

						if (token.type == TokenType::TEXT) ++num_labels;

						token = next_token;
					}
					while (TryGetFront(tokens, next_token));
				}

				if (num_labels > 0)
				{
					for (auto& t : expression)
					{
						for (auto it = label_scopes.rbegin(); it != label_scopes.rend(); it++)
						{
							if (!it->contains(t.token)) continue;

							t.token = std::to_string((*it)[t.token]);
							t.type = TokenType::INTEGER;
							--num_labels;
							break;
						}
					}
				}

				if (num_labels > 0)
				{
					pending_expressions.push_back
					(
						{
							memory,
							*current_address,
							byte_mode,
							expression,
							num_labels
						}
					);

					for (auto it = pending_expressions.back().expression.begin(); it != pending_expressions.back().expression.end(); it++)
					{
						if (it->type != TokenType::TEXT) continue;

						pending_memory_labels.back().push_back(MemoryLabel(--pending_expressions.end(), &*it));
					}
				}
				else
				{
					TokenList answer = SolveExpression(expression, *current_address + memory_start);

					long long value = std::stoll(answer.back().token);

					if (answer.front().type == TokenType::MINUS) value = -value;

					if (byte_mode)
					{
						if (value < -255 || value > 255) ThrowException("Constant is too large for byte (" + std::to_string(value) + ")", answer.back());

						PutByte(memory, *current_address, static_cast<byte>(value));
					}
					else
					{
						if (value < -4294967295 || value > 4294967295) ThrowException("Constant is too large for word (" + std::to_string(value) + ")", answer.back());

						memory[*current_address >> 2] = static_cast<word>(value);
					}
				}

				(*current_address) += byte_mode ? 1 : 4;

				if (*current_address + memory_start > *cur_end)
				{
					*cur_end = *current_address + memory_start;
				}
			}
				break;

			case TokenType::STRING: // This is a string constant
			{
				tokens.pop_front();

				if (cur_start == nullptr)
				{
					cur_start = &data_start;
					cur_end = &data_end;
				}

				if (*cur_start == NULL_ADDRESS)
				{
					*cur_start = *current_address + memory_start;
					*cur_end = *cur_start;
				}

				if (*current_address + token.token.size() + terminate_mode >= memory_range) ThrowException("String exceeds memory", token);

				for (char c : token.token)
				{
					PutByte(memory, *current_address, c);
					(*current_address)++;
				}

				if (terminate_mode)
				{
					PutByte(memory, *current_address, 0);
					(*current_address)++;
				}

				if (*current_address + memory_start > *cur_end)
				{
					*cur_end = *current_address + memory_start;
				}
			}
				break;

			case TokenType::MARKER_PREPROCESSOR: // #
				ParsePreprocessor(working_dir, tokens, terminate_mode, byte_mode, print_intermediate);
				break;

			case TokenType::MARKER_FUNCTION: // @
				ParseFunction(tokens);
				break;

			case TokenType::MARKER_VARIABLE: // $
				ParseVariable(tokens);
				break;

			case TokenType::SCOPE_FUNCTION_OPEN: // @{
			{
				tokens.pop_front();
				func_scope_openings.push_back(token);
				func_scopes.push_back({});
			}
				break;

			case TokenType::SCOPE_FUNCTION_CLOSE: // }@
			{
				tokens.pop_front();

				if (func_scope_openings.empty()) ThrowException("Unmatched closing scope", token);

				func_scope_openings.pop_back();
				func_scopes.pop_back();
			}
				break;

			case TokenType::SCOPE_VARIABLE_OPEN: // ${
			{
				tokens.pop_front();
				variable_scope_openings.push_back(token);
				variable_scopes.push_back({});
			}
				break;

			case TokenType::SCOPE_VARIABLE_CLOSE: // }$
			{
				tokens.pop_front();

				if (variable_scope_openings.empty()) ThrowException("Unmatched closing scope", token);

				variable_scope_openings.pop_back();
				variable_scopes.pop_back();
			}
				break;

			case TokenType::SCOPE_CONDITION_OPEN: // ?{
			{
				tokens.pop_front();
				cond_scope_openings.push_back(token);
				cond_scopes.push_back({});

				if (!TryGetFront(tokens, token)) break;
				if (token.type != TokenType::TEXT) break;
				if (!IsCond(token.token)) break;

				tokens.pop_front();
				cond_scopes.back().has_cond = true;
				cond_scopes.back().cond = cond_names.at(token.token);
			}
				break;

			case TokenType::SCOPE_CONDITION_CLOSE: // }?
			{
				tokens.pop_front();

				if (cond_scope_openings.empty()) ThrowException("Unmatched closing scope", token);

				cond_scope_openings.pop_back();
				cond_scopes.pop_back();
			}
				break;

			case TokenType::SCOPE_LABEL_OPEN: // :{
			{
				tokens.pop_front();

				label_scope_openings.push_back(token);
				label_scopes.push_back({});
				pending_labels.push_back({});
				pending_memory_labels.push_back({});
			}
				break;

			case TokenType::SCOPE_LABEL_CLOSE: // }:
			{
				if (label_scope_openings.empty()) ThrowException("Unmatched closing scope", token);

				tokens.pop_front();
				label_scope_openings.pop_back();

				std::list<Token*> cur_scope = pending_labels.back();
				pending_labels.pop_back();

				if (!cur_scope.empty())
				{
					std::list<Token*>& lower_list = pending_labels.back(); // Get second last list
					lower_list.splice(lower_list.end(), cur_scope);  // Move labels down to second last list
				}

				std::list<MemoryLabel> cur_mem_scope = pending_memory_labels.back();
				pending_memory_labels.pop_back();

				if (!cur_mem_scope.empty())
				{
					std::list<MemoryLabel>& lower_list = pending_memory_labels.back(); // Get second last list
					lower_list.splice(lower_list.end(), cur_mem_scope);  // Move labels down to second last list
				}

				label_scopes.pop_back();
			}
				break;

			case TokenType::END_LINE: // \n
				tokens.pop_front();
				break;

			case TokenType::END_FILE:
				tokens.pop_front();
				break;

			case TokenType::INVALID:
				ThrowException("Unrecognised token", token);
				
			default:
				ThrowException("Unexpected token", token);
			}
		}

		return assembly_lines;
	}

	inline void Little32Assembler::ParseFunction(TokenList& tokens)
	{
		Token token;
		//if (!TryGetFront(tokens, token) || token.type != TokenType::MARKER_FUNCTION) return;

		tokens.pop_front();

		func_scopes.back().push_back({});
		OpReplace& new_op = func_scopes.back().back();

		if (!TryConsumeFront(tokens, token) || token.type != TokenType::TEXT) ThrowException("Expected function name", token);
		new_op.op = token.token;
		if (!IsUpper(new_op.op)) ThrowException("Function name must be uppercase", token);

		if (TryGetFront(tokens, token) && token.type == TokenType::ARG_NUM)
		{
			tokens.pop_front();

			new_op.requiredArgs = stoi(token.token);
		}

		if (new_op.op.front() == 'N') ThrowException("Function name cannot start with N", token);
		if (new_op.op.back() == 'S') ThrowException("Function name cannot end with S", token);

		if (!TryConsumeFront(tokens, token) || token.type != TokenType::TEXT) ThrowException("Expected instruction to assign function", token);
		if (!IsUpper(token.token)) ThrowException("Instruction name must be uppercase", token);
		new_op.newop = token.token;

		if (new_op.newop == "N") ThrowException("Function replacement instruction is only N flag", token);
		if (new_op.newop == "S") ThrowException("Function replacement instruction is only S flag", token);
		if (new_op.newop == "NS") ThrowException("Function replacement instruction is only flags", token);

		if (new_op.newop.front() == 'N')
		{
			new_op.newop = new_op.newop.substr(1);
			new_op.newN = true;
		}

		if (new_op.newop.back() == 'S')
		{
			new_op.newop = new_op.newop.substr(0, new_op.newop.length() - 1);
			new_op.newS = true;
		}

		new_op.has_cond = GetCond(tokens, new_op.new_cond);

		if (new_op.has_cond)
		{
			if (cond_scopes.back().has_cond) ThrowException("Function overwrites the scope condition", token);
		}
		else
		{
			new_op.new_cond = cond_scopes.back().cond;
			new_op.has_cond = cond_scopes.back().has_cond;
		}

		new_op.has_shift = GetShift(tokens, new_op.new_shift);

		while (TryConsumeFront(tokens, token))
		{
			if (token.type == TokenType::VARGS)
			{
				if (new_op.requiredArgs != -1) ThrowException("Cannot use '...' in constant length function", token);
				new_op.tokens.push_back(token);
			}
			else if (token.type == TokenType::MARKER_FUNCTION)
			{
				if (new_op.requiredArgs == NULL_ADDRESS) ThrowException("Cannot use '@' in variable length function", token);

				new_op.tokens.push_back(token);

				if (!TryConsumeFront(tokens, token) || token.type != TokenType::INTEGER) ThrowException("'@' must be followed by a number", token);
				if (stoi(token.token) >= new_op.requiredArgs) ThrowException("Arg number greater than number of args", token);

				new_op.tokens.push_back(token);
			}
			else
			{
				new_op.tokens.push_back(token);
			}
		}

		total_ops_defined++;
	}

	inline void Little32Assembler::ParseVariable(TokenList& tokens)
	{
		Token variable;
		//if (!TryGetFront(tokens, token) || token.type != TokenType::MARKER_VARIABLE) return;

		tokens.pop_front(); // Remove marker

		if (!TryConsumeFront(tokens, variable)) ThrowException("Variable name not provided", variable);
		if (variable.type != TokenType::TEXT) ThrowException("Invalid characters in variable name", variable);

		Token token;

		if (TryGetFront(tokens, token) && token.type == TokenType::ASSIGNMENT)
		{
			tokens.pop_front(); // Remove assignment

			if (!variable_scopes.back().contains(variable.token))
			{
				total_variables_defined++;
			}
			else
			{
				variable_scopes.back()[variable.token] = {};
			}

			if (!TryConsumeFront(tokens, token)) ThrowException("Value to assign variable not provided", variable);

			do {
				variable_scopes.back()[variable.token].push_back(token);
			} while (TryConsumeFront(tokens, token));
		}
		else
		{
			auto scope = variable_scopes.rbegin(); // Start with the innermost scope

			while (!scope->contains(variable.token))
			{
				if (++scope == variable_scopes.rend()) ThrowException("Variable is undefined", variable);
			}

			const TokenList var = scope->at(variable.token);
			TokenList var_copy;

			for (auto& tok : var)
			{
				var_copy.push_back({ tok.type, variable.raw_token, tok.token, variable.line, variable.index });
			}

			tokens.insert(tokens.begin(), var_copy.begin(), var_copy.end());
		}
	}

	inline void Little32Assembler::ParseInstruction(TokenList& tokens, std::list<AssemblyLine>& assembly_lines, std::list<std::list<Token*>>& pending_labels)
	{
		Token token;
		//if (!TryGetFront(tokens, token) || token.type != TokenType::TEXT) return;

		if (*current_address & 3) ThrowException("Instruction not aligned", token);

		if (program_start == NULL_ADDRESS)
		{
			program_start = *current_address + memory_start;
			program_end = program_start;
		}

		token = tokens.front();
		tokens.pop_front();

		assembly_lines.push_back({
			token.line,
			memory_start + *current_address,
			memory + (*current_address >> 2),
			token
		});
		AssemblyLine& instruction = assembly_lines.back();

		if (instruction.code.token == "N") ThrowException("Instruction is only N flag", token);
		if (instruction.code.token == "S") ThrowException("Instruction is only S flag", token);
		if (instruction.code.token == "NS") ThrowException("Instruction is only flags", token);

		if (instruction.code.token.front() == 'N')
		{
			instruction.code.token = instruction.code.token.substr(1);
			instruction.N = true;
		}

		if (instruction.code.token.back() == 'S')
		{
			instruction.code.token = instruction.code.token.substr(0, instruction.code.token.length() - 1);
			instruction.S = true;
		}

		instruction.has_cond = GetCond(tokens, instruction.cond);
		instruction.has_shift = GetShift(tokens, instruction.shift);

		total_variables_replaced += ResolveVariables(tokens);
		SplitSquareBrackets(tokens);
		ResolveRegLists(tokens);

		if (TryConsumeFront(tokens, token))
		{
			instruction.args.push_back({});
			TokenList* instruction_arg = &instruction.args.back();

			do
			{
				if (token.type == TokenType::COMMA)
				{
					if (instruction_arg->empty() || tokens.front().type == TokenType::END_LINE) ThrowException("Empty instruction parameter", token);

					instruction.args.push_back({});
					instruction_arg = &instruction.args.back();
				}
				else
				{
					instruction_arg->push_back(token);
				}
			} while (TryConsumeFront(tokens, token));
		}

		OpReplace* op = nullptr;

		std::unordered_set<OpReplace*> used_replacements = {};

		// Replace instruction with preprocessor instruction
		do
		{
			op = nullptr;
			for (auto scope_it = func_scopes.rbegin(); scope_it != func_scopes.rend(); scope_it++)
			{
				for (auto& op_it : *scope_it)
				{
					if (op_it.op != instruction.code.token) continue;
					if (op_it.requiredArgs == -1)
					{
						if (op == nullptr) op = &op_it;
						continue;
					}
					if (op_it.requiredArgs != instruction.args.size()) continue;

					op = &op_it;
					break;
				}
				if (op == nullptr) continue;
				if (op->requiredArgs != instruction.args.size()) continue;
				break;
			}

			if (op == nullptr) break;

			if (used_replacements.contains(op)) ThrowException("This function is recursive", token);
			instruction.code.token = op->newop; // Replace instruction name
			used_replacements.insert(op);

			if (op->has_cond)
			{ // Replace condition
				if (instruction.has_cond) ThrowException("Function overwrites condition", token);
				instruction.cond = op->new_cond;
				instruction.has_cond = true;
			}

			if (op->has_shift)
			{
				if (instruction.has_shift) ThrowException("Function overwrites rotation shift", token);
				instruction.shift = op->new_shift;
				instruction.has_shift = true;
			}

			std::vector<TokenList> old_args = {};
			old_args.insert(old_args.begin(), instruction.args.begin(), instruction.args.end());

			instruction.args = {};

			TokenList op_tokens = op->tokens;
			total_variables_replaced += ResolveVariables(op_tokens);
			SplitSquareBrackets(op_tokens);
			ResolveRegLists(op_tokens);

			if (op_tokens.empty()) continue;

			instruction.args.push_back({});
			TokenList* tokens = &instruction.args.back();
			do
			{
				if (op_tokens.front().type == TokenType::COMMA)
				{
					op_tokens.pop_front(); // Erase ','

					if (tokens->empty() || op_tokens.empty()) ThrowException("Function parameter resolved as empty", token);

					instruction.args.push_back({});
					tokens = &instruction.args.back();
				}
				else
				{
					tokens->push_back(op_tokens.front());
					op_tokens.pop_front();
				}
			} while (!op_tokens.empty());

			if (op->requiredArgs > 0)
			{
				for (auto& a : instruction.args)
				{
					auto b_it = a.begin();
					while (TryGet(a, b_it, token))
					{
						if (token.type != TokenType::MARKER_FUNCTION)
						{
							b_it++;
							continue;
						}

						b_it = a.erase(b_it);  // Erase '@'
						int arg_num = stoi(b_it->token); // String was verified as numeric earlier
						b_it = a.erase(b_it);  // Erase number
						a.insert(b_it, old_args[arg_num].begin(), old_args[arg_num].end()); // Number was verified as in range earlier
					}
				}
			}
			else if (op->requiredArgs == NULL_ADDRESS)
			{
				auto a_it = instruction.args.begin();
				while (a_it != instruction.args.end())
				{
					TokenList arg = {};
					arg.splice(arg.begin(), *a_it);
					auto b_it = arg.begin();
					while (TryConsume(arg, b_it, token))
					{
						if (token.type != TokenType::VARGS)
						{
							a_it->push_back(token);
							continue;
						}

						if (old_args.empty()) continue;

						auto c_it = old_args.begin();
						a_it->insert(a_it->end(), c_it->begin(), c_it->end());
						c_it++;

						while (c_it != old_args.end())
						{
							a_it++;
							a_it = instruction.args.insert(a_it, *c_it);
							c_it++;
						}
					}
					if (a_it->empty()) ThrowException("Function parameter resolved as empty", token);
					a_it++;
				}
			}
		} while (op != nullptr);

		if (instruction.has_cond)
		{
			if (cond_scopes.back().has_cond) ThrowException("Instruction overwrites the scope condition", token);
		}
		else
		{
			instruction.cond = cond_scopes.back().cond;
			instruction.has_cond = cond_scopes.back().has_cond;
		}

		for (auto& a : instruction.args)
		{
			if (a.empty()) ThrowException("Argument cannot be empty", token);

			if (a.front().type >= TokenType::MULTIPLY && a.front().type <= TokenType::RPAREN)
				ThrowException("Expected '" + a.front().token + "' to follow a value", a.front());

			if (a.back().type >= TokenType::LPAREN && a.back().type <= TokenType::RSHIFT)
				ThrowException("Expected '" + a.back().token + "' to precede a value", a.front());

			if (a.size() == 1 && a.front().type == TokenType::REGISTER) continue;
			if (a.size() == 2 && a.front().type == TokenType::MINUS && a.back().type == TokenType::REGISTER) continue;

			for (auto& t : a)
			{
				if (t.type == TokenType::REGISTER)
				{
					ThrowException("Cannot use register in mixed expression", t);
				}

				if ((t.type >= TokenType::LPAREN && t.type <= TokenType::RPAREN) || t.type == TokenType::INTEGER)
					continue;

				if (t.type != TokenType::TEXT) ThrowException("Unexpected token '" + t.token + "' in argument", t);

				pending_labels.back().push_back(&t);

				for (auto it = label_scopes.rbegin(); it != label_scopes.rend(); it++)
				{
					if (!it->contains(t.token)) continue;

					t.token = std::to_string((*it)[t.token]);
					t.type = TokenType::INTEGER;
					pending_labels.back().pop_back();
					break;
				}
			}
		}

		(*current_address) += 4;

		if (*current_address + memory_start > program_end)
		{
			program_end = *current_address + memory_start;
		}
	}

	inline void Little32Assembler::ParsePreprocessor(const std::filesystem::path working_dir, TokenList& tokens, bool& terminate_mode, bool& byte_mode, bool print_intermediate)
	{
		Token token;
		//if (!TryGetFront(tokens, token) || token.type != TokenType::MARKER_PREPROCESSOR) return;

		tokens.pop_front();

		if (!TryConsumeFront(tokens, token) || token.type != TokenType::TEXT) ThrowException("Expected preprocessor directive", token);

		if (token.token == "ALIGN")
		{
			if (!TryConsumeFront(tokens, token) || token.type != TokenType::INTEGER) ThrowException("Expected alignment width", token);

			const word width = stoul(token.token);
			const word err = (*current_address + memory_start) % width;
			if (err == 0) return;
			(*current_address) += width - err;
		}
		else if (token.token == "ASCII")
		{
			terminate_mode = false;
		}
		else if (token.token == "ASCIZ")
		{
			terminate_mode = true;
		}
		else if (token.token == "ASSEMBLE")
		{
			if (!TryConsumeFront(tokens, token) || token.type != TokenType::STRING) ThrowException("Expected file name", token);

			auto file_path = (working_dir / token.token).lexically_normal();

			std::ifstream file;

			file.open(file_path);

			if (!file.is_open()) ThrowException("Could not read assembly from file", token);

			Assemble(file_path, file, print_intermediate);
		}
		else if (token.token == "BLOCK")
		{
			if (!TryConsumeFront(tokens, token) || token.type != TokenType::INTEGER) ThrowException("Expected block size", token);

			word size = stoul(token.token);
			if (size == 0) ThrowException("Expected nonzero memory block", token);
			if (*current_address + size >= memory_range) ThrowException("Block exceeds memory", token);

			if (cur_start == nullptr)
			{
				cur_start = &data_start;
				cur_end = &data_end;
			}

			if (*cur_start == NULL_ADDRESS)
			{
				*cur_start = *current_address + memory_start;
				*cur_end = *cur_start;
			}

			for (word i = 0; i < size; i++)
			{
				PutByte(memory, *current_address + i, 0);
			}

			(*current_address) += size;

			if (*current_address + memory_start > *cur_end)
			{
				*cur_end = *current_address + memory_start;
			}
		}
		else if (token.token == "BYTE")
		{
			byte_mode = true;
		}
		else if (token.token == "DATA")
		{
			data_start = *current_address + memory_start;

			if (data_end == NULL_ADDRESS || *current_address + memory_start > data_end)
			{
				data_end = *current_address + memory_start;
			}

			cur_start = &data_start;
			cur_end = &data_end;
		}
		else if (token.token == "ENTRY")
		{
			if (entry_point != Little32Assembler::NULL_ADDRESS) ThrowException("Multiple entry points defined", token);
			if ((*current_address + memory_start) % 4) ThrowException("Entry point is not word-aligned", token);

			entry_point = *current_address + memory_start;
		}
		else if (token.token == "FILE")
		{
			if (cur_start == nullptr)
			{
				cur_start = &data_start;
				cur_end = &data_end;
			}

			if (*cur_start == NULL_ADDRESS)
			{
				*cur_end = *cur_start = *current_address + memory_start;
			}

			if (*current_address & 3) ThrowException("File not word aligned", token);
			if (!TryConsumeFront(tokens, token) || token.type != TokenType::STRING) ThrowException("Expected file name", token);

			auto file_path = (working_dir / token.token).lexically_normal();

			std::ifstream file;
			file.open(file_path);

			if (!file.is_open()) ThrowException("Could not read from file", token);

			std::string file_contents;
			StreamToString(file, file_contents);

			if (*current_address + 5 + file_contents.length() > memory_range) ThrowException("File is too long for memory", token);

			memory[*current_address >> 2] = file_contents.length();
			*current_address += 4;

			for (const char c : file_contents)
			{
				PutByte(memory, (*current_address)++, c);
			}

			PutByte(memory, (*current_address)++, 0);

			if (*current_address + memory_start > *cur_end)
			{
				*cur_end = *current_address + memory_start;
			}
		}
		else if (token.token == "LINES")
		{
			if (cur_start == nullptr)
			{
				cur_start = &data_start;
				cur_end = &data_end;
			}

			if (*cur_start == NULL_ADDRESS)
			{
				*cur_end = *cur_start = *current_address + memory_start;
			}

			if (*current_address & 3) ThrowException("File not word aligned", token);
			if (!TryConsumeFront(tokens, token) || token.type != TokenType::STRING) ThrowException("Expected file name", token);

			auto file_path = (working_dir / token.token).lexically_normal();

			std::ifstream file;
			file.open(file_path);

			if (!file.is_open()) ThrowException("Could not read lines from file", token);

			std::vector<std::string> lines;
			std::string line;

			size_t total_size = 0;

			while (std::getline(file, line))
			{
				lines.push_back(line);
				total_size += 4; // Pointer
				total_size += line.length() + 1; // Length of line and null terminator
			}

			if (*current_address + 4 + total_size > memory_range) ThrowException("Lines are too long for memory", token);

			memory[*current_address >> 2] = lines.size();
			*current_address += 4;

			word line_ptr = *current_address + 4 * lines.size();

			for (auto& l : lines)
			{
				memory[*current_address >> 2] = memory_start + line_ptr;
				*current_address += 4;

				for (const char c : l)
				{
					PutByte(memory, line_ptr++, c);
				}
				PutByte(memory, line_ptr++, 0);
			}

			*current_address = line_ptr;

			if (*current_address + memory_start > *cur_end)
			{
				*cur_end = *current_address + memory_start;
			}
		}
		else if (token.token == "PROGRAM")
		{
			program_start = *current_address + memory_start;

			if (program_end == NULL_ADDRESS || *current_address + memory_start > program_end)
			{
				program_end = *current_address + memory_start;
			}

			cur_start = &program_start;
			cur_end = &program_end;
		}
		else if (token.token == "RAM")
		{
			if (memory == ram) return;
			if (ram == nullptr)
			{
				if (!TryGetFront(tokens, token)) return;
				if (token.token != "FORCE") return;
				tokens.pop_front();
				ThrowException("RAM is not available on this system", token);
			}

			memory = ram;
			memory_start = ram_start;
			memory_range = ram_range;
			current_address = &ram_current_address;
		}
		else if (token.token == "RANDOM")
		{
			if (!TryConsumeFront(tokens, token) || token.type != TokenType::INTEGER) ThrowException("Expected random size", token);

			word size = stoul(token.token);
			if (size == 0) ThrowException("Expected nonzero random size", token);
			if (*current_address + size >= memory_range) ThrowException("Random data exceeds memory", token);

			if (cur_start == nullptr)
			{
				cur_start = &data_start;
				cur_end = &data_end;
			}

			if (*cur_start == NULL_ADDRESS)
			{
				*cur_end = *cur_start = *current_address + memory_start;
			}

			word bytes = sizeof(word);
			word val = rand();
			PutByte(memory, (*current_address)++, val);
			size--;

			while (size > 0)
			{
				bytes--;
				val >>= 8;

				if (bytes == 0)
				{
					bytes = sizeof(word);
					val = rand();
				}

				PutByte(memory, (*current_address)++, val);
				size--;
			}

			if (*current_address + memory_start > *cur_end)
			{
				*cur_end = *current_address + memory_start;
			}
		}
		else if (token.token == "ROM")
		{
			if (memory == rom) return;
			if (rom == nullptr)
			{
				if (!TryGetFront(tokens, token)) return;
				if (token.token != "FORCE") return;
				tokens.pop_front();
				ThrowException("ROM is not available on this system", token);
			}

			memory = rom;
			memory_start = rom_start;
			memory_range = rom_range;
			current_address = &rom_current_address;
		}
		else if (token.token == "SEED")
		{
			if (!TryGetFront(tokens, token) || token.type != TokenType::INTEGER)
			{
				srand((unsigned int)time(0));
			}
			else
			{
				tokens.pop_front();
				srand(stoul(token.token));
			}
		}
		else if (token.token == "WORD")
		{
			byte_mode = false;
		}
		else ThrowException("Preprocessor directive not recognised", token);
	}

	Little32Assembler::TokenList Little32Assembler::SolveExpression(const TokenList& tokens, const word address)
	{
		assert(!tokens.empty());

		// Integer or register
		if (tokens.size() == 1)
		{
			if (tokens.front().type == TokenType::REGISTER ||
				tokens.front().type == TokenType::INTEGER) return tokens;
			else ThrowException("Unexpected token in argument", tokens.front());
		}

		// ~int or -int or -reg
		if (tokens.size() == 2)
		{
			switch (tokens.back().type)
			{
			case TokenType::REGISTER:
				if (tokens.front().type != TokenType::MINUS) ThrowException("Unexpected token before register", tokens.front());
				return tokens;

			case TokenType::INTEGER:
				if (tokens.front().type == TokenType::NOT) break;
				else if (tokens.front().type != TokenType::MINUS) ThrowException("Unexpected token before integer", tokens.front());
				return tokens;

			default:
				ThrowException("Unexpected token in argument", tokens.back());
			}
		}

		size_t paren_depth = 0;
		TokenList expression; // This expression without any parentheses

		TokenList paren_contents;

		// Resolve parentheses sub-expressions
		for (auto& tok : tokens)
		{
			switch(tok.type)
			{
			case TokenType::LPAREN:
				if (paren_depth == 0) paren_contents.clear();

				paren_depth++;
				break;

			case TokenType::RPAREN:
				if (paren_depth == 0) ThrowException("Too many closing parentheses", tok);

				paren_depth--;

				if (paren_depth != 0) break;
				else if (paren_contents.empty()) ThrowException("Empty parentheses in expression", tok);

				expression.splice(expression.end(), SolveExpression(paren_contents, *current_address + memory_start));
				break;

			case TokenType::NOT:
			case TokenType::MINUS:
			case TokenType::PLUS:
			case TokenType::MULTIPLY:
			case TokenType::DIVIDE:
			case TokenType::MODULO:
			case TokenType::OR:
			case TokenType::AND:
			case TokenType::XOR:
			case TokenType::LSHIFT:
			case TokenType::RSHIFT:
			case TokenType::INTEGER:
				if (paren_depth == 0)
				{
					expression.push_back(tok);
				}
				else
				{
					paren_contents.push_back(tok);
				}
				break;

			case TokenType::REGISTER:
				ThrowException("Registers are not accepted in integer expressions", tok);

			default:
				ThrowException("Unknown token in expression", tok);
			}
		}

		if (expression.back().type != TokenType::INTEGER) ThrowException("Expected integer at end of expression", expression.back());
		
		bool was_op = false;
		TokenList::iterator last_num = --expression.end();
		int32_t last_val;
		bool changed_val = false;
		TokenList::iterator last_op;

		// Resolve unary operators from back to front
		for (TokenList::reverse_iterator it = ++expression.rbegin(); it != expression.rend(); ++it)
		{
			if (it->type == TokenType::INTEGER)
			{
				if (changed_val)
				{
					last_num->token = std::to_string(last_val);
					changed_val = false;
				}
				if(!was_op) ThrowException("Expected operator/s between integers", expression.back());
				last_num = --(it.base());
				was_op = false;
				continue;
			}
			else if (!was_op)
			{
				last_op = --(it.base());
				was_op = true;

				continue;
			}

			auto last_token = *last_op;

			*last_op = *it;
			expression.erase(--(it.base()));
			--it;

			if (last_token.type == TokenType::PLUS) continue;

			if (!changed_val)
			{
				last_val = static_cast<int32_t>(std::stol(last_num->token));
				changed_val = true;
			}

			switch (last_token.type)
			{
			case TokenType::MINUS:
				last_val = -last_val;
				break;
			case TokenType::NOT:
				last_val = ~last_val;
				break;
			case TokenType::RELATIVE_MARKER:
				last_val = static_cast<int32_t>(static_cast<word>(last_val) - address);
				break;
			default:
				ThrowException("Unknown unary operator", last_token);
			}
		}

		// If an operator is at the very front it should be applied
		if (was_op)
		{
			auto last_token = *last_op;

			expression.erase(expression.begin());

			if (last_token.type != TokenType::PLUS)
			{
				if (!changed_val)
				{
					last_val = static_cast<int32_t>(std::stol(last_num->token));
					changed_val = true;
				}

				switch (last_token.type)
				{
				case TokenType::MINUS:
					last_val = -last_val;
					break;
				case TokenType::NOT:
					last_val = ~last_val;
					break;
				case TokenType::RELATIVE_MARKER:
					last_val = static_cast<int32_t>(static_cast<word>(last_val) - address);
					break;
				default:
					ThrowException("Unknown unary operator", last_token);
				}

				last_num->token = std::to_string(last_val);
			}
			else if (changed_val)
			{
				last_num->token = std::to_string(last_val);
			}
		}

		std::vector<int32_t> values;
		std::vector<TokenType> operators;

		values.push_back(static_cast<int32_t>(std::stol(expression.front().token)));

		for (auto it = ++expression.begin(); it != expression.end();)
		{
			operators.push_back(it->type);
			++it;
			values.push_back(static_cast<int32_t>(std::stol(it->token)));
			++it;
		}

		struct Operation
		{
			TokenType type;
			int32_t(*op)(int32_t, int32_t);
		};

		const Operation ops1[] =
		{
			{
				TokenType::MULTIPLY,
				[](int32_t a, int32_t b) -> int32_t { return a * b; }
			},
			{
				TokenType::DIVIDE,
				[](int32_t a, int32_t b) -> int32_t { return a / b; }
			},
			{
				TokenType::MODULO,
				[](int32_t a, int32_t b) -> int32_t { return a % b; }
			}
		};
		const Operation ops2[] =
		{
			{
				TokenType::PLUS,
				[](int32_t a, int32_t b) -> int32_t { return a + b; }
			},
			{
				TokenType::MINUS,
				[](int32_t a, int32_t b) -> int32_t { return a - b; }
			}
		};
		const Operation ops3[] =
		{
			{
				TokenType::LSHIFT,
				[](int32_t a, int32_t b) -> int32_t { return a << b; }
			},
			{
				TokenType::RSHIFT,
				[](int32_t a, int32_t b) -> int32_t { return a >> b; }
			}
		};
		const Operation ops4[] =
		{
			{
				TokenType::AND,
				[](int32_t a, int32_t b) -> int32_t { return a & b; }
			}
		};
		const Operation ops5[] =
		{
			{
				TokenType::XOR,
				[](int32_t a, int32_t b) -> int32_t { return a ^ b; }
			}
		};
		const Operation ops6[] =
		{
			{
				TokenType::OR,
				[](int32_t a, int32_t b) -> int32_t { return a | b; }
			}
		};

		struct OpLevel
		{
			const Operation* op;
			const size_t op_count;
		};

		const OpLevel ops[] =
		{
			{ ops1, sizeof(ops1) / sizeof(Operation) },
			{ ops2, sizeof(ops2) / sizeof(Operation) },
			{ ops3, sizeof(ops3) / sizeof(Operation) },
			{ ops4, sizeof(ops4) / sizeof(Operation) },
			{ ops5, sizeof(ops5) / sizeof(Operation) },
			{ ops6, sizeof(ops6) / sizeof(Operation) }
		};

		for (auto& op : ops)
		{
			for (size_t i = 0; i < operators.size();)
			{
				size_t j = op.op_count;

				while (j--)
				{
					if (operators[i] == op.op[j].type) goto op_found;
				}

				++i;
				continue;

			op_found:
				values[i + 1] = op.op[j].op(values[i], values[i + 1]);
				values.erase(values.begin() + i);
				operators.erase(operators.begin() + i);
			}
		}

		assert(operators.empty());
		assert(values.size() == 1);

		TokenList output;

		if (values[0] < 0)
		{
			output.push_back({ TokenType::MINUS, {}, "-" });
			values[0] = -values[0];
		}

		output.push_back(
		{
			TokenType::INTEGER,
			{
				tokens.front().raw_token.data(),
				(tokens.back().raw_token.data() - tokens.front().raw_token.data()) + tokens.back().raw_token.size()
			},
			std::to_string(values[0]),
			tokens.front().line,
			tokens.front().index
		});

		return output;
	}

	void Little32Assembler::Assemble(const std::filesystem::path file_path, std::istream& code, bool print_intermediate)
	{
		using namespace std;

		string file_contents;
		StreamToString(code, file_contents);

		Assemble(file_path, file_contents, print_intermediate);
	}

	void Little32Assembler::Assemble(const std::filesystem::path file_path, std::string_view file_contents, bool print_intermediate)
	{
		using namespace std;

		const auto norm_path = file_path.lexically_normal();

		if (file_stack.empty())
		{
			FlushScopes();
		}
		else
		{
			for (auto file : file_stack)
			{
				if (norm_path == file) throw std::runtime_error("Infinite assembly loop detected (File: '" + norm_path.string() + "')");
			}
		}

		file_stack.push_back(norm_path);

		// Strip byte order mark, if it exists, as unicode is ignored anyway
		if (file_contents.size() >= 2)
		{
			const char a = file_contents[0];
			const char b = file_contents[1];
			const char c = file_contents.size() > 2 ? file_contents[2] : 0;
			const char d = file_contents.size() > 3 ? file_contents[3] : 0;

			if (
				file_contents.size() >= 4 &&
				(
					(a == (char)0x00 && b == (char)0x00 && c == (char)0xfe && d == (char)0xff) ||
					(a == (char)0xff && b == (char)0xfe && c == (char)0x00 && d == (char)0x00)
				)
			)
			{
				file_contents = file_contents.substr(4);
			}
			else if (
				file_contents.size() >= 3 &&
				(
					(a == (char)0xef && b == (char)0xbb && c == (char)0xbf)
				)
			)
			{
				file_contents = file_contents.substr(3);
			}
			else if (
				(
					(a == (char)0xfe && b == (char)0xff) ||
					(a == (char)0xff && b == (char)0xfe)
				)
			)
			{
				file_contents = file_contents.substr(2);
			}
		}

		if (file_contents.empty()) return;

		srand((unsigned int)time(0));

		static const unordered_map<PackType, size_t> pack_args
		{
			{PackType::None,         0},
			{PackType::BranchOffset, 1},
			{PackType::RegList,      2},
			{PackType::Reg2,         2},
			{PackType::Flex2,        2},
			{PackType::Flex2i,       2},
			{PackType::Flex3,        3},
			{PackType::Flex3i,       3},
			{PackType::Reg3,         3},
		};

		if (rom != nullptr) // Prefer ROM - lends memory safety to code
		{
			memory = rom;
			memory_start = rom_start;
			memory_range = rom_range;
			current_address = &rom_current_address;
		}
		else if (ram != nullptr)
		{
			memory = ram;
			memory_start = ram_start;
			memory_range = ram_range;
			current_address = &ram_current_address;
		}
		else throw exception("Assembler has no memory to write to");

		bool entry_defined = false;

		cond_scopes = { {false, 0} };

		TokenList file_tokens = {};
		size_t pos = 0;
		RawLine line =
		{
			1,
			file_contents.substr(0, file_contents.find_first_of('\n'))
		};
		size_t line_no = 1;
		GetTokens(file_contents, line, file_tokens, pos);
		file_tokens.push_back({ TokenType::END_FILE, file_contents.substr(file_contents.size(),0), "", line, pos });

		ConvertNumbers(file_tokens);
		list<AssemblyLine> assembly_lines = ParseTokens(file_path.parent_path(), file_tokens, print_intermediate);

		if (file_stack.size() == 1)
		{
			if (!label_scope_openings.empty())
			{
				FlushScopes();
				ThrowException("Unmatched opening scope", label_scope_openings.back());
			}
			if (!cond_scope_openings.empty())
			{
				FlushScopes();
				ThrowException("Unmatched opening scope", cond_scope_openings.back());
			}
			if (!func_scope_openings.empty())
			{
				FlushScopes();
				ThrowException("Unmatched opening scope", func_scope_openings.back());
			}
			if (!pending_labels.front().empty())
			{
				ThrowException("Could not resolve label", *pending_labels.front().front());
			}
			if (!pending_memory_labels.front().empty())
			{
				ThrowException("Could not resolve label", *pending_memory_labels.front().front().token);
			}
		}

		if (print_intermediate)
		{
			printf("Intermediate output:\n");
			for (auto& l : assembly_lines)
			{
				printf("0x%08X: %s%s%s ", l.addr, l.N ? "N" : "", l.code.token.c_str(), l.S ? "S" : "");
				for (auto& a : l.args)
				{
					for (auto& t : a)
					{
						printf("%s", t.token.c_str());
					}
					if (&a != &(l.args.back())) printf(", ");
				}
				if (l.has_cond) printf(" ?%s", cond_names_regular[l.cond]);
				printf("\n");
			}
			printf("\n");
		}

		for (auto& l : assembly_lines)
		{
			ResolveRelatives(l);

			Instruction def = instructions.at(l.code.token);

			if (!def.allow_shift && l.has_shift) ThrowException("Cannot set rotation shift for " + l.code.token,l.code);
			if (!def.allow_N && l.N) ThrowException("Cannot set N flag for " + l.code.token, l.code);
			if (!def.allow_S && l.S) ThrowException("Cannot set S flag for " + l.code.token, l.code);
			if (l.args.size() != pack_args.at(def.packing)) ThrowException("Expected " + to_string(pack_args.at(def.packing)) + " argument/s for " + l.code.token, l.code);

			for (auto& a : l.args) a = SolveExpression(a, l.addr);

			*(l.mem) = (l.cond << 28) | (l.N << 27) | def.code | (l.S << 21);

			auto arg = l.args.begin();

			switch (def.packing)
			{
			case PackType::None:
				if (l.has_shift) ThrowException("Cannot use rotation shift for " + l.code.token, l.code);
				break;

			case PackType::Reg3:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 12;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in third argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 8;
				break;

			case PackType::Flex3i:
				if (next(arg)->front().type == TokenType::MINUS)
				{
					next(arg)->pop_front();
					*(l.mem) ^= (1 << 22) | (1 << 27); // Switch to negative alternate, and set N flag
				}

				if (l.args.back().front().type == TokenType::MINUS)
				{
					l.args.back().pop_front();
					*(l.mem) ^= (1 << 22);
				}
				[[fallthrough]];
			case PackType::Flex3:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 12;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in third argument", *std::next(arg->begin()));

				if (arg->front().type == TokenType::INTEGER)
				{
					word val = rotl(stoul(arg->front().token), l.shift);
					word min_val = val;
					word shift = 0;

					for (int i = 1; i < 16; i++)
					{
						if (rotr(val, i * 2) >= min_val) continue;
						min_val = rotr(val, i * 2);
						shift = i;
					}

					if (min_val > 0xFF) ThrowException("Immediate value (" + std::to_string(val) + ") is too large", arg->front());
					*(l.mem) |= 1 << 20; // set 'i'
					*(l.mem) |= min_val << 4;
					*(l.mem) |= shift;
				}
				else
				{
					if (l.shift & 1) ThrowException("Register rotation shifts must be even", arg->front());
					*(l.mem) |= ToReg(arg->front()) << 8;
					*(l.mem) |= l.shift >> 1;
				}
				break;

			case PackType::Flex2i:
				if (l.args.back().front().type == TokenType::MINUS)
				{
					l.args.back().pop_front();
					*(l.mem) ^= (1 << 22);
				}
				[[fallthrough]];
			case PackType::Flex2:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));

				if (arg->front().type == TokenType::INTEGER)
				{
					word val = rotl(stoul(arg->front().token), l.shift);
					word min_val = val;
					word shift = 0;

					for (int i = 1; i < 16; i++)
					{
						if (rotr(val, i * 2) >= min_val) continue;
						min_val = rotr(val, i * 2);
						shift = i;
					}

					if (min_val > 0xFFF) ThrowException("Immediate value (" + std::to_string(val) + ") is too large", arg->front());
					*(l.mem) |= 1 << 20; // set 'i'
					*(l.mem) |= min_val << 4;
					*(l.mem) |= shift;
				}
				else
				{
					if (l.shift & 1) ThrowException("Register rotation shifts must be even", arg->front());
					*(l.mem) |= ToReg(arg->front()) << 12;
					*(l.mem) |= l.shift >> 1;
				}
				break;

			case PackType::Reg2:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 12;
				break;

			case PackType::RegList:
				if (l.has_shift) ThrowException("Cannot use rotation shift for " + l.code.token, l.code);
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
			{
				word list = stoul(arg->front().token);
				if (list > 0xFFFF) ThrowException("'" + arg->front().token + "' is not a register set", arg->front());
				*(l.mem) |= list;
			}
				break;

			case PackType::BranchOffset:
			{
				bool is_negative;
				word off = GetBranchOffset(*arg, l.shift, is_negative);
				if (off && is_negative) off |= 1 << 27; // Avoid setting -0 because that is reserved for RFE/RET
				*(l.mem) |= off;
			}
				break;
			}
		}

		file_stack.pop_back();
	}
}