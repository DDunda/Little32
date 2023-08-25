${:{

$keyboard R4
$char_mem R5
$key R6

MOV $keyboard, KEYBOARD
MOV $char_mem, CHAR_MEM

MOV R0, on_down
RWW R0, [$keyboard]
HALT

on_down:
	PUSH {LR}

	RRW $key, [$keyboard+8]
	
	LSR R0, $key, 28
	BL .to_hex
	RWB R0, [$char_mem]

	LSR R0, $key, 24
	BL .to_hex
	RWB R0, [$char_mem+1]
	
	LSR R0, $key, 20
	BL .to_hex
	RWB R0, [$char_mem+2]

	LSR R0, $key, 16
	BL .to_hex
	RWB R0, [$char_mem+3]

	LSR R0, $key, 12
	BL .to_hex
	RWB R0, [$char_mem+4]

	LSR R0, $key, 8
	BL .to_hex
	RWB R0, [$char_mem+5]

	LSR R0, $key, 4
	BL .to_hex
	RWB R0, [$char_mem+6]

	MOV R0, $key
	BL .to_hex
	RWB R0, [$char_mem+7]

	POP {LR}
	RFE

to_hex:
	AND R0, R0, 15
	CMP R0, 10
	ADD R0, R0, 48 ?LT
	ADD R0, R0, 55 ?GE
	RET

}:}$