#include "L32_String.h"

#include "L32_Computer.h"
#include "L32_ICore.h"

#include <iostream>
#include <vector>

namespace Little32
{
	void DisassembleMemory(Computer& computer, word start, word end, word offset, bool print_NOP)
	{
		for (word laddr = start, addr = start; addr < end; addr += 4)
		{
			word v = computer.Read(addr + offset);
			std::string instruction = computer.core->Disassemble(v);
			if (instruction == "NOP") continue;
			if (addr - laddr > 12) printf("...\n");

			else if(print_NOP)
			{
				if (addr - laddr > 4) printf("0x%08X: NOP\n", laddr + 4 + offset);
				if (addr - laddr > 8) printf("0x%08X: NOP\n", laddr + 8 + offset);
			}
			else if (addr - laddr > 4)
			{
				printf("...\n");
			}

			printf("0x%08X: %s\n", addr + offset, instruction.c_str());
			laddr = addr;
		}
	}

	void PrintMemory(Computer& computer, word start, word end, word offset, bool print_null)
	{
		word laddr = start;
		word end2 = ((end - start) & 0xFFFFFFFC) + start; // just in case
		if (!print_null)
		{
			for (; laddr != end2; laddr += 4)
			{
				if (computer.Read(laddr + offset) != 0) break;
			}
			if (laddr == end2) return;
		}

		printf("                            .   ,   .   ,   .   ,   .   , unsigned32    signed32 CP437  ASCII\n");

		const std::string CodePage437[256] =
		{
			// " ","☺","☻","♥","♦","♣","♠","•","◘","○","◙","♂","♀","♪","♫","☼",
			// "►","◄","↕","‼","¶","§","▬","↨","↑","↓","→","←","∟","↔","▲","▼",
			// " ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/",
			// "0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
			// "@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
			// "P","Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
			// "`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o",
			// "p","q","r","s","t","u","v","w","x","y","z","{","|","}","~","⌂",
			// "Ç","ü","é","â","ä","à","å","ç","ê","ë","è","ï","î","ì","Ä","Å",
			// "É","æ","Æ","ô","ö","ò","û","ù","ÿ","Ö","Ü","¢","£","¥","₧","ƒ",
			// "á","í","ó","ú","ñ","Ñ","ª","º","¿","⌐","¬","½","¼","¡","«","»",
			// "░","▒","▓","│","┤","╡","╢","╖","╕","╣","║","╗","╝","╜","╛","┐",
			// "└","┴","┬","├","─","┼","╞","╟","╚","╔","╩","╦","╠","═","╬","╧",
			// "╨","╤","╥","╙","╘","╒","╓","╫","╪","┘","┌","█","▄","▌","▐","▀",
			// "α","ß","Γ","π","Σ","σ","µ","τ","Φ","Θ","Ω","δ","∞","φ","ε","∩",
			// "≡","±","≥","≤","⌠","⌡","÷","≈","°","∙","·","√","ⁿ","²","■"," "

			// This is the above translated to UTF8. Excuse this mess.
			" ",           "\xE2\x98\xBA","\xE2\x98\xBB","\xE2\x99\xA5","\xE2\x99\xA6","\xE2\x99\xA3","\xE2\x99\xA0","\xE2\x80\xA2",
			"\xE2\x97\x98","\xE2\x97\x8B","\xE2\x97\x99","\xE2\x99\x82","\xE2\x99\x80","\xE2\x99\xAA","\xE2\x99\xAB","\xE2\x98\xBC",
			"\xE2\x96\xBA","\xE2\x97\x84","\xE2\x86\x95","\xE2\x80\xBC","\xC2\xB6",    "\xC2\xA7",    "\xE2\x96\xAC","\xE2\x86\xA8",
			"\xE2\x86\x91","\xE2\x86\x93","\xE2\x86\x92","\xE2\x86\x90","\xE2\x88\x9F","\xE2\x86\x94","\xE2\x96\xB2","\xE2\x96\xBC",
			" ",           "!",           "\"",          "#",           "$",           "%",           "&",           "'",
			"(",           ")",           "*",           "+",           ",",           "-",           ".",           "/",
			"0",           "1",           "2",           "3",           "4",           "5",           "6",           "7",
			"8",           "9",           ":",           ";",           "<",           "=",           ">",           "?",
			"@",           "A",           "B",           "C",           "D",           "E",           "F",           "G",
			"H",           "I",           "J",           "K",           "L",           "M",           "N",           "O",
			"P",           "Q",           "R",           "S",           "T",           "U",           "V",           "W",
			"X",           "Y",           "Z",           "[",           "\\",          "]",           "^",           "_",
			"`",           "a",           "b",           "c",           "d",           "e",           "f",           "g",
			"h",           "i",           "j",           "k",           "l",           "m",           "n",           "o",
			"p",           "q",           "r",           "s",           "t",           "u",           "v",           "w",
			"x",           "y",           "z",           "{",           "|",           "}",           "~",           "\xE2\x8C\x82",
			"\xC3\x87",    "\xC3\xBC",    "\xC3\xA9",    "\xC3\xA2",    "\xC3\xA4",    "\xC3\xA0",    "\xC3\xA5",    "\xC3\xA7",
			"\xC3\xAA",    "\xC3\xAB",    "\xC3\xA8",    "\xC3\xAF",    "\xC3\xAE",    "\xC3\xAC",    "\xC3\x84",    "\xC3\x85",
			"\xC3\x89",    "\xC3\xA6",    "\xC3\x86",    "\xC3\xB4",    "\xC3\xB6",    "\xC3\xB2",    "\xC3\xBB",    "\xC3\xB9",
			"\xC3\xBF",    "\xC3\x96",    "\xC3\x9C",    "\xC2\xA2",    "\xC2\xA3",    "\xC2\xA5",    "\xE2\x82\xA7","\xC6\x92",
			"\xC3\xA1",    "\xC3\xAD",    "\xC3\xB3",    "\xC3\xBA",    "\xC3\xB1",    "\xC3\x91",    "\xC2\xAA",    "\xC2\xBA",
			"\xC2\xBF",    "\xE2\x8C\x90","\xC2\xAC",    "\xC2\xBD",    "\xC2\xBC",    "\xC2\xA1",    "\xC2\xAB",    "\xC2\xBB",
			"\xE2\x96\x91","\xE2\x96\x92","\xE2\x96\x93","\xE2\x94\x82","\xE2\x94\xA4","\xE2\x95\xA1","\xE2\x95\xA2","\xE2\x95\x96",
			"\xE2\x95\x95","\xE2\x95\xA3","\xE2\x95\x91","\xE2\x95\x97","\xE2\x95\x9D","\xE2\x95\x9C","\xE2\x95\x9B","\xE2\x94\x90",
			"\xE2\x94\x94","\xE2\x94\xB4","\xE2\x94\xAC","\xE2\x94\x9C","\xE2\x94\x80","\xE2\x94\xBC","\xE2\x95\x9E","\xE2\x95\x9F",
			"\xE2\x95\x9A","\xE2\x95\x94","\xE2\x95\xA9","\xE2\x95\xA6","\xE2\x95\xA0","\xE2\x95\x90","\xE2\x95\xAC","\xE2\x95\xA7",
			"\xE2\x95\xA8","\xE2\x95\xA4","\xE2\x95\xA5","\xE2\x95\x99","\xE2\x95\x98","\xE2\x95\x92","\xE2\x95\x93","\xE2\x95\xAB",
			"\xE2\x95\xAA","\xE2\x94\x98","\xE2\x94\x8C","\xE2\x96\x88","\xE2\x96\x84","\xE2\x96\x8C","\xE2\x96\x90","\xE2\x96\x80",
			"\xCE\xB1",    "\xC3\x9F",    "\xCE\x93",    "\xCF\x80",    "\xCE\xA3",    "\xCF\x83",    "\xC2\xB5",    "\xCF\x84",
			"\xCE\xA6",    "\xCE\x98",    "\xCE\xA9",    "\xCE\xB4",    "\xE2\x88\x9E","\xCF\x86",    "\xCE\xB5",    "\xE2\x88\xA9",
			"\xE2\x89\xA1","\xC2\xB1",    "\xE2\x89\xA5","\xE2\x89\xA4","\xE2\x8C\xA0","\xE2\x8C\xA1","\xC3\xB7",    "\xE2\x89\x88",
			"\xC2\xB0",    "\xE2\x88\x99","\xC2\xB7",    "\xE2\x88\x9A","\xE2\x81\xBF","\xC2\xB2",    "\xE2\x96\xA0"," "
		};

		for (word addr = laddr; addr < end; addr += 4)
		{
			const word v = computer.Read(addr + offset);
			if (v == 0 && !print_null) continue;
			std::string cp437 = "";
			std::string ascii = "";
			word v2 = v;
			for (size_t i = 0; i < 4; i++)
			{
				const uint8_t c = v2 & 0xFF;
				v2 >>= 8;

				cp437 += CodePage437[c];

				if (c >= ' ' && c <= '~')
				{
					if (c == '\\' || c == '"') ascii += '\\';
					ascii += c;
					continue;
				}

				ascii += '\\';

				switch (c)
				{
				case '\a': ascii += 'a'; break;
				case '\b': ascii += 'b'; break;
				case '\f': ascii += 'f'; break;
				case '\n': ascii += 'n'; break;
				case '\r': ascii += 'r'; break;
				case '\t': ascii += 't'; break;
				case '\v': ascii += 'v'; break;
				default:
					ascii += 'x';
					ascii += "0123456789ABCDEF"[(c >> 4) & 0b1111];
					ascii += "0123456789ABCDEF"[(c >> 0) & 0b1111];
					break;
				}
			}  
			printf("0x%08X: 0x%08X 0b%s % 10u %+ 11i \"", addr + offset, v, ToBinary(v).c_str(), v, static_cast<int32_t>(v));
			std::cout << cp437 << "\" \"" << ascii << '\"' << std::endl;
			laddr = addr;
		}
	}
}