$clocks_per_frame = 27 // Enough to call on_down a single time
$frame_delay = 125 // 8 FPS

${:{

$keyboard = R4
$char_mem = R5
$key      = R6

#ENTRY
MOV $char_mem, CHAR_MEM

MOV $keyboard, keyboard
RRW $keyboard, [$keyboard]
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

	AND R0, R1, $key ROTR 28
	RRB R0, [R0+chars]
	RWB R0, [$char_mem]

	AND R0, R1, $key ROTR 24
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+1]
	
	AND R0, R1, $key ROTR 20
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+2]

	AND R0, R1, $key ROTR 16
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+3]

	AND R0, R1, $key ROTR 12
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+4]

	AND R0, R1, $key ROTR 8
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+5]

	AND R0, R1, $key ROTR 4
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+6]

	AND R0, R1, $key ROTR 0
	RRB R0, [R0+chars]
	RWB R0, [$char_mem+7]

	RFE

#DATA
keyboard: KEYBOARD

#BYTE
#ASCII
chars:
	"0123456789ABCDEF"
}:}$