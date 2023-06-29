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

#pragma once
#include <cstdint>
#include <cmath>

// Struct to track a sign bit, separate from logsin value
struct logsin_t  {
	logsin_t() = default;
	logsin_t(uint16_t v) { val = v; }
	operator uint16_t&() { return val; }
	uint16_t val=0;
	bool sign=false;
};

// In: 12 bit phase. Out: logsin_t with 14 bit (4.10) value and separate sign bit
struct SinTab {
	static const int LG_N = 10;
	static const int N = 1<<LG_N;
	SinTab() {
		for(int i=0; i<N; i++) {
			table[i] = int(.5002-N*log(sin((i+.5)/N*M_PI/2))/log(2));
		}
	}
	logsin_t operator()(uint32_t v) const {
		// If bit 11 set, invert phase, then truncate
		logsin_t s = table[ ((v&(1<<LG_N)) ? ~v : v) & (N-1) ];
		s.sign = v&(1<<(LG_N+1)); // if bit 12 set, sign is negative
		return s;
	}
	uint16_t table[N];
};

// In: 14 bit Q4.10. Out: 2^x, 14 bit (signal) or 22 bit (frequency)
// Output range 64 to 4,191,232 (22 bits), and 0 to 16,372 (14 bit)
// Expanding the table to the full 14 bits shaves a few cycles
// by precalculating the mask/shift of the int and fract parts:
// i.e. table[x] rather than table[ x & 0x3FF ] << ((x>>10) & 0xF));
struct ExpTab {
	ExpTab() {
		for(int i=0; i<16*N; i++) { // Save 22 bits
			table[i] = (uint32_t(.5+2*N*pow(2, double(i&(N-1))/N)) << ((i>>LG_N)&0xF)) >> 5;
		}
	}
	uint32_t get22(uint32_t v) const { return table[v]; }
	uint32_t get14(uint32_t v) const { return get22(v)>>8; }

	// Return full 15-bit resolution
	int32_t invertLogSinClean(logsin_t ls) const { return ls.sign ? -get14(ls) : get14(ls); }

	// Return 12-bit + shift resolution
	int32_t invertLogSin(logsin_t ls) const { return ls.sign ? -shift(get14(ls)) : shift(get14(ls)); }
	uint32_t shift(uint32_t v) const {
		if(v&0xFFFFF000) v &= ~0b111; // <= 3 leading 0's, lose 3 bits (shift 0)
		else if(v&0xFFFFF800) v &= ~0b11; // 4 leading 0's, lose 2 bits (shift 1)
		else if(v&0xFFFFFC00) v &= ~0b1; // 5 leading 0's, lose 1 bit (shift 2)
		// else full resolution (shift 3)
		return v;
	}

	static const int LG_N = 10; // 10 bit fraction
	static const int N = 1<<LG_N; // 1024
	uint32_t table[16*N]; // 16384 = (1<<14)
};

class OPS {
public:
	OPS(uint16_t (*f)[16], uint16_t (*e)[16])
		: frequency(f), envelope(e)
		{ }

	// Output with voice order shuffled as per array
	int32_t  out[16] = {0};

private:
	// Hardware registers
	uint32_t phase[6][16] = {0};
	int32_t  fren1[16] = {0};
	int32_t  fren2[16] = {0};
	int32_t  mren[16] = {0};

	// Output ordering of voices (this may have some subtle affect on aliasing)
	// Shirriff's analysis:
	//int order[16] = { 0, 12, 4, 10, 2, 14, 6,  9, 1, 13, 5, 11, 3, 15, 7,  8 };
	// Yamaha's "DX7 Technical Analysis" document:
	int order[16] = { 0,  8, 4, 12, 2, 10, 6, 14, 1,  9, 5, 13, 3, 11, 7, 15 };

	// Saved state
	int32_t  modout[16] = {0};
	int32_t  signal[16] = {0};
	uint8_t  com[16] = {0};

	uint16_t comtab[6] = { // log2(i+1) in 4.10 format
		0b00000<<7, 0b01000<<7, 0b01101<<7,
		0b10000<<7, 0b10011<<7, 0b10101<<7
	};

