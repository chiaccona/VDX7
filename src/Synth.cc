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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <unistd.h>

#include "Synth.h"

DX7Synth::DX7Synth(const char* rf) : dx7(toSynth, toGui, rf) {

	setMidiVelocity(0.4);

	// Set up libsamplerate converter callback
	int error;
	if (!(src_state = src_callback_new(
			DX7Synth::fillCallback,
			// FIX make this selectable
			//SRC_SINC_MEDIUM_QUALITY, 1,
			SRC_SINC_FASTEST, 1,
			&error,
			(void*)this))) {
		fprintf(stderr, "src_callback_new failed: %s\n", src_strerror (error));
		throw("libsamplerate");
	}
}

void DX7Synth::setSampleRate(double fs) {
fprintf(stderr, "Sample Rate = %.0f\n", fs);
	FS = fs;
	ratio = FS/49096.0;
	// DX7's master clock rate
	cpuCyclesPerBuf = BufSize * (((9.4265e6  / 2) / 4) / FS);
	fprintf(stderr, "cpuCyclesPerBuf = %f\n", cpuCyclesPerBuf);
	dx7.midiFilter.set_f(10.6/fs);
}

// Process event messages, handing off to CPU
void DX7Synth::processMessage(Message msg) {
	switch(Message::CtrlID(msg.byte1)) {
		case Message::CtrlID::volume: // Volume control slider
			volume = pow(2, msg.byte2/127.0) - 1.0;
			break;

		case Message::CtrlID::sustain: // Sustain pedal
			dx7.sustain(msg.byte2);
			break;

		case Message::CtrlID::porta: // Portamento pedal
			dx7.porta(msg.byte2);
			break;

		case Message::CtrlID::cartridge: // Cartridge
			// Cartridge present (clear bit 5 of P_CRT_PEDALS_LCD)
			if(msg.byte2) dx7.cartPresent(true);
				else  dx7.cartPresent(false);
			break;

		case Message::CtrlID::cartridge_file: { // Cartridge filename
				// FIX max path length > 256
				int len = msg.byte2;
				char filename[len+1];
				toSynth->getBinary((uint8_t*)filename, len);
				filename[len] = 0;
				dx7.cartLoad(filename);
			}
			break;

		case Message::CtrlID::cartridge_num: // GUI requested factory cart load
			dx7.setBank(msg.byte2, true);
			break;

		case Message::CtrlID::protect: // Cartridge protect
			// Cartridge protected (set bit 6 P_CRT_PEDALS_LCD)
			if(msg.byte2) dx7.cartWriteProtect(true);
				else  dx7.cartWriteProtect(false);
			break;

		case Message::CtrlID::send_state: { // GUI requested to resend display state 
				// Send LCD state to GUI
				const uint8_t *state=0;
				uint8_t len = dx7.lcd.save(state);
				toGui->lcd_state(state, len);
			}
			// Send LEDs
			toGui->led1_setval(dx7.P_LED1);
			toGui->led2_setval(dx7.P_LED2);
			// Send cartridge state
			if(dx7.cartNum>=0) toGui->cartridge_num(dx7.cartNum);
			else if (!dx7.cartFile.empty()) toGui->cartridge_name((uint8_t*)dx7.cartFile.c_str(), dx7.cartFile.size());
			break;

		// Handoff event to CPU
		default:
			// DX7's internal velocity is inverted, since it counts the
			// time from contact break to make. The internal keyboard must
			// have a maximum of velocity 1, because 0 is interpreted as
			// note off. The velocity curve is applied in parseMIDI() (except in
			// raw MIDI mode, where the firmware applies a curve).
			// msg.byte1>158 is a key event.
			if(msg.byte1>158 && msg.byte2!=0) msg.byte2 = 128 - msg.byte2;
			dx7.msg = msg;
			dx7.haveMsg = true;
			break;
	}
//fprintf(stderr, "msg: %02X(%d) %02X(%d)\n", msg.byte1, msg.byte1, msg.byte2, msg.byte2);
}

