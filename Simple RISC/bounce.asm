$x R0
$y R1
$vx R2
$vy R3
$out R4
$box R5
$empty R6

MOV $x, 16
MOV $y, 70
MOV $vx, 7
MOV $vy, 5
MOV $out, CHAR_MEM
MOV $box, 0xFE          // char box = '■';
MOV $empty, 0x20        // char empty = ' ';

MOV R7, frame_int
RWW R7, [$out+256]      // set_frame_callback(frame_int);

HALT                    // while(true) {}

frame_int:              // void frame_int() {
:{
	LSR R7, $y, 4
	LSL R7, R7, 8
	ADD R7, $x
	LSR R7, R7, 4
	RWB $empty,[$out+R7]//     CHAR_MEM[(y >> 4) * 16 + (x >> 4)] = box;

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
  
	LSR R7, $y, 4
	LSL R7, R7, 8
	ADD R7, $x
	LSR R7, R7, 4
	RWB $box, [$out+R7] //     CHAR_MEM[(y >> 4) * 16 + (x >> 4)] = box;
   RFE                  //     return;
                        // }
}: