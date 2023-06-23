#pragma once

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

//                                                         CCCCN1ccccSi   ,   .   ,   .   ,
#define  ADD(  cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000100000000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  ADDS( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000100001000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NADD(  cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001100000000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NADDS( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001100001000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  ADDi( cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000100000100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define  ADDSi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000100001100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define NADDi( cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001100000100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define NADDSi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001100001100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
//                                                         CCCCN1ccccSi   ,   .   ,   .   ,

//                                                         CCCCN1ccccSi   ,   .   ,   .   ,
#define  SUB(  cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000100010000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  SUBS( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000100011000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NSUB(  cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001100010000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NSUBS( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001100011000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  SUBi( cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000100010100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define  SUBSi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000100011100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define NSUBi( cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001100010100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define NSUBSi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001100011100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
//                                                         CCCCN1ccccSi   ,   .   ,   .   ,


//                                                    CCCCN1ccccSi   ,   .   ,   .   ,
#define MOV(  cond, r0, r1, sh) ram.memory[pc++] =  0b00000111100000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define MOVS( cond, r0, r1, sh) ram.memory[pc++] =  0b00000111101000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define MVN(  cond, r0, r1, sh) ram.memory[pc++] =  0b00001111100000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define MVNS( cond, r0, r1, sh) ram.memory[pc++] =  0b00001111101000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define MOVi( cond, r0, im, sh) ram.memory[pc++] = (0b00000111100100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define MOVSi(cond, r0, im, sh) ram.memory[pc++] = (0b00000111101100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define MVNi( cond, r0, im, sh) ram.memory[pc++] = (0b00001111100100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define MVNSi(cond, r0, im, sh) ram.memory[pc++] = (0b00001111101100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
//                                                    CCCCN1ccccSi   ,   .   ,   .   ,

//                                                     CCCCN1ccccSi   ,   .   ,   .   ,
#define  INV(  cond, r0, r1, sh) ram.memory[pc++] =  0b00000111110000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define  INVS( cond, r0, r1, sh) ram.memory[pc++] =  0b00000111111000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define NINV(  cond, r0, r1, sh) ram.memory[pc++] =  0b00001111110000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define NINVS( cond, r0, r1, sh) ram.memory[pc++] =  0b00001111111000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | sh;
#define  INVi( cond, r0, im, sh) ram.memory[pc++] = (0b00000111110100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define  INVSi(cond, r0, im, sh) ram.memory[pc++] = (0b00000111111100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define NINVi( cond, r0, im, sh) ram.memory[pc++] = (0b00001111110100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
#define NINVSi(cond, r0, im, sh) ram.memory[pc++] = (0b00001111111100000000000000000000 | ((cond) << 28) | ((r0) << 16) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 22);
//                                                     CCCCN1ccccSi   ,   .   ,   .   ,

//                                                        CCCCN0011BWi   ,   .   ,   .   ,
#define  RRW( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000001100000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  RWW( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000001101000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  RRB( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000001110000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  RWB( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00000001111000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NRRW( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001001100000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NRWW( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001001101000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NRRB( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001001110000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define NRWB( cond, r0, r1, r2, sh) ram.memory[pc++] =  0b00001001111000000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | ((r2) << 8) | sh;
#define  RRWi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000001100100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define  RWWi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000001101100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define  RRBi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000001110100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define  RWBi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00000001111100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define NRRWi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001001100100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define NRWWi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001001101100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define NRRBi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001001110100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
#define NRWBi(cond, r0, r1, im, sh) ram.memory[pc++] = (0b00001001111100000000000000000000 | ((cond) << 28) | ((r0) << 16) | ((r1) << 12) | (((im) * (1 - 2 * ((im) < 0))) << 4) | sh) ^ (((im) < 0) << 27);
//                                                        CCCCN0011BWi   ,   .   ,   .   ,

//                                            CCCCN01L   .   ,   .   ,   .   ,
#define B( cond, offset) ram.memory[pc++] = 0b00000010000000000000000000000000 | ((cond) << 28) | (((offset) < 0) << 27) | ((offset) * (1 - 2 * ((offset) < 0)));
#define BL(cond, offset) ram.memory[pc++] = 0b00000011000000000000000000000000 | ((cond) << 28) | (((offset) < 0) << 27) | ((offset) * (1 - 2 * ((offset) < 0)));
//                                            CCCCN01L   .   ,   .   ,   .   ,