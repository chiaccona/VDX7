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
#include <cstring>

class HD44780 {
public:
	// Display state
	char line1[17] = {0};
	char line2[17] = {0};
	uint8_t cursor_pos;
	uint8_t cursor_line;
	bool cursor_on;
	bool cursor_blink;
	bool display_on;
	bool lines = 0; // false=1 line, true=2 lines

	HD44780() { inst(0x01); update(); } // clear

	uint8_t state[92];
	uint8_t save(const uint8_t* &d) {
		uint8_t *data = state;
		for(int i=0; i<80; i++) *data++ = ddram[i];
		*data++ = ac;
		*data++ = shift;
		*data++ = S;
		*data++ = I_D;
		*data++ = on;
		*data++ = C;
		*data++ = B;
		*data++ = ddmode;
		*data++ = datalen;
		*data++ = lcdfont;
		*data++ = lines;
		d = state; // pass state buffer to caller
		return 92; // length of state buffer
	}

	void restore(uint8_t *data, uint8_t len) {
		if(len != 92) return;
		for(int i=0; i<80; i++) ddram[i] = (char)*data++;
		ac = *data++;
		shift = *data++;
		S = *data++;
		I_D = *data++;
		on = *data++;
		C = *data++;
		B = *data++;
		ddmode = *data++;
		datalen = *data++;
		lcdfont = *data++;
		lines = *data++;
		update_cursor();
		update();
	}

	// HD44780 data bus command
	void data(uint8_t x) {
		if(ddmode) {
			ddram[ac] = x;
			if(I_D) ac++; else ac--;
			if(ac<0) ac = 0;
			if(ac>79) ac = 79;
			if(S&&I_D) { // Shift right
				if(lines) { if(++shift>39) shift = 0; }
					else if(++shift>79) shift = 0;
			} else if(S) { // Shift left
				if(lines) { if(--shift<0) shift = 39; }
					else if(--shift<0) shift = 79;
			}
		} else { // No cgram
			cgram[ac] = x;
			if(I_D) ac++; else ac--;
			if(ac<0) ac = 0;
			if(ac>63) ac = 63;
		}
		update_cursor();
		update();
	}
	
	// HD44780 instruction set command
	void inst(uint8_t x) {
		auto bit = [](uint8_t x, int n) { return (x>>n)&1; };
		if(bit(x,7)) { // ddram address
			ac = x&0x7F;
			if(lines) { if(ac>=64) ac -= 24; }
				else if(ac>=80) ac -= 80;
			ddmode = true;
			update_cursor();
		} else if(bit(x,6)) { // cgram address (no cgram)
			ac = x&0x3F;
			ddmode = false;
			update_cursor();
		} else if(bit(x,5)) { // function set
			// Always 8-bit, "5x8" font
			datalen = bit(x,4);
			lines = bit(x,3);
			lcdfont = bit(x,2);
			update_cursor();
		} else if(bit(x,4)) { // shift/move
			if(bit(x,3)) { // shift
				if(bit(x,2)) {
					if(lines) { if(++shift>39) shift = 0; }
						else if(++shift>79) shift = 0;
				} else {
					if(lines) { if(--shift<0) shift = 39; }
						else if(--shift<0) shift = 79;
				}
			} else { // move
				if(bit(x,2)) { if(++ac > 79) ac = 0;
				} else { if(--ac < 0) ac = 79; }
			}
			update_cursor();
		} else if(bit(x,3)) { // display, cursor, blink
			on = bit(x,2);
			C = bit(x,1);
			B = bit(x,0);
			update_cursor();
		} else if(bit(x,2)) { // entry mode
			I_D = bit(x,1);
			S = bit(x,0);
		} else if(bit(x,1)) { // home
			ac = 0;
			shift = 0;
			update();
			//S = false;
			update_cursor();
		} else if(bit(x,0)) { // clear
			ac = 0;
			for(int i=0; i<80; i++) ddram[i] = 0x20;
			update_cursor();
		}
	}
	void update_cursor() {
		cursor_pos = lines ? (ac-shift+40)%40 : (ac-shift+80)%80;
		cursor_line = (lines && ac>=40) ? 2 : 1;
		display_on = on;
		cursor_on = C;
		cursor_blink = B;
	}

private:
	// State
	char ddram[80];
	uint8_t cgram[64] = {0}; // unused
	uint8_t ac = 0;
	uint8_t shift = 0;
	bool S=0, I_D=0, on=0, C=0, B=0; // shift, direction, on, cursor, blink
	bool ddmode = true;
	bool datalen = 0; // ignored, always 8 bit mode
	bool lcdfont = 0; // 5x8, 5x10 - ignored, always 5x8

	// Update display
	void update() {
		if(lines) {
			if(shift>24) {
				memcpy(line1, ddram, shift-24);
				memcpy(line1, ddram+shift, 40-shift);
				memcpy(line2, ddram+40, shift-24);
				memcpy(line2, ddram+shift+40, 40-shift);
			} else {
				memcpy(line1, ddram+shift, 16);
				memcpy(line2, ddram+shift+40, 16);
			}
		} else {
			if(shift>64) {
				memcpy(line1, ddram, shift-64);
				memcpy(line1, ddram+shift, 80-shift);
			} else {
				memcpy(line1, ddram+shift, 16);
			}
		}
	}
};

