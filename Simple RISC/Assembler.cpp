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

	Assembler::FormatException::FormatException(size_t line_no, const std::string& line, const char* const message) :
		line_no(line_no),
		line(line),
		inner_message(message),
		message(std::string("Improper format: ") + std::string(message) + " (line: " + std::to_string(line_no) + ")" ),
		exception("Improper format") {}

	Assembler::FormatException::FormatException(size_t line_no, const std::string& line, const std::string& message) :
		line_no(line_no),
		line(line),
		inner_message(message),
		message(std::string("Improper format: ") + message + " (line: " + std::to_string(line_no) + ")"),
		exception("Improper format") {}

	void Assembler::ThrowException(const char* const msg) const { throw FormatException(_current_line.line_no, *_current_line.line, msg); }
	void Assembler::ThrowException(const std::string& msg) const { throw FormatException(_current_line.line_no, *_current_line.line, msg); }

	constexpr bool Contains(const std::string& str, const std::string& token, size_t& i) noexcept {
		i = str.find(token);
		return i != str.npos;
	}

	constexpr bool Contains(const std::string& str, char c, size_t& i) noexcept {
		i = str.find(c);
		return i != str.npos;
	}

	constexpr void TrimStart(std::string& str) {
		if (str.empty()) return;
		size_t i = str.find_first_not_of(" \t\r\n");
		if (i == str.npos) {
			str.clear();
		}
		else if(i != 0) {
			str.erase(0, i);
		}
	}

	constexpr void TrimEnd(std::string& str) {
		if (str.empty()) return;
		size_t i = str.find_last_not_of(" \t\r\n");
		if (i == str.npos) {
			str.clear();
		}
		else if (i != 0) {
			str.erase(i + 1);
		}
	}

	constexpr void TrimString(std::string& str) {
		if (str.empty()) return;
		size_t i = str.find_first_not_of(" \t\r\n");
		if (i == str.npos) {
			str.clear();
			return;
		}
		size_t j = str.find_last_not_of(" \t\r\n");
		/*if (i == 0) {
			str =  str.substr(0, j);
			return;
		}*/
		str = str.substr(i, (j + 1) - i);
	}

	template<class _Ty, class _Alloc>
	bool TryGet(const std::list<_Ty, _Alloc>& l, const typename std::list<_Ty, _Alloc>::iterator& it, _Ty& out) noexcept {
		if (it == l.end()) return false;

		out = *it;

		return true;
	}

	template<class _Ty, class _Alloc>
	bool TryConsume(std::list<_Ty, _Alloc>& l, typename std::list<_Ty, _Alloc>::iterator& it, _Ty& out) {
		if (it == l.end()) return false;

		out = *it;
		it = l.erase(it);

		return true;
	}

	template<class _Ty, class _Alloc>
	bool TryGetFront(const std::list<_Ty, _Alloc>& l, _Ty& out) noexcept {
		if (l.empty()) return false;

		out = l.front();

		return true;
	}

	template<class _Ty, class _Alloc>
	bool TryConsumeFront(std::list<_Ty, _Alloc>& l, _Ty& out) noexcept {
		if (l.empty()) return false;

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

	uint64_t decToI(std::string str, uint64_t max) {
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

	uint64_t binToI(std::string str, uint64_t max) {
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

	uint64_t octToI(std::string str, uint64_t max) {
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

	uint64_t hexToI(std::string str, uint64_t max) {
		if (str.length() > 2 && str[1] == 'x') {
			if (str[0] != '0') return max;
			str = str.substr(2);
		}
		if (!IsChars(str, "0123456789abcdefABCDEF_")) return max;
		uint64_t v = 0;
		for (auto c : str) {
			if (c == '_') continue;
			v <<= 4;
			if (c >= '0' && c <= '9') v += c - '0';
			else if (c >= 'a' && c <= 'f') v += 10 + c - 'a';
			else if (c >= 'A' && c <= 'F') v += 10 + c - 'A';
			if (v >= max) return max;
		}
		return v;
	}

	uint64_t Assembler::xToI(std::string str, word base, uint64_t max) {
		uint64_t val = max;

		switch (base)
		{
		case 16: val = hexToI(str, max); break;
		case 10: val = decToI(str, max); break;
		case 8:  val = octToI(str, max); break;
		case 2:  val = binToI(str, max); break;
		default: ThrowException("This number cannot be processed");
		}

		if (val == max) ThrowException("This number is too large");

		return val;
	}

	word Assembler::ToReg(const std::string& str) const {
		for (int i = 0; i < 16; i++) {
			if (str == regNames[i]) return i;
		}

		ThrowException("'" + str + "' is not a register");
	}

	constexpr bool IsAlpha(const std::string& str) noexcept {
		for (auto c : str) {
			if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) return false;
		}

		return true;
	}

	constexpr int IsNumber(const std::string& str) noexcept {
		if (str.empty()) return 0;
		if (str.length() == 1) return 10 * (str[0] >= '0' && str[0] <= '9');
		if (IsChar(str, '_')) return 0;
		if (str[0] != '0') return 10 * IsChars(str, "0123456789_");
		if (str.length() == 2) return 8 * (str[1] >= '0' && str[1] <= '7');
		if (str[1] == 'x') return 16 * IsChars(str, "0123456789abcdefABCDEF_", 2);
		if (str[1] == 'b') return 2 * IsChars(str, "01_", 2);
		return 8 * IsChars(str, "01234567_");
	}

	constexpr bool IsLower(const std::string& str) noexcept {
		for (auto c : str) {
			if (c < 'a' || c > 'z') return false;
		}

		return true;
	}

	constexpr bool IsUpper(const std::string& str) noexcept {
		for (auto c : str) {
			if (c < 'A' || c > 'Z') return false;
		}

		return true;
	}

	constexpr bool IsReg(const std::string& str) noexcept {
		for (int i = 0; i < 16; i++) {
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

	size_t GetTokens(std::string line, Assembler::token_list& tokens) {
		using namespace std;

		size_t token_count = 0;

		TrimString(line);

		/*const auto _GetTokens = [&](const char* const term) {
			size_t i;

			const size_t tlen = strlen(term);

			while (!line.empty() && Contains(line, term, i)) {
				GetTokens(line.substr(0, i), tokens);
				tokens.push_back(term);
				line = line.substr(i + tlen);
			}
		};*/

		size_t len = line.length();

		if (len == 0) return 0;
		if (len == 1) {
			tokens.push_back(line);
			return 1;
		}

		size_t i;

#define _GetTokens(term,l)\
		while (len >= l && Contains(line, term, i)) {\
			token_count += GetTokens(line.substr(0, i), tokens) + 1;\
			tokens.push_back(term);\
			len -= i + l;\
			line = line.substr(i + l);\
			if(len == 0) return token_count;\
		}

		_GetTokens("...",3);
		_GetTokens("@{",2);
		_GetTokens("}@",2);
		_GetTokens("${",2);
		_GetTokens("}$",2);
		_GetTokens("?{",2);
		_GetTokens("}?",2);
		_GetTokens(":{",2);
		_GetTokens("}:",2);
		_GetTokens("<<",2);
		_GetTokens(">>",2);
		//_GetTokens("'");
		_GetTokens("@",1);
		_GetTokens("$",1);
		_GetTokens("?",1);
		_GetTokens(":",1);
		_GetTokens(",",1);
		_GetTokens("(",1);
		_GetTokens(")",1);
		_GetTokens("[",1);
		_GetTokens("]",1);
		_GetTokens("{",1);
		_GetTokens("}",1);
		_GetTokens("+",1);
		_GetTokens("-",1);
		_GetTokens("#",1);
		//_GetTokens("="  );

		while (len >= 1 && Contains(line, ".", i)) {
			if (i == 0 || i == len - 1 || line[i - 1] < '0' || line[i - 1] > '9' || line[i + 1] < '0' || line[i + 1] > '9') {
				token_count += GetTokens(line.substr(0, i), tokens) + 1;
				tokens.push_back(".");
				len -= i + 1;
				line = line.substr(i + 1);
				if (len == 0) return token_count;
			}
			else {
				size_t start = line.find_last_not_of("0123456789.", i) + 1;
				size_t end  = line.find_first_not_of("0123456789.", i);
				if (start == string::npos) {
					token_count++;

					if (end == string::npos) {
						tokens.push_back(line);
						return token_count;
					}

					tokens.push_back(line.substr(0,end));
				}
				else {
					token_count += GetTokens(line.substr(0, start), tokens) + 1;

					if (end == string::npos) {
						tokens.push_back(line.substr(start));
						return token_count;
					}

					tokens.push_back(line.substr(start,end-start));
				}

				line = line.substr(end);
				len -= end;
			}
		}

#undef _GetTokens

		stringstream ss(line);
		string t;

		while (getline(ss, t, ' ')) {
			TrimStart(t);
			if (t.length() == 0) continue;
			tokens.push_back(t);
			token_count++;
		}

		return token_count;
	}

	bool Assembler::GetCond(token_list& l, byte& cond) const {
		using namespace std;

		auto it = std::find(l.begin(), l.end(), "?");

		if (it == l.end()) return false;

		it = l.erase(it); // Erase '?'

		if (it == l.end() || !IsCond(*it)) ThrowException("Expected condition for '?' statement");
		cond = condNames.at(*it);
		it = l.erase(it); // Erase condition
		
		if (it != l.end()) ThrowException("Line does not end after '?' statement");
		return true;
	}

	bool Assembler::GetShift(token_list& l, byte& shift) const {
		using namespace std;

		auto it = find(l.begin(), l.end(), "<<");
		it = it == l.end() ? find(l.begin(), l.end(), ">>") : it;

		if (it == l.end()) return false;

		bool rshift = *it == ">>" ? true : false;
		it = l.erase(it);

		if (it == l.end() || !IsNumeric(*it)) ThrowException("Expected number after shift");

		shift = stoul(*it);
		it = l.erase(it);

		if (shift >= 32) ThrowException("Shift is too large");
		if (it != l.end() &&*it != "?") ThrowException("Shift must be at end of instruction");

		if (rshift && shift) shift = 32 - shift;

		return true;
	}

	word Assembler::GetBranchOffset(token_list& l, byte shift, bool& isNegative) const {
		using namespace std;

		isNegative = false;
		if (l.front() == "-") {
			l.pop_front();
			isNegative = true;
		}

		if (l.size() != 1) ThrowException("Expected one value for offset");
		if (!IsNumeric(l.front())) ThrowException("Expected offset to be a number");

		word offset = stoul(l.front());
		offset = std::rotl(offset, shift);

		if (offset & 3) ThrowException("Expected offset to be word aligned");
		if (offset > 0x03FFFFFC) ThrowException("Offset is too large");

		return offset >> 2;
	}

	bool IsFloat(std::string str) {
		if (str.size() < 3) return false;
		if (std::count(str.begin(), str.end(), '.') != 1) return false;

		auto i = std::find(str.begin(), str.end(), '.');
		if (i == str.begin() || i == str.end()) return false;

		return IsChars(str, "0123456789.");
	}

	size_t Assembler::ConvertNumbers(token_list& tokens) const {
		size_t numbers_replaced = 0;

		for (auto& t : tokens) {
			int numBase = IsNumber(t);
			if (!numBase) {
				if (IsFloat(t)) {
					float f = std::stof(t);
					word val = *(word*)&f;
					t = std::to_string(val);
				}
				continue;
			}
			if (numBase == 10) {
				uint64_t long_val = stoul(t);
				word word_Val = long_val;
				if (long_val > word_Val) ThrowException("This number is too large for a word");
				continue;
			}
			numbers_replaced++;

			word _v = 0, v = 0;


			if (numBase == 2 || numBase == 16) t = t.substr(2);
			else if (numBase == 8) t = t.substr(1);

			for (auto c : t) {
				if (c == '_') continue;
				v *= numBase;
				if (c >= '0' && c <= '9') v += c - '0';
				else if (c >= 'a' && c <= 'f') v += 10 + c - 'a';
				else if (c >= 'A' && c <= 'F') v += 10 + c - 'A';

				if (_v > v) ThrowException("This number is too large for a word");
				_v = v;
			}
			t = std::to_string(v);
		}

		return numbers_replaced;
	}

	size_t Assembler::SplitSquareBrackets(token_list& l) const {
		using namespace std;

		size_t brackets_replaced = 0;

		auto it = l.begin();
		string token;

		while (TryGet(l, it, token)) {
			if (token == "]") ThrowException("Closing square bracket without opening");
			if (token != "[") {
				it++;
				continue;
			}
			brackets_replaced++;
			it = l.erase(it); // Erase '['

			auto start = it;

			if (!TryGet(l, it, token)) ThrowException("Square brackets must be closed");
			if (token == "]") ThrowException("Square brackets must contain contents");
			if (token == "[") ThrowException("Square brackets cannot be nested");
			if (token == ",") ThrowException("Square brackets cannot contain commas");

			if (token == "+" && token == "-") { // Bracket starts with a sign - Interpreted as single value relative to PC
				l.insert(it, "PC");
			}
			else {
				string first_token = token;
				it++;
				while (TryGet(l, it, token) && token != "]") {
					if (token == "[") ThrowException("Square brackets cannot be nested");
					if (token == ",") ThrowException("Square brackets cannot contain commas");
					if (token == "+" || token == "-") break;
					it++;
				}

				if (it == l.end()) ThrowException("Square brackets must be closed");

				if (token == "]") { // Occurs when no signs were encountered - Interpreted as single value relative to PC
					if (IsReg(first_token)) { // User entered a register; consider contents as relative to it
						l.insert(it, ",");
						l.insert(it, "0");
					}
					else {  // User entered a value; consider contents as relative to PC
						l.insert(start, "PC");
						l.insert(start, ",");
					}
					it = l.erase(it); // Erase ']'
					continue;
				}
			}

			if (token == "+") *it = ",";
			else l.insert(it, ",");
			it++;

			if (!TryGet(l, it, token)) ThrowException("Square brackets must be closed");
			if (token == "]") ThrowException("Sign in square brackets must be followed by offset");

			do {
				if (token == "[") ThrowException("Square brackets cannot be nested");
				if (token == ",") ThrowException("Square brackets cannot contain commas");
				if (token == "+" || token == "-") ThrowException("Square brackets can only have one offset");
				it++;
			} while (TryGet(l, it, token) && token != "]");

			if (token != "]") ThrowException("Square brackets must be closed");
			it = l.erase(it); // Erase ']'
		}

		return brackets_replaced;
	}

	size_t Assembler::ResolveVariables(token_list& l) const {
		using namespace std;

		size_t vars_replaced = 0;
		auto it = l.begin();
		string token;

		while (TryGet(l, it, token)) {
			if (token != "$") {
				it++;
				continue;
			}
			vars_replaced++;

			it = l.erase(it);

			string var_name;
			if (!TryConsume(l, it, var_name)) ThrowException("Variable name not provided");

			if (!IsChars(var_name, valid_text_chars)) ThrowException("Invalid characters in variable name");

			auto scope = variable_scopes.rbegin(); // Start with the innermost scope

			while (!scope->contains(var_name))
			{
				if (++scope == variable_scopes.rend()) ThrowException("Variable is undefined");
			}

			const token_list var = scope->at(var_name);

			it = l.insert(it, var.begin(), var.end());
		}
		return vars_replaced;
	}

	size_t Assembler::ResolveRelatives(AssemblyLine& l) const {
		using namespace std;
		
		size_t relatives_replaced = 0;

		for (auto& a : l.args) {
			for (auto it = a.begin(); it != a.end(); it++) {
				if (*it != ".") continue;

				it = a.erase(it);
				if (it == a.end() || !IsNumeric(*it)) ThrowException("Expected number after '.'");

				long int addr = stoul(*it) - l.addr;
				if (addr < 0) {
					addr *= -1;
					a.insert(it, "-");
				}

				relatives_replaced++;
				*it = to_string(addr);
			}
		}

		return relatives_replaced;
	}
	
	size_t Assembler::ResolveRegLists(token_list& l) const {
		using namespace std;

		size_t lists_replaced = 0;
		auto it = l.begin();
		string token;

		while (TryGet(l, it, token)) {
			if (token != "{") {
				it++;
				continue;
			}
			lists_replaced++;

			word list = 0;
			it = l.erase(it); // Erase '{'

			while (TryConsume(l, it, token) && token != "}") {
				word reg1 = 0;
				for (; reg1 < 16; reg1++) {
					if (regNames[reg1] == token) break;
				}
				if (reg1 == 16) ThrowException("Expected register in register list");

				if (!TryConsume(l, it, token)) ThrowException("Expected register list to continue");

				if (token == "," || token == "}") {
					if (list & (1 << reg1)) ThrowException("Register list has duplicate registers");
					list |= 1 << reg1;
					if (token == "}") break;
				} else if (token == "-") {
					if (!TryConsume(l, it, token)) ThrowException("Expected second register for range");
					word reg2 = 0;
					for (; reg2 < 16; reg2++) {
						if (regNames[reg2] == token) break;
					}
					if (reg2 == 16) ThrowException("Expected register in register list");
					if (reg1 > reg2) ThrowException("Expected register range to be from min to max");
					if (reg1 == reg2) ThrowException("Expected register range to be greater than 1");
					word range = ((0xFFFF >> (reg1 + (15 - reg2))) << reg1) & 0xFFFF;

					if (list & range) ThrowException("Register list has duplicate registers");
					list |= range;
				}
				else ThrowException("Unexpected value in register list");
			}

			if (token != "}") ThrowException("Register list must be closed");

			l.insert(it, to_string(list));
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

	void Assembler::Assemble(std::istream& code, bool print_intermediate) {
		using namespace std;

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

		list<string> raw_lines;
		list<AssemblyLine> assemblyLines;

		struct cond_scope { bool has_cond = false; word cond = 0; };

		list<cond_scope> cond_scopes{ {false, 0} };
		size_t cond_depth = 0;
		size_t variable_depth = 0;
		size_t op_depth = 0;

		list<list<string*>> pendingLabels = { {} };
		size_t label_depth = 0;

		string line;
		string token;
		bool byte_mode = false;

		_current_line = { 0, nullptr };

		while (getline(code, line)) {
			raw_lines.push_back(line);
			RemoveComments(line);
			_current_line.line = &(raw_lines.back());
			_current_line.line_no++;
			token_list line_tokens = {};
			GetTokens(line, line_tokens);
			if (line_tokens.empty()) continue;

			ConvertNumbers(line_tokens);

			// Process tokens
			while (TryConsumeFront(line_tokens, token))
			{
				bool isNegative = token == "-";
				bool isPositive = token == "+";
				if (isNegative || isPositive || IsNumeric(token)) { // This is a constant value
					if (byte_mode) {
						if (*current_address >= memory_range) ThrowException("This value would be written beyond memory");
						//if(isNegative) ThrowException("Bytes cannot be negative");

						if (isPositive || isNegative) {
							if (!TryConsumeFront(line_tokens, token)) ThrowException("Positive symbol not followed by anything");
							if (!IsNumeric(token)) ThrowException("Positive symbol not followed by number");
						}

						word val = stoul(token);

						if(val > 255) ThrowException("Value is too large");

						if (isNegative) val = 1 + ~val;

						PutByte(memory, *current_address, val);
						(*current_address)++;
					}
					else {
						if ((*current_address) & 3) ThrowException("Word not aligned");
						if (((*current_address) >> 2) >= (memory_range >> 2)) ThrowException("This value would be written beyond memory");

						if (isPositive || isNegative) {
							if (!TryConsumeFront(line_tokens, token)) ThrowException("Sign symbol not followed by anything");
							if (!IsNumeric(token)) ThrowException("Sign symbol not followed by number");
						}

						word val = stoul(token);

						if (isNegative) val = 1 + ~val;

						memory[(*current_address) >> 2] = val;
						(*current_address) += 4;
					}
					data_end = *current_address + memory_start;
				}
				else if (!line_tokens.empty() && line_tokens.front() == ":") { // This is a label
					if (!IsChars(token, valid_text_chars)) ThrowException("Invalid characters in label");
					if (label_scopes.back().contains(token)) ThrowException("Label already defined in scope");

					line_tokens.pop_front(); // Erase ':'
					label_scopes.back()[token] = *current_address + memory_start;

					string num = to_string(label_scopes.back()[token]);

					auto lit = pendingLabels.back().begin();
					while (lit != pendingLabels.back().end()) {
						if (*(*lit) == token) { // Resolve this label
							*(*lit) = num;
							lit = pendingLabels.back().erase(lit);
						}
						else { // Needs another label
							lit++;
						}
					}
				}
				else if (token == "#") {
					if (!TryConsumeFront(line_tokens, token)) ThrowException("Preprocessor directive not provided");
					if (token == "BLOCK") {
						if (!TryConsumeFront(line_tokens, token) || !IsNumeric(token)) ThrowException("Block size not provided");
						word size = stoul(token);
						if (size == 0) ThrowException("Expected nonzero memory block");
						if(*current_address + size >= memory_range) ThrowException("Block exceeds memory");

						if (data_start == program_start) data_start = *current_address + memory_start;

						for (int i = 0; i < size; i++) {
							PutByte(memory, *current_address + i, 0);
						}

						(*current_address) += size;
						data_end += size;
					}
					else if (token == "BYTE") {
						if (data_start == program_start) data_start = *current_address + memory_start;
						byte_mode = true;
					}
					else if (token == "WORD") {
						if (data_start == program_start) data_start = *current_address + memory_start;
						byte_mode = false;
					}
					else if (token == "ALIGN") {
						if (!TryConsumeFront(line_tokens, token)) ThrowException("Alignment width not provided");
						word width = stoul(token);
						word err = (*current_address + memory_start) % width;
						if (err == 0) continue;
						*current_address -= err;
						*current_address += width;
					}
					else if (token == "DATA") {
						if (ram != nullptr) {
							memory = ram;
							memory_start = ram_start;
							memory_range = ram_range;
							current_address = &ram_current_address;
						}
						if(data_start == program_start) data_start = *current_address + memory_start;
					}
					else if (token == "ENTRY") {
						if(entry_defined) ThrowException("Multiple entry points defined");
						if((*current_address + memory_start) % 4) ThrowException("Entry point is not word-aligned");

						entry_defined = true;
						entry_point = *current_address + memory_start;
					}
					else ThrowException("Preprocessor directive not recognised");
				}
				else if (token == "@") {
					func_scopes.back().push_back({});
					OpReplace& new_op = func_scopes.back().back();

					if (!TryConsumeFront(line_tokens, new_op.op)) ThrowException("Function name not provided");
					if (!IsUpper(new_op.op)) ThrowException("Function name must be uppercase");

					if (TryGetFront(line_tokens, token) && token == "(") {
						line_tokens.pop_front();
						string num;
						if (!TryConsumeFront(line_tokens, num) || num == ")") ThrowException("Required args not provided");
						if (!IsNumeric(num)) ThrowException("Required args is not numeric");
						new_op.requiredArgs = stoi(num);
						if (!TryConsumeFront(line_tokens, token) || token != ")") ThrowException("Brackets not closed");
					}
					if (new_op.op.front() == 'N') ThrowException("Function name cannot start with N");
					if (new_op.op.back() == 'S') ThrowException("Function name cannot end with S");

					if (!TryConsumeFront(line_tokens, new_op.newop)) ThrowException("Instruction to assign function not provided");
					if (!IsUpper(new_op.newop)) ThrowException("Instruction name must be uppercase");

					if (new_op.newop == "N") ThrowException("Function replacement instruction is only N flag");
					if (new_op.newop == "S") ThrowException("Function replacement instruction is only S flag");
					if (new_op.newop == "NS") ThrowException("Function replacement instruction is only flags");

					if (new_op.newop.front() == 'N') {
						new_op.newop = new_op.newop.substr(1);
						new_op.newN = true;
					}

					if (new_op.newop.back() == 'S') {
						new_op.newop = new_op.newop.substr(0, new_op.newop.length() - 1);
						new_op.newS = true;
					}

					new_op.has_cond = GetCond(line_tokens, new_op.new_cond);

					if (new_op.has_cond) {
						if (cond_scopes.back().has_cond) ThrowException("Function overwrites the scope condition");
					}
					else {
						new_op.new_cond = cond_scopes.back().cond;
						new_op.has_cond = cond_scopes.back().has_cond;
					}

					new_op.has_shift = GetShift(line_tokens, new_op.new_shift);

					while (TryConsumeFront(line_tokens, token)) {
						if (token == "...") {
							if (new_op.requiredArgs != -1) ThrowException("Cannot use '...' in constant length function");
							new_op.tokens.push_back(token);
						}
						else if (token == "@") {
							if (new_op.requiredArgs == -1) ThrowException("Cannot use '@' in variable length function");
							new_op.tokens.push_back(token);
							if (!TryConsumeFront(line_tokens, token) || !IsNumeric(token)) ThrowException("'@' must be followed by a number");
							if (stoi(token) >= new_op.requiredArgs) ThrowException("Arg number greater than number of args");
							new_op.tokens.push_back(token);
						}
						else {
							new_op.tokens.push_back(token);
						}
					}

					total_ops_defined++;
					break;
				}
				else if (token == "@{") {
					op_depth++;
					func_scopes.push_back({});
				}
				else if (token == "}@") {
					if (op_depth == 0) ThrowException("Too many closing function scopes");

					op_depth--;
					func_scopes.pop_back();
				}
				else if (token == "${") {
					variable_depth++;
					variable_scopes.push_back({});
				}
				else if (token == "}$") {
					if (variable_depth == 0) ThrowException("Too many closing variable scopes");

					variable_depth--;
					variable_scopes.pop_back();
				}
				else if (token == "?{") {
					cond_depth++;
					cond_scopes.push_back({});
					// ThrowException("No condition was provided for the condition scope");
					if (line_tokens.empty()) continue;
					//ThrowException("Invalid condition was provided for the condition scope");
					if (!IsCond(line_tokens.front())) continue;

					cond_scopes.back().has_cond = true;
					cond_scopes.back().cond = condNames.at(line_tokens.front());
					line_tokens.pop_front();
				}
				else if (token == "}?") {
					if (cond_depth == 0) ThrowException("Too many closing condition scopes");

					cond_depth--;
					cond_scopes.pop_back();
				}
				else if (token == ":{") {
					label_depth++;
					label_scopes.push_back({});
					pendingLabels.push_back({});
				}
				else if (token == "}:") {
					if (label_depth == 0) ThrowException("Too many closing label scopes");
					label_depth--;

					if (pendingLabels.back().size() > 0) {
						std::list<string*>& lower_list = *(pendingLabels.rbegin()++); // Get second last list
						lower_list.splice(lower_list.end(), pendingLabels.back());  // Move labels down to second last list
					}
					pendingLabels.pop_back();
					label_scopes.pop_back();
				}
				else if (token == "$") {
					string var_name;
					if (!TryConsumeFront(line_tokens, var_name)) ThrowException("Variable name not provided");
					if (!IsChars(var_name, valid_text_chars)) ThrowException("Invalid characters in variable name");

					if (!variable_scopes.back().contains(var_name)) {
						total_variables_defined++;
					}

					if (line_tokens.empty()) ThrowException("Value to assign variable not provided");

					variable_scopes.back()[var_name] = line_tokens;
					line_tokens.clear();

					/*while (TryConsumeFront(line_tokens, token)) {
						variable_scopes.back()[var_name].push_back(token);
					*/
					break;
				}
				else { // This is an instruction
					if ((*current_address) & 3) ThrowException("Instruction not aligned");
					if (!IsUpper(token)) ThrowException("Invalid characters in instruction code");

					assemblyLines.push_back({
						_current_line,
						memory_start + *current_address,
						memory + ((*current_address) >> 2),
						token
					});
					AssemblyLine& instruction = assemblyLines.back();
					if (instruction.code.length() == 0) ThrowException("Instruction name is empty");

					if (instruction.code == "N") ThrowException("Instruction is only N flag");
					if (instruction.code == "S") ThrowException("Instruction is only S flag");
					if (instruction.code == "NS") ThrowException("Instruction is only flags");

					if (instruction.code.front() == 'N') {
						instruction.code = instruction.code.substr(1);
						instruction.N = true;
					}

					if (instruction.code.back() == 'S') {
						instruction.code = instruction.code.substr(0, instruction.code.length() - 1);
						instruction.S = true;
					}

					instruction.has_cond = GetCond(line_tokens, instruction.cond);
					instruction.has_shift = GetShift(line_tokens, instruction.shift);

					total_variables_replaced += ResolveVariables(line_tokens);
					SplitSquareBrackets(line_tokens);
					ResolveRegLists(line_tokens);

					if (!line_tokens.empty()) {
						instruction.args.push_back({});
						list<string>* tokens = &instruction.args.back();
						do {
							if (line_tokens.front() == ",") {
								line_tokens.pop_front(); // Erase ','

								if (tokens->empty() || line_tokens.empty()) ThrowException("Empty instruction parameter");

								instruction.args.push_back({});
								tokens = &instruction.args.back();
							}
							else {
								tokens->push_back(line_tokens.front());
								line_tokens.pop_front();
							}
						} while (!line_tokens.empty());
					}


					OpReplace* op = nullptr;

					unordered_set<OpReplace*> used_replacements = {};

					// Replace instruction with preprocessor instruction
					do {
						op = nullptr;
						for (auto scope_it = func_scopes.rbegin(); scope_it != func_scopes.rend(); scope_it++) {
							for (auto& op_it : *scope_it) {
								if (op_it.op != instruction.code) continue;
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

						if (used_replacements.contains(op)) ThrowException("This function is recursive");
						instruction.code = op->newop; // Replace instruction name
						used_replacements.insert(op);

						if (op->has_cond) { // Replace condition
							if (instruction.has_cond) ThrowException("Function overwrites condition");
							instruction.cond = op->new_cond;
							instruction.has_cond = true;
						}

						if (op->has_shift) {
							if (instruction.has_shift) ThrowException("Function overwrites shift");
							instruction.shift = op->new_shift;
							instruction.has_shift = true;
						}

						vector<list<string>> old_args = {};
						old_args.insert(old_args.begin(), instruction.args.begin(), instruction.args.end());

						instruction.args = {};

						list<string> op_tokens = op->tokens;
						total_variables_replaced += ResolveVariables(op_tokens);
						SplitSquareBrackets(op_tokens);
						ResolveRegLists(op_tokens);

						if (op_tokens.empty()) continue;

						instruction.args.push_back({});
						list<string>* tokens = &instruction.args.back();
						do {
							if (op_tokens.front() == ",") {
								op_tokens.pop_front(); // Erase ','

								if (tokens->empty() || op_tokens.empty()) ThrowException("Function parameter resolved as empty");

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
									if (token != "@") {
										b_it++;
										continue;
									}

									b_it = a.erase(b_it);  // Erase '@'
									int arg_num = stoi(*b_it); // String was verified as numeric earlier
									b_it = a.erase(b_it);  // Erase number
									a.insert(b_it, old_args[arg_num].begin(), old_args[arg_num].end()); // Number was verified as in range earlier
								}
							}
						}
						else if (op->requiredArgs == -1) {
							auto a_it = instruction.args.begin();
							while (a_it != instruction.args.end()) {
								list<string> arg = {};
								arg.splice(arg.begin(), *a_it);
								auto b_it = arg.begin();
								while (TryConsume(arg, b_it, token)) {
									if (token != "...") {
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
								if (a_it->empty()) ThrowException("Function parameter resolved as empty");
								a_it++;
							}
						}
					} while (op != nullptr);

					if (instruction.has_cond) {
						if (cond_scopes.back().has_cond) ThrowException("Instruction overwrites the scope condition");
					}
					else {
						instruction.cond = cond_scopes.back().cond;
						instruction.has_cond = cond_scopes.back().has_cond;
					}

					for (auto& a : instruction.args) {
						if (a.empty()) ThrowException("Argument cannot be empty");

						if (a.front() == "<<" || a.front() == ">>") ThrowException("Expected '" + a.front() + "' to follow a value");
						if (a.back() == "<<" || a.back() == ">>" || a.back() == "-" || a.back() == ".") ThrowException("Expected '" + a.front() + "' to follow a value");

						for (auto& t : a) {
							if (t == "<<" || t == ">>" || t == "-" || t == "." || IsNumeric(t) || IsReg(t)) continue;
							if (!IsChars(t, valid_text_chars)) ThrowException("Unexpected token '" + t + "' in argument");

							pendingLabels.back().push_back(&t);

							for (auto it = label_scopes.rbegin(); it != label_scopes.rend(); it++) {
								if (!it->contains(t)) continue;

								t = to_string((*it)[t]);
								pendingLabels.back().pop_back();
								break;
							}
						}
					}

					*current_address += 4;
					program_end = *current_address + memory_start;
					break;
				}
			}

			if (!line_tokens.empty()) ThrowException("Unexpeced token/s in line");
		}

		data_end = *current_address + memory_start;
		if (data_start == program_start) data_start = program_end;

		if (label_depth != 0)
		{
			FlushScopes();
			ThrowException("Did not close every label scope");
		}
		if (cond_depth != 0)
		{
			FlushScopes();
			ThrowException("Did not close every condition scope");
		}
		if (op_depth != 0)
		{
			FlushScopes();
			ThrowException("Did not close every function scope");
		}

		if (!pendingLabels.front().empty()) {
			string err = "Could not resolve all labels (";
			unordered_set<string> labels;
			for (auto& label : pendingLabels.front()) {
				labels.insert(*label);
			}
			auto it = labels.begin();
			while (true) {
				err += *(it++);
				if (it == labels.end()) break;
				err += ", ";
			}
			err += ")";
			ThrowException(err);
		}

		if (print_intermediate) {
			printf("Intermediate output:\n");
			for (auto& l : assemblyLines) {
				printf("0x%08X: %s%s%s ", l.addr, l.N ? "N" : "", l.code.c_str(), l.S ? "S" : "");
				for (auto& a : l.args) {
					for (auto& t : a) {
						printf("%s", t.c_str());
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
			if (!definitions.contains(l.code)) ThrowException("Unknown instruction '" + l.code + "'");

			InstructionDefinition def = definitions.at(l.code);

			if (!def.allow_shift && l.has_shift) ThrowException("Cannot set shift for " + l.code);
			if (!def.allow_N && l.N) ThrowException("Cannot set N flag for " + l.code);
			if (!def.allow_S && l.S) ThrowException("Cannot set S flag for " + l.code);
			if (l.args.size() != pack_args.at(def.packing)) ThrowException("Expected " + to_string(pack_args.at(def.packing)) + " argument/s for " + l.code);

			*(l.mem) = (l.cond << 28) | (l.N << 27) | def.code | (l.S << 21);

			auto arg = l.args.begin();

			switch (def.packing) {
			case None:
				if (l.has_shift) ThrowException("Cannot use shift for " + l.code);
				break;

			case Reg3:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument");
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument");
				*(l.mem) |= ToReg(arg->front()) << 12;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in third argument");
				*(l.mem) |= ToReg(arg->front()) << 8;
				break;

			case Flex3i:
				if (next(arg)->front() == "-") {
					next(arg)->pop_front();
					*(l.mem) ^= (1 << 22) | (1 << 27); // Switch to negative alternate, and set N flag
				}

				if (l.args.back().front() == "-") {
					l.args.back().pop_front();
					*(l.mem) ^= (1 << 22);
				}

			case Flex3:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument");
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument");
				*(l.mem) |= ToReg(arg->front()) << 12;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in third argument");

				if (IsNumeric(arg->front())) {
					word val = rotl(stoul(arg->front()), l.shift);
					word min_val = val;
					word shift = 0;

					for (int i = 1; i < 16; i++) {
						if (rotr(val, i * 2) >= min_val) continue;
						min_val = rotr(val, i * 2);
						shift = i;
					}

					if (min_val > 0xFF) ThrowException("Immediate value is too large");
					*(l.mem) |= 1 << 20; // set 'i'
					*(l.mem) |= min_val << 4;
					*(l.mem) |= shift;
				}
				else
				{
					if (l.shift & 1) ThrowException("Register shifts must be even");
					*(l.mem) |= ToReg(arg->front()) << 8;
					*(l.mem) |= l.shift >> 1;
				}
				break;

			case Flex2i:
				if (l.args.back().front() == "-") {
					l.args.back().pop_front();
					*(l.mem) ^= (1 << 22);
				}

			case Flex2:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument");
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument");

				if (IsNumeric(arg->front())) {
					word val = rotl(stoul(arg->front()), l.shift);
					word min_val = val;
					word shift = 0;

					for (int i = 1; i < 16; i++) {
						if (rotr(val, i * 2) >= min_val) continue;
						min_val = rotr(val, i * 2);
						shift = i;
					}

					if (min_val > 0xFFF) ThrowException("Immediate value is too large");
					*(l.mem) |= 1 << 20; // set 'i'
					*(l.mem) |= min_val << 4;
					*(l.mem) |= shift;
				}
				else
				{
					if (l.shift & 1) ThrowException("Register shifts must be even");
					*(l.mem) |= ToReg(arg->front()) << 12;
					*(l.mem) |= l.shift >> 1;
				}
				break;

			case Reg2:
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument");
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument");
				*(l.mem) |= ToReg(arg->front()) << 12;
				break;

			case RegList:
				if (l.has_shift) ThrowException("Cannot use shift for " + l.code);
				if (arg->size() != 1) ThrowException("Unexpected token/s in first argument");
				*(l.mem) |= ToReg(arg->front()) << 16;
				arg++;

				if (arg->size() != 1) ThrowException("Unexpected token/s in second argument");
			{
				word list = stoul(arg->front());
				if (list > 0xFFFF) ThrowException("'" + arg->front() + "' is not a register set");
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