	// ROM lookup tables
	static const SinTab sintab;
	static const ExpTab exptab;

	// The algorithm ROM
	// SEL(1) / FREN MREN (3) / COM  (1)
	enum SEL { SEL0, SEL1, SEL2, SEL3, SEL4, SEL5 };
	struct algoROM_t { SEL sel; bool A, C, D; uint8_t COM; };
	static const algoROM_t algoROM[32][6];


public:
	// CPU interface
	// byte 1: mute|clear sync|set sync|reg select(5), CPU memory mapped at 0x2804
	// byte 2: | algorithm (5) | feedback level (3), CPU memory mapped at 0x2805
	void setAlgorithm(uint8_t byte1, uint8_t byte2) {
		if(!(byte1&(1<<7)) && !(byte1&(1<<2))) {
			if(byte1&(1<<6)) keySync = false;
				else if(byte1&(1<<5)) keySync = true;
			if(byte1&(1<<4)) {
				for(int i=0; i<16; i++) {
					algorithm[i] = byte2>>3;
					feedback[i] = byte2&0x7;
				}
			} else {
				algorithm[byte1&0xF] = byte2>>3;
				feedback[byte1&0xF] = byte2&0x7;
			}
		}
		// Ignore test modes
	}
private:
	uint8_t  algorithm[16] = {0};
	uint8_t  feedback[16] = {0};
	bool     keySync = false;

	// EGS interface: 6 Ops x 16 Voices
	// 14 bit log frequency (pitch)
	// 12 bit log envelope
	uint16_t (*frequency)[16];	// frequency[6][16]
	uint16_t (*envelope)[16];	// envelope[6][16]

	bool clean_ = false;

public:
	// Optionally produce full resolution output
	void clean(bool v) { clean_ = v; }

	void keyOn(int n) { // Key sync resets phase
		if(keySync) for(int i=0; i<6; i++) phase[i][n] = 0;
	}

	// Compute one master clock cycle
	// Note alternatively implementing an array of 16 voice objects, rather
	// than indexing arrays of variables by voice as done here, would look
	// much more tidy, but is in fact about 15% slower on X86, presumably
	// because of cache traffic.
	// Marked to force inlining on GCC because it also turns out to be faster
	// (but in general it's not such a good idea to try to outwit a compiler).
	void INLINE clock(int op, int voice) {
////////////////////////////////////////////////////////////////////////
////////////////// Set up operator block ///////////////////////////////
////////////////////////////////////////////////////////////////////////

		// Index phase
		uint32_t phi = phase[op][voice];

		// Advance phase
		phase[op][voice] += exptab.get22(frequency[op][voice]);
		phase[op][voice] &= (1<<23)-1;

		phi >>= 11; // drop low order bits

		// Add modulation (overflow will wrap)
		phi += modout[voice];

		// Look up logsin
		logsin_t logsin = sintab(phi);

		// Add envelope
		// The 12-bit envelope needs shifted up 2 bits
		// in order to correctly zero out the 14-bit
		// logsin. The OPS must be doing this??
		logsin += envelope[op][voice]<<2;

		// Add COM (adds attenuation)
		logsin += comtab[com[voice]];

		// Clamp overflows, and complement
		if(logsin&0x4000) logsin = 0x3FFF;
		logsin ^= 0x3FFF;

		// Invert log, computing next signal
		if(clean_) signal[voice] = exptab.invertLogSinClean(logsin);
			else signal[voice] = exptab.invertLogSin(logsin);

////////////////////////////////////////////////////////////////////////
////////////////// Compute next modulation /////////////////////////////
////////////////////////////////////////////////////////////////////////

		// Get algorithm for this voice and op
		// Could optimize out voice lookup for DX7 since all 16 are always the same
		const algoROM_t& algo = algoROM[ algorithm[voice] ][op];

		int32_t msum = 0;
		if(algo.C) msum += mren[voice];
		if(algo.D) msum += signal[voice];

		// Run algorithm. This should optimize to a jump table
		switch(algo.sel) {
			case SEL0: modout[voice] = 0; break;
			case SEL1: modout[voice] = signal[voice]; break;
			case SEL2: modout[voice] = msum; break;
			case SEL3: modout[voice] = mren[voice]; break;
			case SEL4: modout[voice] = fren1[voice]; break;
			case SEL5: modout[voice] = (fren1[voice]+fren2[voice])>>(1+(7-feedback[voice])); break;
		}
		mren[voice] = msum;
		if(algo.A) {
			fren2[voice] = fren1[voice];
			fren1[voice] = signal[voice];
		}
		com[voice] = algo.COM;

		// Output
		if(op==5) out[order[voice]] = mren[voice];
	}
};

