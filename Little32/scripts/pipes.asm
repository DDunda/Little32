$char_mem = R4
$x        = R5
$y        = R6
$dir      = R7
$rand     = R8
$delay    = R9

$frame_delay = 50
$clock_count = 19

MOV $char_mem, CHAR_MEM
MOV $rand, seed
MOV $delay, 0

MOV $x, 8
MOV $y, 8

MOV R1, $rand
BL .random_xs32
AND $dir, R0, 0xF // Sets the direction to one of four directions, for both from and to
                  // 0: right
                  // 1: left
                  // 2: down
                  // 3: up
                  // 0bXX__ - From
                  // 0b__XX - To
				  // 0bX_X_ - Vertical
				  // 0b_X_X - Negative

MOV R0, frame_int
MOV R1, RENDER_INT
RWW R0, [R1]
HALT

frame_int:
	SUBS $delay, 1
	RFE ?GE

	MOV $delay, 1
	
	PUSH {R0-R4,LR}

	AND $dir, $dir, 0b0011  // dir &= 0b0011;

	MOV R1, $rand
	BL .random_xs32
	AND R0, R0, 0b0011      // new_dir = rand_x32(seed) & 0b0011;

	XOR R1, R0, $dir
	CMP R1, 1
	MOV R0, $dir ?EQ        // if (dir ^ new_dir == 1) new_dir = dir;
	ORR $dir, R0, $dir ROTL 2 // dir = (dir << 2) | new_dir;

	MOV R1, pipe_graphics
	RRB R1, [R1+$dir]

	ORR R2, $x, $y ROTL 4
	RWB R1, [$char_mem+R2]  // char_mem[(y << 4) | x] = pipe_graphics[dir];

	TST R0, 1
	MOV R1, -1 ?ZC
	MOV R1,  1 ?ZS          // i = new_dir & 2 ? -1 : 1;

	TST R0, 2
	SWP $x, $y ?ZC          // if (new_dir & 1) swap(x,y);
	ADD $x, $x, R1          // x += i;
	AND $x, $x, 15          // x &= 15;
	SWP $x, $y ?ZC          // if (new_dir & 1) swap(x,y);

	POP {R0-R4,LR}
	RFE

random_xs32:
${:{
	$x     = R0
	$state = R1 // Address of seed
	$tmp   = R2

	RRW $x, [$state]

	LSL $tmp, $x, 13
	XOR $x, $x, $tmp // x ^= x << 13;

	LSR $tmp, $x, 17
	XOR $x, $x, $tmp // x ^= x >> 17;

	LSL $tmp, $x, 5
	XOR $x, $x, $tmp // x ^= (x << 5);

	RWW $x, [$state]

	RET
}:}$


// Multiply A in R1 and B in R2 into R0
multiply:
${:{
	$ret = R0
	$a   = R1
	$b   = R2

	MOV $ret, 0         // word ret = 0;
multiply_loop:          // while(1) {
	CMP $a, 0
	CMP $b, 0 ?ZC
	RET ?ZS             //     if (a == 0 || b == 0) return ret;

	TST $a, 1

	ADD $ret, $b ?ZC    //     if(a & 1) ret += b;

	LSR $a, $a, 1       //     a >>= 1;
	LSL $b, $b, 1       //     b <<= 1;

	B .multiply_loop    // }
}:}$


#DATA
#WORD
seed:
	#RANDOM 4
	
#ASCII
pipe_graphics:
	//    \ To 
	// From   0 1 2 3
	//      0 ═ ═ ╗ ╝
	//      1 ═ ═ ╔ ╚
	//      2 ╚ ╝ ║ ║
	//      3 ╔ ╗ ║ ║
	      "\xCDCDBBBC"
		  "\xCDCDC9C8"
		  "\xC8BCBABA"
		  "\xC9BBBABA"