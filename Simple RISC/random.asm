$colr_mem R4
$rand R8
$delay R9

$frame_delay 10
$clock_count 903

MOV $colr_mem, COLOUR_MEM
MOV $rand, seed
MOV $delay, 0

MOV R0, frame_int
MOV R1, RENDER_INT
RWW R0, [R1]
HALT

frame_int:
	SUBS $delay, 1
	RFE ?GE

	MOV $delay, 5

	PUSH {R0,R5,LR}
	MOV R5, 252

colour_loop:
	MOV R1, $rand
	BL .random_xs32

	RWW R0, [$colr_mem+R5]
	SUBS R5, 4
	BNC .colour_loop

	POP {R0,R5,LR}
	RFE

random_xs32:
${:{
	$x R0
	$state R1 // Address of seed
	$tmp R2

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

#DATA
#WORD
seed:
	#RANDOM 4