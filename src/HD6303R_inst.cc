/**
 *  VDX7 - Virtual DX7 synthesizer emulation
 *  Copyright (C) 2023  chiaccona@gmail.com 
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#include "HD6303R.h"

void HD6303R::initInst() {
	Instruction *r= instructions;
	//        OP    GRP     INST    AM   R  W Byt Cyc Code
	r[0x89]={0x89, "adc ", "adca", "im", 0, 0, 2,  2, [this]{ immed2  (); R=A+OP+C; A8(A,OP,R); A=R; }};
	r[0x99]={0x99, "adc ", "adca", "di", 1, 0, 2,  3, [this]{ direct2 (); R=A+OP+C; A8(A,OP,R); A=R; }};
	r[0xa9]={0xa9, "adc ", "adca", "in", 1, 0, 2,  4, [this]{ index2  (); R=A+OP+C; A8(A,OP,R); A=R; }};
	r[0xb9]={0xb9, "adc ", "adca", "ex", 1, 0, 3,  4, [this]{ extend  (); R=A+OP+C; A8(A,OP,R); A=R; }};
	r[0xc9]={0xc9, "adc ", "adcb", "im", 0, 0, 2,  2, [this]{ immed2  (); R=B+OP+C; A8(B,OP,R); B=R; }};
	r[0xd9]={0xd9, "adc ", "adcb", "di", 1, 0, 2,  3, [this]{ direct2 (); R=B+OP+C; A8(B,OP,R); B=R; }};
	r[0xe9]={0xe9, "adc ", "adcb", "in", 1, 0, 2,  4, [this]{ index2  (); R=B+OP+C; A8(B,OP,R); B=R; }};
	r[0xf9]={0xf9, "adc ", "adcb", "ex", 1, 0, 3,  4, [this]{ extend  (); R=B+OP+C; A8(B,OP,R); B=R; }};
	r[0x1b]={0x1b, "add ", "aba ", "id", 0, 0, 1,  1, [this]{ implied (); R=A+B; A8(A,B,R); A=R; }};
	r[0x3a]={0x3a, "add ", "abx ", "id", 0, 0, 1,  1, [this]{ implied (); IX+=B; }};
	r[0x8b]={0x8b, "add ", "adda", "im", 0, 0, 2,  2, [this]{ immed2  (); R=A+OP; A8(A,OP,R); A=R; }};
	r[0x9b]={0x9b, "add ", "adda", "di", 1, 0, 2,  3, [this]{ direct2 (); R=A+OP; A8(A,OP,R); A=R; }};
	r[0xab]={0xab, "add ", "adda", "in", 1, 0, 2,  4, [this]{ index2  (); R=A+OP; A8(A,OP,R); A=R; }};
	r[0xbb]={0xbb, "add ", "adda", "ex", 1, 0, 3,  4, [this]{ extend  (); R=A+OP; A8(A,OP,R); A=R; }};
	r[0xcb]={0xcb, "add ", "addb", "im", 0, 0, 2,  2, [this]{ immed2  (); R=B+OP; A8(B,OP,R); B=R; }};
	r[0xdb]={0xdb, "add ", "addb", "di", 1, 0, 2,  3, [this]{ direct2 (); R=B+OP; A8(B,OP,R); B=R; }};
	r[0xeb]={0xeb, "add ", "addb", "in", 1, 0, 2,  4, [this]{ index2  (); R=B+OP; A8(B,OP,R); B=R; }};
	r[0xfb]={0xfb, "add ", "addb", "ex", 1, 0, 3,  4, [this]{ extend  (); R=B+OP; A8(B,OP,R); B=R; }};
	r[0xc3]={0xc3, "add ", "addd", "im", 0, 0, 3,  3, [this]{ immed3  (); R2=D+OP2; A16(D,OP2,R2); D=R2; }};
	r[0xd3]={0xd3, "add ", "addd", "di", 1, 0, 2,  4, [this]{ direct16(); R2=D+OP2; A16(D,OP2,R2); D=R2; }};
	r[0xe3]={0xe3, "add ", "addd", "in", 1, 0, 2,  5, [this]{ index16 (); R2=D+OP2; A16(D,OP2,R2); D=R2; }};
	r[0xf3]={0xf3, "add ", "addd", "ex", 1, 0, 3,  5, [this]{ extend16(); R2=D+OP2; A16(D,OP2,R2); D=R2; }};
	r[0x61]={0x61, "and ", "aim ", "in", 0, 1, 3,  7, [this]{ index3  (); M[ADDR]&=OP; L8(M[ADDR]); }};
	r[0x71]={0x71, "and ", "aim ", "di", 0, 1, 3,  6, [this]{ direct3 (); M[ADDR]&=OP; L8(M[ADDR]); }};
	r[0x84]={0x84, "and ", "anda", "im", 0, 0, 2,  2, [this]{ immed2  (); A&=OP; L8(A); }};
	r[0x94]={0x94, "and ", "anda", "di", 1, 0, 2,  3, [this]{ direct2 (); A&=OP; L8(A); }};
	r[0xa4]={0xa4, "and ", "anda", "in", 1, 0, 2,  4, [this]{ index2  (); A&=OP; L8(A); }};
	r[0xb4]={0xb4, "and ", "anda", "ex", 1, 0, 3,  4, [this]{ extend  (); A&=OP; L8(A); }};
	r[0xc4]={0xc4, "and ", "andb", "im", 0, 0, 2,  2, [this]{ immed2  (); B&=OP; L8(B); }};
	r[0xd4]={0xd4, "and ", "andb", "di", 1, 0, 2,  3, [this]{ direct2 (); B&=OP; L8(B); }};
	r[0xe4]={0xe4, "and ", "andb", "in", 1, 0, 2,  4, [this]{ index2  (); B&=OP; L8(B); }};
	r[0xf4]={0xf4, "and ", "andb", "ex", 1, 0, 3,  4, [this]{ extend  (); B&=OP; L8(B); }};
	r[0x68]={0x68, "asl ", "asl ", "in", 1, 1, 2,  6, [this]{ index2  (); asl(M[ADDR]); }};
	r[0x78]={0x78, "asl ", "asl ", "ex", 1, 1, 3,  6, [this]{ extend  (); asl(M[ADDR]); }};
	r[0x48]={0x48, "asl ", "asla", "id", 0, 0, 1,  1, [this]{ implied (); asl(A); }};
	r[0x58]={0x58, "asl ", "aslb", "id", 0, 0, 1,  1, [this]{ implied (); asl(B); }};
	r[0x05]={0x05, "asl ", "asld", "id", 0, 0, 1,  1, [this]{ implied (); asld(D); }};
	r[0x67]={0x67, "asr ", "asr ", "in", 1, 1, 2,  6, [this]{ index2  (); asr(M[ADDR]); }};
	r[0x77]={0x77, "asr ", "asr ", "ex", 1, 1, 3,  6, [this]{ extend  (); asr(M[ADDR]); }};
	r[0x47]={0x47, "asr ", "asra", "id", 0, 0, 1,  1, [this]{ implied (); asr(A); }};
	r[0x57]={0x57, "asr ", "asrb", "id", 0, 0, 1,  1, [this]{ implied (); asr(B); }};
	r[0x85]={0x85, "bit ", "bita", "im", 0, 0, 2,  2, [this]{ immed2  (); L8(A&OP); }};
	r[0x95]={0x95, "bit ", "bita", "di", 1, 0, 2,  3, [this]{ direct2 (); L8(A&OP); }};
	r[0xa5]={0xa5, "bit ", "bita", "in", 1, 0, 2,  4, [this]{ index2  (); L8(A&OP); }};
	r[0xb5]={0xb5, "bit ", "bita", "ex", 1, 0, 3,  4, [this]{ extend  (); L8(A&OP); }};
	r[0xc5]={0xc5, "bit ", "bitb", "im", 0, 0, 2,  2, [this]{ immed2  (); L8(B&OP); }};
	r[0xd5]={0xd5, "bit ", "bitb", "di", 1, 0, 2,  3, [this]{ direct2 (); L8(B&OP); }};
	r[0xe5]={0xe5, "bit ", "bitb", "in", 1, 0, 2,  4, [this]{ index2  (); L8(B&OP); }};
	r[0xf5]={0xf5, "bit ", "bitb", "ex", 1, 0, 3,  4, [this]{ extend  (); L8(B&OP); }};
	r[0x24]={0x24, "bra ", "bcc ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(!C); }};
	r[0x25]={0x25, "bra ", "bcs ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(C); }};
	r[0x27]={0x27, "bra ", "beq ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(Z); }};
	r[0x2c]={0x2c, "bra ", "bge ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(N^(V==0)); }};
	r[0x2e]={0x2e, "bra ", "bgt ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(!(Z|(N^V))); }};
	r[0x22]={0x22, "bra ", "bhi ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(!(C|Z)); }};
	r[0x2f]={0x2f, "bra ", "ble ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(Z|(N^V)); }};
	r[0x23]={0x23, "bra ", "bls ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(C|Z); }};
	r[0x2d]={0x2d, "bra ", "blt ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(N^V); }};
	r[0x2b]={0x2b, "bra ", "bmi ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(N); }};
	r[0x26]={0x26, "bra ", "bne ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(!Z); }};
	r[0x2a]={0x2a, "bra ", "bpl ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(!N); }};
	r[0x20]={0x20, "bra ", "bra ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(1); }};
	r[0x21]={0x21, "bra ", "brn ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(0); }};
	r[0x28]={0x28, "bra ", "bvc ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(!V); }};
	r[0x29]={0x29, "bra ", "bvs ", "im", 0, 0, 2,  3, [this]{ immed2  (); bra(V); }};
	r[0x8d]={0x8d, "bsr ", "bsr ", "im", 0, 0, 2,  5, [this]{ immed2  (); bsr(); }};
	r[0x0c]={0x0c, "clr ", "clc ", "id", 0, 0, 1,  1, [this]{ implied (); C=0; }};
	r[0x0e]={0x0e, "clr ", "cli ", "id", 0, 0, 1,  1, [this]{ implied (); I=0; }};
	r[0x6f]={0x6f, "clr ", "clr ", "in", 1, 1, 2,  5, [this]{ index2  (); M[ADDR]=0; N=V=C=0; Z=1; }};
	r[0x7f]={0x7f, "clr ", "clr ", "ex", 1, 1, 3,  5, [this]{ extend  (); M[ADDR]=0; N=V=C=0; Z=1; }};
	r[0x4f]={0x4f, "clr ", "clra", "id", 0, 0, 1,  1, [this]{ implied (); A=0; N=V=C=0; Z=1; }};
	r[0x5f]={0x5f, "clr ", "clrb", "id", 0, 0, 1,  1, [this]{ implied (); B=0; N=V=C=0; Z=1; }};
	r[0x0a]={0x0a, "clr ", "clv ", "id", 0, 0, 1,  1, [this]{ implied (); V=0; }};
	r[0x11]={0x11, "cmp ", "cba ", "id", 0, 0, 1,  1, [this]{ implied (); R=A-B; S8(A,B,R); }};
	r[0x81]={0x81, "cmp ", "cmpa", "im", 0, 0, 2,  2, [this]{ immed2  (); R=A-OP; S8(A,OP,R); }};
	r[0x91]={0x91, "cmp ", "cmpa", "di", 1, 0, 2,  3, [this]{ direct2 (); R=A-OP; S8(A,OP,R); }};
	r[0xa1]={0xa1, "cmp ", "cmpa", "in", 1, 0, 2,  4, [this]{ index2  (); R=A-OP; S8(A,OP,R); }};
	r[0xb1]={0xb1, "cmp ", "cmpa", "ex", 1, 0, 3,  4, [this]{ extend  (); R=A-OP; S8(A,OP,R); }};
	r[0xc1]={0xc1, "cmp ", "cmpb", "im", 0, 0, 2,  2, [this]{ immed2  (); R=B-OP; S8(B,OP,R); }};
	r[0xd1]={0xd1, "cmp ", "cmpb", "di", 1, 0, 2,  3, [this]{ direct2 (); R=B-OP; S8(B,OP,R); }};
	r[0xe1]={0xe1, "cmp ", "cmpb", "in", 1, 0, 2,  4, [this]{ index2  (); R=B-OP; S8(B,OP,R); }};
	r[0xf1]={0xf1, "cmp ", "cmpb", "ex", 1, 0, 3,  4, [this]{ extend  (); R=B-OP; S8(B,OP,R); }};
	r[0x8c]={0x8c, "cmp ", "cpx ", "im", 0, 0, 3,  3, [this]{ immed3  (); R2=IX-OP2; S16(IX,OP2,R2); }};
	r[0x9c]={0x9c, "cmp ", "cpx ", "di", 1, 0, 2,  4, [this]{ direct16(); R2=IX-OP2; S16(IX,OP2,R2); }};
	r[0xac]={0xac, "cmp ", "cpx ", "in", 1, 0, 2,  5, [this]{ index16 (); R2=IX-OP2; S16(IX,OP2,R2); }};
	r[0xbc]={0xbc, "cmp ", "cpx ", "ex", 1, 0, 3,  5, [this]{ extend16(); R2=IX-OP2; S16(IX,OP2,R2); }};
	r[0x63]={0x63, "com ", "com ", "in", 1, 1, 2,  6, [this]{ index2  (); M[ADDR]=~OP; C=1; L8(M[ADDR]); }};
	r[0x73]={0x73, "com ", "com ", "ex", 1, 1, 3,  6, [this]{ extend  (); M[ADDR]=~OP; C=1; L8(M[ADDR]); }};
	r[0x43]={0x43, "com ", "coma", "id", 0, 0, 1,  1, [this]{ implied (); A=~A; C=1; L8(A); }};
	r[0x53]={0x53, "com ", "comb", "id", 0, 0, 1,  1, [this]{ implied (); B=~B; C=1; L8(B); }};
	r[0x19]={0x19, "daa ", "daa ", "id", 0, 0, 1,  2, [this]{ implied (); daa(); }};
	r[0x6a]={0x6a, "dec ", "dec ", "in", 1, 1, 2,  6, [this]{ index2  (); M[ADDR]--; D8(M[ADDR]); }};
	r[0x7a]={0x7a, "dec ", "dec ", "ex", 1, 1, 3,  6, [this]{ extend  (); M[ADDR]--; D8(M[ADDR]); }};
	r[0x4a]={0x4a, "dec ", "deca", "id", 0, 0, 1,  1, [this]{ implied (); A--; D8(A); }};
	r[0x5a]={0x5a, "dec ", "decb", "id", 0, 0, 1,  1, [this]{ implied (); B--; D8(B); }};
	r[0x34]={0x34, "dec ", "des ", "id", 0, 0, 1,  1, [this]{ implied (); SP--; }};
	r[0x09]={0x09, "dec ", "dex ", "id", 0, 0, 1,  1, [this]{ implied (); IX--; Z=!IX; }};
	r[0x65]={0x65, "eor ", "eim ", "in", 0, 1, 3,  7, [this]{ index3  (); M[ADDR]^=OP; L8(M[ADDR]); }};
	r[0x75]={0x75, "eor ", "eim ", "di", 0, 1, 3,  6, [this]{ direct3 (); M[ADDR]^=OP; L8(M[ADDR]); }};
	r[0x88]={0x88, "eor ", "eora", "im", 0, 0, 2,  2, [this]{ immed2  (); A^=OP; L8(A); }};
	r[0x98]={0x98, "eor ", "eora", "di", 1, 0, 2,  3, [this]{ direct2 (); A^=OP; L8(A); }};
	r[0xa8]={0xa8, "eor ", "eora", "in", 1, 0, 2,  4, [this]{ index2  (); A^=OP; L8(A); }};
	r[0xb8]={0xb8, "eor ", "eora", "ex", 1, 0, 3,  4, [this]{ extend  (); A^=OP; L8(A); }};
	r[0xc8]={0xc8, "eor ", "eorb", "im", 0, 0, 2,  2, [this]{ immed2  (); B^=OP; L8(B); }};
	r[0xd8]={0xd8, "eor ", "eorb", "di", 1, 0, 2,  3, [this]{ direct2 (); B^=OP; L8(B); }};
	r[0xe8]={0xe8, "eor ", "eorb", "in", 1, 0, 2,  4, [this]{ index2  (); B^=OP; L8(B); }};
	r[0xf8]={0xf8, "eor ", "eorb", "ex", 1, 0, 3,  4, [this]{ extend  (); B^=OP; L8(B); }};
	r[0x18]={0x18, "exg ", "xgdx", "id", 0, 0, 1,  2, [this]{ implied (); OP2=IX; IX=D; D=OP2; }};
	r[0x6c]={0x6c, "inc ", "inc ", "in", 1, 1, 2,  6, [this]{ index2  (); M[ADDR]++; I8(M[ADDR]); }};
	r[0x7c]={0x7c, "inc ", "inc ", "ex", 1, 1, 3,  6, [this]{ extend  (); M[ADDR]++; I8(M[ADDR]); }};
	r[0x4c]={0x4c, "inc ", "inca", "id", 0, 0, 1,  1, [this]{ implied (); A++; I8(A); }};
	r[0x5c]={0x5c, "inc ", "incb", "id", 0, 0, 1,  1, [this]{ implied (); B++; I8(B); }};
	r[0x31]={0x31, "inc ", "ins ", "id", 0, 0, 1,  1, [this]{ implied (); SP++               ;}};
	r[0x08]={0x08, "inc ", "inx ", "id", 0, 0, 1,  1, [this]{ implied (); IX++; Z=!IX; }};
	r[0x6e]={0x6e, "jmp ", "jmp ", "in", 1, 0, 2,  3, [this]{ index2  (); PC=ADDR; }};
	r[0x7e]={0x7e, "jmp ", "jmp ", "ex", 1, 0, 3,  3, [this]{ extend  (); PC=ADDR; }};
	r[0x9d]={0x9d, "jsr ", "jsr ", "di", 1, 1, 2,  5, [this]{ direct2 (); push(PC); PC=ADDR; }};
	r[0xad]={0xad, "jsr ", "jsr ", "in", 1, 1, 2,  5, [this]{ index2  (); push(PC); PC=ADDR; }};
	r[0xbd]={0xbd, "jsr ", "jsr ", "ex", 1, 1, 3,  6, [this]{ extend  (); push(PC); PC=ADDR; }};
	r[0x86]={0x86, "ld  ", "ldaa", "im", 0, 0, 2,  2, [this]{ immed2  (); A=OP; L8(A); }};
	r[0x96]={0x96, "ld  ", "ldaa", "di", 1, 0, 2,  3, [this]{ direct2 (); A=OP; L8(A); }};
	r[0xa6]={0xa6, "ld  ", "ldaa", "in", 1, 0, 2,  4, [this]{ index2  (); A=OP; L8(A); }};
	r[0xb6]={0xb6, "ld  ", "ldaa", "ex", 1, 0, 3,  4, [this]{ extend  (); A=OP; L8(A); }};
	r[0xc6]={0xc6, "ld  ", "ldab", "im", 0, 0, 2,  2, [this]{ immed2  (); B=OP; L8(B); }};
	r[0xd6]={0xd6, "ld  ", "ldab", "di", 1, 0, 2,  3, [this]{ direct2 (); B=OP; L8(B); }};
	r[0xe6]={0xe6, "ld  ", "ldab", "in", 1, 0, 2,  4, [this]{ index2  (); B=OP; L8(B); }};
	r[0xf6]={0xf6, "ld  ", "ldab", "ex", 1, 0, 3,  4, [this]{ extend  (); B=OP; L8(B); }};
	r[0xcc]={0xcc, "ld  ", "ldd ", "im", 0, 0, 3,  3, [this]{ immed3  (); D=OP2; L16(D); }};
	r[0xdc]={0xdc, "ld  ", "ldd ", "di", 1, 0, 2,  4, [this]{ direct2 (); D=getm16(ADDR); L16(D); }};
	r[0xec]={0xec, "ld  ", "ldd ", "in", 1, 0, 2,  5, [this]{ index2  (); D=getm16(ADDR); L16(D); }};
	r[0xfc]={0xfc, "ld  ", "ldd ", "ex", 1, 0, 3,  5, [this]{ extend  (); D=getm16(ADDR); L16(D); }};
	r[0x8e]={0x8e, "ld  ", "lds ", "im", 0, 0, 3,  3, [this]{ immed3  (); SP=OP2; L16(SP); }};
	r[0x9e]={0x9e, "ld  ", "lds ", "di", 1, 0, 2,  4, [this]{ direct2 (); SP=getm16(ADDR); L16(SP); }};
	r[0xae]={0xae, "ld  ", "lds ", "in", 1, 0, 2,  5, [this]{ index2  (); SP=getm16(ADDR); L16(SP); }};
	r[0xbe]={0xbe, "ld  ", "lds ", "ex", 1, 0, 3,  5, [this]{ extend  (); SP=getm16(ADDR); L16(SP); }};
	r[0xce]={0xce, "ld  ", "ldx ", "im", 0, 0, 3,  3, [this]{ immed3  (); IX=OP2; L16(IX); }};
	r[0xde]={0xde, "ld  ", "ldx ", "di", 1, 0, 2,  4, [this]{ direct2 (); IX=getm16(ADDR); L16(IX); }};
	r[0xee]={0xee, "ld  ", "ldx ", "in", 1, 0, 2,  5, [this]{ index2  (); IX=getm16(ADDR); L16(IX); }};
	r[0xfe]={0xfe, "ld  ", "ldx ", "ex", 1, 0, 3,  5, [this]{ extend  (); IX=getm16(ADDR); L16(IX); }};
	r[0x64]={0x64, "lsr ", "lsr ", "in", 1, 1, 2,  6, [this]{ index2  (); lsr(M[ADDR]); }};
	r[0x74]={0x74, "lsr ", "lsr ", "ex", 1, 1, 3,  6, [this]{ extend  (); lsr(M[ADDR]); }};
	r[0x44]={0x44, "lsr ", "lsra", "id", 0, 0, 1,  1, [this]{ implied (); lsr(A); }};
	r[0x54]={0x54, "lsr ", "lsrb", "id", 0, 0, 1,  1, [this]{ implied (); lsr(B); }};
	r[0x04]={0x04, "lsr ", "lsrd", "id", 0, 0, 1,  1, [this]{ implied (); lsrd(D); }};
	r[0x3d]={0x3d, "mul ", "mul ", "id", 0, 0, 1,  7, [this]{ implied (); D=A*B; C=bit(D,7); }};
	r[0x60]={0x60, "neg ", "neg ", "in", 1, 1, 2,  6, [this]{ index2  (); M[ADDR]=-OP; I8(M[ADDR]); C=!Z; }};
	r[0x70]={0x70, "neg ", "neg ", "ex", 1, 1, 3,  6, [this]{ extend  (); M[ADDR]=-OP; I8(M[ADDR]); C=!Z; }};
	r[0x40]={0x40, "neg ", "nega", "id", 0, 1, 1,  1, [this]{ implied (); A=-A; I8(A); C=!Z; }};
	r[0x50]={0x50, "neg ", "negb", "id", 0, 1, 1,  1, [this]{ implied (); B=-B; I8(B); C=!Z; }};
	r[0x01]={0x01, "nop ", "nop ", "id", 0, 0, 1,  1, [this]{ implied (); }};
	r[0x62]={0x62, "or  ", "oim ", "in", 0, 1, 3,  7, [this]{ index3  (); M[ADDR]|=OP; V=0; L8(M[ADDR]); }};
	r[0x72]={0x72, "or  ", "oim ", "di", 0, 1, 3,  6, [this]{ direct3 (); M[ADDR]|=OP; V=0; L8(M[ADDR]); }};
	r[0x8a]={0x8a, "or  ", "oraa", "im", 0, 0, 2,  2, [this]{ immed2  (); A|=OP; V=0; L8(A); }};
	r[0x9a]={0x9a, "or  ", "oraa", "di", 1, 0, 2,  3, [this]{ direct2 (); A|=OP; V=0; L8(A); }};
	r[0xaa]={0xaa, "or  ", "oraa", "in", 1, 0, 2,  4, [this]{ index2  (); A|=OP; V=0; L8(A); }};
	r[0xba]={0xba, "or  ", "oraa", "ex", 1, 0, 3,  4, [this]{ extend  (); A|=OP; V=0; L8(A); }};
	r[0xca]={0xca, "or  ", "orab", "im", 0, 0, 2,  2, [this]{ immed2  (); B|=OP; V=0; L8(B); }};
	r[0xda]={0xda, "or  ", "orab", "di", 1, 0, 2,  3, [this]{ direct2 (); B|=OP; V=0; L8(B); }};
	r[0xea]={0xea, "or  ", "orab", "in", 1, 0, 2,  4, [this]{ index2  (); B|=OP; V=0; L8(B); }};
	r[0xfa]={0xfa, "or  ", "orab", "ex", 1, 0, 3,  4, [this]{ extend  (); B|=OP; V=0; L8(B); }};
	r[0x32]={0x32, "pull", "pula", "id", 0, 0, 1,  3, [this]{ implied (); A=M[++SP]; }};
	r[0x33]={0x33, "pull", "pulb", "id", 0, 0, 1,  3, [this]{ implied (); B=M[++SP]; }};
	r[0x38]={0x38, "pull", "pulx", "id", 0, 0, 1,  4, [this]{ implied (); pull(IX); }};
	r[0x36]={0x36, "push", "psha", "id", 0, 1, 1,  4, [this]{ implied (); M[SP--]=A; }};
	r[0x37]={0x37, "push", "pshb", "id", 0, 1, 1,  4, [this]{ implied (); M[SP--]=B; }};
	r[0x3c]={0x3c, "push", "pshx", "id", 0, 1, 1,  5, [this]{ implied (); push(IX); }};
	r[0x69]={0x69, "rol ", "rol ", "in", 1, 1, 2,  6, [this]{ index2  (); rol(M[ADDR]); }};
	r[0x79]={0x79, "rol ", "rol ", "ex", 1, 1, 3,  6, [this]{ extend  (); rol(M[ADDR]); }};
	r[0x49]={0x49, "rol ", "rola", "id", 0, 0, 1,  1, [this]{ implied (); rol(A); }};
	r[0x59]={0x59, "rol ", "rolb", "id", 0, 0, 1,  1, [this]{ implied (); rol(B); }};
	r[0x66]={0x66, "ror ", "ror ", "in", 1, 1, 2,  6, [this]{ index2  (); ror(M[ADDR]); }};
	r[0x76]={0x76, "ror ", "ror ", "ex", 1, 1, 3,  6, [this]{ extend  (); ror(M[ADDR]); }};
	r[0x46]={0x46, "ror ", "rora", "id", 0, 0, 1,  1, [this]{ implied (); ror(A); }};
	r[0x56]={0x56, "ror ", "rorb", "id", 0, 0, 1,  1, [this]{ implied (); ror(B); }};
	r[0x3b]={0x3b, "rts ", "rti ", "id", 0, 0, 1, 10, [this]{ implied (); rti(); }};
	r[0x39]={0x39, "rts ", "rts ", "id", 0, 0, 1,  5, [this]{ implied (); pull(PC); }};
	r[0x82]={0x82, "sbc ", "sbca", "im", 0, 0, 2,  2, [this]{ immed2  (); R=A-OP-C; S8(A,OP,R); A=R; }};
	r[0x92]={0x92, "sbc ", "sbca", "di", 1, 0, 2,  3, [this]{ direct2 (); R=A-OP-C; S8(A,OP,R); A=R; }};
	r[0xa2]={0xa2, "sbc ", "sbca", "in", 1, 0, 2,  4, [this]{ index2  (); R=A-OP-C; S8(A,OP,R); A=R; }};
	r[0xb2]={0xb2, "sbc ", "sbca", "ex", 1, 0, 3,  4, [this]{ extend  (); R=A-OP-C; S8(A,OP,R); A=R; }};
	r[0xc2]={0xc2, "sbc ", "sbcb", "im", 0, 0, 2,  2, [this]{ immed2  (); R=B-OP-C; S8(B,OP,R); B=R; }};
	r[0xd2]={0xd2, "sbc ", "sbcb", "di", 1, 0, 2,  3, [this]{ direct2 (); R=B-OP-C; S8(B,OP,R); B=R; }};
	r[0xe2]={0xe2, "sbc ", "sbcb", "in", 1, 0, 2,  4, [this]{ index2  (); R=B-OP-C; S8(B,OP,R); B=R; }};
	r[0xf2]={0xf2, "sbc ", "sbcb", "ex", 1, 0, 3,  4, [this]{ extend  (); R=B-OP-C; S8(B,OP,R); B=R; }};
	r[0x0d]={0x0d, "set ", "sec ", "id", 0, 0, 1,  1, [this]{ implied (); C=1; }};
	r[0x0f]={0x0f, "set ", "sei ", "id", 0, 0, 1,  1, [this]{ implied (); I=1; }};
	r[0x0b]={0x0b, "set ", "sev ", "id", 0, 0, 1,  1, [this]{ implied (); V=1; }};
	r[0x97]={0x97, "st  ", "staa", "di", 1, 1, 2,  3, [this]{ direct2 (); M[ADDR]=A; V=0; L8(M[ADDR]); }};
	r[0xa7]={0xa7, "st  ", "staa", "in", 1, 1, 2,  4, [this]{ index2  (); M[ADDR]=A; V=0; L8(M[ADDR]); }};
	r[0xb7]={0xb7, "st  ", "staa", "ex", 1, 1, 3,  4, [this]{ extend  (); M[ADDR]=A; V=0; L8(M[ADDR]); }};
	r[0xd7]={0xd7, "st  ", "stab", "di", 1, 1, 2,  3, [this]{ direct2 (); M[ADDR]=B; V=0; L8(M[ADDR]); }};
	r[0xe7]={0xe7, "st  ", "stab", "in", 1, 1, 2,  4, [this]{ index2  (); M[ADDR]=B; V=0; L8(M[ADDR]); }};
	r[0xf7]={0xf7, "st  ", "stab", "ex", 1, 1, 3,  4, [this]{ extend  (); M[ADDR]=B; V=0; L8(M[ADDR]); }};
	r[0xdd]={0xdd, "st  ", "std ", "di", 1, 1, 2,  4, [this]{ direct2 (); stom16(ADDR,D); L16(D); }};
	r[0xed]={0xed, "st  ", "std ", "in", 1, 1, 2,  5, [this]{ index2  (); stom16(ADDR,D); L16(D); }};
	r[0xfd]={0xfd, "st  ", "std ", "ex", 1, 1, 3,  5, [this]{ extend  (); stom16(ADDR,D); L16(D); }};
	r[0x9f]={0x9f, "st  ", "sts ", "di", 1, 1, 2,  4, [this]{ direct2 (); stom16(ADDR,SP); L16(SP); }};
	r[0xaf]={0xaf, "st  ", "sts ", "in", 1, 1, 2,  5, [this]{ index2  (); stom16(ADDR,SP); L16(SP); }};
	r[0xbf]={0xbf, "st  ", "sts ", "ex", 1, 1, 3,  5, [this]{ extend  (); stom16(ADDR,SP); L16(SP); }};
	r[0xdf]={0xdf, "st  ", "stx ", "di", 1, 1, 2,  4, [this]{ direct2 (); stom16(ADDR,IX); L16(IX); }};
	r[0xef]={0xef, "st  ", "stx ", "in", 1, 1, 2,  5, [this]{ index2  (); stom16(ADDR,IX); L16(IX); }};
	r[0xff]={0xff, "st  ", "stx ", "ex", 1, 1, 3,  5, [this]{ extend  (); stom16(ADDR,IX); L16(IX); }};
	r[0x10]={0x10, "sub ", "sba ", "id", 0, 0, 1,  1, [this]{ implied (); R=A-B; S8(A,B,R); A=R; }};
	r[0x80]={0x80, "sub ", "suba", "im", 0, 0, 2,  2, [this]{ immed2  (); R=A-OP; S8(A,OP,R); A=R; }};
	r[0x90]={0x90, "sub ", "suba", "di", 1, 0, 2,  3, [this]{ direct2 (); R=A-OP; S8(A,OP,R); A=R; }};
	r[0xa0]={0xa0, "sub ", "suba", "in", 1, 0, 2,  4, [this]{ index2  (); R=A-OP; S8(A,OP,R); A=R; }};
	r[0xb0]={0xb0, "sub ", "suba", "ex", 1, 0, 3,  4, [this]{ extend  (); R=A-OP; S8(A,OP,R); A=R; }};
	r[0xc0]={0xc0, "sub ", "subb", "im", 0, 0, 2,  2, [this]{ immed2  (); R=B-OP; S8(B,OP,R); B=R; }};
	r[0xd0]={0xd0, "sub ", "subb", "di", 1, 0, 2,  3, [this]{ direct2 (); R=B-OP; S8(B,OP,R); B=R; }};
	r[0xe0]={0xe0, "sub ", "subb", "in", 1, 0, 2,  4, [this]{ index2  (); R=B-OP; S8(B,OP,R); B=R; }};
	r[0xf0]={0xf0, "sub ", "subb", "ex", 1, 0, 3,  4, [this]{ extend  (); R=B-OP; S8(B,OP,R); B=R; }};
	r[0x83]={0x83, "sub ", "subd", "im", 0, 0, 3,  3, [this]{ immed3  (); R2=D-OP2; S16(D,OP2,R2); D=R2; }};
	r[0x93]={0x93, "sub ", "subd", "di", 1, 0, 2,  4, [this]{ direct16(); R2=D-OP2; S16(D,OP2,R2); D=R2; }};
	r[0xa3]={0xa3, "sub ", "subd", "in", 1, 0, 2,  5, [this]{ index16 (); R2=D-OP2; S16(D,OP2,R2); D=R2; }};
	r[0xb3]={0xb3, "sub ", "subd", "ex", 1, 0, 3,  5, [this]{ extend16(); R2=D-OP2; S16(D,OP2,R2); D=R2; }};
	r[0x3f]={0x3f, "swi ", "swi ", "id", 0, 0, 1, 12, [this]{ implied (); swi(); }};
	r[0x16]={0x16, "tfr ", "tab ", "id", 0, 0, 1,  1, [this]{ implied (); B=A; V=0; L8(B); }};
	r[0x06]={0x06, "tfr ", "tap ", "id", 0, 0, 1,  1, [this]{ implied (); setCCR(A); }};
	r[0x17]={0x17, "tfr ", "tba ", "id", 0, 0, 1,  1, [this]{ implied (); A=B; V=0; L8(A); }};
	r[0x07]={0x07, "tfr ", "tpa ", "id", 0, 0, 1,  1, [this]{ implied (); A=getCCR(); }};
	r[0x30]={0x30, "tfr ", "tsx ", "id", 0, 0, 1,  1, [this]{ implied (); IX=SP+1; }};
	r[0x35]={0x35, "tfr ", "txs ", "id", 0, 0, 1,  1, [this]{ implied (); SP=IX-1; }};
	r[0x6b]={0x6b, "tst ", "tim ", "in", 1, 0, 3,  5, [this]{ index3  (); V=0; L8(M[ADDR]&OP); }};
	r[0x7b]={0x7b, "tst ", "tim ", "di", 1, 0, 3,  4, [this]{ direct3 (); V=0; L8(M[ADDR]&OP); }};
	r[0x6d]={0x6d, "tst ", "tst ", "in", 1, 0, 2,  4, [this]{ index2  (); V=0; L8(OP); }};
	r[0x7d]={0x7d, "tst ", "tst ", "ex", 1, 0, 3,  4, [this]{ extend  (); V=0; L8(OP); }};
	r[0x4d]={0x4d, "tst ", "tsta", "id", 0, 0, 1,  1, [this]{ implied (); V=0; L8(A); }};
	r[0x5d]={0x5d, "tst ", "tstb", "id", 0, 0, 1,  1, [this]{ implied (); V=0; L8(B); }};
	r[0x1a]={0x1a, "wait", "slp ", "id", 0, 0, 1,  4, [this]{ implied (); isleep(); }};
	r[0x3e]={0x3e, "wait", "wai ", "id", 0, 0, 1,  9, [this]{ implied (); wait(); }};
}

// CCR utilities /////////////////////////////
void HD6303R::A8(uint8_t op1, uint8_t op2, uint8_t r) { // additive arithmetic 8 bit
	uint8_t c = (op1 & op2) | (op2 & ~r) | (~r & op1);
	H = c & (1<<3);
	N = r & (1<<7);
	Z = !r;
	V = ( (op1 & op2 & ~r) | (~op1 & ~op2 & r ) ) & (1<<7);
	C = c & (1<<7);
}
void HD6303R::A16(uint16_t op1, uint16_t op2, uint16_t r) { // additive arithmetic 16 bit
	N = r & (1<<15);
	Z = !r;
	V = ( (op1 & op2 & ~r) | (~op1 & ~op2 & r) ) & (1<<15);
	C = ( (op1 & op2) | (op2 & ~r) | (~r & op1) ) & (1<<15);
}
void HD6303R::S8(uint8_t op1, uint8_t op2, uint8_t r) { // subtractive arithmetic 8 bit
	N = r & (1<<7);
	Z = !r;
	V = ( (op1 & ~op2 & ~r) | (~op1 & op2 & r) ) & (1<<7);
	C = ( (~op1 & op2) | (op2 & r) | (r & ~op1) ) & (1<<7);
}
void HD6303R::S16(uint16_t op1, uint16_t op2, uint16_t r) { // subtractive arithmetic 16 bit
	N = r & (1<<15);
	Z = !r;
	V = ( (op1 & ~op2 & ~r) | (~op1 & op2 & r) ) & (1<<15);
	C = ( (~op1 & op2) | (op2 & r) | (r & ~op1) ) & (1<<15);
}
void HD6303R::L8(uint8_t r) { // logical operations 8 bit
	N = r & (1<<7);
	Z = !r;
	V = 0;
}
void HD6303R::L16(uint16_t r) { // logical operations 16 bit
	N = r & (1<<15);
	Z = !r;
	V = 0;
}
void HD6303R::D8(uint8_t r) { // decrement 8 bit
	N = r & (1<<7);
	Z = !r;
	V = (r==0x7F);
}
void HD6303R::I8(uint8_t r) { // increment 8 bit
	N = r & (1<<7);
	Z = !r;
	V = (r==0x80);
}

// Instruction helpers
void HD6303R::isleep() { halt = true; }
void HD6303R::wait() {
	halt = true;
	push(PC);
	push(IX);
	memory[SP--] = A;
	memory[SP--] = B;
	memory[SP--] = getCCR();
}

void HD6303R::bra(bool c) { if(c) PC += extend8(OP); }
void HD6303R::bsr() { push(PC); PC += extend8(OP); }

void HD6303R::asl(uint8_t &x) { // 8 bit
	C = bit(x,7);
	x <<= 1;
	N = bit(x,7);
	Z = !x;
	V = N^C;
}

void HD6303R::asld(uint16_t &x) { // 16 bit
	C = bit(x,15);
	x <<= 1;
	N = bit(x,15);
	Z = !x;
	V = N^C;
}

void HD6303R::asr(uint8_t &x) { // 8 bit
	C = bit(x,0);
	x = int8_t(x)>>1;
	N = bit(x,7);
	Z = !x;
	V = N^C;
}

void HD6303R::lsr(uint8_t &x) { // 8 bit
	C = bit(x,0);
	x >>= 1;
	N = 0;
	Z = !x;
	V = N^C;
}

void HD6303R::lsrd(uint16_t &x) { // 16 bit
	C = bit(x,0);
	x >>= 1;
	N = 0;
	Z = !x;
	V = N^C;
}

void HD6303R::rol(uint8_t &x) { // rotate left
	bool c = bit(x,7);
	x <<= 1;
	x |= C;
	C = c;
	N = 0;
	Z = !x;
	V = N^C;
}

void HD6303R::ror(uint8_t &x) { // rotate right
	bool c = bit(x,0);
	x >>= 1;
	x |= (C<<7);
	C = c;
	N = 0;
	Z = !x;
	V = N^C;
}

void HD6303R::pull(uint16_t &x) {
	x = memory[++SP] << 8;
	x |= memory[++SP];
}

void HD6303R::rti() {
	setCCR(memory[++SP]);
	B = memory[++SP];
	A = memory[++SP];
	pull(IX);
	pull(PC);
}

void HD6303R::daa() {
	uint8_t c = 0;
	uint8_t lsn = (A & 0x0F);
	uint8_t msn = (A & 0xF0) >> 4;
	if (H || (lsn > 9)) c |= 0x06;
	if (C || (msn > 9) || ((msn > 8) && (lsn > 9))) c |= 0x60;
	uint16_t t = uint16_t(A) + c;
	C |= bit(t, 8);
	A = uint8_t(t);
	N = bit(A, 7);
	Z = !A;
}


// Addressing modes /////////////////////////

void HD6303R::direct2() {
	ADDR = memory[PC++];
	OP = memory[ADDR];
}

void HD6303R::direct16() {
	ADDR = memory[PC++];
	OP2 = getm16(ADDR);
}

void HD6303R::direct3() { // AIM
	OP = memory[PC++];
	ADDR = memory[PC++];
}

void HD6303R::extend() {
	ADDR = memory[PC++]<<8;
	ADDR |= memory[PC++];
	OP = memory[ADDR];
}

void HD6303R::extend16() {
	ADDR = memory[PC++]<<8;
	ADDR |= memory[PC++];
	OP2 = getm16(ADDR);
}


void HD6303R::immed2() {
	OP = memory[PC++];
}

void HD6303R::immed3() {
	OP2 = memory[PC++]<<8;
	OP2 |= memory[PC++];
}

void HD6303R::implied() { } 

void HD6303R::index2() {
	ADDR = IX + memory[PC++]; // Unsigned add
	OP = memory[ADDR];
}

void HD6303R::index16() {
	ADDR = IX + memory[PC++]; // Unsigned add
	OP2 = getm16(ADDR);
}
void HD6303R::index3() { // AIM
	OP = memory[ PC++ ];
	ADDR = IX + memory[PC++];
}

