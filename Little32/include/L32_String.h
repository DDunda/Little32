#pragma once

#ifndef L32_String_h_
#define L32_String_h_

#include <stdexcept>
#include <string>
#include <unordered_map>

#include "L32_BigInt.h"
#include "L32_Types.h"
#include "L32_VarValue.h"

namespace Little32
{
	struct Computer;

	// The readable names of registers by index
	static constexpr const char* const REGISTER_NAMES[16] { "R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","R12","SP","LR","PC" };
	// The readable names of conditions by index
	static constexpr const char* const CONDITION_NAMES[16]{ "AL","GT","GE","HI","HS","EQ","MI","VS","VC","PL","NE","LO","LS","LT","LE","NV" };
	// The variant names of conditions by index
	static constexpr const char* const CONDITION_NAMES_ALT[16]{ "","","","","CS","ZS","NS","","","NC","ZC","CC","","","","" };
	// All condition names
	static const std::unordered_map<std::string, byte> CONDITION_IDS
	{
		{"AL",0b0000},
		{"GT",0b0001},
		{"GE",0b0010},
		{"HI",0b0011},
		{"CS",0b0100},{"HS",0b0100},
		{"ZS",0b0101},{"EQ",0b0101},
		{"NS",0b0110},{"MI",0b0110},
		{"VS",0b0111},
		{"VC",0b1000},
		{"NC",0b1001},{"PL",0b1001},
		{"ZC",0b1010},{"NE",0b1010},
		{"CC",0b1011},{"LO",0b1011},
		{"LS",0b1100},
		{"LT",0b1101},
		{"LE",0b1110},
		{"NV",0b1111}
	};

	// Converts a word to its binary representation to an optionally specified number of bits
	constexpr std::string ToBinary(word value, byte pad = 32)
	{
		std::string str = "";

		for (; pad > 0; pad--) str += "0";

		for (size_t i = str.length(); i > 0 && value; value >>= 1, i--){
			if (value & 1) str[i - 1] = '1';
		}

		for (; value; value >>= 1)
		{
			if (value & 1) str = "1" + str;
			else str = "0" + str;
		}

		return str;
	}

	// Converts a list of registers encoded in bits into a human readable format (e.g. {R0-R7,R9,LR})
	constexpr std::string RegListToString(word list)
	{
		using namespace std;

		list &= 0x0000FFFF;

		if (list == 0) return "{}";

		vector<word> regs;
		for (word i = 0; list; i++, list >>= 1)
		{
			if (list & 1) regs.push_back(i);
		}

		word rsize = (word)regs.size();
		string str;

		for (size_t i = 0;;)
		{
			word start = regs[i];
			word end = start;

			str += REGISTER_NAMES[start];

			if ((i + 2 < rsize) && regs[i + 2] == end + 2)
			{
				end += 2;
				i += 2;

				while ((i + 1 < rsize) && (regs[i + 1] == end + 1))
				{
					end++;
					i++;
				}

				str += "-";
				str += REGISTER_NAMES[end];
			}

			if (++i == rsize) return "{" + str + "}";

			str += ", ";
		}
	}

	void DisassembleMemory(Computer& computer, word start, word end, word offset = 0, bool print_NOP = false);

	void PrintMemory(Computer& computer, word start, word end, word offset = 0, bool print_null = false);

	constexpr bool IsChar(std::string_view str, const char c, size_t off = 0)
	{
		return str.find_first_not_of(c, off) == std::string::npos;
	}

	constexpr bool IsChars(std::string_view str, const char* const chars, size_t off = 0) noexcept
	{
		return chars != nullptr && str.find_first_not_of(chars, off) == std::string::npos;
	}

	constexpr bool IsChars(std::string_view str, const char min_char, const char max_char, size_t off = 0)
	{
		for (auto c : str.substr(off))
		{
			if (c < min_char || c > max_char) return false;
		}
		return true;
	}

	constexpr bool IsDecimal(std::string_view str) noexcept {
		if (str.empty()) return false;
		if (str.length() == 1) return str[0] >= '0' && str[0] <= '9';
		if (str[0] == '0') return false;
		if (IsChar(str, '_')) return false;
		return IsChars(str, "0123456789_");
	}

