#include <unordered_set>
#include <sstream>
#include <cstdint>

#include "Assembler.h"
#include "SR_String.h"
#include "RAM.h"
#include "ROM.h"

namespace SimpleRISC {
	const char Assembler::valid_text_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

	Assembler::FormatException::FormatException(size_t line_no, std::string line, const char* const message) :
		line_no(line_no),
		line(line),
		inner_message(message),
		exception("Improper format") {
		this->message = std::string("Improper format: ") + inner_message + " (line: " + std::to_string(this->line_no) + ")";
	}

	void Assembler::ThrowException(const char* const msg) const { throw FormatException(_current_line.line_no, *_current_line.line, msg); }

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

	constexpr bool IsChar(const std::string& str, const char c, size_t off = 0) noexcept {
		return str.find_first_not_of(c,off) == std::string::npos;
	}

	constexpr bool IsChars(const std::string& str, const char* const chars,size_t off = 0) noexcept {
		return chars != nullptr && str.find_first_not_of(chars, off) == std::string::npos;
	}

	constexpr bool IsNumeric(const std::string& str) noexcept {
		for (auto c : str) {
			if (c < '0' || c > '9') return false;
		}

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

	uint64_t Assembler::xToI(const RawLine& rline, std::string str, word base, uint64_t max) {
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

	constexpr bool IsCond(const std::string& str) noexcept {
		for (int i = 0; i < 24; i++) {
			if (str == condNames[i]) return true;
		}

		return false;
	}

	constexpr void RemoveComments(std::string& str) noexcept {
		size_t i = str.find("//", 0, 2);
		if (i == str.npos) return;
		str.erase(i);
	}

	void PutByte(word* memory, word address, byte value) {
		word x = ((sizeof(word) - 1 - (address % sizeof(word))) * 8);

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
		_GetTokens(".",1);
		_GetTokens("+",1);
		_GetTokens("-",1);
		_GetTokens("#",1);
		//_GetTokens("="  );

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

	std::string Assembler::GetCond(token_list& l) const {
		using namespace std;

		auto it = std::find(l.begin(), l.end(), "?");

		if (it == l.end()) return "";

		it = l.erase(it); // Erase '?'
		string cond;
		if (!TryConsume(l, it, cond)) ThrowException("No condition provided for '?' statement");
		if (!IsCond(cond)) ThrowException("Token is not condition in '?' statement");
		if (it != l.end()) ThrowException("Line does not end after '?' statement");
		return cond;
	}

	size_t Assembler::ConvertNumbers(token_list& tokens) const {
		size_t numbers_replaced = 0;

		for (auto& t : tokens) {
			int numBase = IsNumber(t);
			if (!numBase) continue;
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
				it++;
				while (TryGet(l, it, token) && token != "]") {
					if (token == "[") ThrowException("Square brackets cannot be nested");
					if (token == ",") ThrowException("Square brackets cannot contain commas");
					if (token == "+" || token == "-") break;
					it++;
				}

				if (it == l.end()) ThrowException("Square brackets must be closed");

				if (token == "]") { // Occurs when no signs were encountered - Interpreted as single value relative to PC
					l.insert(start, "PC");
					l.insert(start, ",");
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
	}

	void Assembler::AddLabels(const std::unordered_map<std::string, word>& addresses) {
		for (auto& address : addresses) {
			constant_addresses[address.first] = address.second;
		}
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

	void Assembler::Assemble(const char* code, bool print_intermediate) {
		using namespace std;

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
		} else throw exception("Destination for assembly has not been set");

		program_start = *current_address + memory_start;
		program_end = program_start;
		data_start = program_start;
		data_end = program_start;

		if (code == nullptr) return;

		list<string> raw_lines;
		list<AssemblyLine> assemblyLines;

		list<string> cond_scopes{ "" };
		size_t cond_depth = 0;
		size_t variable_depth = 0;
		size_t op_depth = 0;

		list<list<string*>> pendingLabels = { {} };
		label_scopes.push_back(constant_addresses);
		size_t label_depth = 0;

		stringstream ss(code);
		string line;
		string token;
		bool byte_mode = false;

		_current_line = { 0, nullptr };

		while (getline(ss, line)) {
			raw_lines.push_back(line);
			RemoveComments(line);
			_current_line.line = &line;
			string line_part;
			stringstream ss2(line);
			while (getline(ss2, line_part, ';')) {
				token_list line_tokens = {};
				GetTokens(line_part, line_tokens);
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
							word err = (*current_address) % width;
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

						new_op.newcond = GetCond(line_tokens);

						if (cond_scopes.back() != "") {
							if (new_op.newcond != "") ThrowException("Function overwrites the scope condition");
							new_op.newcond = cond_scopes.back();
						}

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
						cond_scopes.push_back("");
						// ThrowException("No condition was provided for the condition scope");
						if (line_tokens.empty()) continue;
						//ThrowException("Invalid condition was provided for the condition scope");
						if (!IsCond(line_tokens.front())) continue;

						cond_scopes.back() = line_tokens.front();
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

						assemblyLines.push_back({ _current_line, *current_address + memory_start, token });
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

						instruction.cond = GetCond(line_tokens);

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

						string original_cond = instruction.cond;

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

							if (op->newcond != "") { // Replace condition
								if (original_cond != "") ThrowException("Function overwrites instruction condition");
								if (instruction.cond != "") ThrowException("Function overwrites function condition");
								instruction.cond = op->newcond;
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
								auto a_it = instruction.args.begin();
								while (a_it != instruction.args.end()) {
									auto b_it = a_it->begin();
									while (TryGet(*a_it, b_it, token)) {
										if (token != "@") {
											b_it++;
											continue;
										}

										b_it = a_it->erase(b_it);  // Erase '@'
										int arg_num = stoi(*b_it); // String was verified as numeric earlier
										b_it = a_it->erase(b_it);  // Erase number
										a_it->insert(b_it, old_args[arg_num].begin(), old_args[arg_num].end()); // Number was verified as in range earlier
									}
									a_it++;
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

						if (cond_scopes.back() != "") {
							if (instruction.cond != "") ThrowException("Instruction overwrites the scope condition");
							instruction.cond = cond_scopes.back();
						}

						for (auto& a : instruction.args) {
							for (auto& t : a) {
								if (t == "<<" || t == ">>" || t == "-" || t == "." || IsNumeric(t) || IsReg(t)) continue;
								if (!IsChars(t, valid_text_chars)) ThrowException("Unexpected token in instruction");
								bool label_found = false;
								for (auto it = label_scopes.rbegin(); it != label_scopes.rend(); it++) {
									if (!it->contains(t)) continue;
									t = to_string((*it)[t]);
									label_found = true;
									break;
								}
								if (!label_found) pendingLabels.back().push_back(&t);
							}

							/*if (*t == "<<" || *t == ">>") ThrowException("Shifts must follow a constant or register");

							if (IsReg(*t)) {
								t++;
								if (t == a.end()) continue;
								if (*t != "<<" && *t != ">>") ThrowException("Unexpected token after register");
								t++;
								if (t == a.end() || !IsNumeric(*t)) ThrowException("Expected number after shift");
								if (stoul(*t) >= 32) ThrowException("Shift is too large");
								if (t != a.end()) ThrowException("Unexpected token after register shift");
								continue;
							}

							while (t != a.end()) {
								if (*t == "-" ||
									*t == "." ||
									isReg) {
									t++;
									continue;
								}
								if (IsNumeric(*t)) {
									auto _t = next(t);
									if (_t == a.end()) break;
									word val = xToI(rline, *t, numBase, 0x100000000);
									if (*_t == "<<") {
										_t = a.erase(_t);
										if (_t == a.end() || !IsNumeric(*_t)) ThrowException("Shift must be followed by number");
										word sh = stoul(token);
										if (sh > 32) ThrowException("Shift is too large");
										_t = a.erase(_t);
										val = rotl(val, sh);
										a.erase(_t);
									}
									else if (*_t == ">>") {
										_t = a.erase(_t);
										if (_t == a.end()) break;
										a.erase(_t);
									}
									t++;
								}
							}*/
						}

						*current_address += 4;
						program_end = *current_address + memory_start;
						break;
					}
				}

				if (!line_tokens.empty()) ThrowException("Unexpeced token/s in line");
			}
			_current_line.line_no++;
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

		if (!pendingLabels.front().empty()) ThrowException("Could not resolve all labels");

		for (auto& l : assemblyLines) {
			for (auto& a : l.args) {
				auto it = a.begin();
				while (it != a.end()) {
					if (*it == ".") {
						it = a.erase(it);
						if (it == a.end() || !IsNumeric(*it)) ThrowException("Expected number after '.'");

						long int addr = stoul(*it);
						addr -= l.addr;
						if (addr < 0) {
							addr *= -1;
							a.insert(it, "-");
						}
						*it = to_string(addr);
						it++;
					}
					else {
						it++;
					}
				}
			}
		}

		if (print_intermediate) {
			for (auto& l : assemblyLines) {
				printf("0x%08X: %s%s%s ", l.addr, l.N ? "N" : "", l.code.c_str(), l.S ? "S" : "");
				for (auto& a : l.args) {
					for (auto& t : a) {
						printf("%s", t.c_str());
					}
					if (&a != &(l.args.back())) printf(", ");
				}
				if (l.cond != "") printf(" ?%s", l.cond.c_str());
				printf("\n");
			}

			printf("\n\n");
		}
	}
}