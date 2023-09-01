#include <unordered_set>
#include <sstream>
#include <cstdint>
#include <bit>

#include "Assembler.h"
#include "SR_String.h"
#include "RAM.h"
#include "ROM.h"

namespace SimpleRISC {
	const char Assembler::valid_text_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

	Assembler::FormatException::FormatException(const std::string_view line, const Token& token, const char* const message) :
		line_no(token.line_no),
		line(line),
		inner_message(message),
		message(
			std::string("Improper format: ") + std::string(message) + ": (line " + (token.line_no == 0 ? "??" : std::to_string(token.line_no)) + ")\n\n" +
			std::string(line) + "\n" +
			std::string(token.raw_token.data() - line.data(), ' ') + std::string(token.raw_token.length(), '^')
		),
		exception("Improper format") {}

	Assembler::FormatException::FormatException(const std::string_view line, const Token& token, const std::string& message) :
		line_no(token.line_no),
		line(line),
		inner_message(message),
		message(
			std::string("Improper format: ") + message + ": (line " + ( token.line_no == 0 ? "??" : std::to_string(token.line_no) ) + ")\n\n" +
			std::string(line) + "\n" +
			std::string(token.raw_token.data() - line.data(), ' ') + std::string(token.raw_token.length(), '^')
		),
		exception("Improper format") {}

	void Assembler::ThrowException(const char* const msg, const Token& token) const { throw FormatException(_current_line.line, token, msg); }
	void Assembler::ThrowException(const std::string& msg, const Token& token) const { throw FormatException(_current_line.line, token, msg); }

	constexpr bool Contains(std::string_view str, const std::string& token, size_t& i) noexcept {
		i = str.find(token);
		return i != str.npos;
	}

	constexpr bool Contains(std::string_view str, char c, size_t& i) noexcept {
		i = str.find(c);
		return i != str.npos;
	}

	constexpr bool Contains(std::string_view str, const std::string& token) noexcept
	{
		return str.find(token) != str.npos;
	}

	constexpr bool Contains(std::string_view str, char c) noexcept
	{
		return str.find(c) != str.npos;
	}

	constexpr void TrimStart(std::string_view& str) {
		if (str.empty()) return;
		size_t i = str.find_first_not_of(" \t\r\n");
		if (i == str.npos) {
			str = {};
		}
		else if(i != 0) {
			str = str.substr(i);
		}
	}

	constexpr void TrimEnd(std::string_view& str) {
		if (str.empty()) return;
		size_t i = str.find_last_not_of(" \t\r\n");
		if (i == str.npos) {
			str = {};
		}
		else if (i != 0) {
			str = str.substr(0,i + 1);
		}
	}

	constexpr void TrimString(std::string_view& str) {
		if (str.empty()) return;
		size_t i = str.find_first_not_of(" \t\r\n");
		if (i == str.npos) {
			str = {};
			return;
		}
		size_t j = str.find_last_not_of(" \t\r\n");
		/*if (i == 0) {
			str =  str.substr(0, j);
			return;
		}*/
		str = str.substr(i, (j + 1) - i);
	}

	bool TryGet(const Assembler::token_list& l, const Assembler::token_list::iterator& it, Assembler::Token& out, bool allowEOL = false) noexcept {
		if (it == l.end()) return false;
		if (!allowEOL && it->type == Assembler::TokenType::EOL) return false;

		out = *it;

		return true;
	}

	bool TryConsume(Assembler::token_list& l, Assembler::token_list::iterator& it, Assembler::Token& out, bool allowEOL = false) noexcept
	{
		if (it == l.end()) return false;
		if (!allowEOL && it->type == Assembler::TokenType::EOL) return false;

		out = *it;
		it = l.erase(it);

		return true;
	}

	bool TryGetFront(const Assembler::token_list& l, Assembler::Token& out, bool allowEOL = false) noexcept
	{
		if (l.empty()) return false;
		if (!allowEOL && l.front().type == Assembler::TokenType::EOL) return false;

		out = l.front();

		return true;
	}

	bool TryConsumeFront(Assembler::token_list& l, Assembler::Token& out, bool allowEOL = false) noexcept
	{
		if (l.empty()) return false;
		if (!allowEOL && l.front().type == Assembler::TokenType::EOL) return false;

		out = l.front();
		l.pop_front();

		return true;
	}

	template<class _Ty, class _Alloc>
	bool rTryGet(const std::list<_Ty, _Alloc>& l, const typename std::list<_Ty, _Alloc>::iterator& it, _Ty& out) noexcept {
		if (it == l.rend()) return false;

		out = *it;

		return true;
	}

	constexpr bool IsDecimal(const std::string& str) noexcept {
		if (str.empty()) return false;
		if (str.length() == 1) return str[0] >= '0' && str[0] <= '9';
		if (str[0] == '0') return false;
		if (IsChar(str, '_')) return false;
		return IsChars(str, "0123456789_");
	}

	constexpr bool IsBinary(const std::string& str) noexcept {
		if (str.length() < 3) return false;
		if (str[0] != '0' || str[1] != 'b') return false;
		if (IsChar(str, '_', 2)) return false;
		return IsChars(str, "01_", 2);
	}

	constexpr bool IsOctal(const std::string& str) noexcept {
		if (str.length() < 2) return false;
		if (str[0] != '0') return false;
		if (IsChar(str, '_', 1)) return false;
		return IsChars(str, "01234567_", 1);
	}

	constexpr bool IsHexadecimal(const std::string& str) noexcept {
		if (str.length() < 3) return false;
		if (str[0] != '0' || str[1] != 'x') return false;
		if (IsChar(str, '_', 2)) return false;
		return IsChars(str, "0123456789abcdefABCDEF_", 2);
	}

	uint64_t decToI(std::string_view str, uint64_t max) {
		if (!IsChars(str,"0123456789_")) return max;
		uint64_t v = 0;
		for (auto c : str) {
			if (c == '_') continue;
			v *= 10;
			v += c - '0';
			if (v >= max) return max;
		}
		return v;
	}

	uint64_t binToI(std::string_view str, uint64_t max) {
		if (str.length() > 2 && str[1] == 'b') {
			if (str[0] != '0') return max;
			str = str.substr(2);
		}
		if (!IsChars(str, "01_")) return max;
		uint64_t v = 0;
		for (auto c : str) {
			if (c == '_') continue;
			v <<= 1;
			v += c - '0';
			if (v >= max) return max;
		}
		return v;
	}

	uint64_t octToI(std::string_view str, uint64_t max) {
		if (!IsChars(str, "01234567_")) return max;
		uint64_t v = 0;
		for (auto c : str) {
			if (c == '_') continue;
			v <<= 3;
			v += c - '0';
			if (v >= max) return max;
		}
		return v;
	}

	uint64_t hexToI(std::string_view str, uint64_t max)
	{
		if (str.length() > 2 && str[1] == 'x')
		{
			if (str[0] != '0') return max;
			str = str.substr(2);
		}

		if (!IsChars(str, "0123456789abcdefABCDEF_")) return max;

		uint64_t v = 0;
		for (auto c : str)
		{
			if (c == '_') continue;
			v <<= 4;
			if (c >= '0' && c <= '9') v += c - '0';
			else if (c >= 'a' && c <= 'f') v += 10 + c - 'a';
			else if (c >= 'A' && c <= 'F') v += 10 + c - 'A';
			if (v >= max) return max;
		}

		return v;
	}