int DX7Synth::fillBuffer() {
	// Sync DX7 CPU clock to Jack sampling rate
	// cpuCyclesPerBuf := BufSize * (((9.4265Mhz / 2) / 4) / SampleRate) 
	// E.g. for 48khz and 128byte buffer, run for minimum 3142 cycles
	// at 0.8486713 usec/cycle = 2.667 msec CPU time
	// Generate 2 voice subsamples every 3 CPU cycles
	// 16 voice subsamples mix down to 1 output sample
	// Native DX7 SR is 49,096.354 samp/sec (9.4265e6/2/4)*(2/3)/16
	// So 130.9236 samples would be generated per 128 sample Jack buffer,
	// hence need to rate adapt to match Jack sample rate
	cyc_count += cpuCyclesPerBuf;
	int outCnt = 0;
	Message msg;
	while(cyc_count > 0) {
		// Process messages
		if(!dx7.haveMsg) // CPU is ready
			if(toSynth->pop(msg)) processMessage(msg);

		// Run one instruction
		dx7.run();

		// Clock the EGS and OPS. Each CPU cycle is 4 master clock ticks
		// FIX BufSize
		if(outCnt<2*BufSize) dx7.egs.clock(buffer, outCnt, 4*dx7.inst->cycles);

		cyc_count -= dx7.inst->cycles;
	}
	return outCnt; 
}

// Produces one BufSize buffer of output
void DX7Synth::run() {
	// Audio
	int rc = src_callback_read(src_state, ratio, BufSize, outputBuffer);
	if (rc < BufSize) {
		fprintf(stderr, "src_callback_read: short output (%d != %d)\n", rc, BufSize);
		return;
	}

	// MIDI volume is filtered in hardware by a 10hz lowpass smoother,
	// but still quantized to only 8 levels.
	// Controller 11 midiExpression is quantized to 128 levels
	// 1e-18 is for denorm protection
	float mv = dx7.midiVolTab[dx7.midiVolume] + midiExpression + 1e-18;
	if(mv > 1.0) mv = 1.0;
	for(int i=0; i<BufSize; i++) {
		outputBuffer[i] *= volume * dx7.midiFilter.operate(mv);
	}
}

// Implement MIDI directly through sub-CPU handoff
// Return true if message was processed here, otherwise false
// forwards it on to the CPU serial interface.
//
// Note that the DX7 can only input MIDI at 31.25khz, so it's possible
// a flurry of messages (e.g. from USB or a sequencer) could
// overflow the midiSerialRx buffer and drop messages (a warning
// is printed on stderr).
bool DX7Synth::parseMIDI(const uint32_t size, const uint8_t *const buffer) {
	if(size < 1 || size > 3) return false;
	char chan = buffer[0] & 0xF;
	if(chan != dx7.getMidiRxChannel()) return false;
	switch(buffer[0]&0xF0) {
	case 0x80: if(buffer[1]>=36) toSynth->key_off(buffer[1]-36); return true;
	case 0x90: if(buffer[1]>=36) toSynth->key_on(buffer[1]-36, midiVelocity[buffer[2]]); return true;
	case 0xB0:
		switch(buffer[1]) {
		// Ctrl 0 (bank change MSB) triggers a reset bug in the native DX7 firmware,
		// so eat it here and don't pass on to the CPU
		case 0: return true;
		// Analog controllers
		case 1: toSynth->analog(Message::CtrlID::modulate, buffer[2]); return true;
		case 2: toSynth->analog(Message::CtrlID::breath, buffer[2]); return true;
		case 4: toSynth->analog(Message::CtrlID::foot, buffer[2]); return true;
		case 6: toSynth->analog(Message::CtrlID::data, buffer[2]); return true;
		// Controller 7 is forwarded to the MIDI serial interface
		// for the DX7's original 3-bit DAC volume control
		case 7: return false;
		// Use Controller 11 as a "smoother" volume control than Controller 7
		// Note  controller 7 and 11 are additive - e.g. if one is at max, the other
		// won't appear to have any effect. For full range of controller 11, set
		// controller 7 to 0. For a swell effect, set controller 7 to, e.g., 64 (50%), and
		// controller 11 will then swell volume between 50% and 100%.
		case 11: midiExpression = buffer[2]/127.0; return true;
		// Bank change: load factory ROM cartridge (numbered 0-7)
		case 32: dx7.setBank(buffer[2]%8, true); return true;
		// Sustain and Portamento Pedals
		case 64: toSynth->analog(Message::CtrlID::sustain, buffer[2]); return true;
		case 65: toSynth->analog(Message::CtrlID::porta, buffer[2]); return true;
		case 123:// All notes off
				// Works around a DX7 firmware bug that fails to clear stuck voices
				fprintf(stderr, "all notes off\n");
				for(int i=0; i<61; i++) toSynth->key_off(i);
				return false; // also forward to serial

		// Turn on "clean" mode (no modelling of DX7 DAC)
		case 98: dx7.egs.clean(buffer[2]); printf("clean=%d\n", bool(buffer[2])); return true;

		// Otherwise pass controller on to serial interface
		default: return false;
		}
		return false; // not reached
	// MIDI Channel pressure
	case 0xD0: toSynth->analog(Message::CtrlID::aftertouch, buffer[1]); return true;
	// Bender - MSB only
	case 0xE0: toSynth->analog(Message::CtrlID::pitchbend, buffer[2]); return true;
	// Pass on to serial interface
	default: return false;
	}
	return false; // not reached
}