inline const SinTab OPS::sintab;
inline const ExpTab OPS::exptab;

// Algorithm ROM definitions
inline const OPS::algoROM_t OPS::algoROM[32][6] = {
// Signals: { SEL, A, C, D, COM }
//	OP:     6               5               4               3               2               1
	{{SEL1,1,0,0,0}, {SEL1,0,0,0,0}, {SEL1,0,0,0,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 1 
	{{SEL1,0,0,0,0}, {SEL1,0,0,0,0}, {SEL1,0,0,0,1}, {SEL5,0,0,1,0}, {SEL1,1,1,0,1}, {SEL0,0,1,1,0}}, // 2 
	{{SEL1,1,0,0,0}, {SEL1,0,0,0,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 3 
	{{SEL1,0,0,0,0}, {SEL1,0,0,0,1}, {SEL0,1,0,1,0}, {SEL1,0,1,0,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 4 
	{{SEL1,1,0,0,2}, {SEL0,0,0,1,0}, {SEL1,0,1,0,2}, {SEL0,0,1,1,0}, {SEL1,0,1,0,2}, {SEL5,0,1,1,0}}, // 5 
	{{SEL1,0,0,0,2}, {SEL0,1,0,1,0}, {SEL1,0,1,0,2}, {SEL0,0,1,1,0}, {SEL1,0,1,0,2}, {SEL5,0,1,1,0}}, // 6 
	{{SEL1,1,0,0,0}, {SEL0,0,0,1,0}, {SEL2,0,1,1,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 7 
	{{SEL1,0,0,0,0}, {SEL5,0,0,1,0}, {SEL2,1,1,1,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,1}, {SEL0,0,1,1,0}}, // 8 
	{{SEL1,0,0,0,0}, {SEL0,0,0,1,0}, {SEL2,0,1,1,1}, {SEL5,0,0,1,0}, {SEL1,1,1,0,1}, {SEL0,0,1,1,0}}, // 9 
	{{SEL0,0,0,1,0}, {SEL2,0,1,1,1}, {SEL5,0,0,1,0}, {SEL1,1,1,0,0}, {SEL1,0,1,0,1}, {SEL0,0,1,1,0}}, // 10 
	{{SEL0,1,0,1,0}, {SEL2,0,1,1,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 11 
	{{SEL0,0,0,1,0}, {SEL0,0,1,1,0}, {SEL2,0,1,1,1}, {SEL5,0,0,1,0}, {SEL1,1,1,0,1}, {SEL0,0,1,1,0}}, // 12 
	{{SEL0,1,0,1,0}, {SEL0,0,1,1,0}, {SEL2,0,1,1,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 13 
	{{SEL0,1,0,1,0}, {SEL2,0,1,1,0}, {SEL1,0,0,0,1}, {SEL0,0,0,1,0}, {SEL1,0,1,0,1}, {SEL5,0,1,1,0}}, // 14 
	{{SEL0,0,0,1,0}, {SEL2,0,1,1,0}, {SEL1,0,0,0,1}, {SEL5,0,0,1,0}, {SEL1,1,1,0,1}, {SEL0,0,1,1,0}}, // 15 
	{{SEL1,1,0,0,0}, {SEL0,0,0,1,0}, {SEL1,0,1,0,0}, {SEL0,0,1,1,0}, {SEL2,0,1,1,0}, {SEL5,0,0,1,0}}, // 16 
	{{SEL1,0,0,0,0}, {SEL0,0,0,1,0}, {SEL1,0,1,0,0}, {SEL5,0,1,1,0}, {SEL2,1,1,1,0}, {SEL0,0,0,1,0}}, // 17 
	{{SEL1,0,0,0,0}, {SEL1,0,0,0,0}, {SEL5,0,0,1,0}, {SEL0,1,1,1,0}, {SEL2,0,1,1,0}, {SEL0,0,0,1,0}}, // 18 
	{{SEL1,1,0,0,2}, {SEL4,0,0,1,2}, {SEL0,0,1,1,0}, {SEL1,0,1,0,0}, {SEL1,0,1,0,2}, {SEL5,0,1,1,0}}, // 19 
	{{SEL0,0,0,1,0}, {SEL2,0,1,1,2}, {SEL5,0,0,1,0}, {SEL1,1,1,0,2}, {SEL4,0,1,1,2}, {SEL0,0,1,1,0}}, // 20 
	{{SEL1,0,0,1,3}, {SEL3,0,0,1,3}, {SEL5,0,1,1,0}, {SEL1,1,1,0,3}, {SEL4,0,1,1,3}, {SEL0,0,1,1,0}}, // 21 
	{{SEL1,1,0,0,3}, {SEL4,0,0,1,3}, {SEL4,0,1,1,3}, {SEL0,0,1,1,0}, {SEL1,0,1,0,3}, {SEL5,0,1,1,0}}, // 22 
	{{SEL1,1,0,0,3}, {SEL4,0,0,1,3}, {SEL0,0,1,1,0}, {SEL1,0,1,0,3}, {SEL0,0,1,1,3}, {SEL5,0,1,1,0}}, // 23 
	{{SEL1,1,0,0,4}, {SEL4,0,0,1,4}, {SEL4,0,1,1,4}, {SEL0,0,1,1,4}, {SEL0,0,1,1,4}, {SEL5,0,1,1,0}}, // 24 
	{{SEL1,1,0,0,4}, {SEL4,0,0,1,4}, {SEL0,0,1,1,4}, {SEL0,0,1,1,4}, {SEL0,0,1,1,4}, {SEL5,0,1,1,0}}, // 25 
	{{SEL0,1,0,1,0}, {SEL2,0,1,1,2}, {SEL0,0,0,1,0}, {SEL1,0,1,0,2}, {SEL0,0,1,1,2}, {SEL5,0,1,1,0}}, // 26 
	{{SEL0,0,0,1,0}, {SEL2,0,1,1,2}, {SEL5,0,0,1,0}, {SEL1,1,1,0,2}, {SEL0,0,1,1,2}, {SEL0,0,1,1,0}}, // 27 
	{{SEL5,0,0,1,0}, {SEL1,1,1,0,0}, {SEL1,0,1,0,2}, {SEL0,0,1,1,0}, {SEL1,0,1,0,2}, {SEL0,0,1,1,2}}, // 28 
	{{SEL1,1,0,0,3}, {SEL0,0,0,1,0}, {SEL1,0,1,0,3}, {SEL0,0,1,1,3}, {SEL0,0,1,1,3}, {SEL5,0,1,1,0}}, // 29 
	{{SEL5,0,0,1,0}, {SEL1,1,1,0,0}, {SEL1,0,1,0,3}, {SEL0,0,1,1,3}, {SEL0,0,1,1,3}, {SEL0,0,1,1,3}}, // 30 
	{{SEL1,1,0,0,4}, {SEL0,0,0,1,4}, {SEL0,0,1,1,4}, {SEL0,0,1,1,4}, {SEL0,0,1,1,4}, {SEL5,0,1,1,0}}, // 31 
	{{SEL0,1,0,1,5}, {SEL0,0,1,1,5}, {SEL0,0,1,1,5}, {SEL0,0,1,1,5}, {SEL0,0,1,1,5}, {SEL5,0,1,1,5}}, // 32 
};