	constexpr bool IsBinary(std::string_view str) noexcept {
		if (str.length() < 3) return false;
		if (str[0] != '0' || str[1] != 'b') return false;
		if (IsChar(str, '_', 2)) return false;
		return IsChars(str, "01_", 2);
	}

	constexpr bool IsOctal(std::string_view str) noexcept {
		if (str.length() < 2) return false;
		if (str[0] != '0') return false;
		if (IsChar(str, '_', 1)) return false;
		return IsChars(str, "01234567_", 1);
	}

	constexpr bool IsHexadecimal(std::string_view str) noexcept {
		if (str.length() < 3) return false;
		if (str[0] != '0' || str[1] != 'x') return false;
		if (IsChar(str, '_', 2)) return false;
		return IsChars(str, "0123456789abcdefABCDEF_", 2);
	}

	constexpr bool IsNumeric(std::string_view str)
	{
		for (auto c : str)
		{
			if (c < '0' || c > '9') return false;
		}

		return true;
	}

	constexpr std::string_view TrimStart(std::string_view str, const std::string_view chars = " \t\r\n")
	{
		if (str.empty()) return str;
		size_t i = str.find_first_not_of(chars);
		if (i == str.npos)
		{
			return str.substr(str.length());
		}
		else if (i != 0)
		{
			return str.substr(i);
		}
	}

	constexpr std::string_view TrimEnd(std::string_view str, const std::string_view chars = " \t\r\n")
	{
		if (str.empty()) return str;
		size_t i = str.find_last_not_of(chars);
		if (i == str.npos)
		{
			return str.substr(0, 0);
		}
		else if (i != 0)
		{
			return str.substr(0, i + 1);
		}
	}

	constexpr std::string_view TrimString(std::string_view str, const std::string_view chars = " \t\r\n")
	{
		if (str.empty()) return str;
		size_t i = str.find_first_not_of(chars);
		if (i == str.npos)
		{
			return str.substr(str.length());
		}
		size_t j = str.find_last_not_of(chars);
		return str.substr(i, (j + 1) - i);
	}

	constexpr bool Contains(std::string_view str, const std::string& token, size_t& i) noexcept
	{
		return (i = str.find(token)) != str.npos;
	}

	constexpr bool Contains(std::string_view str, char c, size_t& i) noexcept
	{
		return (i = str.find(c)) != str.npos;
	}

	constexpr bool Contains(std::string_view str, const std::string_view token) noexcept
	{
		return str.find(token) != str.npos;
	}

	constexpr bool Contains(std::string_view str, char c) noexcept
	{
		return str.find(c) != str.npos;
	}

	constexpr bool _decToI32_validate(std::string_view str)
	{
		if (str.empty()) return false;
		str = str.substr(str.front() == '-' || str.front() == '+');
		return (str.length() == 1 && str.front() >= '0' && str.front() <= '9') || 
		       (str.length() >  1 && str.front() != '0' && str.length() <= 10 && IsNumeric(str)); // -2147483648 and 2147483647 have 10 digits
	}

	constexpr bool _decToI32(std::string_view str, int32_t& out)
	{
		out = 0;
		if (str.front() == '-')
		{
			str = str.substr(1);
			for (char c : str)
			{
				out *= 10;
				out -= (int32_t)c - '0';
				if (out > 0) return false; // Overflow
			}
		}
		else
		{
			for (char c : str)
			{
				out *= 10;
				out += (int32_t)c - '0';
				if (out < 0) return false; // Overflow
			}
		}
		return true;
	}

	inline constexpr bool decToI32(std::string_view str, int32_t& out)
	{
		return _decToI32_validate(str) && _decToI32(str, out);
	}

