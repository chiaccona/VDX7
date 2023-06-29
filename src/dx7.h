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

#include <cstring>
#include <string>
#include <map>
#include "HD6303R.h"
#include "HD44780.h"
#include "EGS.h"
#include "Message.h"
#include "filter.h"

// Firmware ROM and factory voice binaries
// Symbol names are created by "ld -r -b binary" from the raw file.
// These are declared here as arrays in order to quiet
// a compiler warning about the size being unknown.
extern const uint8_t _binary_firmware_bin_start[16384];
extern const uint8_t _binary_voices_bin_start[32768];

// Basic circular buffer, 2^N elements
template <class T, int N> struct Buffer {
	static const int size = 1<<N;
	T buffer[size];
	int readIdx=0, writeIdx=0;
	void write(const T& byte) {
		buffer[writeIdx++] = byte;
		writeIdx &= (size-1);
		if(empty()) fprintf(stderr, "MidiBuffer overflow\n");
	}
	bool empty() { return readIdx == writeIdx; }
	bool read(T& data) {
		if(empty()) return false;
		data = buffer[readIdx];
		++readIdx &= (size-1);
		return true;
	}
	T& read() { // return invalid if empty
		T& data = buffer[readIdx];
		if(!empty()) ++readIdx &= (size-1);
		return data;
	}
};

// DX7 hardware emulation
struct DX7: public HD6303R {
	DX7(ToSynth*& ts, ToGui*& tg, const char *ramfile=0);
	~DX7();
	void start();
	void run(); // Execute one instruction and update peripherals

	// References set to refer back to Synth's communication pointers
	ToSynth*& toSynth;
	ToGui*& toGui;

	// Initialize internal or cartridge patch memory to factory cartridge 0-7
	void setBank(int n, bool cart = false);

	// The EGS (and OPS contained within)
	EGS egs{memory+0x3000};

	// Battery backed-up RAM file handling
	const char* ramfile=0;
	int restoreRAM(const char *ramfile);
	int saveRAM(const char *ramfile);

	void tune(int tuning); // Master tuning -256 to +255, at about .3 cents/step. 0=A440

	// Interrupt to transfer events from sub-cpu to main
	bool byte1Sent = false, haveMsg = false;

	Message msg; // Queue of messages from GUI

	// MIDI buffers size 2^13=8192 bytes
	Buffer<uint8_t, 13> midiSerialRx, midiSerialTx;
	uint8_t getMidiRxChannel() { return M_MIDI_RX_CH; }

	// MIDI volume control through DAC
	// Volume levels implemented in hardware as DAC reference voltages
	uint8_t midiVolume = 7; // 0-7
	const float midiVolTab[8] = { 0, 710/4790.0, 200/4790.0, 2590/4790.0,
		100/4790.0, 1390/4790.0, 380/4790.0, 4790/4790.0 };
	LP1 midiFilter; // 10hz analog lowpass smooths transitions

	// Load a cartridge in *.SYX format
	int cartLoad(const char *f);
	int cartSave(const char *f);
	bool saveCart = false; // dirty flag, to save cart on exit
	std::string cartFile; // filename to save cart on exit
	int cartNum = -1; // Factory cart number 0-7, -1 if not loaded
	void cartWriteProtect(bool protect); // Set
	bool cartWriteProtect() { return P_CRT_PEDALS_LCD & 0b1000000; } // Get
	void cartPresent(bool present); // Set
	bool cartPresent() { return !(P_CRT_PEDALS_LCD & 0b100000); } // Get

	// Pedal status
	void sustain(bool on) { if(on) P_CRT_PEDALS_LCD |= 1; else P_CRT_PEDALS_LCD &= 0xFE; } // Set/clear bit 0
	void porta(bool on) { if(on) P_CRT_PEDALS_LCD |= 2; else P_CRT_PEDALS_LCD &= 0xFD; } // Set/clear bit 1 }

	// Load a firmware ROM
	int loadROM(const char *romfile);

	// Display hardware
	HD44780 lcd;

	//// Debugging ///////
	void printEGS();

	///// Variables /////

	// Peripheral address space - memory mapped in hardware
//	uint8_t  &P_LCD_DATA                          =  memory[0x2800];
//	uint8_t  &P_LCD_CTRL                          =  memory[0x2801];
	uint8_t  &P_CRT_PEDALS_LCD                    =  memory[0x2802];
//	uint8_t  &P_8255_CTRL                         =  memory[0x2803];
	uint8_t  &P_OPS_MODE                          =  memory[0x2804];
	uint8_t  &P_OPS_ALG_FDBK                      =  memory[0x2805];
	uint8_t  &P_DAC                               =  memory[0x280A];
//	uint8_t  &P_ACEPT                             =  memory[0x280C];
	uint8_t  &P_LED1                              =  memory[0x280E];
	uint8_t  &P_LED2                              =  memory[0x280F];

	// EGS address space - memory mapped in hardware
//	uint8_t  *P_EGS_VOICE_PITCH                   = &memory[0x3000]; // 16x2 byte
//	uint8_t  *P_EGS_OP_PITCH                      = &memory[0x3020]; // 6x2 byte
	uint8_t  *P_EGS_OP_DETUNE                     = &memory[0x3030]; // 6
	uint8_t  *P_EGS_OP_EG_RATES                   = &memory[0x3040]; // 6x4
	uint8_t  *P_EGS_OP_EG_LEVELS                  = &memory[0x3060]; // 6x4
	uint8_t  *P_EGS_OP_LEVELS                     = &memory[0x3080]; // 6x16
	uint8_t  *P_EGS_OP_SENS_SCALING               = &memory[0x30E0]; // 6
	uint8_t  &P_EGS_AMP_MOD                       =  memory[0x30F0];
	uint8_t  &P_EGS_VOICE_EVENTS                  =  memory[0x30F1];
	uint8_t  &P_EGS_PITCH_MOD_HIGH                =  memory[0x30F2];
	uint8_t  &P_EGS_PITCH_MOD_LOW                 =  memory[0x30F3];

	// Cartridge address space
//	uint8_t  *P_CRT_START                         = &memory[0x4000]; // 2048
//	uint8_t  *P_CRT_START_IC2                     = &memory[0x4800]; // 2048

	// RAM address space
	// These depend on specific locations in the firmware and may not
	// map correctly on modified firmware. Setting Master Tune is a command line
	// option, and Midi RX channel is used by Synth
	uint8_t  &M_MASTER_TUNE                       =  memory[0x2311];
	uint8_t  &M_MASTER_TUNE_LOW                   =  memory[0x2312];
	uint8_t  &M_MIDI_RX_CH                        =  memory[0x2573];
};

