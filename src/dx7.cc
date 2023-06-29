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

#include "dx7.h"

DX7::DX7(ToSynth*& ts, ToGui*& tg, const char *rf)
	: toSynth(ts), toGui(tg), ramfile(rf) {
	// Load ROM
	const uint8_t *rom = _binary_firmware_bin_start;
	for(int addr = 0xC000; addr<0x10000; addr++) memory[addr] = *rom++;
	cartFile.reserve(256); // avoid runtime std::string allocation
}

// Load a firmware ROM
int DX7::loadROM(const char *romfile) {
	if(!romfile) return(-2);
	FILE *fp = fopen(romfile, "r");
	if(!fp) {
		fprintf(stderr, "Can't open dx7 firmware (%s)\n", romfile);
		return(-1);
	}
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(size != 16384) {
		fprintf(stderr, "Not a dx7 firmware (size %ld != 16384)\n", size);
		fclose(fp);
		return(-2);
	}

	int start = 0xC000;
	size_t count = fread(memory+start, 1, 16384, fp);
	if(count != 16384) {
		fprintf(stderr, "Read error size=%d read=%ld\n", 16384, count);
		fclose(fp);
		return(-3);
	}
	fclose(fp);
	return(0);
}

void DX7::start() {
	// battery voltage is "LOW" ~1.9v until we successfully restore the RAM
	toSynth->analog(Message::CtrlID::battery, 49);

	// Load RAM
	int err = restoreRAM(ramfile);
	if(err) { // No ram
		fprintf(stderr, "Can't restore RAM (%d)\n", err);
		// clear RAM
		for(int i=0; i<6144; i++) memory[0x1000+i] = 0;
		tune(0); // Default master tuning to A440 = 0x100
	} else {
		fprintf(stderr, "Restored RAM (%s)\n", ramfile);
		fprintf(stderr, "Master Tune 0x%01X%02X\n", M_MASTER_TUNE, M_MASTER_TUNE_LOW);
	}

	P_CRT_PEDALS_LCD = 0x00;

	// Portamento pedal being unplugged causes P_CRT_PEDALS_LCD bit 1 to be set,
	// which by default enables portamento all the time (rather than just when pedal is down)
	// Using the pedal (midi ctrl 65) switches bit off when pedal is up.
	porta(true);

	// Default, cartridge not present, and write protected
	// Sets P_CRT_PEDALS_LCD appropriately
	cartPresent(false);
	cartWriteProtect(true);

	// 0b10000000 for LCD busy status
	// should stay off, LCD has no need to delay
	P_CRT_PEDALS_LCD &= ~0b10000000;

	reset(); // Start processor
}

DX7::~DX7() {
	int err = saveRAM(ramfile);
	if(err) fprintf(stderr, "Can't save RAM (%d)\n", err);
		else fprintf(stderr, "Saved RAM (%s)\n", ramfile);

	// Save cart if R/W
	if(!cartWriteProtect() && saveCart && !cartFile.empty()) {
		int err = cartSave(cartFile.c_str());
		if(err) fprintf(stderr, "Can't save cartridge (%d)\n", err);
			else fprintf(stderr, "Saved cartridge (%s)\n", cartFile.c_str());
	}
}

void DX7::tune(int tuning) {
	if(tuning < 256 && tuning >= -256) {
		uint16_t t = tuning+256;
		M_MASTER_TUNE = t>>8;
		M_MASTER_TUNE_LOW = t&0xFF;
	}
	fprintf(stderr, "Setting Master Tune 0x%01X%02X\n", M_MASTER_TUNE, M_MASTER_TUNE_LOW);
}

