$clock_count 2611 // Enough to call frame_int a single time
$frame_delay 50 // 20 FPS

${
$x R0
$i R1
$char_mem R2
$colour_mem R3
$gradient R4
$colours R5

$tmp R6
$tmp2 R7

#ENTRY
MOV $x, 7                      // word  x = 7;
MOV $i, 0                      // word  i = 0;
MOV $char_mem, CHAR_MEM
MOV $colour_mem, COLOUR_MEM
MOV $gradient, gradient
MOV $colours, colours

MOV $tmp2, RENDER_INT
MOV $tmp, frame_int
RWW $tmp, [$tmp2]              // set_frame_callback(frame_int);
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

#DATA
#BYTE
//gradient:                      // char gradient[17] " ░▒▓█▓▒░        ";
//  0x20 0xb0 0xb1 0xb2
//  0xdb 0xb2 0xb1 0xb0
//  0x20 0x20 0x20 0x20
//  0x20 0x20 0x20 0x20
//gradient:                      // char gradient[17] " ·•■█■•·        ";
//  0x20 0xfa 0x07 0xfe
//  0xdb 0xfe 0x07 0xfa
//  0x20 0x20 0x20 0x20
//  0x20 0x20 0x20 0x20
gradient:                      // char gradient[17] "·•■·•■██            ";
  0xfa 0x07 0xfe 0xfa
  0x07 0xfe 0xdb 0xdb
  0x20 0x20 0x20 0x20
  0x20 0x20 0x20 0x20
colours:
  0x08 0x08 0x08 0x8c
  0x8c 0x8c 0x8c 0xcf
  0x00 0x00 0x00 0x00
  0x00 0x00 0x00 0x00