$clocks_per_frame = 26 // Enough to call on_down a single time
$frame_delay = 125 // 8 FPS

${:{

$char_mem = R4
$key      = R5
$mask     = R6

#ROM FORCE
#ENTRY
MOV $mask, 0xF

MOV R0, on_down
RWW R0, [.KEYBOARD]

RRW R0, [.zeroes]
RWW R0, [.CHAR_MEM]
RWW R0, PC, .CHAR_MEM+4 // [.CHAR_MEM+4]
HALT

on_down:
	RRW $key, PC, .KEYBOARD+8 // [.KEYBOARD+8]

	AND R0, $mask, $key ROTR 28
	RRB R0, [R0+chars]
	RWB R0, [.CHAR_MEM]

	AND R0, $mask, $key ROTR 24
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+1 // [.CHAR_MEM+1]
	
	AND R0, $mask, $key ROTR 20
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+2 // [.CHAR_MEM+2]

	AND R0, $mask, $key ROTR 16
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+3 // [.CHAR_MEM+3]

	AND R0, $mask, $key ROTR 12
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+4 // [.CHAR_MEM+4]

	AND R0, $mask, $key ROTR 8
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+5 // [.CHAR_MEM+5]

	AND R0, $mask, $key ROTR 4
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+6 // [.CHAR_MEM+6]

	AND R0, $mask, $key ROTR 0
	RRB R0, [R0+chars]
	RWB R0, PC, .CHAR_MEM+7 // [.CHAR_MEM+7]

	RFE

#DATA
zeroes: 0x30303030
keyboard: KEYBOARD

#BYTE
#ASCII
chars:
	"0123456789ABCDEF"
}:}$