void DX7Synth::queueMidiRx(const uint32_t size, const uint8_t *const buffer) {

// debug - Ctrl 99 prints EGS trace
if((buffer[0]&0xF0) == 0xB0 && buffer[1]==99) { dx7.printEGS(); return; }

	if(serial || !parseMIDI(size, buffer))
		// Send MIDI to DX7 serial interface
		for(unsigned i=0; i<size; i++) dx7.midiSerialRx.write(buffer[i]);
}

void DX7Synth::queueSysEx(const uint32_t size, const uint8_t *const buffer) {
    for (uint32_t i = 0; i < size; ++i) {
        dx7.midiSerialRx.write(buffer[i]);
    }
}


// Parse MIDI stream coming from CPU into discrete messages for Jack
bool DX7Synth::queueMidiTx(uint32_t& s, uint8_t* &buffer) {

	buffer = midibuf;
	while(!dx7.midiSerialTx.empty()) {
		uint8_t byte = dx7.midiSerialTx.read();
		switch(state) {
		case Ctrl:
			switch(byte&0xF0) {
			case 0x80: // noteOff(3)
			case 0x90: // noteOn(3)
			case 0xA0: // aftertouch(3)
			case 0xB0: // controller(3)
				midibuf[0] = byte; size=3; state= Data1;
				break;

			case 0xC0: // program(2)
			case 0xD0: // chanPressure(2)
				midibuf[0] = byte; size=2; state= Data1;
				break;

			case 0xE0: // pitchbend(chan, (buf[2]<<7)|buf[1]); break;
				midibuf[0] = byte; size=3; state= Data1;
				break;

			case 0xF0: { // discard these except sysex
				switch(byte&0x0F) {
				case 0x00: {
						state = Sysex;
						size = 0;
						midibuf[size++] = byte;
					}
					break;
				case 0x02: break;//songposition
				case 0x03: break;//songselect
				case 0x06: break;//tunereq
				case 0x07: state = Ctrl; break;//eox
				case 0x08: break;//clock
				case 0x0A: break;//start
				case 0x0B: break;//stop
				case 0x0E: break;//active
				case 0x0F: break;//reset
				default: break;
				}
			}
			}
			break;

		case Data1:
			midibuf[1] = byte;
			if(size==2) {
				state = Ctrl;
				s = size;
				return true;
			} else state = Data2;
			break;

		case Data2:
			midibuf[2] = byte;
			state = Ctrl;
			s = size;
			return true;
			break;

		case Sysex:
			if(byte&0x80 && byte!=0xF7) break; // ignore RT messages
			midibuf[size++] = byte;
			if(byte==0xF7 || size>=maxSysex) {
				state = Ctrl;
				s = size;
				return true;
			}
			break;
		}
	}
	return false; // no more complete events
}

// Power law velocity curve (c<1.0 convex, c>1.0 concave, c=1.0 linear)
void DX7Synth::setMidiVelocity(float c) {
	if(c<0.25 || c>4.0) c = 1.0;
	for(int i=0; i<128; i++) midiVelocity[i] = int(127*pow(i/127.0, c) + .5);
}

void DX7Synth::setMidiVelocityPWL(uint8_t *points) {
	midiVelocity[0] = 0;
	struct Pt { int x, y; };
	auto nextpt = [&]() { return Pt{*points++, *points++}; };
	Pt p1 = nextpt();
	do {
		Pt p2 = nextpt();
		float slope = float(p2.y-p1.y)/(p2.x-p1.x);
		for(int x=p1.x; x<=p2.x; x++) midiVelocity[x] = p1.y+(x-p1.x)*slope;
		p1 = p2;
	} while(p1.x<127);
}

void DX7Synth::parseMidiVelocityArgs(char *s) {
	if(!strchr(s, ',')) { // float arg e.g. "-V 0.4"  - use power law curve
		setMidiVelocity(atof(s));
		return;
	}
	// else piecewise linear curve
	// arg format "-V x1,y1:x2,y2: ... xn,yn"
	int n, x, y;
	uint8_t v[100], *p=v;
	auto min = [](int a1, int a2) { return (a1<a2 ? a1 : a2); };
	while(sscanf(s, "%d,%d:%n", &x, &y, &n)==2) {
		*p++ = min(x,127); *p++ = min(y,127); s+=n;
		if(p-v >= 50) break;
	}
	v[0] = 1; *(p-2) = 127; // x starts at 1 and ends at 127
	setMidiVelocityPWL(v);
}

