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

void HD6303R::step() {
	if(halt) return;

	opcode = memory[PC++];
	inst = instructions+opcode;

	OP = OP2 = ADDR = 0; // Need to reset for OCR and P_ACEPT

	if(inst->mode == 0) { // Illegal opcode
		fprintf(stderr, "Illegal opcode %02X PC=%04X\n", opcode, PC);
		trap();
		return;
	}

	// Save these
	uint8_t saveTCSR = TCSR, saveTRCSR = TRCSR;

	// Run the instruction
	inst->action();

	// Top 3 bits of TCSR and TRCSR are read-only, so fix if written to
	if(saveTCSR != TCSR) TCSR = (TCSR&0x1F) | (saveTCSR&0xE0);
	if(saveTRCSR != TRCSR) TRCSR = (TRCSR&0x1F) | (saveTRCSR&0xE0);

	// Clock update
	cycle += inst->cycles;
	sci_tx_counter += inst->cycles;
	sci_rx_counter += inst->cycles;

	// Handle external IRQ
	if(!irqpin) irq();

	///////////////////////////////////////
	// Timer 1 interrupt processing
	///////////////////////////////////////
	uint32_t p_timer1 = getm16(0x09); // Previous counter value in FRCH
	uint32_t timer1 = p_timer1 + inst->cycles; // New counter value
	stom16(0x09, timer1&0xFFFF); // handle wrap, and update

	// Timer1 OCR match handling
	uint32_t ocr = getm16(0x0B);
	if(timer1>=ocr && p_timer1<ocr) { // Set TCSR OCF flag
		TCSR = set(TCSR, OCF);
		readTCSR = wroteOCR = false;
	}
	// Handle OCF flag
	// Instruction read TCSR with OCF set, so set flag
	if(ADDR==0x08 && inst->r && bit(TCSR,OCF)) readTCSR = true;  // inst read TCSR
	// Instruction wrote OCR after reading TCSR (writes should only be 2-byte STD,
	// but 1-byte STA/B not prevented)
	else if((ADDR==0x0B || ADDR==0x0C) && readTCSR && inst->w) wroteOCR = true;
	// Clear TCSR OCF flag after both events
	if(readTCSR && wroteOCR) TCSR = clear(TCSR, OCF);

	// Trigger Output Compare Interrupt request
	if(bit(TCSR, OCF) && bit(TCSR, EOCI)) oci();

#if 0
	// Not needed for DX7, so not compiled, for performance
	// Note: ICF and OLVL not implemented, but similar (NB ICF has higher priority than OCI)
	// Timer1 overflow handling
	if(timer1 > 0xFFFF) {
		TCSR = set(TCSR, TOF);
		readTCSR = false;
	}
	if(ADDR==0x08 && inst->r && bit(TCSR,TOF)) readTCSR = true;  // inst read TCSR
	// Instruction read FRCH after reading TCSR
	else if((ADDR==0x09 || ADDR==0x0A) && readTCSR && inst->r) TCSR = clear(TCSR, TOF);
	// Trigger Timer Overflow Interrupt request
	if(bit(TCSR, TOF) && bit(TCSR, ETOI)) toi();
#endif

	///////////////////////////////////////
	// Handle external serial interrupt SCI
	///////////////////////////////////////

	// Transmit: Write to TDR after read of TRCSR
	if(ADDR == 0x13 && readTRCSR && inst->w) {
		clear(TRCSR, TDRE); // Clear TDRE
		sci_tx_counter = 0; // reset timer
	}

	// Receive: Read of RDR after read of TRCSR
	if(ADDR == 0x12 && readTRCSR && inst->r) {
		clear(TRCSR, RDRF); // Clear RDRF
		clear(TRCSR, ORFE); // Clear ORFE
		sci_rx_counter = 0; // reset timer
	}

	// Read of TRCSR sets flag
	if(ADDR==0x11 && inst->r) readTRCSR = true;

	// Trigger SCI interrupt request if TE && TIE && TDRE || RE && RIE && RDRF
	constexpr const uint8_t mask1 = 1<<TDRE | 1<<TIE | 1<<TE;
	constexpr const uint8_t mask2 = 1<<RDRF | 1<<RIE | 1<<RE;
	if((TRCSR & mask1) == mask1 || (TRCSR & mask2) == mask2) sci();
}

// Sender waits the appropriate baud rate as measured by sci_rx_counter, then calls
// this to send a byte. Ignore the byte if RE is not set. If RDRF is still set (CPU
// interrupt has not read the RDR yet), set ORFE indicating overflow. 
void HD6303R::clockInData(uint8_t byte) {
	if(!bit(TRCSR,RE)) return;
	RDR = byte;
	if(bit(TRCSR, RDRF)) set(TRCSR, ORFE); // ORFE overrun
	set(TRCSR, RDRF); // Set RDRF
	readTRCSR = false;
}

