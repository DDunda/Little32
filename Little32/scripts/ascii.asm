$text_ptr = R0
$char_mem = R1
$colr_mem = R2
$i = R3

#ROM
#PROGRAM
#ALIGN 4
#ENTRY
MOV $text_ptr, hello_world
MOV $char_mem, CHAR_MEM
MOV $colr_mem, COLOUR_MEM
MOV $i, 0
MOV R4, 15
B .check

start:
RWB R5, [$char_mem+$i]
RWB R4, [$colr_mem+$i]
ADD R4, 16
AND R4, R4, 240
LSR R5, R4, 4
AND R5, R5, 15
XOR R5, R5, 15
ORR R4, R4, R5
INC $i
check:
RRB R5, [$text_ptr+$i]
CMP R5, 0
BZC .start

HALT

#DATA
#ASCIZ
hello_world:
	"Hello, World! Lorem ipsum dolor innit........................ :)\0 Secret text secret text!"