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

#include <cstdio>
#include <cstdint>
#include <samplerate.h>

#include "Message.h"
#include "dx7.h"

class Synth {
public:
	// Audio buffer size
	static constexpr const int BufSize = 128;
	Synth() { }
	~Synth() { }

	virtual void setSampleRate(double fs) = 0; // SR
	virtual void run() = 0; // Generate audio to output buffers

	// Midi I/O
	virtual void queueMidiRx(const uint32_t size, const uint8_t *const buffer) {}
	virtual void queueSysEx(const uint32_t size, const uint8_t *const buffer) {}
	virtual bool queueMidiTx(uint32_t& size, uint8_t* &buffer) { return false; }

	float outputBuffer[Synth::BufSize]; // Audio buffer
};

class DX7Synth : public Synth {
public:
	DX7Synth(const char* rf=0);
	virtual ~DX7Synth() { }

	DX7 dx7; // The hardware emulator

	double FS = 48000.0;
	double ratio = 48000.0/49096.0;
	double cpuCyclesPerBuf = 0, cyc_count = 0;

	virtual void setSampleRate(double fs);
	virtual void run();
	void start() { dx7.start(); }

	// Audio buffer output from DX7 at native SR
	// FIX size this properly, 2x "should be" enough
	float buffer[2*BufSize] = {0};
	int fillBuffer(); // DX7 audio generator
	void processMessage(Message msg); // Hand off events to DX7 CPU
	SRC_STATE *src_state; // libsamplerate state variable

	// Communication interfaces (Lock-Free Queues)
	ToSynth *toSynth; // local
	ToGui *toGui; // remote

	// Master volume slider
	float volume = pow(2.0,0.75)-1.0; // = 0.68 (matches to initial GUI volume ctrl)

	// Setting of Expression pedal (MIDI controller 11)
	float midiExpression = 0.0;

	// MIDI velocity curve
	uint8_t midiVelocity[128] = { 0 };
	void parseMidiVelocityArgs(char *s);
	// Power law velocity curve (c<1.0 convex, c>1.0 concave, c=1.0 linear)
	void setMidiVelocity(float c);
	// Piecewise linear velocity curve
	void setMidiVelocityPWL(uint8_t *points);

	// Receive MIDI input and send to DX7
	virtual void queueMidiRx(const uint32_t size, const uint8_t *const buffer);
	virtual void queueSysEx(const uint32_t size, const uint8_t *const buffer);
	bool parseMIDI(const uint32_t size, const uint8_t *const buffer);
	bool serial = false; // Use DX7's native serial interface rather than sub-CPU for MIDI
	void useSerialMidi(bool on) { serial = on; }

	// Parse DX7's MIDI output stream and break up into events for Jack.
	// Returns true when a complete event is found in the TX stream;
	// The size and buffer args are set appropriately to return
	// the event to the Jack loop
	virtual bool queueMidiTx(uint32_t& size, uint8_t* &buffer);
	enum State { Ctrl, Data1, Data2, Sysex };
	State state = Ctrl; // state of midi parser
	static const int maxSysex = 4104; // A bulk voice dump is 4104 (6+4096) bytes
	uint8_t midibuf[maxSysex];
	uint32_t size = 0; // track midi msg size

	// Callback for libsamplerate
	static long fillCallback(void *cb_data, float **audio) {
		DX7Synth *me = (DX7Synth*)cb_data;
		*audio = me->buffer;
		return me->fillBuffer();
	}
};

