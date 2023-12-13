#include "L32_ConfigParser.h"

namespace Little32
{
	/*inline void VarValue::FreeType() noexcept
	{
		switch (type)
		{
		case ValueType::OBJECT:
			if (object_value != nullptr) delete object_value;
			break;

		case ValueType::STRING:
			if (string_value != nullptr) delete string_value;
			break;

		case ValueType::LIST:
			if (list_values != nullptr) delete list_values;
			break;

		case ValueType::UNRESOLVED_REFERENCE:
			if (reference_value != nullptr) delete reference_value;
			break;
		}

		for (size_t i = 0; i < 16; ++i) data[i] = 0;

		type = ValueType::NONE;
	}

	inline VarValue::VarValue() { type = ValueType::NONE; }

	inline VarValue::VarValue(const VarValue& other)
	{
		type = other.type;

		switch (type)
		{
		case ValueType::OBJECT:

			return;

		case ValueType::STRING:
			string_value = new std::string(*(other.string_value));
			return;

		case ValueType::INTEGER:
			integer_value = other.integer_value;
			return;

		case ValueType::BOOLEAN:
			boolean_value = other.boolean_value;
			return;

		case ValueType::VECTOR:
			vector_value = other.vector_value;
			return;

		case ValueType::COLOUR:
			colour_value = other.colour_value;
			return;

		case ValueType::LIST:
			list_values = new std::list<VarValue>(*other.list_values);
			return;

		case ValueType::UNRESOLVED_REFERENCE:
			reference_value = new VarReference(*other.reference_value);
			return;
		}
	}

	inline VarValue::~VarValue() { FreeType(); }

	inline constexpr VarValue::ValueType VarValue::GetType() const { return type; }

	inline constexpr ConfigObject& VarValue::GetObjectValue() const noexcept
	{
		assert(type == ValueType::OBJECT);

		return *object_value;
	}

	inline VarValue* VarValue::GetTypeFromObject(const std::string& name, VarValue::ValueType type)
	{
		if (!object_value->contains(name)) return nullptr;
		if ((*object_value)[name].GetType() != type) return nullptr;

		return &(*object_value)[name];
	}

	inline VarValue* VarValue::GetFromObject(const std::string& name)
	{
		if (!object_value->contains(name)) return nullptr;

		return &(*object_value)[name];
	}

	inline std::string& VarValue::GetStringValue() const noexcept
	{
		assert(type == ValueType::STRING);

		return *string_value;
	}

	inline constexpr int32_t VarValue::GetIntegerValue() const noexcept
	{
		assert(type == ValueType::INTEGER);

		return integer_value;
	}

	inline constexpr bool VarValue::GetBooleanValue() const noexcept
	{
		assert(type == ValueType::BOOLEAN);

		return boolean_value;
	}

	inline constexpr SDL::Point VarValue::GetVectorValue() const noexcept
	{
		assert(type == ValueType::VECTOR);

		return vector_value;
	}

	inline constexpr SDL::Colour VarValue::GetColourValue() const noexcept
	{
		assert(type == ValueType::COLOUR);

		return colour_value;
	}

	inline std::list<VarValue>& VarValue::GetListValues() const noexcept
	{
		assert(type == ValueType::LIST);

		return *list_values;
	}

	inline void VarValue::AddCloneToThisList(const VarValue& to_copy)
	{
		assert(type == ValueType::LIST);

		list_values->push_back(VarValue(to_copy));
	}

	inline void VarValue::AddCloneToThisObject(const std::string& str, const VarValue& to_copy)
	{
		assert(type == ValueType::OBJECT);

		(*object_value)[str] = VarValue(to_copy);
	}

	inline constexpr VarValue* VarValue::GetParent() const { return parent; }*/

	inline constexpr bool ConfigParser::IsInteger(const std::string_view in_str)
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

