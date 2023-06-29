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
#include <cstdio>

// Force G++ to inline EGS and OPS clock functions
#ifdef INLINE_EGS
#define INLINE __attribute__ ((always_inline))
#else
#define INLINE
#endif

#include "OPS.h"
#include "filter.h"

// Based on MSFA analysis
// Envelope is inverted - 0xFFF is "off" and 0 is maximum
// i.e. represents an attenuation

struct Envelope {
	Envelope() { }

	enum Stage { S0, S1, S2, S3 };

	uint8_t *rates=0, *levels=0;
    uint8_t *outparam=0;
	bool keyoff = false;
	uint8_t ratescaling = 0;

	// These are signed to catch
	// underflows and compares, but not strictly necessary
	// because firmware ensures target capped at 64
	// (outparam>=0x04), and max decrement is 32.
	int16_t level = 0xFF0;
	int16_t target = 0xFF0;

	bool rising = false;
	Stage stage = S3;
	bool compress = true; // flag for compressed stage 0 delay

	uint16_t *clock = 0; // One master clock for all envelopes (need to init)
	uint8_t nshift = 0, pshift = 0; // value stored as positive or negative
	uint16_t small = 0; // clock bitmask for slow rates
	uint8_t mask = 0; // flags for fractional qrate output

	void key_on() { keyoff = false; advance(); }
	void key_off() { keyoff = true; advance(); }

	void init(uint8_t *r, uint8_t *l, uint8_t *op, uint16_t *clk) {
    	rates = r; levels = l; outparam = op; clock = clk;
		key_off();
	}

	// Output is a 12 bit attenuation (i.e. inverted, 0xFFF is "zero")
	uint16_t getsample() {
		if (stage>1 && (level == target)) return level;  // short circuit if target reached
		if ((!(*clock & small)) // fall thru once every 2^nshift cycles
			&& (mask & (1<<((*clock>>nshift)&7))) // Clock out fractional qrates
			) {
			if (rising) { // "rising" down
				if (level>0x94C) level = 0x94C; // jumpstart: 4096-1716=2380=0x94C
				int slope = (level>>8) + 2; // 11 to 2 by log(level)
				level -= slope<<pshift;
				// NB level can't actually underflow due to
				// firmware capping outparam at 0x04. This (signed) code should work
				// even if outparam is not capped (the actual hardware would glitch)
				if (level <= target) { // go to next stage
					level = target;
					advance();
				}
			} else { // "falling" up 
				level += 1<<pshift;
				if (level >= target) { // go to next stage
					level = target;
					advance();
				}
			}
		}
		return level; // stage 2 sustained
	}

	void advance() {
		// Stage state machine, with delay compression logic
		switch(stage) {
			case S0:
				if(keyoff) { stage = S3; compress = true; }
				else { stage = S1; compress = false; }
				break;
			case S1: if(keyoff) stage = S3; else stage = S2; break;
			case S2: if(keyoff) stage = S3; else return; break;
			case S3: if(!keyoff) stage = S0; else return; break;
		}
		updateLevel();
		updateRate();
	}
	void updateLevel() {
		// max target is 0x040 (with outparam=0x04, levels[stage]=0x00)
		target = (levels[stage]<<6) + ((*outparam)<<4);
		if(target>0xFF0) target = 0xFF0; // Minimum level is 4096-16

		// Set up delay when env level and prev level >= 40 (i.e. env
		// level param <20).  Small levels are inaudible, hence
		// experienced as a delay. Magic delay constant is 479 decaying
		// envelope steps, which, in conjunction with qrate, creates
		// the required delay time.
		// FIX: speculative implementation of "bug" behavior (if my
		// speculation is correct this would actually have to have been designed
		// behavior in the DX7). Delays when in stage 0 are compressed (shortened)
		// by a factor of 32 if the previous stage 0 delay did not fully
		// complete, otherwise it reverts to the full length delay.
		// Stage 1 delays are always full length. No delays in stage 2 and 3
		// (because because they only exit on key events). Behavior is
		// used e.g. in "WATER GDN" and "CHIMES" voices.

		int prev = (stage+3)&3;
		if(stage<2 && levels[stage] >= 40 && levels[prev] >= 40) {
			// compressed delay is round(479/32) = 15
			int delay = (stage==0 && compress) ? 15 : 479;
			target = level + delay;
			if(target>0xFF0) { // clamp
				level = 0xFF0-delay;
				target = 0xFF0;
			}
		}
		rising = (target < level); // attack mode: actually falling, since inverted
	}