void DX7::run() {
	step();

	// Serial baud rate timer.
	// Hardware actually uses an external clock to get the MIDI 31.25k baud rate
	if(sci_tx_counter>=377) { // 377 = ((9.4265Mhz/2)/4) / 3125;  31.25k baud = 3125 bytes/sec
		// Serial TX -  get the transmitted byte from CPU and save to a buffer to transmit 
		uint8_t byte;
		if(clockOutData(byte)) midiSerialTx.write(byte);
	}
	if(sci_rx_counter>=377) { // 31.25k baud
		// Serial RX - if data available in RX buffer, wait for RDRF to clear, then send to CPU
		if(!midiSerialRx.empty() && bit(TRCSR,RDRF)==0) clockInData(midiSerialRx.read());
	}

	// Should not happen (debug guard)
	if(SP <= 0x263F && SP > 0) printf("Stack smashed 0x%04X\n", SP);

	// Start of sub-CPU Event Handshake:
	// P20/C2 high means main CPU is ready for next message
	// haveMsg means Synth/GUI has given us a message to process
	if(bit(PORT2,0) && haveMsg && !byte1Sent) {
		PORT1 = msg.byte1;
		clear(PORT2,1); // Sub-CPU clears pin 21, telling CPU to read in the byte
		irqpin = false; // Active low triggers IRQ interrupt
		byte1Sent = true;
	}

	// Only need to look at instructions that write memory from here on
	if(inst->w == false) return;

	// Writing to peripheral controller 0x280*
	if((ADDR&0xFFF0)==0x2800) {

		if(ADDR==0x2800) { // P_LCD_DATA
			// FIX keeping these in both DSP and GUI
			if(memory[0x2801]==4) {
				toGui->lcd_inst(memory[ADDR]);
				lcd.inst(memory[ADDR]);
			}
			else if(memory[0x2801]==5) {
				toGui->lcd_data(memory[ADDR]);
				lcd.data(memory[ADDR]);
			}
		}
		
		else if(ADDR==0x2801) { // P_LCD_CTRL
			// can ignore - always Written before P_LCD_DATA
		}

		else if(ADDR==0x280C) { // &P_ACEPT
			// Completion of sub-CPU Event Handshake
			// After HANDLER_IRQ interrupt has run for a while...
			// P_ACEPT flipflop is set by hardware when 0x280C is put on the system bus,
			// driving C1 low on the Sub-CPU, telling us we can send the second byte
			if(byte1Sent) {
				PORT1 = msg.byte2;
				clear(PORT2,1); // Sub-CPU signals to read in the second byte
				irqpin = false; // Hardware also activates IRQ, but it's now masked
				byte1Sent = false;
			} else { // We're done
				irqpin = true; // P_ACEPT resets IRQ READY flipflop
				haveMsg = false; // Tell Synth/GUI we're ready for another message
			}
		}

		else if(ADDR==0x280E) { // P_LED1 & P_LED2
			// LED update, writes to these addresses propagate to LED controller
			// CPU always writes 0x280F before 0x280E
			toGui->led1_setval(P_LED1);
			toGui->led2_setval(P_LED2);
		}

		else if(ADDR==0x2805) { // P_OPS_MODE & P_OPS_ALG_FDBK
			// Algorithm update. Firmware always writes 0x2804 before 0x2805
			egs.setAlgorithm(P_OPS_MODE, P_OPS_ALG_FDBK);
		}

		else if(ADDR==0x280A) { // P_DAC - MIDI volume control
			midiVolume = P_DAC&7;
		}
	}

	// Writing to EGS address space 0x30**
	// Trigger EGS to update internal pitch registers
	if((ADDR&0xFF00)==0x3000) egs.update(uint8_t(ADDR));

	// Writing to Cartidge 0x4***, flag to save it on exit
	if((ADDR&0xF000)==0x4000) saveCart = true;
}