	inline constexpr bool ConfigParser::IsFloat(const std::string_view in_str)
	{
		assert(!in_str.empty());

		std::string_view str = in_str;

		if (str[0] == '-' || str[0] == '+')
		{
			str = str.substr(1);
			if (str.empty()) return false;
		}

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

	inline constexpr bool ConfigParser::IsVarName(const std::string_view in_str)
	{
		assert(!in_str.empty());

		if (in_str[0] < 'A' || in_str[0] > 'z' || (in_str[0] > 'Z' && in_str[0] < 'a' && in_str[0] != '_')) return false;

		for (auto c : in_str.substr(1))
		{
			if (c < '0' || c > 'z' || (c > '9' && c < 'A') || (c > 'Z' && c < 'a' && c != '_')) return false;
		}
		return true;
	}

	size_t ConfigParser::GetTokens(std::string_view file, RawLine* line, size_t& pos, std::list<RawLine>& lines, TokenList& tokens)
	{
		using namespace std;

		size_t token_count = 0;

		if (file.empty()) return 0;

		while (isspace(file[0]))
		{
			if (file[0] == '\n')
			{
				//tokens.push_back({ TokenType::END_LINE, file.substr(0,1), "\\n", line, pos });
				//token_count++;

				lines.push_back
				(
					{
						line->line_no + 1,
						file.substr(1, file.find_first_of('\n', 1) - 1)
					}
				);

				line = &lines.back();

				pos = 0;
			}
			else
			{
				pos++;
			}
			file = file.substr(1);
			if (file.empty()) return token_count;
		}

		size_t str_pos = file.find('"');
		size_t comment_pos = file.find("!!");

		while (str_pos != string::npos || comment_pos != string::npos)
		{
			if (str_pos < comment_pos) // String
			{
				token_count += GetTokens(file.substr(0, str_pos), line, pos, lines, tokens) + 1;
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
					tokens.push_back({ TokenType::INVALID, file.substr(0,1), "\"", line, start_pos });
				}
				else
				{
					tokens.push_back({ is_invalid ? TokenType::INVALID : TokenType::STRING, file.substr(0,end + 1), str, line, is_invalid ? invalid_pos : start_pos + 1 });
				}

				token_count++;

				file = file.substr(end + 1);
				pos += end + 1;
			}
			else // Comment
			{
				token_count += GetTokens(file.substr(0, comment_pos), line, pos, lines, tokens);
				file = file.substr(comment_pos + 2);
				pos += 2;

				size_t newline_pos;

				if (!Contains(file, '\n', newline_pos)) return token_count; // No newline after comment; means there is no more code

				file = file.substr(newline_pos);
				pos += newline_pos;

				//tokens.push_back({ TokenType::END_LINE, file.substr(0,1), "\\n", line, pos });
				//token_count++;
				file = file.substr(1);
				pos = 0;

				lines.push_back
				(
					{
						line->line_no + 1,
						file.substr(0, file.find_first_of('\n'))
					}
				);

				line = &lines.back();
			}

			if (file.empty()) return token_count;

			str_pos = file.find('"');
			comment_pos = file.find("!!");
		}

		size_t token_pos;

#define _GetTokens(type,term,l)\
			while (Contains(file, term, token_pos))\
			{\
				token_count += GetTokens(file.substr(0, token_pos), line, pos, lines, tokens) + 1;\
				tokens.push_back({ TokenType::type, file.substr(token_pos,l), term, line, pos });\
				file = file.substr(token_pos + l);\
				pos += l;\
				if (file.empty()) return token_count;\
			}

		_GetTokens(DOUBLE_SLASH, "//", 2);
		_GetTokens(DOUBLE_DOT, "..", 2);
		_GetTokens(LREF, "<", 1);
		_GetTokens(SLASH, "/", 2);
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
			token_count += GetTokens(file.substr(0, colour_pos), line, pos, lines, tokens) + 1;
			file = file.substr(colour_pos);

			size_t colour_end = file.find_first_of(" \f\v\t\r\n", colour_pos);
			size_t colour_size = (colour_end == string::npos ? file.length() : colour_end) - (colour_pos + 1);

			if (colour_size == 0 || colour_size > 8 || !IsChars(file.substr(1, colour_size), "0123456789abcdefABCDEF"))
			{
				tokens.push_back({ TokenType::INVALID, file.substr(0,1), "#", line, pos });
				file = file.substr(1);
				pos++;
				continue;
			}

			tokens.push_back({ TokenType::COLOUR, file.substr(0, colour_size + 1), string(file.substr(1, colour_size)), line, pos });
			file = file.substr(colour_size + 1);
			pos += colour_size + 1;
		}

#undef _GetTokens

		while (!file.empty())
		{
			while (!Contains("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", file[0]))
			{
				if (file[0] == '\n')
				{
					//tokens.push_back({ TokenType::END_LINE, file.substr(0,1), "\\n", line, pos });
					//token_count++;

					file = file.substr(1);
					pos = 0;

					lines.push_back
					(
						{
							line->line_no + 1,
							file.substr(0, file.find_first_of('\n'))
						}
					);

					line = &lines.back();
				}
				else if (isspace(file[0]))
				{
					file = file.substr(1);
					pos++;
				}
				else
				{
					size_t last = file.find_first_of(" \f\v\t\r\n");

					token_count++;

					if (last == string::npos)
					{
						tokens.push_back({ TokenType::INVALID, file, string(file), line, pos });
						pos += file.size();
						return token_count;
					}

					tokens.push_back({ TokenType::INVALID, file.substr(0,last), string(file.substr(0,last)), line, pos });

					file = file.substr(last);
					pos += last;
				}

				if (file.empty()) return token_count;
			}

			string_view t = file.substr(0, file.find_first_of(" \f\v\t\r\n"));
			file = file.size() == t.size() ? string_view() : file.substr(t.size());

			Token tok { TokenType::INVALID, t, string(t), line, pos };

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

	void ConfigParser::ParseFile(std::string_view file_contents)
	{
		using namespace std;

		list<RawLine> lines =
		{
			{
				1, // Begin at line 1
				file_contents.substr(0, file_contents.find_first_of('\n'))
			}
		};

		size_t line_pos = 0;

		TokenList tokens = {};

		GetTokens(file_contents, &lines.back(), line_pos, lines, tokens);
	}

	void ConfigParser::ParseFile(std::istream& file_stream)
	{
		using namespace std;

		string file_contents;
		StreamToString(file_stream, file_contents);

		ParseFile(file_contents);
	}
};