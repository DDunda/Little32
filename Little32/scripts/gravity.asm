$char_mem R4
$x R5
$y R6
$vx R7
$vy R8
$px R9
$py R10
$g R11

$frame_delay 10
$clock_count 1600

MOV $char_mem, CHAR_MEM

MOV $x, _x
MOV $y, _y
MOV $vx, _vx
MOV $vy, _vy
MOV $px, _px
MOV $py, _py
MOV $g, _g

RRW $x, [$x]
RRW $y, [$y]
RRW $vx, [$vx]
RRW $vy, [$vy]
RRW $px, [$px]
RRW $py, [$py]
RRW $g, [$g]

MOV R12, _mul
RRW R12, [R12]

MULF $vx, $vx, R12
MULF $vy, $vy, R12

MOV R0, frame_int
MOV R1, RENDER_INT
RWW R0, [R1]
HALT

frame_int:
    FTOI R0, $x
    FTOI R1, $y

    CMP R0, 0
    BLT .skip_draw_1
    CMP R0, 15
    BGT .skip_draw_1
    CMP R1, 0
    BLT .skip_draw_1
    CMP R1, 15
    BGT .skip_draw_1

    MOV R2, R1 << 4
    ADD R2, R0
    MOV R3, 0xFA // '·'
    RWB R3, [$char_mem+R2]
skip_draw_1:

    MOV R12, 64
repeat:

    SUBF R2, $px, $x
    MULF R0, R2, R2
    SUBF R3, $py, $y
    MULF R1, R3, R3
    ADDF R0, R0, R1
    DIVF R0, $g, R0       // g / ((px - x) ^ 2 + (py - y) ^ 2)
    MULF R2, R2, R0
    MULF R3, R3, R0

    ADDF $vx, $vx, R2
    ADDF $vy, $vy, R3

    ADDF $x, $x, $vx
    ADDF $y, $y, $vy
    
    SUBS R12, R12, 1
    BGE .repeat

    FTOI R0, $x
    FTOI R1, $y

    CMP R0, 0
    BLT .skip_draw_2
    CMP R0, 15
    BGT .skip_draw_2
    CMP R1, 0
    BLT .skip_draw_2
    CMP R1, 15
    BGT .skip_draw_2

    MOV R2, R1 << 4
    ADD R2, R2, R0
    MOV R3, 0xFE // '■'
    RWB R3, [$char_mem+R2]

skip_draw_2:
    RFE


#WORD
_x:
    8.0
_y:
    1.0
_vx:
    0.7
_vy:
    0.0
_px:
    8.0
_py:
    8.0
_mul:
    0.001
_g:
    0.000003