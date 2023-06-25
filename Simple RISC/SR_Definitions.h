#pragma once

#define N_flag 0b1000
#define Z_flag 0b0100
#define C_flag 0b0010
#define V_flag 0b0001

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8
#define R9 9
#define R10 10
#define R11 11
#define R12 12
#define SP 13
#define LR 14
#define PC 15

#define AL 0b0000
#define GT 0b0001
#define GE 0b0010
#define HI 0b0011
#define CS 0b0100
#define ZS 0b0101
#define NS 0b0110
#define VS 0b0111
#define VC 0b1000
#define NC 0b1001
#define ZC 0b1010
#define CC 0b1011
#define LS 0b1100
#define LT 0b1101
#define LE 0b1110

#define HS CS
#define EQ ZS
#define MI NS
#define PL NC
#define NE ZC
#define LO CC