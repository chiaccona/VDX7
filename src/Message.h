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
#include "LFQ.h"

struct Message {
	enum class CtrlID : uint8_t;
	Message(const uint8_t b1=0, const uint8_t b2=0) : byte1(b1), byte2(b2) {}
	Message(const Message::CtrlID id, const uint8_t data) : byte1(uint8_t(id)), byte2(data) {}

	// For LV2
	operator int() const { return (byte1<<8)|byte2; }
	Message(const uint16_t v) : byte1((v>>8)&0xFF), byte2(v&0xFF) {}

	// Data
	uint8_t byte1, byte2;

	// Message Control IDs (byte 1)
	enum class CtrlID : uint8_t {
		// Front panel buttons
		b_1, b_2, b_3, b_4, b_5, b_6, b_7, b_8, // 0-7
		b_9, b_10, b_11, b_12, b_13, b_14, b_15, b_16, // 8-15
		b_17, b_18, b_19, b_20, b_21, b_22, b_23, b_24, // 16-23
		b_25, b_26, b_27, b_28, b_29, b_30, b_31, b_32, // 24-31
		b_w, b_x, b_y, b_z, // 32-35
		b_chr, b_dash, b_dot, b_sp, // 36-39
		b_no, b_yes, // 40,41

		// Pedals
		sustain, porta, // 42, 43

		// Cartridge, memory protect
		cartridge, protect, // 44, 45

		// Master volume slider
		volume, // 46

		// Request to resend display state
		send_state, // 47
		cartridge_file, // 48

		// No message sent
		none,	// 49

		// Analog sources
		data=144, pitchbend=145, modulate=146, // 144-146
		foot=147, breath=148, aftertouch=149, battery=150, // 147-150

		// Button events
		buttondown=152, buttonup=153, // 152, 153

		// 159 - 219 : keys

		// Protocol for OCI messages
		// Button events:
		// Byte 1 = 152 -> button down event, Byte 2 = button+80
		// Byte 1 = 153 -> button up event, Byte 2 = button+80
		// Analog events:
		// Byte 1 = 144 -> data slider, Byte 2 = value
		// Byte 1 = 145 -> pitchbend, Byte 2 = value
		// Byte 1 = 146 -> mod wheel, Byte 2 = value
		// Byte 1 = 147 -> foot controller, Byte 2 = value
		// Byte 1 = 148 -> breath controller, Byte 2 = value
		// Byte 1 = 149 -> aftertouch, Byte 2 = value
		// Byte 1 = 150 -> battery voltage, Byte 2 = value
		// Byte 1 = 151 -> synth shutdown, Byte2 = don't care
		// Key events:
		// Byte 1 = 159-219 -> keys (0-60), Byte 2 velocity (0=keyoff, 1=max, 127=min)

		// Interface from synth to GUI (CtrlID followed by data byte)
		lcd_inst = 230,
		lcd_data = 231,
		led1_setval = 232,
		led2_setval = 233,
		cartridge_num = 234,
		cartridge_name = 235, // data byte is length of binary data string to follow
		lcd_state = 236, // data byte is length of binary data string to follow
	};
};

// Abstract base to define a comm channel
struct Interface {
	Interface() {}
	virtual ~Interface() {}
	virtual void push(Message m) = 0;
	virtual int pop(Message &m) = 0;
	void sendBinary(Message::CtrlID id, const uint8_t *data, uint8_t len);
	int getBinary(uint8_t *data, uint8_t len);
};

// Comm channel with a Lock-Free Queue
struct AppInterface : public virtual Interface {
	virtual ~AppInterface() {}
	virtual void push(Message m) { lfq.push(m); }
	virtual int pop(Message &m) { return lfq.pop(m); }
	CircularFifo<Message, 1024> lfq;
};

// Comm interface for Synth-to-Gui messages
struct ToGui : public virtual Interface {
	virtual ~ToGui() {}
	void lcd_inst(uint8_t v) { push({Message::CtrlID::lcd_inst, v}); }
	void lcd_data(uint8_t v) { push({Message::CtrlID::lcd_data, v}); }
	void led1_setval(uint8_t v) { push({Message::CtrlID::led1_setval, v}); }
	void led2_setval(uint8_t v) { push({Message::CtrlID::led2_setval, v}); }
	void cartridge_num(uint8_t v) { push({Message::CtrlID::cartridge_num, v}); }
	void cartridge_name(const uint8_t *data, uint8_t len) { sendBinary(Message::CtrlID::cartridge_name, data, len); } 
	void lcd_state(const uint8_t *data, uint8_t len) { sendBinary(Message::CtrlID::lcd_state, data, len); } 
};

// Comm interface for Gui-to-Synth messages
struct ToSynth : public virtual Interface {
	virtual ~ToSynth() {}
	void key_on(uint8_t key, uint8_t vel) { if(key<61) push({uint8_t(159+key), vel}); }
	void key_off(uint8_t key) { if(key<61) push({uint8_t(159+key), 0}); }
	void buttondown(Message::CtrlID button) { push({Message::CtrlID::buttondown, uint8_t(uint8_t(button)+80)}); }
	void buttonup(Message::CtrlID button) { push({Message::CtrlID::buttonup, uint8_t(uint8_t(button)+80)}); }
	void analog(Message::CtrlID source, uint8_t val) { push({source, val}); }
	void sustain(bool down) { push({Message::CtrlID::sustain, down}); }
	void porta(bool down) { push({Message::CtrlID::porta, down}); }
	void cartridge(bool p) { push({Message::CtrlID::cartridge, p}); }
	void protect(bool p) { push({Message::CtrlID::protect, p}); }
	void cartridge_file(const uint8_t *data, uint8_t len) { sendBinary(Message::CtrlID::cartridge_file, data, len); } 
	void load_cartridge_num(uint8_t v) { push({Message::CtrlID::cartridge_num, v}); }
	void requestState() { push({Message::CtrlID::send_state, 0}); }
};

// Main interfaces
// App creates one of each of these.
// LV2 plugin creates App_ToSynth (with LFQ) and LV2_ToGui (send to host)
// LV2 Gui creates App_ToGui (with LFQ) and LV2_ToSynth (send to host)
struct App_ToGui : public ToGui, public AppInterface { virtual ~App_ToGui() {} };
struct App_ToSynth : public ToSynth, public AppInterface { virtual ~App_ToSynth() {} };


// Protocol to send and receive binary strings
inline void Interface::sendBinary(Message::CtrlID id, const uint8_t *data, uint8_t len)  {
	// Send length of data followed by byte pairs
	Message msg{uint8_t(id), len};
	push(msg);
	if(len&1) { // odd length
		for(int i=0; i<len-1; i+=2) push({data[i], data[i+1]});
		push({data[len-1], 0});
	} else for(int i=0; i<len; i+=2) push({data[i], data[i+1]});
}

inline int Interface::getBinary(uint8_t *data, uint8_t len) {
	Message msg;
	if(len&1) { // odd length
		for(int i=0; i<len-1; i+=2) {
			if(!pop(msg)) return 1; //error
			*data++ = msg.byte1;
			*data++ = msg.byte2;
		}
		if(!pop(msg)) return 1; //error
		*data = msg.byte1;
	} else { // even
		for(int i=0; i<len; i+=2) {
			if(!pop(msg)) return 1; //error
			*data++ = msg.byte1;
			*data++ = msg.byte2;
		}
	}
	return 0;
}
