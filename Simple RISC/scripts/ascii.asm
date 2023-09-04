MOV R0, hello_world
MOV R1, CHAR_MEM
MOV R2, COLOUR_MEM
MOV R3, 0
MOV R4, 15
B .check

start:
RWB R5, [R1+R3]
RWB R4, [R2+R3]
ADD R4, 16
AND R4, R4, 240
LSR R5, R4, 4
AND R5, R5, 15
XOR R5, R5, 15
ORR R4, R4, R5
ADD R3, 1
check:
RRB R5, [R0+R3]
CMP R5, 0
BZC .start

HALT

#DATA
#ASCIZ
hello_world:
	"Hello, World! Lorem ipsum dolor innit........................ :)\0 Secret text secret text!"