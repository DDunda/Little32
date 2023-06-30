$x R0
$i R1
$y R2
$g R3

MOV $x, 7               // word  x = 7;
MOV $i, 0               // word  i = 0;
MOV $y, CHAR_MEM
MOV $g, gradient
MOV R4, frame_int
RWW R4, [$y+256]        // set_frame_callback(frame_int);
HALT                    // while(true) {}

frame_int:              // void frame_int() {
	CMP $x, 0
	ADD $x, $x, 10 ?LT  //     if(x < 0) x += 10;
	RRB R4, [$g+$x]
	RWB R4, [$y+$i]     //     CHAR_MEM[i] = gradient[x];
	ADD $i, $i, 1       //     i++;
	SUB $x, $x, 1       //     x--;
	ANDS R4, $i, 15
	BNE .frame_int      //     if((i & 15) != 0) continue;
	SUB $x, $x, 3       //     x -= 3;
	CMP $i, 256
	BLT .frame_int      //     if(i < 256) continue;
	MOV $i, 0           //     i = 0;
	ADD $x, $x, 3       //     x += 3;
	RFE                 //     return;
	                    // }

#DATA
#BYTE
gradient:               // char gradient[17] " °±Û±°        ";
  0x20 0xb0 0xb1 0xb2
  0xdb 0xb2 0xb1 0xb0
  0x20 0x20 0x20 0x20
  0x20 0x20 0x20 0x20