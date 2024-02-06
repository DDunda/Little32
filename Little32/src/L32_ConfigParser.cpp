#include "L32_ConfigParser.h"

#include "L32_BigInt.h"
#include "L32_ConfigObject.h"
#include "L32_VarReference.h"
#include "L32_VarValue.h"
#include "L32_Types.h"

namespace Little32
{
	inline constexpr bool IsVarName(const std::string_view in_str)
	{
		assert(!in_str.empty());

		if (in_str[0] < 'A' || in_str[0] > 'z' || (in_str[0] > 'Z' && in_str[0] < 'a' && in_str[0] != '_')) return false;

		for (auto c : in_str.substr(1))
		{
			if (c < '0' || c > 'z' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a' && c != '_')) return false;
		}
		return true;
	}

	inline static void ThrowException(const std::string_view msg, const ConfigParser::Token& token) { throw ConfigParser::FormatException(token, msg, ConfigParser::tab_width); }

	inline static constexpr bool IsInteger(const std::string_view in_str)
	{
		assert(!in_str.empty());

		std::string_view str = in_str;

		if (str[0] == '-' || str[0] == '+')
		{
			str = str.substr(1);
			if (str.empty()) return false;
		}

		if (str[0] == '0') return str.length() == 1;
		if (!IsChars(str, '0', '9')) return false;

		return true;
	}

	inline constexpr bool IsFloat(const std::string_view in_str)
	{
		assert(!in_str.empty());

		std::string_view str = in_str;

		if (str[0] == '-' || str[0] == '+')
		{
			str = str.substr(1);
			if (str.empty()) return false;
		}

		std::string lower = std::string(' ', str.length());

		for (size_t i = str.length(); i-- > 0;)
		{
			lower[i] = std::tolower(str[i]);
		}

		if (lower == "nan" || lower == "inf" || lower == "infinity") return true;

		size_t i = 0;
		if (!Contains(str, '.', i)) return false;
		if (i == 0) return false;
		if (str[0] == '0' && i != 1) return false;
		if (!IsNumeric(str.substr(0, i))) return false;

		str = str.substr(i + 1);

		if (str.empty()) return false;

		if (Contains(str, 'e', i))
		{
			std::string_view end = str.substr(i + 1);
			if (end.empty()) return false;
			if (end[0] == '-' || end[0] == '+')
			{
				end = end.substr(1);
				if (end.empty()) return false;
			}

			if (end[0] == '0' && end.length() != 1) return false;
			if (!IsNumeric(end)) return false;

			str = str.substr(0, i);
			if (str.empty()) return false;
		}

		if (!IsNumeric(str)) return false;

		return true;
	}

	inline void    _AssertContinues(ConfigParser::TokenList& tokens)
	{
		if (tokens.empty())    throw std::runtime_error("Expected token, got end of file");
	}
	inline void    _AssertType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		if (tokens.front().type != type) ThrowException("Expected token of type '" + std::string(ConfigParser::type_names.at(type)) + "', got '" + std::string(ConfigParser::type_names.at(tokens.front().type)) + "'", tokens.front());
	}
	inline static void    _AssertNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		if (tokens.front().type == type) ThrowException("Expected token not of type '" + std::string(ConfigParser::type_names.at(type)), tokens.front());
	}

	inline static bool    _TryAssertContinues(ConfigParser::TokenList& tokens) noexcept
	{
		return !tokens.empty();
	}
	inline static bool    _TryAssertType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type) noexcept
	{
		return tokens.front().type == type;
	}
	inline static bool    _TryAssertNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type) noexcept
	{
		return tokens.front().type != type;
	}

	// No checking, unsafe. To be used with assert or external safety checks
	inline static ConfigParser::ConfigParser::Token     _Get(ConfigParser::TokenList& tokens) noexcept
	{
		return tokens.front();
	}
	inline static void      _Get(ConfigParser::TokenList& tokens, ConfigParser::Token& out) noexcept
	{
		out = tokens.front();
	}
	inline static void     _Skip(ConfigParser::TokenList& tokens) noexcept
	{
		tokens.pop_front();
	}
	inline static ConfigParser::Token _Consume(ConfigParser::TokenList& tokens) noexcept
	{
		const ConfigParser::Token tok = _Get(tokens);
		_Skip(tokens);
		return tok;
	}
	inline static void  _Consume(ConfigParser::TokenList& tokens, ConfigParser::Token& out) noexcept
	{
		_Get(tokens, out);
		_Skip(tokens);
	}

	// Ensures that there is a next token of any type
	inline static void    AssertContinues(ConfigParser::TokenList& tokens)
	{
		_AssertContinues(tokens);
	}
	// Ensures that there is a next token, and that it is a certain type
	inline static void    AssertType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		_AssertContinues(tokens);
		_AssertType(tokens, type);
	}
	// Ensures that there is a next token, and that it is not a certain type
	inline static void    AssertNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		_AssertContinues(tokens);
		_AssertNotType(tokens, type);
	}
	// Checks if there is a next token of any type
	inline static bool TryAssertContinues(ConfigParser::TokenList& tokens) noexcept
	{
		return _TryAssertContinues(tokens);
	}
	// Checks if there is a next token, and if it is a certain type
	inline static bool TryAssertType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type) noexcept
	{
		return _TryAssertContinues(tokens) && _TryAssertType(tokens, type);
	}
	// Checks if there is a next token, and if it is not a certain type
	inline static bool TryAssertNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type) noexcept
	{
		return _TryAssertContinues(tokens) && _TryAssertNotType(tokens, type);
	}

	inline static ConfigParser::Token    GetAny(ConfigParser::TokenList& tokens)
	{
		AssertContinues(tokens);
		return _Get(tokens);
	}
	inline static ConfigParser::Token    GetType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		AssertType(tokens, type);
		return _Get(tokens);
	}
	inline static ConfigParser::Token    GetNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		AssertNotType(tokens, type);
		return _Get(tokens);
	}
	inline static bool  TryGetAny(ConfigParser::TokenList& tokens, ConfigParser::Token& out) noexcept
	{
		if (!TryAssertContinues(tokens)) return false;
		_Get(tokens, out);
		return true;
	}
	inline static bool  TryGetType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type, ConfigParser::Token& out) noexcept
	{
		if (!TryAssertType(tokens, type)) return false;
		_Get(tokens, out);
		return true;
	}
	inline static bool  TryGetNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type, ConfigParser::Token& out) noexcept
	{
		if (!TryAssertNotType(tokens, type)) return false;
		_Get(tokens, out);
		return true;
	}

	inline static void    SkipAny(ConfigParser::TokenList& tokens)
	{
		AssertContinues(tokens);
		_Skip(tokens);
	}
	inline static void    SkipType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		AssertType(tokens, type);
		_Skip(tokens);
	}
	inline static void    SkipNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		AssertNotType(tokens, type);
		_Skip(tokens);
	}
	inline static bool TrySkipAny(ConfigParser::TokenList& tokens)
	{
		if (!TryAssertContinues(tokens)) return false;
		_Skip(tokens);
		return true;
	}
	inline static bool TrySkipType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		if (!TryAssertType(tokens, type)) return false;
		_Skip(tokens);
		return true;
	}
	inline static bool TrySkipNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		if (!TryAssertNotType(tokens, type)) return false;
		_Skip(tokens);
		return true;
	}

	inline static ConfigParser::Token    ConsumeAny(ConfigParser::TokenList& tokens)
	{
		AssertContinues(tokens);
		return _Consume(tokens);
	}
	inline static ConfigParser::Token    ConsumeType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		AssertType(tokens, type);
		return _Consume(tokens);
	}
	inline static ConfigParser::Token    ConsumeNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type)
	{
		AssertNotType(tokens, type);
		return _Consume(tokens);
	}
	inline static bool  TryConsumeAny(ConfigParser::TokenList& tokens, ConfigParser::Token& out)
	{
		if (!TryAssertContinues(tokens)) return false;
		_Consume(tokens, out);
		return true;
	}
	inline static bool  TryConsumeType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type, ConfigParser::Token& out)
	{
		if (!TryAssertType(tokens, type)) return false;
		_Consume(tokens, out);
		return true;
	}
	inline static bool  TryConsumeNotType(ConfigParser::TokenList& tokens, ConfigParser::TokenType type, ConfigParser::Token& out)
	{
		if (!TryAssertNotType(tokens, type)) return false;
		_Consume(tokens, out);
		return true;
	}

	VarValue* ConfigParser::GetValue(TokenList& tokens, ConfigObject& root, VarValue* parent)
	{
		if (!_TryAssertContinues(tokens)) throw std::runtime_error("Expected value, got end of file");

		const Token t = _Consume(tokens);
		VarValue* out = new VarValue();
		out->parent = parent;

		out->original_line_start = t.line->line_no;
		out->original_line_end = t.line->line_no;

		out->file_pos_start = t.file_index;
		out->file_pos_end = t.file_index + t.raw_token.size();

		switch (t.type)
		{
		case TokenType::INTEGER: // 0123456789
		{
			out->SetIntegerValue(new BigInt(t.token));
			return out;
		}
		case TokenType::FLOAT: // 0.0e+0
		{
			out->SetFloatValue(std::stof(t.token));
			return out;
		}
		case TokenType::STRING: // "..."
		{
			std::string* str = new std::string();
			*str = t.token;
			out->SetStringValue(str);
			return out;
		}
		case TokenType::COLOUR: // #0-#FFFFFFFF
		{
			assert(t.token.length() > 0 && t.token.length() < 9);
			const int i = 'a';
			const int j = 'A';
			const int k = '9';

			char r;
			char g;
			char b;
			char a = 255;

			char grey;

			switch (t.token.length())
			{
			case 1:
				grey = 0x11 * ((t.token[0] & 0b1111) + (9 * (t.token[0] > '9')));
				out->SetColourValue(SDL::Colour(grey, grey, grey, 255));
				break;

			case 2:
				grey = (((t.token[0] & 0b1111) + (9 * (t.token[0] > '9'))) << 4)
					| ((t.token[1] & 0b1111) + (9 * (t.token[1] > '9')));
				out->SetColourValue(SDL::Colour(grey, grey, grey, 255));
				break;

			case 4:
				a = 0x11 * ((t.token[3] & 0b1111) + (9 * (t.token[3] > '9')));
			case 3:
				r = 0x11 * ((t.token[0] & 0b1111) + (9 * (t.token[0] > '9')));
				g = 0x11 * ((t.token[1] & 0b1111) + (9 * (t.token[1] > '9')));
				b = 0x11 * ((t.token[2] & 0b1111) + (9 * (t.token[2] > '9')));
				out->SetColourValue(SDL::Colour(r, g, b, a));
				break;

			case 7:
				a = 0x11 * ((t.token[3] & 0b1111) + (9 * (t.token[3] > '9')));
			case 6:
				r = (((t.token[0] & 0b1111) + (9 * (t.token[0] > '9'))) << 4)
					| ((t.token[1] & 0b1111) + (9 * (t.token[1] > '9')));
				g = (((t.token[2] & 0b1111) + (9 * (t.token[2] > '9'))) << 4)
					| ((t.token[3] & 0b1111) + (9 * (t.token[3] > '9')));
				b = (((t.token[4] & 0b1111) + (9 * (t.token[4] > '9'))) << 4)
					| ((t.token[5] & 0b1111) + (9 * (t.token[5] > '9')));
				out->SetColourValue(SDL::Colour(r, g, b, a));
				break;

			case 5:
				r = 0x11 * ((t.token[0] & 0b1111) + (9 * (t.token[0] > '9')));
				g = 0x11 * ((t.token[1] & 0b1111) + (9 * (t.token[1] > '9')));
				b = 0x11 * ((t.token[2] & 0b1111) + (9 * (t.token[2] > '9')));
				a = (((t.token[3] & 0b1111) + (9 * (t.token[3] > '9'))) << 4)
					| ((t.token[4] & 0b1111) + (9 * (t.token[4] > '9')));
				out->SetColourValue(SDL::Colour(r, g, b, a));
				break;

			case 8:
				r = (((t.token[0] & 0b1111) + (9 * (t.token[0] > '9'))) << 4)
					| ((t.token[1] & 0b1111) + (9 * (t.token[1] > '9')));
				g = (((t.token[2] & 0b1111) + (9 * (t.token[2] > '9'))) << 4)
					| ((t.token[3] & 0b1111) + (9 * (t.token[3] > '9')));
				b = (((t.token[4] & 0b1111) + (9 * (t.token[4] > '9'))) << 4)
					| ((t.token[5] & 0b1111) + (9 * (t.token[5] > '9')));
				a = (((t.token[6] & 0b1111) + (9 * (t.token[6] > '9'))) << 4)
					| ((t.token[7] & 0b1111) + (9 * (t.token[7] > '9')));
				out->SetColourValue(SDL::Colour(r, g, b, a));
				break;

			default:
				assert(false);
			}
			return out;
		}
		case TokenType::LREF: // <
		{
			VarReference* vr = new VarReference();

			vr->is_root_reference = false;

			Token token;

			_AssertContinues(tokens);
			if (_TryAssertType(tokens, TokenType::DOUBLE_SLASH)) // //
			{
				_Skip(tokens);
				vr->is_root_reference = true;

				_AssertContinues(tokens);
			}

			if (tokens.front().type != TokenType::VARNAME && tokens.front().type != TokenType::INTEGER && tokens.front().type != TokenType::DOUBLE_DOT)
				ThrowException("Expected token of type '" + std::string(type_names.at(TokenType::VARNAME)) + "', '" + std::string(type_names.at(TokenType::INTEGER)) + "', or '" + std::string(type_names.at(TokenType::DOUBLE_DOT)) + "', but got '" + std::string(type_names.at(tokens.front().type)) + "'", tokens.front());
			_Consume(tokens, token);
			vr->names.push_back(token.token);

			while (TrySkipType(tokens, TokenType::SLASH)) // abcd/abcd
			{
				_AssertContinues(tokens);

				if (tokens.front().type != TokenType::VARNAME && tokens.front().type != TokenType::INTEGER && tokens.front().type != TokenType::DOUBLE_DOT)
					ThrowException("Expected token of type '" + std::string(type_names.at(TokenType::VARNAME)) + "', '" + std::string(type_names.at(TokenType::INTEGER)) + "', or '" + std::string(type_names.at(TokenType::DOUBLE_DOT)) + "', but got '" + std::string(type_names.at(tokens.front().type)) + "'", tokens.front());

				vr->names.push_back(_Consume(tokens).token);
			}

			SkipType(tokens, TokenType::RREF); // >

			out->SetReferenceValue(vr);

			return out;
		}
		case TokenType::LPAREN: // (
		{
			const std::string num1s = ConsumeType(tokens, TokenType::INTEGER).token; // 123
			SkipType(tokens, TokenType::COMMA);                                      // ,
			const std::string num2s = ConsumeType(tokens, TokenType::INTEGER).token; // 123
			AssertType(tokens, TokenType::RPAREN);                                   // )

			out->original_line_end = tokens.front().line->line_no;
			out->file_pos_end = tokens.front().file_index + tokens.front().raw_token.size();

			_Skip(tokens);

			int32_t num1;
			int32_t num2;

			if (!_decToI32(num1s, num1)) ThrowException("X value is too large (must be from -2147483648 to 2147483647", t);
			if (!_decToI32(num2s, num2)) ThrowException("Y value is too large (must be from -2147483648 to 2147483647", t);

			out->SetVectorValue(SDL::Point(num1, num2));
			return out;
		}
		case TokenType::LBRACKET: // {
		{
			Token token;

			out->SetObjectValue(new ConfigObject());

			_AssertContinues(tokens);

			if (!_TryAssertType(tokens, TokenType::RBRACKET))
			{
				_AssertType(tokens, TokenType::VARNAME);
				token = _Consume(tokens);
				SkipType(tokens, TokenType::ASSIGN);

				out->object_values->settings[token.token] = *GetValue(tokens, root, out);

				_AssertContinues(tokens);
				if (_TryAssertType(tokens, TokenType::COMMA))
				{
					_Skip(tokens);
					_AssertContinues(tokens);
				}

				while (!_TryAssertType(tokens, TokenType::RBRACKET))
				{
					_AssertType(tokens, TokenType::VARNAME);
					token = _Consume(tokens);

					if (out->object_values->settings.count(token.token) > 0) ThrowException("Duplicate child name detected", token);

					SkipType(tokens, TokenType::ASSIGN);

					out->object_values->settings[token.token] = *GetValue(tokens, root, out);

					_AssertContinues(tokens);
					if (_TryAssertType(tokens, TokenType::COMMA)) // Allows a comma on last element
					{
						_Skip(tokens);
						_AssertContinues(tokens);
					}
				}
			}

			out->original_line_end = tokens.front().line->line_no;
			out->file_pos_end = tokens.front().file_index + tokens.front().raw_token.size();

			_Skip(tokens); // }

			return out;
		}
		case TokenType::LBRACE: // [
		{
			out->SetListValue(new std::vector<VarValue>());

			_AssertContinues(tokens);

			if (!_TryAssertType(tokens, TokenType::RBRACE))
			{
				do
				{
					out->list_values->push_back(*GetValue(tokens, root, out));

					_AssertContinues(tokens);
					if (_TryAssertType(tokens, TokenType::COMMA)) // Allows a comma on last element
					{
						_Skip(tokens);
						_AssertContinues(tokens);
					}
				}
				while (!_TryAssertType(tokens, TokenType::RBRACE));
			}

			out->original_line_end = tokens.front().line->line_no;
			out->file_pos_end = tokens.front().file_index + tokens.front().raw_token.size();

			_Skip(tokens); // ]

			return out;
		}

		default:
			ThrowException("Got unexpected token for value (" + std::string(type_names.at(t.type)) + ")", t);
		}
	}

	size_t ConfigParser::GetTokens(std::string_view file, std::list<RawLine>::const_iterator& line, size_t& file_pos, size_t& pos, TokenList& tokens)
	{
		using namespace std;

		size_t token_count = 0;

		if (file.empty()) return 0;

		const std::string_view seperators = " ;\n\t\v\f\r";

		while (seperators.find(file[0]) != std::string::npos)
		{
			if (file[0] == '\n')
			{
				line++;
				pos = 0;
			}
			else
			{
				pos++;
			}
			file = file.substr(1);
			file_pos++;
			if (file.empty()) return token_count;
		}

		size_t str_pos = file.find('"');
		size_t comment_pos = file.find("!!");

		while (str_pos != string::npos || comment_pos != string::npos)
		{
			if (str_pos < comment_pos) // String
			{
				token_count += GetTokens(file.substr(0, str_pos), line, file_pos, pos, tokens) + 1;
				file = file.substr(str_pos);

				size_t start_pos = pos;
				size_t end = 0;
				size_t i = 0;

				bool is_invalid = false;
				size_t invalid_pos;

				string str = "";

				if (Contains(file.substr(1), '"', i) && i < file.find('\n'))
				{
					for (size_t it = 1; file[it] != '\n'; it++)
					{
						char c = file[it];

						if (c == '"')
						{
							end = it;
							break;
						}

						if (c != '\\')
						{
							if (isprint(c) || isspace(c))
							{
								str += c;
							}
							else
							{
								str += ' ';

								if (!is_invalid)
								{
									is_invalid = true;
									invalid_pos = pos;
								}
							}
							continue;
						}

						it++;

						c = file[it];

						if (c == '\n') break;

						switch (c)
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
							for (;;)
							{
								char value = 0;

								assert(it + 2 < file.size());

								c = file[it + 1];

								if (c >= '0' && c <= '9')      value = (c - '0') << 4;
								else if (c >= 'a' && c <= 'f') value = (10 + c - 'a') << 4;
								else if (c >= 'A' && c <= 'F') value = (10 + c - 'A') << 4;
								else
								{
									str += 'x';
									break;
								}

								c = file[it + 2];

								if (c >= '0' && c <= '9')      value += c - '0';
								else if (c >= 'a' && c <= 'f') value += 10 + c - 'a';
								else if (c >= 'A' && c <= 'F') value += 10 + c - 'A';
								else
								{
									str += 'x';
									break;
								}

								it += 2;
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
							if (isprint(c) || isspace(c))
							{
								str += c;
							}
							else
							{
								str += ' ';

								if (!is_invalid)
								{
									is_invalid = true;
									invalid_pos = pos;
								}
							}
							break;
						}
					}
				}

				if (end == 0)
				{
					tokens.push_back({ TokenType::INVALID, file.substr(0,1), "\"", file_pos, &*line, start_pos });
				}
				else
				{
					tokens.push_back({ is_invalid ? TokenType::INVALID : TokenType::STRING, file.substr(0,end + 1), str, file_pos, &*line, is_invalid ? invalid_pos : start_pos + 1 });
				}

				token_count++;

				file = file.substr(end + 1);
				file_pos += end + 1;
				pos += end + 1;
			}
			else // Comment
			{
				token_count += GetTokens(file.substr(0, comment_pos), line, file_pos, pos, tokens);
				file = file.substr(comment_pos + 2);
				file_pos += 2;
				pos += 2;

				size_t newline_pos;

				if (!Contains(file, '\n', newline_pos)) return token_count; // No newline after comment; means there is no more code

				file = file.substr(newline_pos);
				file_pos += newline_pos;
				pos += newline_pos;

				//tokens.push_back({ TokenType::END_LINE, file.substr(0,1), "\\n", line, pos });
				//token_count++;
				file = file.substr(1);
				file_pos++;
				line++;
				pos = 0;
			}

			if (file.empty()) return token_count;

			str_pos = file.find('"');
			comment_pos = file.find("!!");
		}

		size_t token_pos;

#define _GetTokens(type,term,l)\
		while (Contains(file, term, token_pos))\
		{\
			token_count += GetTokens(file.substr(0, token_pos), line, file_pos, pos, tokens) + 1;\
			tokens.push_back({ TokenType::type, file.substr(token_pos,l), term, file_pos, &*line, pos });\
			file = file.substr(token_pos + l);\
			file_pos += l;\
			pos += l;\
			if (file.empty()) return token_count;\
		}

		_GetTokens(DOUBLE_SLASH, "//", 2);
		_GetTokens(DOUBLE_DOT, "..", 2);
		_GetTokens(LREF, "<", 1);
		_GetTokens(SLASH, "/", 1);
		_GetTokens(RREF, ">", 1);
		_GetTokens(ASSIGN, "=", 1);
		_GetTokens(LPAREN, "(", 1);
		_GetTokens(RPAREN, ")", 1);
		_GetTokens(COMMA, ",", 1);
		_GetTokens(LBRACKET, "{", 1);
		_GetTokens(RBRACKET, "}", 1);
		_GetTokens(LBRACE, "[", 1);
		_GetTokens(RBRACE, "]", 1);

		for (size_t colour_pos = file.find('#'); colour_pos != string::npos; colour_pos = file.find('#'))
		{
			token_count += GetTokens(file.substr(0, colour_pos), line, file_pos, pos, tokens) + 1;
			file = file.substr(colour_pos);

			const size_t colour_end = file.find_first_of(seperators, colour_pos);
			const size_t colour_size = (colour_end == string::npos ? file.length() : colour_end) - (colour_pos + 1);

			if (colour_size == 0 || colour_size > 8 || !IsChars(file.substr(1, colour_size), "0123456789abcdefABCDEF"))
			{
				tokens.push_back({ TokenType::INVALID, file.substr(0,1), "#", file_pos , &*line, pos });
				file = file.substr(1);
				pos++;
				file_pos++;
				continue;
			}

			tokens.push_back({ TokenType::COLOUR, file.substr(0, colour_size + 1), string(file.substr(1, colour_size)), file_pos, &*line, pos });
			file = file.substr(colour_size + 1);
			file_pos += colour_size + 1;
			pos += colour_size + 1;
		}

#undef _GetTokens

		while (!file.empty())
		{
			while (!Contains("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", file[0]))
			{
				if (seperators.find(file[0]) != std::string::npos)
				{
					if (file[0] == '\n')
					{
						line++;
						pos = 0;
					}
					else
					{
						pos++;
					}

					file = file.substr(1);
					file_pos++;
				}
				else
				{
					size_t last = file.find_first_of(" \f\v\t\r\n");

					token_count++;

					if (last == string::npos)
					{
						tokens.push_back({ TokenType::INVALID, file, string(file), file_pos, &*line, pos });
						file_pos += file.size();
						pos += file.size();
						return token_count;
					}

					tokens.push_back({ TokenType::INVALID, file.substr(0,last), string(file.substr(0,last)), file_pos, &*line, pos });

					file = file.substr(last);
					file_pos += last;
					pos += last;
				}

				if (file.empty()) return token_count;
			}

			string_view t = file.substr(0, file.find_first_of(" \f\v\t\r\n"));
			file = file.size() == t.size() ? string_view() : file.substr(t.size());

			Token tok { TokenType::INVALID, t, string(t), file_pos, &*line, pos };

			file_pos += t.size();

			pos += t.size();

			if (IsInteger(t))
			{
				tok.type = TokenType::INTEGER;
			}
			else if (IsFloat(t))
			{
				tok.type = TokenType::FLOAT;
			}
			else if (IsVarName(t))
			{
				tok.type = TokenType::VARNAME;
			}

			tokens.push_back(tok);

			token_count++;
		}

		return token_count;
	}

	ConfigObject* ConfigParser::ParseFile(std::string_view file_contents)
	{
		using namespace std;

		size_t file_pos = 0;
		size_t line_pos = 0;

		TokenList tokens = {};

		list<RawLine> lines = {};

		string_view file_view = file_contents;

		size_t newline;
		size_t line_no = 1;

		while ((newline = file_view.find_first_of('\n')) != string::npos)
		{
			lines.push_back
			(
				{
					line_no++,
					file_view.substr(0, newline)
				}
			);

			file_view = file_view.substr(newline + 1);
		}

		lines.push_back
		(
			{
				line_no,
				file_view
			}
		);

		auto it = lines.cbegin();

		GetTokens(file_contents, it, file_pos, line_pos, tokens);

		ConfigObject* root = new ConfigObject();

		if (tokens.empty()) return root;
		
		Token token = ConsumeType(tokens, TokenType::VARNAME);
		SkipType(tokens, TokenType::ASSIGN);
		root->settings[token.token] = *GetValue(tokens, *root, nullptr);

		while (_TryAssertContinues(tokens))
		{
			_AssertType(tokens, TokenType::VARNAME);
			const Token name = _Consume(tokens);

			if (root->settings.count(name.token) > 0) ThrowException("Duplicate child name detected", name);

			SkipType(tokens, TokenType::ASSIGN);
			root->settings[name.token] = *GetValue(tokens, *root, nullptr);
		}

		return root;
	}

	ConfigObject* ConfigParser::ParseFile(std::istream& file_stream)
	{
		using namespace std;

		string file_contents;
		StreamToString(file_stream, file_contents);

		return ParseFile(file_contents);
	}
};