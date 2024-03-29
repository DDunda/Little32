﻿${
$x = R0
$i = R1
$char_mem = R2
$colour_mem = R3
$gradient = R4
$colours = R5

$tmp = R6
$tmp2 = R7

#ROM
#PROGRAM
#ENTRY
MOV $x, 7                      // word  x = 7;
MOV $i, 0                      // word  i = 0;
MOV $char_mem, CHAR_MEM
MOV $colour_mem, COLOUR_MEM
MOV $gradient, gradient
MOV $colours, colours
MOV $tmp, frame_int
MOV $tmp2, RENDER_INT
RWW $tmp, [$tmp2]      // set_frame_callback(frame_int);
HALT                           // while(true) {}

frame_int:                     // void frame_int() {
	CMP $x, 0                  //     do {
	ADD $x, $x, 10 ?LT         //         if(x < 0) x += 10;
	RRB $tmp, [$gradient+$x]
	RWB $tmp, [$char_mem+$i]   //         CHAR_MEM[i] = gradient[x];
	RRB $tmp, [$colours+$x]
	RWB $tmp, [$colour_mem+$i] //         COLOUR_MEM[i] = colours[x];
	ADD $i, $i, 1              //         i++;
	SUB $x, $x, 1              //         x--;
	ANDS $tmp, $i, 15
	BNE .frame_int             //         if((i & 15) != 0) continue;
	SUB $x, $x, 3              //         x -= 3;
	CMP $i, 256
	BLT .frame_int             //     } while(i < 256);
	MOV $i, 0                  //     i = 0;
	ADD $x, $x, 3              //     x += 3;
	RFE                        //     return;
	                           // }
}$

// Still in ROM
#DATA
#BYTE
gradient:                      // char gradient[17] "·•■·•■██            ";
  0xfa 0x07 0xfe 0xfa
  0x07 0xfe 0xdb 0xdb
  0x20 0x20 0x20 0x20
  0x20 0x20 0x20 0x20
colours:                       // KDB, KDB, KDB, DBB, DBB, DBB, DBB, BW, KK, KK, KK, KK, KK, KK, KK, KK
  0x05 0x05 0x05 0x5d
  0x5d 0x5d 0x5d 0xdf
  0x00 0x00 0x00 0x00
  0x00 0x00 0x00 0x00