	void updateRate() {
		uint8_t qrate = rates[stage]+ratescaling;
		if(qrate>63) qrate = 63;

		// nshift: 0-3=11, 4-7=10 ... 40-43=1, 44-63=0
		// pshift: 0-47=0, 48-51=1, 52-55=2, 56-59=3, 60-63=4
		pshift = nshift = small = 0;
		if(qrate<44) {
			nshift = 11 - (qrate>>2); // clock shift value
			small = (1<<nshift) - 1; // bitmask to detect shifted clock transition
		} else pshift = (qrate>>2) - 11; // increment shift value
		
		mask = outmask[qrate&3]; // fractional qrate flags
	}

	// Bit patterns indicating when to clock out samples
	// when qrate is not divisible by 4
	// (little end first)
	static constexpr const uint8_t outmask[4] = { 0xAA, 0xEA, 0xEE, 0xFE };
};

class EGS {
public:
	EGS(uint8_t *m) :
		mem(m),
		opDetune(m+0x30),
		opEGrates((uint8_t(*)[4])(m+0x40)),
		opEGlevels((uint8_t(*)[4])(m+0x60)),
		opLevels((uint8_t(*)[16])(m+0x80)),
		opSensScale(m+0xE0),
		ampMod(*(m+0xF0)),
		voiceEvents(*(m+0xF1)),
		ops(frequency, envelope)
	{
		for(int i=0; i<256; i++) mem[i] = 0xFF; // Initially all ones
		for(int op=0; op<6; op++) {
			for(int voice=0; voice<16; voice++) { // initialize pointers in envelopes
				env[op][voice].init(opEGrates[op], opEGlevels[op], &(opLevels[op][voice]), &env_clock);
			}
		}
	}

private:
	uint8_t *mem; // Pointer to start of EGS buffers at cpu.memory[0x3000]

	// EGS registers
	uint16_t voicePitch[16]={0};
	uint16_t opPitch[6]={0};
	uint8_t *opDetune;			// opDetune[6]
	uint8_t (*opEGrates)[4];	// opEGrates[6][4]
	uint8_t (*opEGlevels)[4];	// opEGlevels[6][4]
	uint8_t (*opLevels)[16];	// opLevels[6][16]
	uint8_t *opSensScale;		// opSensScale[6]
	uint8_t &ampMod;
	uint8_t &voiceEvents;
	int16_t pitchMod=0;

	// Envelopes
	Envelope env[6][16];
	int currOp=0, currVoice=0;
	uint16_t env_clock = 0; // Master envelope clock

	// Outputs to OPS
	uint16_t frequency[6][16] = {0};
	uint16_t envelope[6][16] = {0};

	// OPS chip
	OPS ops;

	// 5th order decimation filter (Sallen-Key)
	// Introduces a bit of aliasing noise consistent with hardware synth
	Filter skFilter;
	float filter(int32_t *out) {
		float ret = 0;
		// Gain trim for 15 bit full scale
		constexpr const float cgain = 1.0/float(1<<15);
		constexpr const float gain = 16.0/float(1<<15);
		// Optionally no filtering or decimation
		// Otherwise, apply S-K filter as in hardware
		if(clean_) for(int v=0; v<16; v++) ret += cgain * out[v];
		else for(int v=0; v<16; v++) ret = skFilter.operate(gain * out[v]);
		return ret;
	}

	bool clean_ = false;

public:
	// Optionally allow full resolution output and no filtering,
	// removing the "dirty" signal processing, i.e.
	// the S-K filtering, and the level shifter circuitry
	void clean(bool v) { clean_ = v; ops.clean(v); }//fprintf(stderr, "clean(%d)\n", v); }

	// CPU to OPS interface
	void setAlgorithm(uint8_t mode, uint8_t algo) { ops.setAlgorithm(mode, algo); }

	// Each clock tick computes one operator for one voice
	// 6x16=96 ticks generates one audio output sample
	// at DX7 native SR 49.096khz
	// Returns output samples in outbuf and increments buffer index count
	void INLINE clock(float* outbuf, int &count, int cycles) {
		for(int i=0; i<cycles; i++) {

			// Advance envelope
			uint16_t e = env[currOp][currVoice].getsample();

			// Amplitude modulation
			int ampModSens = (opSensScale[currOp]>>3);

			// Based on comparison to an audio track in Massey's book,
			// amp mode sensitivity shifts the ampmod value as follows:
			if(ampModSens) e += ampMod<<ampModSens; // No modulation when ampModSens==0

			if(e>0xFFF) e = 0xFFF;
			envelope[currOp][currVoice] = e;

			// Run OPS
			ops.clock(currOp, currVoice);

			// Increment voice and op
			if(++currVoice == 16) {
				currVoice = 0;
				if(++currOp == 6) {
					// Output after all 16x6 ops are computed
					// Apply S-K filter and decimate
					outbuf[count++] = filter(ops.out);
					currOp = 0;
					env_clock++; // increment envelope clock
				}
			}
		}
	}

