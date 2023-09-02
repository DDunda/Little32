$clock_count 26 // Enough to call frame_int a single time
$frame_delay 16 // ~60 FPS

${
$x R0
$y R1
$vx R2
$vy R3
$out R4
$cout R5
$box R6
$empty R7
$col1 R8
$col2 R9
$tmp R10

#ENTRY
MOV $x, 16
MOV $y, 70
MOV $vx, 7
MOV $vy, 5
MOV $out, CHAR_MEM
MOV $cout, COLOUR_MEM
MOV $box, 0xFE          // char box = '■';
MOV $empty, 0xfa        // char empty = '·';

MOV $col1, 0x0F
MOV $col2, 0x07

MOV $tmp, frame_int
RWW $tmp, [$out+512]      // set_frame_callback(frame_int);

HALT                    // while(true) {}

frame_int:              // void frame_int() {
:{
	LSR $tmp, $y, 4
	LSL $tmp, $tmp, 8
	ADD $tmp, $x
	LSR $tmp, $tmp, 4
	RWB $empty,[$out+$tmp]//     CHAR_MEM[(y >> 4) * 16 + (x >> 4)] = empty;
	RWB $col2,[$cout+$tmp]

	ADD $x, $vx         //     x += vx;
	ADD $y, $vy         //     y += vy;

	CMP $x, 255
	BLE .x_check_else   //     if(x > 255) {
	INV $vx, $vx        //         vx = -vx;
	INV $x, $x          //         x = 510 - x;
	ADD $x, 512
	SUB $x, 2
	B .x_check_end
  x_check_else:         //     }
	CMP $x, 0
	BGE .x_check_end    //     else if(x < 0) {
	INV $vx, $vx        //         vx = -vx;
	INV $x, $x          //         x = -x;
  x_check_end:          //     }

	CMP $y, 255
	BLE .y_check_else   //     if(y > 255) {
	INV $vy, $vy        //         vy = -vy;
	INV $y, $y          //         y = 510 - y;
	ADD $y, 512
	SUB $y, 2
	B .y_check_end
  y_check_else:         //     }
	CMP $y, 0
	BGE .y_check_end    //     else if(y < 0) {
	INV $vy, $vy        //         vy = -vy;
	INV $y, $y          //         y = -y;
  y_check_end:          //     }
  
	LSR $tmp, $y, 4
	LSL $tmp, $tmp, 8
	ADD $tmp, $x
	LSR $tmp, $tmp, 4
	RWB $box, [$out+$tmp] //     CHAR_MEM[(y >> 4) * 16 + (x >> 4)] = box;
	RWB $col1,[$cout+$tmp]
   RFE                  //     return;
                        // }
}:
}$