	inline constexpr std::string ToOrdinal(size_t num)
	{		
		if ((num % 100) / 10 == 1) // The teens, *1*th: 11th, 12th, 13th... 111th, 112th, 113th... 211th, 212th, 213th...
			return std::to_string(num) + "th";

		switch (num % 10) // Everything else:
		{
		case 1:  return std::to_string(num) + "st"; // *1st
		case 2:  return std::to_string(num) + "nd"; // *2nd
		case 3:  return std::to_string(num) + "rd"; // *3rd
		default: return std::to_string(num) + "th"; // *0th, *4th, *5th, *6th, *7th, *8th, *9th
		}
	}

	inline constexpr void MatchType(const VarValue& value, ValueType expected, const std::string& var_name)
	{
		if (value.GetType() == expected) return;
		throw std::runtime_error(
			"Expected '" +
				var_name +
			"' to be '" +
				std::string(VALUETYPE_NAMES.at(expected)) +
			"' type, got '" +
				std::string(VALUETYPE_NAMES.at(value.GetType())) +
			"' type"
		);
	}

	template<const int64_t min, const int64_t max>
	inline constexpr void MatchIntRange(const VarValue& value, const std::string& var_name)
	{
		static_assert(min <= max);

		MatchType(value, INTEGER_VAR, var_name);
		const BigInt& bint = value.GetIntegerValue();

		if constexpr (min == max) // x == a == b
		{
			if constexpr (min == 0) // a == 0
			{
				if (bint.bits.empty()) return;
			}
			else if constexpr (min < 0) // a < 0
			{
				if (bint.negative && bint.bits.size() == 1 && bint.bits[0] == -min) return;
			}
			else // a > 0
			{
				if (!bint.negative && bint.bits.size() == 1 && bint.bits[0] == max) return;
			}
		}
		else if constexpr (max == 0) // -a <= x <= 0
		{
			if (bint.bits.empty()) return; // x == 0
			if (bint.negative && bint.bits.size() == 1 && bint.bits[0] <= -min) return; // +a >= -x > 0
		}
		else if constexpr (min == 0) // 0 <= x <= +b
		{
			if (bint.bits.empty()) return; // x == 0
			if (!bint.negative && bint.bits.size() == 1 && bint.bits[0] <= max) return; // 0 < x <= +b
		}
		else if constexpr (max < 0) // -a <= x <= -b
		{
			if (bint.negative && bint.bits.size() == 1 && bint.bits[0] >= -min && bint.bits[0] <= -max) return;
		}
		else if constexpr (min > 0) // +a <= x <= +b
		{
			if (!bint.negative && bint.bits.size() == 1 && bint.bits[0] >= min && bint.bits[0] <= max) return;
		}
		else // -a <= x <= +b
		{
			if (bint.bits.empty()) return; // x == 0
			if (bint.negative) // -x <= +a
			{
				if (bint.bits.size() == 1 && bint.bits[0] <= -min) return;
			}
			else // x <= +b
			{
				if (bint.bits.size() == 1 && bint.bits[0] <= max) return;
			}
		}

		throw std::runtime_error(
			"Expected '" +
			var_name +
			"' to be between " +
				std::to_string(min) +
			" and '" +
				std::to_string(max) +
			", got " +
				bint.ToStringCheap()
		);
	}

	template<const uint64_t min, const uint64_t max>
	inline constexpr void MatchUIntRange(const VarValue& value, const std::string& var_name)
	{
		static_assert(min <= max);

		MatchType(value, INTEGER_VAR, var_name);
		const BigInt& bint = value.GetIntegerValue();

		if constexpr (min == 0)
		{
			if (bint.bits.empty()) return;
			if constexpr (max != 0)
			{
				if (bint.bits.size() == 1 && bint.bits[0] <= max) return;
			}
		}
		else if (!bint.negative && bint.bits.size() == 1)
		{
			if constexpr (min == max)
			{
				if (bint.bits[0] == min) return;
			}
			else
			{
				if (bint.bits[0] >= min && bint.bits[0] <= max) return;
			}
		}

		throw std::runtime_error(
			"Expected '" +
				var_name +
			"' to be between " +
				std::to_string(min) +
			" and '" +
				std::to_string(max) +
			", got " +
				bint.ToStringCheap()
		);
	}
}

#endif