	// When CPU writes to 0x30**, Update EGS 2 byte pitch registers (need to be swabbed and atomic)
	void update(uint8_t ADDR) {
		switch(ADDR>>5) {
			case 0:
				if(ADDR&0x01) { updateVoicePitch(ADDR>>1); return; }
			case 1:
				if(ADDR&0x01) { updateOpPitch((ADDR&0x0F)>>1); return; }
				return; // also catches detune
			case 2:
				if((ADDR>=0x40) && (ADDR<0x57)) {
					if((ADDR&0x03)!=3) return; // only update on fourth rate
					uint8_t op = (ADDR - 0x40)>>2;
					for(int voice=0; voice<16; voice++) env[op][voice].updateRate();
				}
				return;
			case 3:
				if((ADDR>=0x60) && (ADDR<0x77)) {
					if((ADDR&0x03)!=3) return; // only update on fourth level
					uint8_t op = (ADDR - 0x60)>>2;
					for(int voice=0; voice<16; voice++) env[op][voice].updateLevel();
				}
				return;
			case 4:
			case 5:
			case 6:
				return;
			case 7:
				if(ADDR>=0xE0 && ADDR <=0xE5) {
					uint8_t op = ADDR-0xE0;
					for(int voice=0; voice<16; voice++) updateRateScaling(voice, op);
				}
				else if(ADDR==0xF3) { updatePitchMod(); return; }
				else if(ADDR==0xF1) { updateVoiceEvents(); return; }
				return;
		}
	}

private:
	// Update key events
	void updateVoiceEvents() {
		uint8_t voice = voiceEvents>>2;
		bool keyon = voiceEvents&1; // bit 0 is keyon, bit 1 is keyoff
		for(int op=0; op<6; op++) {
			if(keyon) env[op][voice].key_on();
			else env[op][voice].key_off();
		}
		if(keyon) ops.keyOn(voice);
	}

	void updateRateScaling(uint8_t voice, uint8_t op) {
		// Rate Scaling
		// Per the (linear) graph in the user manual:
		//                  rateScaling:  7  6  5  4  3  2  1  0
		//  A(-1) (midi  21, 27.5Hz)  ->  0  0  0  0  0  0  0  0 
		//  F#(7) (midi 102, 2960Hz)  -> 42 36 30 24 18 12  6  0
		//  (in 0-99 rate units, with rate=qrate*64/41
		// The function below replicates this table in qrate units
		// using note frequencies in voicePitch units.
		// In hardware this is surely a ROM lookup table
		int rateScaling = opSensScale[op]&0x7;
		uint16_t rs = int (min(27,max(0,(voicePitch[voice]>>8)-16))*rateScaling/7.0 + 0.5);
		env[op][voice].ratescaling = rs;
	}

	// Update voice pitch registers on CPU write
	void updateVoicePitch(uint8_t voice) {
		voicePitch[voice] = (mem[2*voice]<<8 | mem[2*voice+1])>>2;
		updateFrequency(voice);
		for(int op=0; op<6; op++) updateRateScaling(voice, op);
	}

	// Update op pitch registers on CPU write
	void updateOpPitch(uint8_t op) { opPitch[op] = mem[0x20+2*op]<<8 | mem[0x20+2*op+1]; }

	// Pitchmod is 4x voice (16x 14 bit voice (>>2))
	// Strictly speaking the uint16 to int16 conversion is UB
	// before C++20, but works fine on all common machines
	void updatePitchMod() {
		pitchMod = int16_t(mem[0xF2]<<8 | mem[0xF3])/16;
		for(int voice=0; voice<16; voice++) updateFrequency(voice);
	}

	// Update frequency (pitch) for OPS
	void updateFrequency(uint8_t voice) {
		for(int op=0; op<6; op++) {
			int32_t f = opPitch[op]>>2;
			f += opDetune[op]&0x8 ? -int16_t(opDetune[op]&0x7) : opDetune[op];
			if(!(opPitch[op]&1)) { // Ratio
				f += voicePitch[voice];
				f += pitchMod;
			} // else Fixed
			// Clamp
			if(f>0x3FFF) f=0x3FFF; else if(f<0) f=0;
			frequency[op][voice] = f;
		}
	}

	// Utilities
	int max(int x, int y) { return x>y ? x : y; }
	int min(int x, int y) { return x<y ? x : y; }
};
