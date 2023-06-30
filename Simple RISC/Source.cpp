#include<SDL.hpp>
#include<iostream>
#include <fstream>
#include<bit>

#include "Computer.h"
#include "Device.h"
#include "MemoryMapped.h"
#include "MappedDevice.h"

#include "RAM.h"
#include "ROM.h"
#include "ComputerInfo.h"
#include "CharDisplay.h"

#include "DebugCore.h"
#include "RISCCore.h"

#include "SR_String.h"
#include "PreProcAssember.h"
#include "Assembler.h"

using namespace SDL;
using namespace SimpleRISC;

//#define PROG3

int main(int argc, char* argv[]) {
	Init();
	IMG::Init(IMG_INIT_PNG);

	Input input;

	Point scale { 4,4 };
	Point textSize { 16,16 };
	Point charSize { 8,8 };
	int frameDelay = 50;

	Window w;
	Renderer r;
	Point& windowSize = input.windowSize = textSize * charSize * scale;

	CreateWindowAndRenderer(windowSize, w, r);

	Texture charset = IMG::LoadTexture(r, "Char set.png");

	Computer computer;
	RISCCore core(computer);
	ComputerInfo info(computer);

	RAM ram(512, 256);
	CharDisplay cram(computer, r, charset, charSize, 16, textSize, scale, ram.address_start + ram.address_size);

	computer.AddMapping(info);
	computer.AddMappedDevice(ram);
	computer.AddMappedDevice(cram);

	word start = ram.address_start>>2;
	word CHAR_MEM = cram.address_start>>2;
	word pc = 0;

#if defined PROG1a // Program 1a - Branch, MOV, ADD, status test
	word prog_start = pc;
	//        CCCCN1ccccSi   ,   .   ,   .   ,
	//        CCCCN01L   .   ,   .   ,   .   ,
	PUTWORD 0b00000011000000000000000000000011; // BL +12
	PUTWORD 0b00000100001100000000000000010000; // ADDS R0, R0, 1
	PUTWORD 0b00001010000000000000000000000010; // B -8
	PUTWORD 0b00000111100011111110000000000000; // MOV PC, LR
	word prog_end = pc;
	word mem_end = pc;

#elif defined PROG1b // Program 1b - Program 1a as instruction macros
	word prog_start = pc;
	BL(AL,3)            // BL +12
	ADDSi(AL,R0,R0,1,0) // ADDS R0, R0, 1
	B(AL,-2)            // B -8
	RET(AL)             // RET
	word prog_end = pc;
	word mem_end = pc;
#elif defined PROG2 // Program 2 - Character display test
	word prog_start = pc;
	word start_num = prog_start + 100;

	RRWi(AL, R0, PC, start_num - pc, 1) // RRW R0, [.start_num]
	MOVi(AL, R1, CHAR_MEM, 1)           // MOV R1, CHAR_MEM
	ADDi(AL, R0, R0, 3, 0)              // ADD R0, R0, 3        // do { R0 += 3;
	RWWi(AL, R0, R1, 0, 0)              // RWW R0, [R1]         //      *CHAR_MEM = R0;
	B(AL, -2)                           // B -8                 // } while(true);
	word prog_end = pc;

	pc = start_num;
	PUTWORD 0x20202020;                 // start_num: 0x20202020
	word mem_end = pc;
#elif defined PROG3 // Program 3 - Fully functional demo
	word prog_start = pc;
	word gradient = prog_start + 100;
	word frame_int = prog_start + 8;

	word x = R0;                        // $x R0
	word i = R1;                        // $i R1
	word y = R2;                        // $y R2
	word g = R3;                        // $g R3

	MOVi(AL, x, 7, 0)                   // MOV $x, 7           // word  x = 7;
	MOVi(AL, i, 0, 0)                   // MOV $i, 0           // word  i = 0;
	MOVi(AL, y, CHAR_MEM, 1)            // MOV $y, CHAR_MEM
	MOVi(AL, g,  start+gradient, 1)     // MOV $g, gradient    
	MOVi(AL, R4, start+frame_int, 1)    // MOV R4, frame_int
	RWWi(AL, R4, y, 1, 4)               // RWW R4, [$y+256]    // set_frame_callback(frame_int);
	B(AL, 0)                            // HALT                // while(true) {}

	pc = frame_int;                     // frame_int:          // void frame_int() {
	CMPi(AL, x, 0, 0)                   // CMP $x, 0
	ADDi(LT, x, x, 10, 0)               // ADDLT $x, $x, 10    //     if(x < 0) x += 10;
	RRB(AL, R4, g, x, 0)                // RRB R4, [$g+$x]     //     char c = gradient[x];
	RWB(AL, R4, y, i, 0)                // RWB R4, [$y+$i]     //     CHAR_MEM[i] = c;
	ADDi(AL, i, i, 1, 0)                // ADD $i, $i, 1       //     i++;
	SUBi(AL, x, x, 1, 0)                // SUB $x, $x, 1       //     x--;
	ANDSi(AL, R4, i, 15, 0)             // ANDS R4, $i, 15
	B(NE, (int)frame_int - (int)pc)     // BNE .frame_int      //     if((i & 15) != 0) continue;
	SUBi(AL, x, x, 3, 0)                // SUB $x, $x, 3       //     x -= 3;
	CMPi(AL, i, 1, 4)                   // CMP $i, 256
	B(LT, (int)frame_int-(int)pc)       // BLT .frame_int      //     if(i < 256) continue;
	MOVi(AL, i, 0, 0)                   // MOV $i, 0           //     i = 0;
	ADDi(AL, x, x, 3, 0)                // ADD $x, $x, 3       //     x += 3;
	RFE(AL)                             // RFE                 //     return;
	word prog_end = pc;                 //                     // }
	                                    // #DATA #BYTE
	pc = gradient;                      // gradient:           // char gradient[17] " ░▒▓█▓▒░        ";
	PUTBYTES(0x20,0xb0,0xb1,0xb2)       //   0x20 0xb0 0xb1 0xb2
	PUTBYTES(0xdb,0xb2,0xb1,0xb0)       //   0xdb 0xb2 0xb1 0xb0
	PUTBYTES(0x20,0x20,0x20,0x20)       //   0x20 0x20 0x20 0x20
	PUTBYTES(0x20,0x20,0x20,0x20)       //   0x20 0x20 0x20 0x20
	word mem_end = pc;
#else
	Assembler assembler;

	assembler.AddLabel("CHAR_MEM", CHAR_MEM << 2);
	assembler.SetRAM(ram);

	B(AL, 0); // HALT

	std::ifstream program;
	program.open("program.asm");
	if (!program.is_open()) throw std::exception("Could not open assembly file");

	try {
		assembler.Assemble(program, true);
		program.close();
	}
	catch (const Assembler::FormatException& e) {
		program.close();
		printf("%s\n%s", e.message.c_str(), e.line.c_str());
		return 1;
	}

	word prog_start = (assembler.program_start >> 2) - start;
	word prog_end = (assembler.program_end >> 2) - start;
	word mem_end = (assembler.data_end >> 2) - start;
#endif

	computer.core = &core;
	computer.start_PC = (prog_start + start) << 2;
	computer.start_SP = computer.start_PC + ram.GetRange();
	computer.SoftReset();

	// Disassemble program
	DisassembleMemory(computer, prog_start << 2, prog_end << 2, start << 2);

	// Print values in memory
	PrintMemory(computer, prog_end << 2, mem_end << 2, start << 2);

	for (int frame = 0; input.running; frame++) {
		input.Update();
		
		//printf("PC: 0x%08X  SP: 0x%08X  LR: 0x%08X\n", core.registers[PC], core.registers[SP], core.registers[LR]);
		//printf("\n\nR0: % 10i R1: % 10i R2: % 10i R3: % 10i", core.registers[R0], core.registers[R1], core.registers[R2], core.registers[R3]);
		//printf("\n(%i%i%i%i) 0x%08X: %s", core.N, core.Z, core.C, core.V, core.registers[PC], core.Disassemble(computer.Read(core.registers[PC])).c_str());
		//printf("frame\n");

		computer.Clock(3000);
		cram.Render();

		r.Present();

		Delay(frameDelay);
	}

	Quit();

	return 0;
}