	uint64_t Assembler::xToI(std::string_view str, word base, uint64_t max)
	{
		uint64_t val = max;

		switch (base)
		{
		case 16: val = hexToI(str, max); break;
		case 10: val = decToI(str, max); break;
		case 8:  val = octToI(str, max); break;
		case 2:  val = binToI(str, max); break;
		default: ThrowException("This number cannot be processed", {TokenType::INTEGER, str, std::string(str), _current_line.line_no });
		}

		if (val == max) ThrowException("This number is too large", { TokenType::INTEGER, str, std::string(str), _current_line.line_no });

		return val;
	}

	word Assembler::ToReg(std::string_view str) const
	{
		for (int i = 0; i < 16; i++)
		{
			if (str == regNames[i]) return i;
		}

		ThrowException("'" + std::string(str) + "' is not a register", { TokenType::TEXT, str, std::string(str), _current_line.line_no });
	}

	constexpr bool IsAlpha(std::string_view str) noexcept
	{
		for (auto c : str)
		{
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) return false;
		}

		return true;
	}

	constexpr int IsNumber(std::string_view str) noexcept
	{
		if (str.empty()) return 0;
		if (str.length() == 1) return 10 * (str[0] >= '0' && str[0] <= '9');
		if (IsChar(str, '_')) return 0;
		if (str[0] != '0') return 10 * IsChars(str, "0123456789_");
		if (str.length() == 2) return 8 * (str[1] >= '0' && str[1] <= '7');
		if (str[1] == 'x') return 16 * IsChars(str, "0123456789abcdefABCDEF_", 2);
		if (str[1] == 'b') return 2 * IsChars(str, "01_", 2);
		return 8 * IsChars(str, "01234567_");
	}

	bool IsFloat(std::string_view str)
	{
		if (str.size() < 3) return false;
		if (std::count(str.begin(), str.end(), '.') != 1) return false;
		if (str[0] == '.' || str[str.length() - 1] == '.') return false;

		return IsChars(str, "0123456789.");
	}

	constexpr bool IsLower(const std::string& str) noexcept
	{
		for (auto c : str)
		{
			if (c < 'a' || c > 'z') return false;
		}

		return true;
	}

	constexpr bool IsUpper(const std::string& str) noexcept
	{
		for (auto c : str)
		{
			if (c < 'A' || c > 'Z') return false;
		}

		return true;
	}

	constexpr bool IsReg(const std::string& str) noexcept {
		for (int i = 0; i < 16; i++)
		{
			if (str == regNames[i]) return true;
		}

		return false;
	}

	bool IsCond(const std::string& str) noexcept {
		return condNames.contains(str);
	}

	constexpr void RemoveComments(std::string& str) noexcept {
		size_t i = str.find("//", 0, 2);
		if (i == str.npos) return;
		str.erase(i);
	}

	void PutByte(word* memory, word address, byte value) {
		word x = (address % sizeof(word)) * 8;

		value ^= memory[address / sizeof(word)] >> x;
		memory[address / sizeof(word)] ^= value << x;
	}

	size_t GetTokens(std::string_view line, Assembler::token_list& tokens, size_t line_no = 1) {
		using namespace std;

		size_t token_count = 0;

		//TrimString(line);
		if (line.empty()) return 0;
		
		while (string(" \t\r\n").find(line[0]) != string::npos)
		{
			if (line[0] == '\n')
			{
				tokens.push_back({ Assembler::TokenType::EOL,line.substr(0,1),"\n",line_no });
				line_no++;
			}
			line = line.substr(1);
			if (line.empty()) return 0;
		}

		size_t i = line.find('\"');
		size_t l_comment = line.find("//");
		size_t m_comment = line.find("/*");

		while (i != string::npos || l_comment != string::npos || m_comment != string::npos)
		{
			if (i < l_comment && i < m_comment)
			{
				token_count += GetTokens(line.substr(0, i), tokens, line_no) + 1;
				line_no += count(line.begin(), std::next(line.begin(), i), '\n');
				line = line.substr(i);

				size_t end = 0;

				if (Contains(line.substr(1), '"', i) && i < line.find('\r') && i < line.find('\n'))
				{
					string str = "";

					for (size_t it = 1; it < line.length(); it++)
					{
						char c = line[it];

						if (c == '\\')
						{
							if (++it == line.length() || line[it] == '\r' || line[it] == '\n') break;

							switch (line[it])
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

									c = ++it < line.length() ? line[it] : 0;

									if (c >= '0' && c <= '9')      value = ( c - '0' ) << 4;
									else if (c >= 'a' && c <= 'f') value = ( 10 + c - 'a' ) << 4;
									else if (c >= 'A' && c <= 'F') value = ( 10 + c - 'A' ) << 4;
									else
									{
										it--;
										if (line[it] == 'x') str += 'x';
										break;
									}

									c = ++it < line.length() ? line[it] : 0;

									if (c >= '0' && c <= '9')      value += c - '0';
									else if (c >= 'a' && c <= 'f') value += 10 + c - 'a';
									else if (c >= 'A' && c <= 'F') value += 10 + c - 'A';
									else
									{
										it -= 2;
										if (line[it] == 'x') str += 'x';
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
								str += line[it];
								break;
							}
						}
						else if (c == '"')
						{
							end = it;
							break;
						}
						else
						{
							str += line[it];
						}
					}

					tokens.push_back({ Assembler::TokenType::STRING,line.substr(1,end - 1),str });
				}
				else
				{
					tokens.push_back({ Assembler::TokenType::INVALID,line.substr(0,1),"\"",line_no});
				}

				line = line.substr(end + 1);
			}
			else if (l_comment < m_comment)
			{
				token_count += GetTokens(line.substr(0, l_comment), tokens, line_no);
				auto end = std::next(line.begin(), l_comment);

				line_no += count(line.begin(), end, '\n');

				line = line.substr(l_comment + 2);

				if (!Contains(line, '\n', i)) return token_count;

				line = line.substr(i);
				tokens.push_back({ Assembler::TokenType::EOL,line.substr(0,1),"\n",line_no });
				line = line.substr(1);
				line_no++;
			}
			else
			{
				token_count += GetTokens(line.substr(0, m_comment), tokens, line_no);

				line = line.substr(m_comment + 2);

				if (Contains(line, "*/", i))
				{
					auto commented = line.substr(0, i);
					line = line.substr(i + 2);

					while (Contains(commented, '\n', i))
					{
						commented = commented.substr(i);

						tokens.push_back({ Assembler::TokenType::EOL,commented.substr(0,1),"\n",line_no });

						commented = commented.substr(1);
						line_no++;
					}
				}
				else
				{
					tokens.push_back({ Assembler::TokenType::INVALID,line.substr(0,2),"/*",line_no });
				}

				line = line.substr(2);
			}

			if (line.empty()) return token_count;

			i = line.find('\"');
			l_comment = line.find("//");
			m_comment = line.find("/*");
		}

#define _GetTokens(type,term,l)\
		while (Contains(line, term, i)) {\
			token_count += GetTokens(line.substr(0, i), tokens, line_no) + 1;\
			line_no += count(line.begin(), std::next(line.begin(), i), '\n');\
			tokens.push_back({Assembler::TokenType::type,line.substr(i,l),term,line_no});\
			line = line.substr(i + l);\
			if(line.empty()) return token_count;\
		}

		_GetTokens(VARGS,"...",3);
		_GetTokens(SCOPE_FUNCTION_OPEN,"@{",2);
		_GetTokens(SCOPE_FUNCTION_CLOSE,"}@",2);
		_GetTokens(SCOPE_VARIABLE_OPEN,"${",2);
		_GetTokens(SCOPE_VARIABLE_CLOSE,"}$",2);
		_GetTokens(SCOPE_CONDITION_OPEN,"?{",2);
		_GetTokens(SCOPE_CONDITION_CLOSE,"}?",2);
		_GetTokens(SCOPE_LABEL_OPEN,":{",2);
		_GetTokens(SCOPE_LABEL_CLOSE,"}:",2);
		_GetTokens(LSHIFT,"<<",2);
		_GetTokens(RSHIFT,">>",2);
		_GetTokens(MARKER_PREPROCESSOR,"#", 1);
		_GetTokens(MARKER_FUNCTION,"@",1);
		_GetTokens(MARKER_VARIABLE,"$",1);
		_GetTokens(MARKER_CONDITION,"?",1);
		_GetTokens(MARKER_LABEL,":",1);
		_GetTokens(COMMA,",",1);
		_GetTokens(LPAREN,"(",1);
		_GetTokens(RPAREN,")",1);
		_GetTokens(LBRACKET,"[",1);
		_GetTokens(RBRACKET,"]",1);
		_GetTokens(LBRACE,"{",1);
		_GetTokens(RBRACE,"}",1);
		_GetTokens(PLUS,"+",1);
		_GetTokens(MINUS,"-",1);
#undef _GetTokens

		while (!line.empty()) {
			//TrimStart(line);

			while (!Contains(Assembler::valid_text_chars,line[0]))
			{
				if (line[0] == '\n')
				{
					tokens.push_back({ Assembler::TokenType::EOL,line.substr(0,1),"\n",line_no });
					line_no++;
					line = line.substr(1);
				}
				else if (line[0] == '.')
				{
					tokens.push_back({ Assembler::TokenType::MARKER_RELATIVE,line.substr(0,1),".",line_no });
					line = line.substr(1);
					token_count++;
				}
				else if (Contains(" \t\r", line[0]))
				{
					line = line.substr(1);
				}
				else
				{
					size_t last = line.find_first_of("\n\r\t .abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
					tokens.push_back({ Assembler::TokenType::INVALID,line.substr(0,last),string(line.substr(0,last)),line_no});
					line = line.substr(last);
					token_count++;
				}
				if (line.empty()) return token_count;
			}

			i = line.find_first_not_of(".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
			string_view t = i == string::npos ? line : line.substr(0, i);
			line = i == string::npos ? string_view() : line.substr(i);

			if (IsNumber(t))
			{
				tokens.push_back({ Assembler::TokenType::INTEGER,t,std::string(t) });
			}
			else if (IsFloat(t))
			{
				tokens.push_back({ Assembler::TokenType::FLOAT,t,std::string(t) });
			}
			else if (IsChars(t, Assembler::valid_text_chars))
			{
				tokens.push_back({ Assembler::TokenType::TEXT,t,std::string(t) });
			}
			else
			{
				tokens.push_back({ Assembler::TokenType::INVALID,t,std::string(t) });
			}

			token_count++;
		}

		return token_count;
	}

	Assembler::token_list::iterator FindToken(Assembler::token_list& list, Assembler::TokenType type, size_t offset)
	{
		auto it = std::next(list.begin(), offset);
		const auto end = list.end();

		for (; it != end; it++)
		{
			if (it->type == Assembler::TokenType::EOL) return end;
			if (it->type == type) break;
		}

		return it;
	}

	Assembler::token_list::iterator FindToken(Assembler::token_list& list, Assembler::TokenType type)
	{
		auto it = list.begin();
		const auto end = list.end();

		for (; it != end; it++)
		{
			if (it->type == Assembler::TokenType::EOL) return end;
			if (it->type == type) break;
		}

		return it;
	}

	bool Assembler::GetCond(token_list& l, byte& cond) const {
		using namespace std;

		auto it = FindToken(l, Assembler::TokenType::MARKER_CONDITION);

		if (it == l.end() || it->type == Assembler::TokenType::EOL) return false;

		it = l.erase(it); // Erase '?'

		if (it == l.end() || it->type != Assembler::TokenType::TEXT || !IsCond(it->token)) ThrowException("Expected condition for '?' statement", *it);

		cond = condNames.at(it->token);
		it = l.erase(it); // Erase condition
		
		if (it->type != Assembler::TokenType::EOL) ThrowException("Line does not end after '?' statement", *it);
		return true;
	}

	bool Assembler::GetShift(token_list& l, byte& shift) const {
		using namespace std;

		auto it = FindToken(l, TokenType::LSHIFT);
		it = (it == l.end() || it->type == TokenType::EOL) ? FindToken(l, Assembler::TokenType::RSHIFT) : it;

		if (it == l.end() || it->type == TokenType::EOL) return false;

		bool rshift = it->type == TokenType::RSHIFT;
		it = l.erase(it);

		if (it == l.end() || it->type != TokenType::INTEGER) ThrowException("Expected number after shift", *it);

		shift = stoul(it->token);
		if (shift >= 32) ThrowException("Shift is too large", *it);

		it = l.erase(it);

		if (it != l.end() && it->type != TokenType::EOL && it->type != TokenType::MARKER_CONDITION) ThrowException("Shift must be at end of instruction", *it);

		if (rshift && shift) shift = 32 - shift;

		return true;
	}

	word Assembler::GetBranchOffset(token_list& l, byte shift, bool& isNegative) const {
		using namespace std;

		isNegative = false;
		if (l.front().type == Assembler::TokenType::MINUS)
		{
			l.pop_front();
			isNegative = true;
		}

		if (l.size() == 0) ThrowException("Expected a value for offset", { TokenType::INVALID,string_view(_current_line.line.end(),_current_line.line.end()),"",_current_line.line_no});
		if (l.size() > 1) ThrowException("Expected one value for offset", { TokenType::INVALID,string_view(_current_line.line.end(),_current_line.line.end()),"",_current_line.line_no });
		if (l.front().type != Assembler::TokenType::INTEGER) ThrowException("Expected offset to be a number", l.front());

		word offset = stoul(l.front().token);
		offset = std::rotl(offset, shift);

		if (offset & 3) ThrowException("Expected offset to be word aligned", l.front());
		if (offset > 0x03FFFFFC) ThrowException("Offset is too large", l.front());

		return offset >> 2;
	}

	size_t Assembler::ConvertNumbers(token_list& tokens) const {
		size_t numbers_replaced = 0;

		for (auto& t : tokens)
		{
			if (t.type == Assembler::TokenType::INTEGER)
			{
				int numBase = IsNumber(t.token);

				if (numBase == 10)
				{
					uint64_t long_val = stoul(t.token);
					word word_Val = long_val;
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
			else if (t.type == Assembler::TokenType::FLOAT)
			{
				float f = std::stof(t.token);
				word val = *(word*)&f;
				t.token = std::to_string(val);
				t.type = Assembler::TokenType::INTEGER;
			}
		}

		return numbers_replaced;
	}

	size_t Assembler::SplitSquareBrackets(token_list& l) const {
		using namespace std;

		size_t brackets_replaced = 0;

		auto it = l.begin();
		Token token;

		while (TryGet(l, it, token)) {
			if (token.type == TokenType::RBRACKET) ThrowException("Closing square bracket without opening",token);
			if (token.type != TokenType::LBRACKET) {
				it++;
				continue;
			}
			brackets_replaced++;
			it = l.erase(it); // Erase '['

			auto start = it;

			if (!TryGet(l, it, token)) ThrowException("Square brackets must be closed",token);
			if (token.type == TokenType::RBRACKET) ThrowException("Square brackets must contain contents",token);
			if (token.type == TokenType::LBRACKET) ThrowException("Square brackets cannot be nested",token);
			if (token.type == TokenType::COMMA) ThrowException("Square brackets cannot contain commas",token);

			if (token.type == TokenType::PLUS || token.type == TokenType::MINUS) { // Bracket starts with a sign - Interpreted as single value relative to PC
				l.insert(it, { TokenType::TEXT, {}, "PC", token.line_no });
			}
			else {
				Token first_token = token;
				it++;
				while (TryGet(l, it, token) && token.type != TokenType::RBRACKET) {
					if (token.type == TokenType::LBRACKET) ThrowException("Square brackets cannot be nested",token);
					if (token.type == TokenType::COMMA) ThrowException("Square brackets cannot contain commas",token);
					if (token.type == TokenType::PLUS || token.type == TokenType::MINUS) break;
					it++;
				}

				if (it == l.end() || it->type == TokenType::EOL) ThrowException("Square brackets must be closed",token);

				if (token.type == TokenType::RBRACKET) { // Occurs when no signs were encountered - Interpreted as single value relative to PC
					if (IsReg(first_token.token)) { // User entered a register; consider contents as relative to it
						l.insert(it, { TokenType::COMMA, {}, ",", token.line_no });
						l.insert(it, { TokenType::INTEGER, {}, "0", token.line_no });
					}
					else {  // User entered a value; consider contents as relative to PC
						l.insert(start, { TokenType::TEXT, {}, "PC", token.line_no });
						l.insert(start, { TokenType::COMMA, {}, ",", token.line_no });
					}
					it = l.erase(it); // Erase ']'
					continue;
				}
			}

			if (token.type == TokenType::PLUS) *it = { TokenType::COMMA, {}, ",", token.line_no };
			else l.insert(it, { TokenType::COMMA, {}, ",", token.line_no });
			it++;

			if (!TryGet(l, it, token)) ThrowException("Square brackets must be closed",token);
			if (token.type == TokenType::RBRACKET) ThrowException("Sign in square brackets must be followed by offset",token);

			do {
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

	size_t Assembler::ResolveVariables(token_list& l) const {
		using namespace std;

		size_t vars_replaced = 0;
		auto it = l.begin();
		Token token;

		while (TryGet(l, it, token)) {
			if (token.type != TokenType::MARKER_VARIABLE) {
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

			const token_list var = scope->at(var_name.token);

			it = l.insert(it, var.begin(), var.end());
		}
		return vars_replaced;
	}

	size_t Assembler::ResolveRelatives(AssemblyLine& l) const {
		using namespace std;
		
		size_t relatives_replaced = 0;

		for (auto& a : l.args) {
			for (auto it = a.begin(); it != a.end(); it++) {
				if (it->type != TokenType::MARKER_RELATIVE) continue;

				it = a.erase(it);
				if (it->type != TokenType::INTEGER) ThrowException("Expected number after '.'", *it);

				long int addr = stoul(it->token) - l.addr;
				if (addr < 0) {
					addr *= -1;
					a.insert(it, {TokenType::MINUS,{},"-",it->line_no});
				}

				relatives_replaced++;
				it->token = to_string(addr);
			}
		}

		return relatives_replaced;
	}
	
	size_t Assembler::ResolveRegLists(token_list& l) const {
		using namespace std;

		size_t lists_replaced = 0;
		auto it = l.begin();
		Token token;

		while (TryGet(l, it, token)) {
			if (token.type != TokenType::LBRACE) {
				it++;
				continue;
			}
			lists_replaced++;

			word list = 0;
			it = l.erase(it); // Erase '{'

			while (TryConsume(l, it, token) && token.type != TokenType::RBRACE) {
				word reg1 = 0;
				for (; reg1 < 16; reg1++) {
					if (regNames[reg1] == token.token) break;
				}
				if (reg1 == 16) ThrowException("Expected register in register list",token);

				if (!TryConsume(l, it, token)) ThrowException("Expected register list to continue",token);

				if (token.type == TokenType::COMMA || token.type == TokenType::RBRACE) {
					if (list & (1 << reg1)) ThrowException("Register list has duplicate registers",token);
					list |= 1 << reg1;
					if (token.type == TokenType::RBRACE) break;
				} else if (token.type == TokenType::MINUS) {
					if (!TryConsume(l, it, token)) ThrowException("Expected second register for range",token);
					word reg2 = 0;
					for (; reg2 < 16; reg2++) {
						if (regNames[reg2] == token.token) break;
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

			l.insert(it, { TokenType::INTEGER,{},to_string(list),it->line_no });
		}

		return lists_replaced;
	}

	void Assembler::AddLabel(const std::string& label, word address) {
		constant_addresses[label] = address;
		label_scopes.front()[label] = address;
	}

	void Assembler::AddLabels(const std::unordered_map<std::string, word>& addresses) {
		for (auto& address : addresses) {
			constant_addresses[address.first] = address.second;
			label_scopes.front()[address.first] = address.second;
		}
	}

	bool Assembler::GetLabel(const std::string& label, std::string& label_out) const
	{
		if (!label_scopes.front().contains(label)) return false;

		label_out = label_scopes.front().at(label);

		return true;
	}

	bool Assembler::GetVariable(const std::string& variable, token_list& var_out) const
	{
		if (!variable_scopes.front().contains(variable)) return false;

		var_out = variable_scopes.front().at(variable);

		return true;
	}

	void Assembler::SetRAM(const RAM& ram) {
		this->ram = ram.memory.get();
		ram_start = ram.GetAddress();
		ram_range = ram.GetRange();
		ram_current_address = 0;
	}
	void Assembler::SetRAM(word* memory, word start_address, word range) {
		ram = memory;
		ram_start = start_address;
		ram_range = range;
		ram_current_address = 0;
	}

	void Assembler::SetROM(const ROM& rom) {
		this->rom = rom.memory.get();
		rom_start = rom.GetAddress();
		rom_range = rom.GetRange();
		rom_current_address = 0;
	}
	void Assembler::SetROM(word* memory, word start_address, word range) {
		rom = memory;
		rom_start = start_address;
		rom_range = range;
		rom_current_address = 0;
	}

	constexpr void Assembler::ClearRAM() noexcept {
		ram = nullptr;
		ram_start = 0;
		ram_range = 0;
		ram_current_address = 0;
	}

	constexpr void Assembler::ClearROM() noexcept {
		rom = nullptr;
		rom_start = 0;
		rom_range = 0;
		rom_current_address = 0;
	}

	void Assembler::ClearLabels() noexcept {
		constant_addresses.clear();
	}

	void Assembler::FlushScopes() noexcept {
		variable_scopes = { {} };
		func_scopes = { const_replace };
		label_scopes = { constant_addresses };
	}

	// https://stackoverflow.com/a/116220
	auto read_file(std::istream& stream) -> std::string
	{
		constexpr auto read_size = std::size_t(4096);
		stream.exceptions(std::ios_base::badbit);

		if (not stream)
		{
			throw std::ios_base::failure("file does not exist");
		}

		auto out = std::string();
		auto buf = std::string(read_size, '\0');
		while (stream.read(&buf[0], read_size))
		{
			out.append(buf, 0, stream.gcount());
		}
		out.append(buf, 0, stream.gcount());
		return out;
	}

	void Assembler::Assemble(std::istream& code, bool print_intermediate) {
		using namespace std;

		srand(time(0));

		// Strip byte order mark
		char a, b, c, d;
		a = code.get();
		b = code.get();
		c = code.get();
		d = code.get();

		if (
			(a == (char)0x00 && b == (char)0x00 && c == (char)0xfe && d == (char)0xff) ||
			(a == (char)0xff && b == (char)0xfe && c == (char)0x00 && d == (char)0x00)) {

		}
		else if
			(a == (char)0xef && b == (char)0xbb && c == (char)0xbf) {
			code.seekg(3);
		}
		else if (
			(a == (char)0xfe && b == (char)0xff) ||
			(a == (char)0xff && b == (char)0xfe)) {
			code.seekg(2);
		}
		else {
			code.seekg(0);
		}

		word* memory = nullptr;
		word memory_start = 0;
		word memory_range = 0;
		word* current_address = 0;

		if (rom != nullptr) {
			memory = rom;
			memory_start = rom_start;
			memory_range = rom_range;
			current_address = &rom_current_address;
		}
		else if (ram != nullptr) {
			memory = ram;
			memory_start = ram_start;
			memory_range = ram_range;
			current_address = &ram_current_address;
		}
		else throw exception("Destination for assembly has not been set");

		bool entry_defined = false;

		entry_point = program_start = *current_address + memory_start;
		program_end = program_start;
		data_start = program_start;
		data_end = program_start;

		string file_contents = read_file(code);
		string_view consumable_contents = file_contents;
		list<AssemblyLine> assemblyLines;

		struct cond_scope { bool has_cond = false; word cond = 0; };

		list<cond_scope> cond_scopes{ {false, 0} };
		size_t cond_depth = 0;
		size_t variable_depth = 0;
		size_t op_depth = 0;

		list<list<Token*>> pendingLabels = { {} };
		size_t label_depth = 0;

		stringstream line_reader(file_contents);
		Token token;
		bool byte_mode = false;
		bool terminator_mode = true;

		token_list file_tokens = {};
		GetTokens(file_contents, file_tokens);
		size_t line_no = file_tokens.empty() ? 1 : (file_tokens.back().line_no + (file_tokens.back().type == TokenType::EOL));
		file_tokens.push_back({ TokenType::EOL, {}, "\n", line_no });

		ConvertNumbers(file_tokens);

		size_t line_end = consumable_contents.find_first_of('\n');
		_current_line.line_no = 1;
		_current_line.line = consumable_contents.substr(0, line_end);
		consumable_contents = consumable_contents.substr(line_end + 1);

		while (TryConsumeFront(file_tokens, token, true))
		{
			bool isNegative = token.type == TokenType::MINUS;
			bool isPositive = token.type == TokenType::PLUS;
			if (token.type == TokenType::EOL)
			{
				line_end = consumable_contents.find_first_of('\n');
				_current_line.line_no++;
				_current_line.line = consumable_contents.substr(0, line_end);
				consumable_contents = consumable_contents.substr(line_end + 1);
			}
			else if (isNegative || isPositive || token.type == TokenType::INTEGER) // This is a constant value
			{
				if (data_start == program_start) data_start = *current_address + memory_start;

				if (byte_mode) {
					if (*current_address >= memory_range) ThrowException("This value would be written beyond memory", token);
					//if(isNegative) ThrowException("Bytes cannot be negative");

					if (token.type != TokenType::INTEGER) {
						if (!TryConsumeFront(file_tokens, token)) ThrowException("Sign not followed by anything",token);
						if (token.type != TokenType::INTEGER) ThrowException("Sign not followed by number",token);
					}

					word val = stoul(token.token);

					if(val > 255) ThrowException("Value is too large",token);

					if (isNegative) val = 1 + ~val;

					PutByte(memory, *current_address, val);
					(*current_address)++;
				}
				else {
					if ((*current_address) & 3) ThrowException("Word not aligned",token);
					if (((*current_address) >> 2) >= (memory_range >> 2)) ThrowException("This value would be written beyond memory",token);

					if (isPositive || isNegative) {
						if (!TryConsumeFront(file_tokens, token)) ThrowException("Sign symbol not followed by anything",token);
						if (token.type != TokenType::INTEGER) ThrowException("Sign symbol not followed by number",token);
					}

					word val = stoul(token.token);

					if (isNegative) val = 1 + ~val;

					memory[(*current_address) >> 2] = val;
					(*current_address) += 4;
				}

				data_end = *current_address + memory_start;
			}
			else if (token.type == TokenType::STRING)
			{
				if (data_start == program_start) data_start = *current_address + memory_start;

				for (char c : token.token)
				{
					if (*current_address + terminator_mode >= memory_range) ThrowException("String exceeds memory", token);
					PutByte(memory, *current_address, c);
					( *current_address )++;
				}

				if (terminator_mode)
				{
					PutByte(memory, *current_address, 0);
					( *current_address )++;
				}

				data_end = *current_address + memory_start;
			}
			else if (!file_tokens.empty() && file_tokens.front().type == TokenType::MARKER_LABEL) { // This is a label
				if (token.type != TokenType::TEXT) ThrowException("Invalid characters in label",token);
				if (label_scopes.back().contains(token.token)) ThrowException("Label already defined in scope", token);

				file_tokens.pop_front(); // Erase ':'
				label_scopes.back()[token.token] = *current_address + memory_start;

				string num = to_string(label_scopes.back()[token.token]);

				auto lit = pendingLabels.back().begin();
				while (lit != pendingLabels.back().end()) {
					if ((*lit)->token == token.token) { // Resolve this label
						(*lit)->token = num;
						(*lit)->type = TokenType::INTEGER;
						lit = pendingLabels.back().erase(lit);
					}
					else { // Needs another label
						lit++;
					}
				}
			}
			else if (token.type == TokenType::MARKER_PREPROCESSOR) {
				if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::TEXT) ThrowException("Preprocessor directive not provided", token);
				if (token.token == "BLOCK") {
					if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::INTEGER) ThrowException("Block size not provided", token);

					word size = stoul(token.token);
					if (size == 0) ThrowException("Expected nonzero memory block", token);
					if(*current_address + size >= memory_range) ThrowException("Block exceeds memory", token);

					if (data_start == program_start) data_start = *current_address + memory_start;

					for (int i = 0; i < size; i++) {
						PutByte(memory, *current_address + i, 0);
					}

					(*current_address) += size;

					data_end = *current_address + memory_start;
				}
				else if (token.token == "BYTE") {
					if (data_start == program_start) data_start = *current_address + memory_start;
					byte_mode = true;
				}
				else if (token.token == "WORD") {
					if (data_start == program_start) data_start = *current_address + memory_start;
					byte_mode = false;
				}
				else if (token.token == "ALIGN") {
					if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::INTEGER) ThrowException("Alignment width not provided", token);
					word width = stoul(token.token);
					word err = (*current_address + memory_start) % width;
					if (err == 0) continue;
					*current_address -= err;
					*current_address += width;
				}
				else if (token.token == "DATA") {
					if (ram != nullptr) {
						memory = ram;
						memory_start = ram_start;
						memory_range = ram_range;
						current_address = &ram_current_address;
					}
					if(data_start == program_start) data_start = *current_address + memory_start;
				}
				else if (token.token == "ENTRY") {
					if(entry_defined) ThrowException("Multiple entry points defined", token);
					if((*current_address + memory_start) % 4) ThrowException("Entry point is not word-aligned", token);

					entry_defined = true;
					entry_point = *current_address + memory_start;
				}
				else if (token.token == "ASCII")
				{
					if (data_start == program_start) data_start = *current_address + memory_start;
					terminator_mode = false;
				}
				else if (token.token == "ASCIZ")
				{
					if (data_start == program_start) data_start = *current_address + memory_start;
					terminator_mode = true;
				}
				else if (token.token == "SEED")
				{
					if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::INTEGER)
					{
						srand(time(0));
					}
					else
					{
						srand(stoul(token.token));
					}
				}
				else if (token.token == "RANDOM")
				{
					if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::INTEGER) ThrowException("Random size not provided", token);
					
					word size = stoul(token.token);
					if (size == 0) ThrowException("Expected nonzero random size", token);
					if (*current_address + size >= memory_range) ThrowException("Random data exceeds memory", token);

					if (data_start == program_start) data_start = *current_address + memory_start;

					word bytes = sizeof(word);
					word val = rand();
					PutByte(memory, ( *current_address )++, val);
					size--;
										
					while (size > 0) {
						bytes--;
						val >>= 8;

						if (bytes == 0)
						{
							bytes = sizeof(word);
							val = rand();
						}

						PutByte(memory, ( *current_address )++, val);
						size--;
					} 

					data_end = *current_address + memory_start;
				}
				else ThrowException("Preprocessor directive not recognised", token);
			}
			else if (token.type == TokenType::MARKER_FUNCTION) {
				func_scopes.back().push_back({});
				OpReplace& new_op = func_scopes.back().back();

				if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::TEXT) ThrowException("Function name not provided", token);
				new_op.op = token.token;
				if (!IsUpper(new_op.op)) ThrowException("Function name must be uppercase", token);

				if (TryGetFront(file_tokens, token) && token.type == TokenType::LPAREN) {
					file_tokens.pop_front();
					if (!TryConsumeFront(file_tokens, token) || token.type == TokenType::RPAREN) ThrowException("Required args not provided", token);
					if (token.type != TokenType::INTEGER) ThrowException("Required args is not numeric", token);
					new_op.requiredArgs = stoi(token.token);
					if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::RPAREN) ThrowException("Brackets not closed", token);
				}
				if (new_op.op.front() == 'N') ThrowException("Function name cannot start with N", token);
				if (new_op.op.back() == 'S') ThrowException("Function name cannot end with S", token);

				if (!TryConsumeFront(file_tokens,token) || token.type == TokenType::TEXT) ThrowException("Instruction to assign function not provided", token);
				if (!IsUpper(token.token)) ThrowException("Instruction name must be uppercase", token);
				new_op.newop = token.token;

				if (new_op.newop == "N") ThrowException("Function replacement instruction is only N flag", token);
				if (new_op.newop == "S") ThrowException("Function replacement instruction is only S flag", token);
				if (new_op.newop == "NS") ThrowException("Function replacement instruction is only flags", token);

				if (new_op.newop.front() == 'N') {
					new_op.newop = new_op.newop.substr(1);
					new_op.newN = true;
				}

				if (new_op.newop.back() == 'S') {
					new_op.newop = new_op.newop.substr(0, new_op.newop.length() - 1);
					new_op.newS = true;
				}

				new_op.has_cond = GetCond(file_tokens, new_op.new_cond);

				if (new_op.has_cond) {
					if (cond_scopes.back().has_cond) ThrowException("Function overwrites the scope condition", token);
				}
				else {
					new_op.new_cond = cond_scopes.back().cond;
					new_op.has_cond = cond_scopes.back().has_cond;
				}

				new_op.has_shift = GetShift(file_tokens, new_op.new_shift);

				while (TryConsumeFront(file_tokens, token)) {
					if (token.type == TokenType::VARGS) {
						if (new_op.requiredArgs != -1) ThrowException("Cannot use '...' in constant length function", token);
						new_op.tokens.push_back(token);
					}
					else if (token.type == TokenType::MARKER_FUNCTION) {
						if (new_op.requiredArgs == -1) ThrowException("Cannot use '@' in variable length function", token);
						new_op.tokens.push_back(token);
						if (!TryConsumeFront(file_tokens, token) || token.type != TokenType::INTEGER) ThrowException("'@' must be followed by a number", token);
						if (stoi(token.token) >= new_op.requiredArgs) ThrowException("Arg number greater than number of args", token);
						new_op.tokens.push_back(token);
					}
					else {
						new_op.tokens.push_back(token);
					}
				}

				total_ops_defined++;
			}
			else if (token.type == TokenType::SCOPE_FUNCTION_OPEN) {
				op_depth++;
				func_scopes.push_back({});
			}
			else if (token.type == TokenType::SCOPE_FUNCTION_CLOSE) {
				if (op_depth == 0) ThrowException("Too many closing function scopes", token);

				op_depth--;
				func_scopes.pop_back();
			}
			else if (token.type == TokenType::SCOPE_VARIABLE_OPEN) {
				variable_depth++;
				variable_scopes.push_back({});
			}
			else if (token.type == TokenType::SCOPE_VARIABLE_CLOSE) {
				if (variable_depth == 0) ThrowException("Too many closing variable scopes", token);

				variable_depth--;
				variable_scopes.pop_back();
			}
			else if (token.type == TokenType::SCOPE_CONDITION_OPEN) {
				cond_depth++;
				cond_scopes.push_back({});
				// ThrowException("No condition was provided for the condition scope");
				if (!TryGetFront(file_tokens,token) || token.type != TokenType::TEXT) continue;
				//ThrowException("Invalid condition was provided for the condition scope");
				if (!IsCond(token.token)) continue;

				cond_scopes.back().has_cond = true;
				cond_scopes.back().cond = condNames.at(token.token);
				file_tokens.pop_front();
			}
			else if (token.type == TokenType::SCOPE_CONDITION_CLOSE) {
				if (cond_depth == 0) ThrowException("Too many closing condition scopes", token);

				cond_depth--;
				cond_scopes.pop_back();
			}
			else if (token.type == TokenType::SCOPE_LABEL_OPEN) {
				label_depth++;
				label_scopes.push_back({});
				pendingLabels.push_back({});
			}
			else if (token.type == TokenType::SCOPE_LABEL_CLOSE) {
				if (label_depth == 0) ThrowException("Too many closing label scopes", token);
				label_depth--;

				std::list<Token*> cur_scope = pendingLabels.back();
				pendingLabels.pop_back();

				if (cur_scope.size() > 0) {
					std::list<Token*>& lower_list = pendingLabels.back(); // Get second last list
					lower_list.splice(lower_list.end(), cur_scope);  // Move labels down to second last list
				}

				label_scopes.pop_back();
			}
			else if (token.type == TokenType::MARKER_VARIABLE) {
				if (!TryConsumeFront(file_tokens, token)) ThrowException("Variable name not provided", token);
				if (token.type != TokenType::TEXT) ThrowException("Invalid characters in variable name", token);

				string var_name = token.token;

				if (!variable_scopes.back().contains(var_name)) {
					total_variables_defined++;
				}

				if (file_tokens.front().type == TokenType::EOL) ThrowException("Value to assign variable not provided", token);

				while (TryConsumeFront(file_tokens, token))
				{
					variable_scopes.back()[var_name].push_back(token);
				}
			}
			else if (token.type == TokenType::TEXT) { // This is an instruction
				if ((*current_address) & 3) ThrowException("Instruction not aligned", token);
				if (!IsUpper(token.token)) ThrowException("Invalid characters in instruction code", token);

				assemblyLines.push_back({
					_current_line,
					memory_start + *current_address,
					memory + ((*current_address) >> 2),
					token
				});
				AssemblyLine& instruction = assemblyLines.back();
				if (instruction.code.token.length() == 0) ThrowException("Instruction name is empty", token);

				if (instruction.code.token == "N") ThrowException("Instruction is only N flag", token);
				if (instruction.code.token == "S") ThrowException("Instruction is only S flag", token);
				if (instruction.code.token == "NS") ThrowException("Instruction is only flags", token);

				if (instruction.code.token.front() == 'N') {
					instruction.code.token = instruction.code.token.substr(1);
					instruction.N = true;
				}

				if (instruction.code.token.back() == 'S') {
					instruction.code.token = instruction.code.token.substr(0, instruction.code.token.length() - 1);
					instruction.S = true;
				}

				instruction.has_cond = GetCond(file_tokens, instruction.cond);
				instruction.has_shift = GetShift(file_tokens, instruction.shift);

				total_variables_replaced += ResolveVariables(file_tokens);
				SplitSquareBrackets(file_tokens);
				ResolveRegLists(file_tokens);

				if (TryConsumeFront(file_tokens, token)) {
					instruction.args.push_back({});
					token_list* tokens = &instruction.args.back();

					do
					{
						if (token.type == TokenType::COMMA)
						{
							if (tokens->empty() || file_tokens.front().type == TokenType::EOL) ThrowException("Empty instruction parameter", token);

							instruction.args.push_back({});
							tokens = &instruction.args.back();
						}
						else
						{
							tokens->push_back(token);
						}
					} while (TryConsumeFront(file_tokens, token));
				}


				OpReplace* op = nullptr;

				unordered_set<OpReplace*> used_replacements = {};

				// Replace instruction with preprocessor instruction
				do {
					op = nullptr;
					for (auto scope_it = func_scopes.rbegin(); scope_it != func_scopes.rend(); scope_it++) {
						for (auto& op_it : *scope_it) {
							if (op_it.op != instruction.code.token) continue;
							if (op_it.requiredArgs == -1) {
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

					if (op->has_cond) { // Replace condition
						if (instruction.has_cond) ThrowException("Function overwrites condition", token);
						instruction.cond = op->new_cond;
						instruction.has_cond = true;
					}

					if (op->has_shift) {
						if (instruction.has_shift) ThrowException("Function overwrites shift", token);
						instruction.shift = op->new_shift;
						instruction.has_shift = true;
					}

					vector<token_list> old_args = {};
					old_args.insert(old_args.begin(), instruction.args.begin(), instruction.args.end());

					instruction.args = {};

					token_list op_tokens = op->tokens;
					total_variables_replaced += ResolveVariables(op_tokens);
					SplitSquareBrackets(op_tokens);
					ResolveRegLists(op_tokens);

					if (op_tokens.empty()) continue;

					instruction.args.push_back({});
					token_list* tokens = &instruction.args.back();
					do {
						if (op_tokens.front().type == TokenType::COMMA) {
							op_tokens.pop_front(); // Erase ','

							if (tokens->empty() || op_tokens.empty()) ThrowException("Function parameter resolved as empty", token);

							instruction.args.push_back({});
							tokens = &instruction.args.back();
						}
						else {
							tokens->push_back(op_tokens.front());
							op_tokens.pop_front();
						}
					} while (!op_tokens.empty());

					if (op->requiredArgs > 0) {
						for (auto& a : instruction.args) {
							auto b_it = a.begin();
							while (TryGet(a, b_it, token)) {
								if (token.type != TokenType::MARKER_FUNCTION) {
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
					else if (op->requiredArgs == -1) {
						auto a_it = instruction.args.begin();
						while (a_it != instruction.args.end()) {
							token_list arg = {};
							arg.splice(arg.begin(), *a_it);
							auto b_it = arg.begin();
							while (TryConsume(arg, b_it, token)) {
								if (token.type != TokenType::VARGS){
									a_it->push_back(token);
									continue;
								}

								if (old_args.empty()) continue;

								auto c_it = old_args.begin();
								a_it->insert(a_it->end(), c_it->begin(), c_it->end());
								c_it++;

								while (c_it != old_args.end()) {
									a_it++;
									a_it = instruction.args.insert(a_it, *c_it);
									c_it++;
								}
							}
							if (a_it->empty()) ThrowException("Function parameter resolved as empty",token);
							a_it++;
						}
					}
				} while (op != nullptr);

				if (instruction.has_cond) {
					if (cond_scopes.back().has_cond) ThrowException("Instruction overwrites the scope condition",token);
				}
				else {
					instruction.cond = cond_scopes.back().cond;
					instruction.has_cond = cond_scopes.back().has_cond;
				}

				for (auto& a : instruction.args) {
					if (a.empty()) ThrowException("Argument cannot be empty",token);

					if (a.front().type == TokenType::LSHIFT || a.front().type == TokenType::RSHIFT) ThrowException("Expected '" + a.front().token + "' to follow a value", a.front());
					if (a.back().type == TokenType::LSHIFT || a.back().type == TokenType::RSHIFT || a.back().type == TokenType::MINUS || a.back().type == TokenType::MARKER_RELATIVE) ThrowException("Expected '" + a.front().token + "' to follow a value", a.back());

					for (auto& t : a) {
						if (
							t.type == TokenType::LSHIFT ||
							t.type == TokenType::RSHIFT ||
							t.type == TokenType::MINUS ||
							t.type == TokenType::MARKER_RELATIVE ||
							t.type == TokenType::INTEGER ||
							(t.type == TokenType::TEXT && IsReg(t.token))
						) {
							continue;
						}
						if (t.type != TokenType::TEXT) ThrowException("Unexpected token '" + t.token + "' in argument", t);

						pendingLabels.back().push_back(&t);

						for (auto it = label_scopes.rbegin(); it != label_scopes.rend(); it++) {
							if (!it->contains(t.token)) continue;

							t.token = to_string((*it)[t.token]);
							t.type = TokenType::INTEGER;
							pendingLabels.back().pop_back();
							break;
						}
					}
				}

				*current_address += 4;
				program_end = *current_address + memory_start;
			}
			else ThrowException("Unregognised token", token);
		}

		if (!file_tokens.empty()) ThrowException("Unexpected token/s in file", file_tokens.front());

		data_end = *current_address + memory_start;
		if (data_start == program_start) data_start = program_end;

		if (label_depth != 0)
		{
			FlushScopes();
			ThrowException("Did not close every label scope", { TokenType::EOL,{file_contents.end(),file_contents.end()},"",_current_line.line_no });
		}
		if (cond_depth != 0)
		{
			FlushScopes();
			ThrowException("Did not close every condition scope", { TokenType::EOL,{file_contents.end(),file_contents.end()},"",_current_line.line_no });
		}
		if (op_depth != 0)
		{
			FlushScopes();
			ThrowException("Did not close every function scope", { TokenType::EOL,{file_contents.end(),file_contents.end()},"",_current_line.line_no });
		}

		if (!pendingLabels.front().empty()) {
			string err = "Could not resolve all labels (";
			unordered_set<string> labels;
			for (auto& label : pendingLabels.front()) {
				labels.insert(label->token);
			}
			auto it = labels.begin();
			while (true) {
				err += *(it++);
				if (it == labels.end()) break;
				err += ", ";
			}
			err += ")";
			ThrowException(err, { TokenType::EOL,{file_contents.end(),file_contents.end()},"",_current_line.line_no });
		}

		if (print_intermediate) {
			printf("Intermediate output:\n");
			for (auto& l : assemblyLines) {
				printf("0x%08X: %s%s%s ", l.addr, l.N ? "N" : "", l.code.token.c_str(), l.S ? "S" : "");
				for (auto& a : l.args) {
					for (auto& t : a) {
						printf("%s", t.token.c_str());
					}
					if (&a != &(l.args.back())) printf(", ");
				}
				if (l.has_cond) printf(" ?%s", condNamesRegular[l.cond].c_str());
				printf("\n");
			}
			printf("\n");
		}

		enum pack {
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

		struct InstructionDefinition {
			word code = 0;
			pack packing = None;
			bool allow_N = false;
			bool allow_S = false;
			bool allow_shift = true;
		};

		static const unordered_map<pack, size_t> pack_args = {
			{None, 0},
			{BranchOffset, 1},
			{RegList, 2},
			{Reg2, 2},
			{Flex2, 2},
			{Flex2i, 2},
			{Flex3, 3},
			{Flex3i, 3},
			{Reg3, 3},
		};

		static const unordered_map<string, InstructionDefinition> definitions
		{
			//        N1ppppSi   ,   .   ,   .   ,
			{"ADD",{0b0100000000000000000000000000, Flex3i,  true,  true  }},
			{"SUB",{0b0100010000000000000000000000, Flex3i,  true,  true  }},
			{"ADC",{0b0100100000000000000000000000, Flex3,   true,  true  }},
			{"SBB",{0b0100110000000000000000000000, Flex3,   true,  true  }},
			{"ASL",{0b0101000000000000000000000000, Flex3,   true,  true  }},
			{"ASR",{0b0101010000000000000000000000, Flex3,   true,  true  }},
			{"CMP",{0b0101100000000000000000000000, Flex2i,  true,  false }},
			{"CMN",{0b0101110000000000000000000000, Flex2i,  true,  false }},

			{"ORR",{0b0110000000000000000000000000, Flex3,   true,  true  }},
			{"AND",{0b0110010000000000000000000000, Flex3,   true,  true  }},
			{"XOR",{0b0110100000000000000000000000, Flex3,   true,  true  }},
			{"TST",{0b0110110000000000000000000000, Flex2,   true         }},
			{"LSL",{0b0111000000000000000000000000, Flex3,   true,  true  }},
			{"LSR",{0b0111010000000000000000000000, Flex3,   true,  true  }},
			{"MOV",{0b0111100000000000000000000000, Flex2i,  true,  true  }},
			{"INV",{0b0111110000000000000000000000, Flex2i,  true,  true  }},
			//        N01L   .   ,   .   ,   .   ,
			{"B"  ,{0b0010000000000000000000000000, BranchOffset          }},
			{"BL" ,{0b0011000000000000000000000000, BranchOffset          }},
			{"RFE",{0b1010000000000000000000000000, None                  }},
			{"RET",{0b1011000000000000000000000000, None                  }},
			//        N0011BWx   ,   .   ,   .   ,
			{"RRW",{0b0001100000000000000000000000, Flex3,   true         }},
			{"RWW",{0b0001101000000000000000000000, Flex3,   true         }},
			{"RRB",{0b0001110000000000000000000000, Flex3,   true         }},
			{"RWB",{0b0001111000000000000000000000, Flex3,   true         }},
			//        N00101Wx   ,   .   ,   .   ,
			{"SRR",{0b0001010000000000000000000000, RegList, true         }},
			{"SWR",{0b0001011000000000000000000000, RegList, true         }},
			//        N00100px   ,   .   ,   .   ,
			{"MVM",{0b0001000000000000000000000000, RegList, true         }},
			{"SWP",{0b0001001000000000000000000000, Reg2,    true         }},
			//         N0001ppp   ,   .   ,   .   ,
			{"ADDF",{0b0000100000000000000000000000, Reg3,   true,  false, false  }},
			{"SUBF",{0b0000100100000000000000000000, Reg3,   true,  false, false  }},
			{"MULF",{0b0000101000000000000000000000, Reg3,   true,  false, false  }},
			{"DIVF",{0b0000101100000000000000000000, Reg3,   true,  false, false  }},
			{"ITOF",{0b0000110000000000000000000000, Reg2,   true,  false, false  }},
			{"FTOI",{0b0000110100000000000000000000, Reg2,   true,  false, false  }},
			{"CMPF",{0b0000111000000000000000000000, Reg2,   true,  false, false  }},
			{"CMPFI",{0b0000111100000000000000000000,Reg2,   true,  false, false  }},
		};

		for (auto& l : assemblyLines) {
			_current_line = l.rline;
			ResolveRelatives(l);
			if (!definitions.contains(l.code.token)) ThrowException("Unknown instruction '" + l.code.token + "'",l.code);

			InstructionDefinition def = definitions.at(l.code.token);

			if (!def.allow_shift && l.has_shift) ThrowException("Cannot set shift for " + l.code.token,l.code);
			if (!def.allow_N && l.N) ThrowException("Cannot set N flag for " + l.code.token, l.code);
			if (!def.allow_S && l.S) ThrowException("Cannot set S flag for " + l.code.token, l.code);
			if (l.args.size() != pack_args.at(def.packing)) ThrowException("Expected " + to_string(pack_args.at(def.packing)) + " argument/s for " + l.code.token, l.code);

			*(l.mem) = (l.cond << 28) | (l.N << 27) | def.code | (l.S << 21);

			auto arg = l.args.begin();

			switch (def.packing) {
			case None:
				if (l.has_shift) ThrowException("Cannot use shift for " + l.code.token, l.code);
				break;

			case Reg3:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 12;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in third argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 8;
				break;

			case Flex3i:
				if (next(arg)->front().type == TokenType::MINUS) {
					next(arg)->pop_front();
					*(l.mem) ^= (1 << 22) | (1 << 27); // Switch to negative alternate, and set N flag
				}

				if (l.args.back().front().type == TokenType::MINUS) {
					l.args.back().pop_front();
					*(l.mem) ^= (1 << 22);
				}

			case Flex3:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 12;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in third argument", *std::next(arg->begin()));

				if (arg->front().type == TokenType::INTEGER) {
					word val = rotl(stoul(arg->front().token), l.shift);
					word min_val = val;
					word shift = 0;

					for (int i = 1; i < 16; i++) {
						if (rotr(val, i * 2) >= min_val) continue;
						min_val = rotr(val, i * 2);
						shift = i;
					}

					if (min_val > 0xFF) ThrowException("Immediate value is too large", arg->front());
					*(l.mem) |= 1 << 20; // set 'i'
					*(l.mem) |= min_val << 4;
					*(l.mem) |= shift;
				}
				else
				{
					if (l.shift & 1) ThrowException("Register shifts must be even", arg->front());
					*(l.mem) |= ToReg(arg->front().token) << 8;
					*(l.mem) |= l.shift >> 1;
				}
				break;

			case Flex2i:
				if (l.args.back().front().type == TokenType::MINUS) {
					l.args.back().pop_front();
					*(l.mem) ^= (1 << 22);
				}

			case Flex2:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));

				if (arg->front().type == TokenType::INTEGER) {
					word val = rotl(stoul(arg->front().token), l.shift);
					word min_val = val;
					word shift = 0;

					for (int i = 1; i < 16; i++) {
						if (rotr(val, i * 2) >= min_val) continue;
						min_val = rotr(val, i * 2);
						shift = i;
					}

					if (min_val > 0xFFF) ThrowException("Immediate value is too large", arg->front());
					*(l.mem) |= 1 << 20; // set 'i'
					*(l.mem) |= min_val << 4;
					*(l.mem) |= shift;
				}
				else
				{
					if (l.shift & 1) ThrowException("Register shifts must be even", arg->front());
					*(l.mem) |= ToReg(arg->front().token) << 12;
					*(l.mem) |= l.shift >> 1;
				}
				break;

			case Reg2:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 12;
				break;

			case RegList:
				if (l.has_shift) ThrowException("Cannot use shift for " + l.code.token, l.code);
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument", *std::next(arg->begin()));
				*(l.mem) |= ToReg(arg->front().token) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument", *std::next(arg->begin()));
			{
				word list = stoul(arg->front().token);
				if (list > 0xFFFF) ThrowException("'" + arg->front().token + "' is not a register set", arg->front());
				*(l.mem) |= list;
			}
				break;

			case BranchOffset:
			{
				bool isNegative;
				word off = GetBranchOffset(*arg, l.shift, isNegative);
				if (off && isNegative) off |= 1 << 27; // Avoid setting -0 because that is reserved for RFE/RET
				*(l.mem) |= off;
			}
				break;
			}
		}
	}
}