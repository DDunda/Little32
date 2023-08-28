$clock_count 27 // Enough to call on_down a single time
$frame_delay 125 // 8 FPS

${:{

$keyboard R4
$char_mem R5
$key R6

#ENTRY
MOV $keyboard, KEYBOARD
MOV $char_mem, CHAR_MEM

MOV R0, on_down
RWW R0, [$keyboard]

RRW R0, [.zeroes]
RWW R0, [$char_mem]
RWW R0, [$char_mem+4]
HALT

zeroes: 0x30303030

on_down:
	RRW $key, [$keyboard+8]

	MOV R1, 15

	AND R0, R1, $key >> 28
	RRB R0, [R0+chars]
	RWB R0, [$char_mem]

	AND R0, R1, $key >> 24
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+1]
	
	AND R0, R1, $key >> 20
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+2]

	AND R0, R1, $key >> 16
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+3]

	AND R0, R1, $key >> 12
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+4]

	AND R0, R1, $key >> 8
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+5]

	AND R0, R1, $key >> 4
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+6]

	AND R0, R1, $key >> 0
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+7]

	RFE
	
#BYTE
chars:
	48 49 50 51 52 53 54 55 56 57 // 0 1 2 3 4 5 6 7 8 9
	65 66 67 68 69 70 // A B C D E F
}:}$