// Load a cartridge from a SYSEX formatted file
int DX7::cartLoad(const char *f) {

	// Save existing cart if present and R/W
	if(!cartWriteProtect() && saveCart && !cartFile.empty()) {
		int err = cartSave(cartFile.c_str());
		if(err) fprintf(stderr, "Can't save cartridge (%d)\n", err);
			else fprintf(stderr, "Saved cartridge (%s)\n", cartFile.c_str());
	}
	cartFile.clear();

	FILE *fp = fopen(f, "r");
	if(!fp) {
		fprintf(stderr, "Can't open cartridge \"%s\"\n", f);
		return(-1);
	}
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);

	if(size != 4104) {
		fprintf(stderr, "File \"%s\": Not a *.SYX file (size %ld != 4104)\n", f, size);
		fclose(fp);
		return(-2);
	}
	fseek(fp, 0, SEEK_SET);
	uint8_t header[7]={0};
	size_t count = fread(header, 1, 6, fp);
	if(count != 6 || memcmp(header, "\xf0\x43\x00\x09\x20\x00", 6)) {
		fprintf(stderr, "File \"%s\": Bad SYSEX header\n", f);
		fclose(fp);
		return(-3);
	}

	int start = 0x4000;
	count = fread(memory+start, 1, 4096, fp);
	if(count != 4096) {
		fprintf(stderr, "File \"%s\": Read error size=%d read=%ld\n", f, 4096, count);
		fclose(fp);
		return(-4);
	}
	int checksum=fgetc(fp);
	if(checksum==EOF) {
		fprintf(stderr, "File \"%s\": SYSEX read error\n", f);
		fclose(fp);
		return(-5);
	}
	for(int i=0; i<4096; i++) checksum += *(memory+start+i);
	if(checksum&0x7F) {
		fprintf(stderr, "File \"%s\": SYSEX checksum error sum=%d\n", f, checksum&0x7F);
		fclose(fp);
		return(-6);
	}

	// Cartridge available
	cartFile = f;
	cartNum = -1; // Factory cart not loaded
	cartPresent(true);
	// Default hardware write protected => GUI will default to the same
	cartWriteProtect(true);
	fclose(fp);
	// Tell GUI the filename
	toGui->cartridge_name((uint8_t*)f, strlen(f));
	return(0);
}

// Set hardware cartridge present
void DX7::cartPresent(bool present) {
	if(present) P_CRT_PEDALS_LCD &= ~0b100000;
		else P_CRT_PEDALS_LCD |= 0b100000;
}

// Set hardware cartridge write protected
void DX7::cartWriteProtect(bool protect) {
	if(protect) P_CRT_PEDALS_LCD |= 0b1000000;
		else P_CRT_PEDALS_LCD &= ~0b1000000;
}

// Load patch memory with factory cartridge 0-7
void DX7::setBank(int n, bool cart) {
	// Bank number must be 0-7, corresp. rom1A through rom4B,
	// linked in external object rom.bin.o
	// THe blob contains 8 banks, each 4k in size
	const uint8_t *ram = _binary_voices_bin_start + 4096*(n&0x7);

	if (cart) {
		// Save existing cart if R/W
		if(!cartWriteProtect() && saveCart && !cartFile.empty()) {
			int err = cartSave(cartFile.c_str());
			if(err) fprintf(stderr, "Can't save cartridge (%d)\n", err);
				else fprintf(stderr, "Saved cartridge (%s)\n", cartFile.c_str());
		}

		cartPresent(true);
		cartFile.clear(); // Clear filename
		cartNum = n&0x7;
		for(int addr = 0x4000; addr<0x5000; addr++) memory[addr] = *ram++;
		toGui->cartridge_num(cartNum); // Tell GUI
	} else {
		// Write to internal memory
		for(int addr = 0x1000; addr<0x2000; addr++) memory[addr] = *ram++;
	}
}


// Save a cartridge in Sysex format
int DX7::cartSave(const char *f) {
	FILE *fp = fopen(f, "w");
	if(!fp) return(-1);
	fseek(fp, 0, SEEK_SET);

	uint8_t header[] = { 0xf0, 0x43, 0x00, 0x09, 0x20, 0x00 };
	size_t count = fwrite(header, 1, 6, fp);
	int start = 0x4000;
	int8_t checksum=0;
	for(int i=0; i<4096; i++) checksum += *(memory+start+i);
	checksum = (-checksum) & 0x7F;
	count += fwrite(memory+start, 1, 4096, fp);
	count += fwrite(&checksum, 1, 1, fp);
	uint8_t end=0xF7;
	count += fwrite(&end, 1, 1, fp);
	if(count != 4104) {
		fprintf(stderr, "Write error size=%d wrote=%ld\n", 4104, count);
		fclose(fp);
		return(-3);
	}
	fclose(fp);
	return(0);
}