// Receiver waits the appropriate baud rate as measured by sci_tx_counter, then calls
// this to get the data. If TDRE is clear, returns true and sets the byte arg, otherwise
// returns false to indicate no data to transmit.
bool HD6303R::clockOutData(uint8_t& byte) {
	if(!bit(TRCSR,TE)) return false; // Transmit not enabled
	if(!bit(TRCSR, TDRE)) {
		set(TRCSR, TDRE); // Set TDRE, CPU can send another byte
		readTRCSR = false;
		byte = TDR;
		return true;
	} else return false;
}

void HD6303R::reset() {
	// Registers
	P1DDR  = 0xFE;
	P2DDR  = 0x00;
	PORT1  = 0x00;
	PORT2  = 0x00;
	TCSR   = 0x00;
	FRCH   = 0x00;
	FRCL   = 0x00;
	OCRH   = 0xFF;
	OCRL   = 0xFF;
	ICRH   = 0x00;
	ICRL   = 0x00;
	RMCR   = 0xC0;
	TRCSR  = 0x20;
	RDR    = 0x00;
	TDR    = 0x00;
	RAMCR  = 0x14;

	// Set PC to reset vector
	PC = getm16(0xFFFE);
	I = true;
	// Port2 bits 2-0 latch the external mode selection - which the emulator can ignore
}

// Interrupt requests
// Maskable IRQs return immediately if I is set
// Return value indicates if IRQ was granted
void HD6303R::nmi()  { interrupt(0xFFFC); }
void HD6303R::trap() { interrupt(0xFFEE); }
bool HD6303R::swi()  { return maskable_interrupt(0xFFFA); }
bool HD6303R::irq()  { return maskable_interrupt(0xFFF8); }
bool HD6303R::ici()  { return maskable_interrupt(0xFFF6); }
bool HD6303R::oci()  { return maskable_interrupt(0xFFF4); }
bool HD6303R::toi()  { return maskable_interrupt(0xFFF2); }
bool HD6303R::cmi()  { return maskable_interrupt(0xFFEC); }
bool HD6303R::irq2() { return maskable_interrupt(0xFFEA); }
bool HD6303R::sci()  { return maskable_interrupt(0xFFF0); }

bool HD6303R::maskable_interrupt(uint16_t vector) {
	if(I) return false;
	interrupt(vector);
	return true;
}

void HD6303R::interrupt(uint16_t vector) {
	push(PC);
	push(IX);
	memory[SP--] = A;
	memory[SP--] = B;
	memory[SP--] = getCCR();
	I = 1;
	PC = memory[vector] << 8;
	PC |= memory[vector+1];
}

// Load a program from file
int HD6303R::pgmload(const char *f) {
	FILE *fp = fopen(f, "r");
	if(!fp) return(-1);
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(size > 63000) {
		fprintf(stderr, "size %ld > 63000\n", size);
		fclose(fp);
		return(-2);
	}
	int start = 0x10000 - size;
	size_t count = fread(memory+start, 1, size, fp);
	if(size != count) {
		fprintf(stderr, "size=%ld read=%ld\n", size, count);
		fclose(fp);
		return(-3);
	}
	fclose(fp);

	// reset vector
	reset();
	return(0);
}

// Load a memory segment from file
int HD6303R::memload(const char *f, uint16_t addr) {
	FILE *fp = fopen(f, "r");
	if(!fp) return(-1);
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if(size+addr > 63000) {
		fprintf(stderr, "size+addr %ld+%d > 63000\n", size,addr);
		fclose(fp);
		return(-2);
	}
	int start = addr;
	size_t count = fread(memory+start, 1, size, fp);
	if(size != count) {
		fprintf(stderr, "size=%ld read=%ld\n", size, count);
		fclose(fp);
		return(-3);
	}
	fclose(fp);
	return(0);
}

// Save a memory segment to file
int HD6303R::memsave(const char *f, uint16_t addr, uint16_t bytes) {
	FILE *fp = fopen(f, "w");
	size_t count = fwrite(memory+addr, 1, bytes, fp);
	if(bytes != count) {
		fprintf(stderr, "bytes=%d read=%ld\n", bytes, count);
		fclose(fp);
		return(-4);
	}
	fclose(fp);
	return(0);
}

// Debug tracing
void HD6303R::trace() {
	printf("%016lX inst=%02X %4s %2s OP=%02X OP2=%04X ADDR=%04X "
		"A=%02X B=%02X CCR=%d%d%d%d%d%d SP=%04X IX=%04X PC=%04X ",
		cycle, opcode, instructions[opcode].op, instructions[opcode].mode, OP, OP2, ADDR,
		A, B, H, I, N, Z, V, C, SP, IX, PC);
}

