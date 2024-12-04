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

#include "Widgets.h"

class DX7GUI {
public:
	DX7GUI(bool showKeyboard = true, Window parent=0);
	~DX7GUI();
	void run(); // X11 main loop
	void idle(); // Idle loop body for LV2
	void stop() { running = false; }

	// For LV2
	void restore() { toSynth->requestState(); } // request LCD update from synth
	Window widget() { return panel; } // so host can reparent

	Display* display;
	Window panel;
	Window parent=0;
	bool hidden = true;

	// Communication Lock-Free Queue from Synth
	ToGui *toGui;
	ToSynth *toSynth;

private:
	void processMessages(); // Synth to GUI messages
	void processEvent(); // X11 event body

	bool running = true;

	bool fineAdjust = false; // Ctrl key enables wheel to change control +/- 1

	bool showKeyboard = true;

	//Display* display;
	Visual* visual;
	//Window panel;
	GC gc;

	Button *b_no, *b_yes, *b_w, *b_x, *b_y, *b_z, *b_chr, *b_dash, *b_dot, *b_sp;
	Button *b_1, *b_2, *b_3, *b_4, *b_5, *b_6, *b_7, *b_8;
	Button *b_9, *b_10, *b_11, *b_12, *b_13, *b_14, *b_15, *b_16;
	Button *b_17, *b_18, *b_19, *b_20, *b_21, *b_22, *b_23, *b_24;
	Button *b_25, *b_26, *b_27, *b_28, *b_29, *b_30, *b_31, *b_32;

	Slider *volume, *data;

	LCD *lcd;
	LED *led1, *led2;
	Cartridge *cartridge;

	// Only used when keyboard is activated
	Bender *pitchbend;
	ModWheel *modulate;
	Keyboard *keyboard;

	WidgetBase *hold = 0; // Hold widget for multiple button press
	WidgetList widgets;	// Map of all widgets

	Atom wmDeleteMessage;

	const int width = 1210;
	int height = 360;

	friend class DX7PluginGUI;
};

class DX7Headless {
public:
	DX7Headless() {}
	~DX7Headless() {}
	void run() { fprintf(stderr, "Headless mode. Hit <return> to quit\n"); getchar(); stop(); }
	void idle() { } // Idle loop body for LV2
	void stop() { running = false; }

	// For LV2
	void restore() { }
	Window widget() { return 0; } // FIX will crash LV2

	// Communication Lock-Free Queue from Synth
	ToGui *toGui;
	ToSynth *toSynth;

private:
	bool running = true;
};