// Load the 6K battery backed-up RAM from a file
int DX7::restoreRAM(const char *ramfile) {
	if(!ramfile) return(-2);
	FILE *fp = fopen(ramfile, "r");
	if(!fp) {
		fprintf(stderr, "Can't open dx7 ramfile (%s)\n", ramfile);
		return(-1);
	}
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(size != 6144) {
		fprintf(stderr, "Not a dx7 ramfile (size %ld != 6144)\n", size);
		fclose(fp);
		return(-2);
	}
	int start = 0x1000;
	size_t count = fread(memory+start, 1, 6144, fp);
	if(count != 6144) {
		fprintf(stderr, "Read error size=%d read=%ld\n", 6144, count);
		fclose(fp);
		return(-3);
	}
	fclose(fp);

	// battery voltage is now "OK" ~3.2v
	toSynth->analog(Message::CtrlID::battery, 82);
	return(0);
}

// Save the 6K battery backed-up RAM to a file
int DX7::saveRAM(const char *ramfile) {
	if(!ramfile) return(-2);
	FILE *fp = fopen(ramfile, "w");
	if(!fp) return(-1);
	int start = 0x1000;
	size_t count = fwrite(memory+start, 1, 6144, fp);
	if(count != 6144) {
		fprintf(stderr, "Write error size=%d read=%ld\n", 6144, count);
		fclose(fp);
		return(-3);
	}
	fclose(fp);
	return(0);
}


// Debug utilities

void DX7::printEGS() {
	printf("\n");
	for(int i=0; i<16; i++) {
		uint16_t x = getm16(0x3000+2*i);
		uint16_t y = getm16(0x2140+2*i); // Pitch EG
		printf("V=%2d 0x%04X VOICE_PITCH=0x%04X R=%d EG=%04X ", i, x, x>>2, x&1, y);
		printf("EGS_OP_LEVELS=");
		for(int j=0; j<6; j++) printf(" %02X", *(P_EGS_OP_LEVELS+j*16+i));
		printf("\n");
	}

	for(int i=0; i<6; i++) {
		printf("OP=%d OP_DETUNE=%02X ", 6-i, *(P_EGS_OP_DETUNE+i));
		printf("OP_PITCH=0x%04X ", getm16(0x3020+2*i));
		uint8_t oss = *(P_EGS_OP_SENS_SCALING+i);
		printf("SENS=%02X ", oss>>3);
		printf("SCALING=%02X \n", oss&0x7);
	}

	for(int i=0; i<6; i++) {
		printf("OP=%d EGS_OP_EG_RATES= ", 6-i);
		for(int j=0; j<4; j++) printf("%02X ", *(P_EGS_OP_EG_RATES+4*i+j));
		printf("EGS_OP_EG_LEVELS= ");
		for(int j=0; j<4; j++) printf("%02X ", *(P_EGS_OP_EG_LEVELS+4*i+j));
		printf("\n");
	}

	//; Bit 0 = Key On (?), Bit 1 = Key Off, Bit 2-5 = Voice # 0-15.
	printf("AMP_MOD=0x%02X (%d)\n", int8_t(P_EGS_AMP_MOD), int8_t(P_EGS_AMP_MOD));
	printf("VOICE_EVENTS=%02X OFF=%d ON=%d\n", P_EGS_VOICE_EVENTS>>2,
			P_EGS_VOICE_EVENTS&2,
			P_EGS_VOICE_EVENTS&1);
	printf("PITCH_MOD=0x%02X%02X\n", P_EGS_PITCH_MOD_HIGH, P_EGS_PITCH_MOD_LOW);
	printf("M_NOTE_KEY=0x%02X PITCH=0x%02X%02X\n", memory[0x81], memory[0x9D], memory[0x9E]);
	printf("MASTER_TUNE=0x%02X%02X\n", memory[0x2311], memory[0x2312]);
	printf("ALGORITHM Mode=0x%02X Alg=%2d Fbk=%d\n", memory[0x2804], memory[0x2805]>>3, memory[0x2805]&7);

	printf("M_VOICE_STATUS: ");
	for(int i=0; i<16; i++) printf("%02X%02X ", memory[0x20B0+2*i], memory[0x20B0+2*i+1] );
	printf("\n");
	printf("M_KEY_EVENT:    ");
	for(int i=0; i<16; i++) printf("%d %02X ", memory[0x2168+i]>>7, memory[0x2168+i]&0x7F);
	printf("\n");
}

