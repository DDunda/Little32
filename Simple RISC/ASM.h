const char* assembly_code = "\
$x R0\n\
$i R1\n\
$y R2\n\
$g R3\n\
MOV $x, 7\n\
MOV $i, 0\n\
MOV $y, CHAR_MEM\n\
MOV $g, gradient\n\
MOV R4, frame_int\n\
RWW R4, [$y+256]\n\
HALT\n\
\n\
frame_int:\n\
CMP $x, 0\n\
ADD $x, $x, 10 ?LT\n\
RRB R4, [$g+$x]\n\
RWB R4, [$y+$i]\n\
ADD $i, $i, 1\n\
SUB $x, $x, 1\n\
ANDS R4, $i, 15\n\
BNE .frame_int\n\
SUB $x, $x, 3\n\
CMP $i, 256\n\
BLT .frame_int\n\
MOV $i, 0\n\
ADD $x, $x, 3\n\
RFE\n\
\n\
#DATA\n\
gradient:\n\
0x20b0b1b2\n\
0xdbb2b1b0\n\
0x20202020\n\
0x20202020";