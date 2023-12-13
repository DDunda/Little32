#ifndef L32_ConfigFile_h_
#define L32_ConfigFile_h_
#pragma once

#include "L32_IO.h"
#include "L32_String.h"

#include <rect.hpp>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <string>

namespace Little32
{
	class VarValue;

	typedef std::map<std::string, VarValue> ConfigObject;

	struct VarReference
	{
		bool is_root_reference = false; // '//'
		size_t backreference_count = 0; // '..'

		std::list<std::string> names = {};
	};

	class ConfigParser;

	class VarValue
	{	friend ConfigParser;
	public:
		enum class ValueType
		{
			NONE,
			OBJECT,
			STRING,
			INTEGER,
			BOOLEAN,
			VECTOR,
			COLOUR, // #V or #VV or #RGB or #RGBA or #RRGGBB or #RRGGBBAA 
			LIST,
			UNRESOLVED_REFERENCE
		};

	private:
		VarValue* parent = nullptr;
		ValueType type = ValueType::NONE;

		union
		{
			char data[16];
			ConfigObject* object_value;
			std::string* string_value;
			int32_t integer_value;
			bool boolean_value;
			SDL::Point vector_value;
			SDL::Colour colour_value;
			std::list<VarValue>* list_values;
			VarReference* reference_value;
		};

		inline void FreeType() noexcept;

	public:
		size_t original_line_start;
		size_t original_line_end;

		size_t file_pos_start;
		size_t file_pos_end;

		VarValue();

		VarValue(const VarValue& other);

		~VarValue();

		inline constexpr ValueType GetType() const;

		inline constexpr ConfigObject& GetObjectValue() const noexcept;

		inline VarValue* GetTypeFromObject(const std::string& name, VarValue::ValueType type);

		inline VarValue* GetFromObject(const std::string& name);

		inline std::string& GetStringValue() const noexcept;

		inline constexpr int32_t GetIntegerValue() const noexcept;

		inline constexpr bool GetBooleanValue() const noexcept;

		inline constexpr SDL::Point GetVectorValue() const noexcept;

		inline constexpr SDL::Colour GetColourValue() const noexcept;

		inline std::list<VarValue>& GetListValues() const noexcept;

		void AddCloneToThisList(const VarValue& to_copy);

		void AddCloneToThisObject(const std::string& str, const VarValue& to_copy);

		inline constexpr VarValue* GetParent() const;

		/*bool TryResolveReference(ConfigObject& root)
		{
			assert(type == ValueType::UNRESOLVED_REFERENCE);

			VarValue* value = GetFromReference(root);

			if (value == nullptr) return false;

			value->CopyTo(*this);

			return true;
		}

		void ResolveReference(ConfigObject& root)
		{
			assert(type == ValueType::UNRESOLVED_REFERENCE);

			VarValue* value = GetFromReference(root, parent, *reference_value);

			assert(value != nullptr);

			value->CopyTo(*this);
		}*/
	};

	class ConfigParser
	{
		enum class TokenType
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

		struct RawLine
		{
			size_t line_no;
			std::string_view line;
		};

		struct Token
		{
			TokenType type = TokenType::INVALID;
			std::string_view raw_token = {};
			std::string token = "";

			RawLine* line = nullptr;
			size_t index = 0;
		};

		typedef std::list<Token> TokenList;

		inline static constexpr bool IsInteger(const std::string_view in_str);

		inline static constexpr bool IsFloat(const std::string_view in_str);

		inline static constexpr bool IsVarName(const std::string_view in_str);

		static size_t GetTokens(std::string_view file, RawLine* line, size_t& pos, std::list<RawLine>& lines, TokenList& tokens);

	public:
		
		static void ParseFile(std::string_view file_contents);

		static void ParseFile(std::istream& file_stream);
	};
};

#endif