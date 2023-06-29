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

struct label {
	const char *name; // css node
	int col_offset; // add to column
	int col, row; // position
	int colspan, rowspan; // span
	const char *text; // label text
	Label* label=0; // widget
};

label labels[] = {
	// Sliders
	{	"l_white",		0,	1,	1,	1, 	1,	"VOLUME" },
	{	"l_white",		0,	2,	1,	1, 	1,	"DATA" },

	// Left side
	{	"l_white_ul",	2,	5,	1,	2, 	1,	"MEMORY PROTECT" },
	{	"l_white",		2,	7,	1,	1, 	2,	"OPERATOR\nSELECT" },
	{	"l_white",		2,	4,	2,	1, 	1,	"STORE" },
	{	"l_white",		2,	5,	2,	1, 	1,	"INTERNAL" },
	{	"l_white",		2,	6,	2,	1, 	1,	"CARTRIDGE" },

	{	"l_white",		2,	4,	8,	1, 	2,	"EDIT/\nCOMPARE" },
	{	"l_white_ul",	2,	5,	8,	2, 	1,	"MEMORY SELECT" },

	{	"l_white",		2,	1,	9,	1, 	1,	"NO" },
	{	"l_white",		2,	2,	9,	1, 	1,	"YES" },
	{	"l_white",		2,	5,	9,	1, 	1,	"INTERNAL" },
	{	"l_white",		2,	6,	9,	1, 	1,	"CARTRIDGE" },
	{	"l_white",		2,	7,	9,	1, 	1,	"FUNCTION" },

	{	"l_white",		2,	1,	11,	1, 	1,	"OFF" },
	{	"l_white",		2,	2,	11,	1, 	1,	"ON" },
	{	"l_reverse",	2,	5,	11,	2, 	1,	"PLAY" },

	// Spacer
	{	"l_white",		2,	3,	6,	1, 	1,	"     " },

	// Right side
	{	"l_blue_ul",	14,	1,	1,	6, 	1,	"OPERATOR ON-OFF/EG COPY" },
	{	"l_blue_ul",	14,	9,	1,	6, 	1,	"LFO" },
	{	"l_blue_ul",	14,	15,	1,	2, 	1,	"MOD SENSITIVITY" },
 
	{	"l_blue",		14,	1,	2,	1, 	1,	"1" },
	{	"l_blue",		14,	2,	2,	1, 	1,	"2" },
	{	"l_blue",		14,	3,	2,	1, 	1,	"3" },
	{	"l_blue",		14,	4,	2,	1, 	1,	"4" },
	{	"l_blue",		14,	5,	2,	1, 	1,	"5" },
	{	"l_blue",		14,	6,	2,	1, 	1,	"6" },
	{	"l_blue",		14,	7,	2,	1, 	1,	"ALGORITHM" },
	{	"l_blue",		14,	8,	2,	1, 	1,	"FEEDBACK" },
	{	"l_blue",		14,	9,	2,	1, 	1,	"WAVE" },
	{	"l_blue",		14,	10,	2,	1, 	1,	"SPEED" },
	{	"l_blue",		14,	11,	2,	1, 	1,	"DELAY" },
	{	"l_blue",		14,	12,	2,	1, 	1,	"PMD" },
	{	"l_blue",		14,	13,	2,	1, 	1,	"AMD" },
	{	"l_blue",		14,	14,	2,	1, 	1,	"SYNC" },
	{	"l_blue",		14,	15,	2,	1, 	1,	"PITCH" },
	{	"l_blue",		14,	16,	2,	1, 	1,	"AMPLITUDE" },

	{	"l_orange",		14,	1,	4,	1, 	2,	"MASTER\nTUNE ADJ" },
	{	"l_orange",		14,	2,	4,	1,	1,	"POLY/MONO" },
	{	"l_orange",		14,	3,	4,	1,	1,	"RANGE" },
	{	"l_orange",		14,	4,	4,	1,	1,	"STEP" },
	{	"l_orange",		14,	5,	4,	1,	1,	"MODE" },
	{	"l_orange",		14,	6,	4,	1,	1,	"GLISSANDO" },
	{	"l_orange",		14,	7,	4,	1,	1,	"TIME" },
	{	"l_orange",		14,	9,	4,	1,	1,	"EDIT RECALL" },
	{	"l_orange",		14,	10,	4,	1,	1,	"VOICE INIT" },
	{	"l_orange",		14,	14,	4,	1,	2,	"BATTERY\nCHECK" },
	{	"l_orange",		14,	15,	4,	1,	1,	"SAVE" },
	{	"l_orange",		14,	16,	4,	1,	1,	"LOAD" },

	{	"l_orange_ul",	14,	3,	5,	2,	1,	"PITCH BEND" },
	{	"l_orange_ul",	14,	5,	5,	3,	1,	"PORTAMENTO" },
	{	"l_orange_ul",	14,	15,	5,	2,	1,	"CARTRIDGE" },

	{	"l_blue_ul",	14,	1,	7,	4, 	1,	"OSCILLATOR" },
	{	"l_blue_ul",	14,	5,	7,	2, 	1,	"EG" },
	{	"l_blue_ul",	14,	7,	7,	3, 	1,	"KEYBOARD LEVEL SCALING" },
	{	"l_blue",		14,	10,	7,	1, 	3,	"KEYBOARD\nRATE\nSCALING" },
	{	"l_blue_ul",	14,	11,	7,	2, 	1,	"OPERATOR" },
	{	"l_blue_ul",	14,	13,	7,	2, 	1,	"PITCH EG" },

	{	"l_blue",		14,	1,	8,	1, 	1,	"MODE/" },
	{	"l_blue",		14,	2,	8,	1, 	1,	"FREQUENCY" },
	{	"l_blue",		14,	3,	8,	1, 	1,	"FREQUENCY" },
	{	"l_blue",		14,	7,	8,	1, 	1,	"BREAK" },
	{	"l_blue",		14,	11,	8,	1, 	1,	"OUTPUT" },
	{	"l_blue",		14,	12,	8,	2, 	1,	"KEY VELOCITY" },
	{	"l_blue",		14,	15,	8,	1, 	1,	"KEY" },
	{	"l_blue",		14,	16,	8,	1, 	1,	"VOICE" },

	{	"l_blue",		14,	1,	9,	1, 	1,	"SYNC" },
	{	"l_blue",		14,	2,	9,	1, 	1,	"COARSE" },
	{	"l_blue",		14,	3,	9,	1, 	1,	"FINE" },
	{	"l_blue",		14,	4,	9,	1, 	1,	"DETUNE" },
	{	"l_blue",		14,	5,	9,	1, 	1,	"RATE" },
	{	"l_blue",		14,	6,	9,	1, 	1,	"LEVEL" },
	{	"l_blue",		14,	7,	9,	1, 	1,	"POINT" },
	{	"l_blue",		14,	8,	9,	1, 	1,	"CURVE" },
	{	"l_blue",		14,	9,	9,	1, 	1,	"DEPTH" },
	{	"l_blue",		14,	11,	9,	1, 	1,	"LEVEL" },
	{	"l_blue",		14,	12,	9,	1, 	1,	"SENSITIVITY" },
	{	"l_blue",		14,	13,	9,	1, 	1,	"RATE" },
	{	"l_blue",		14,	14,	9,	1, 	1,	"LEVEL" },
	{	"l_blue",		14,	15,	9,	1, 	1,	"TRANSPOSE" },
	{	"l_blue",		14,	16,	9,	1, 	1,	"NAME" },

	{	"l_orange",		14,	1,	11,	1,	1,	"RANGE" },
	{	"l_orange",		14,	2,	11,	1,	1,	"PITCH" },
	{	"l_orange",		14,	3,	11,	1,	1,	"AMPLITUDE" },
	{	"l_orange",		14,	4,	11,	1,	1,	"EG BIAS" },
	{	"l_orange",		14,	5,	11,	1,	1,	"RANGE" },
	{	"l_orange",		14,	6,	11,	1,	1,	"PITCH" },
	{	"l_orange",		14,	7,	11,	1,	1,	"AMPLITUDE" },
	{	"l_orange",		14,	8,	11,	1,	1,	"EG BIAS" },
	{	"l_orange",		14,	9,	11,	1,	1,	"RANGE" },
	{	"l_orange",		14,	10,	11,	1,	1,	"PITCH" },
	{	"l_orange",		14,	11,	11,	1,	1,	"AMPLITUDE" },
	{	"l_orange",		14,	12,	11,	1,	1,	"EG BIAS" },
	{	"l_orange",		14,	13,	11,	1,	1,	"RANGE" },
	{	"l_orange",		14,	14,	11,	1,	1,	"PITCH" },
	{	"l_orange",		14,	15,	11,	1,	1,	"AMPLITUDE" },
	{	"l_orange",		14,	16,	11,	1,	1,	"EG BIAS" },

	{	"l_orange_ul",	14,	1,	12,	4,	1,	"MODULATION WHEEL" },
	{	"l_orange_ul",	14,	5,	12,	4,	1,	"FOOT CONTROL" },
	{	"l_orange_ul",	14,	9,	12,	4,	1,	"BREATH CONTROL" },
	{	"l_orange_ul",	14,	13,	12,	4,	1,	"AFTER TOUCH" },
};

