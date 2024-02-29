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
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <string.h>
#include <exception>
#include <limits>
#include <vector>
#include <unordered_map>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/xpm.h>

#include "Message.h"
#include "HD44780.h"

// Colors
const unsigned colBG = 0x463130;

const unsigned colFG = 0x0A0A0A;

const unsigned colHL = 0xC0C0C0;
const unsigned colLD = 0xC00000;
const unsigned colWK = 0xFFFFFF;
const unsigned colTR = 0x00FF00;
const unsigned colBK = 0x000000;
const unsigned colON = 0x0000FF;

const unsigned colSP = 0xFF6D5E;
const unsigned colBN = 0x00BBBE;
const unsigned colED = 0x1394E3;
const unsigned colFN = 0xBE7E50;

const unsigned colLCD = 0x0A0A0A;
const unsigned colLCD_BG = 0x74B097;

class DX7GUI;

class GUIException : public std::exception {
public:
	GUIException(const char* m) : errorMessage(m) {}
	const char *errorMessage;
};

class Grid {
public:
	Grid(float px, float py) : px(px), py(py) {}
	float px=0, py=0;
	static int col, row;
	int x() { return int(col*px); }
	int y() { return int(row*py); }
};

class WidgetBase;
typedef std::unordered_map<Window, WidgetBase*> WidgetList;
struct Context {
	Display* d;
	Window p; 
	DX7GUI* panel; 
	WidgetList& widgets;
	ToSynth*& toSynth;
};

class XPixMap {
public:
	XImage *image, *clp;
	Pixmap pix;
	Display* d;
	Window win;
	GC gc;
	XPixMap(Context ctx, const char **data);
	void draw(int x, int y);
	void draw(Grid g) { draw(g.x(), g.y()); }
};

class XText {
public:
	Display* display;
	Window window;
	GC gc;
	XFontStruct *font;
	float spacing = 1.0;

	void setColor(unsigned c)  { XSetForeground (display, gc, c); }
	void setFont(const char *fontname);

	XText(Context ctx);
	void draw(const char* text, int x, int y);
	void draw(const char* text, Grid g) { draw(text, g.x(), g.y()); }
};

class WidgetBase {
public:
	Message::CtrlID id=Message::CtrlID::none;
	const char *label = 0;

	WidgetBase(Context ctx, int x, int y, int w, int h);
	WidgetBase(Context ctx, Grid g, int w, int h) : WidgetBase(ctx, g.x(), g.y(), w, h) { }

	virtual void click(int x, int y) {}		// On Click event
	virtual void drag(int x, int y) {}		// On Drag event
	virtual void release(int x, int y) {}	// On Release event
	virtual void enter(unsigned state, int x, int y) {}	// On Mouse Enter event
	virtual void leave(unsigned state) {}	// On Mouse Leave event
	virtual void wheel_up() {}				// On Wheel Up Leave event
	virtual void wheel_down() {}			// On Wheel Down event
	virtual void incr() {}					// Increment
	virtual void decr() {}					// Decrement
	virtual void draw() {}					// Draw the widget
	virtual void send() {}					// Send value to synth
	virtual void set(int) {}				// Set the widget's value
	virtual int get() { return 0; }			// Get the widget's value
	virtual void setsend(int v) { set(v); send(); }	// Set and send
	virtual void setdraw(int v) { set(v); draw(); } // Set and draw

protected:
	Display* display;
	Window window, parent;
	WidgetList& widgets;	// Map of all widgets
	ToSynth*& toSynth;
	GC gc;
	int px, py, w, h;
	XText xtext;
};

class Widget : public WidgetBase {
public:
	Widget(Context ctx, int x, int y, int w, int h);
	Widget(Context ctx, Grid g, int w, int h) : Widget(ctx, g.x(), g.y(), w, h) {}
};

class Button : public WidgetBase {
public:
	Button(Context ctx, Grid g, const char *l=0, int w=34, int h=20);
	virtual void draw();
	virtual void click(int x, int y) { setsend(true); }
	virtual void release(int x, int y) { setsend(false); }
	virtual void send();
	virtual void set(int v) { down = v; }
	virtual int get() { return true; }
	virtual void incr() { }
	virtual void decr() { }
	bool down = false;
	unsigned color = 0;
};

class Slider : public Widget {
public:
	Slider(Context ctx, Grid g, int w=35, int h=100, int maxvalue=127);
	virtual void draw();
	virtual void click(int x, int y) { setsend(maxvalue*(h-y)/h); }
	virtual void wheel_up() { setsend(value+10); }
	virtual void wheel_down() { setsend(value-10); }
	virtual void drag(int x, int y) { click(x, y); }	// Move slider
	virtual void release(int x, int y) { }
	virtual void leave(unsigned) { }
	virtual void send();
	virtual void set(int v);
	virtual void incr() { setsend(value+1); }
	virtual void decr() { setsend(value-1); }

	int value = 0;
	const int maxvalue;
};

class ModWheel : public Widget {
public:
	ModWheel(Context ctx, Grid g, int w=30, int h=120, int maxvalue=127);
	virtual void draw();
	virtual void click(int x, int y) { setsend(maxvalue*(h-y)/h); }
	virtual void wheel_up() { setsend(value+10); }
	virtual void wheel_down() { setsend(value-10); }
	virtual void drag(int x, int y) { click(x, y); }	// Move slider
	virtual void release(int x, int y) { }
	virtual void leave(unsigned) { }
	virtual void send();
	virtual void set(int v);
	virtual void incr() { setsend(value+1); }
	virtual void decr() { setsend(value-1); }

	int value = 0;
	const int maxvalue;
};

class Bender : public ModWheel {
public:
	Bender(Context ctx, Grid g, int w=30, int h=120, int maxvalue=127);
	virtual void release(int x, int y) { setsend(maxvalue/2); }	// Re-center slider
	virtual void leave(unsigned) { setsend(maxvalue/2); }	// Re-center slider
	virtual void wheel_up() { setsend(value+10); }
	virtual void wheel_down() { setsend(value-10); }
};


class Keyboard : public Widget {
private:
	class Key : public Widget {
	public:
		Key(Context ctx, int k, bool wh, int x, int y, int w, int h);
		virtual void draw();
		virtual void click(int x, int y);
		virtual void release(int x, int y) { on = false; velocity = 0; send(); }
		virtual void enter(unsigned state, int x, int y);
		virtual void leave(unsigned state);
		virtual void send();
		void raise() { XRaiseWindow(display, window); }
		bool isWhite() { return iswhite; }

	private:
		GC gc;
		bool on = false;
		const uint8_t key;
		uint8_t velocity=0;
		const bool iswhite;
	};

public:
	struct InverseKeymap { char c; int n; };

	static const InverseKeymap inverseKeymap[]; // Key mappings
	char keymap[256] = {0};

	static const int keywid = 30, keyhgt = 180;
	static const int octaves = 5;
	static const int numkeys = 12*octaves+1;

	Key *keys[numkeys];

	Keyboard(Context ctx, Grid g);
	virtual void keypress(unsigned k);
	virtual void keyrelease(unsigned k);
};

class LCD : public WidgetBase {
public:
	LCD(Context ctx, Grid g, int w=128, int h=56);
	virtual void draw();
	virtual void click(int x, int y) { if(x<w/2) inst(0x10); else inst(0x14); }

	// HD44780 data bus
	void data(uint8_t x) { driver.data(x); }
	// HD44780 instruction set
	void inst(uint8_t x) { driver.inst(x); }

	void saveState() { }
	void restoreState(uint8_t *data, uint8_t len) { driver.restore(data, len); draw(); }

private:
	HD44780 driver;

	XFontStruct *font = 0;
	int blinkOn = 0; // blink counter

	// Geometry
	int bg_x, bg_y, bg_w, bg_h;
	int cwidth, cheight;
	int baseline1, baseline2, descent;
	int pad = 2;
};

class LED : public Widget {
public:
	LED(Context ctx, Grid g, int w=30, int h=50);
	virtual void draw();
	void setval(uint8_t v) { value = v; /*draw();*/ }
	void setdec(uint8_t v) { if(v<10) value = decmap[v]; /*draw();*/ }

private:
	uint8_t value = 0;
	uint8_t decmap[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99,
		0x92, 0x82, 0xF8, 0x80, 0x98};

};

#include <filesystem>
struct FileMgr {
	FileMgr(Context c, std::string path=".");
	virtual ~FileMgr() { XDestroyWindow(ctx.d, win); }

	// scrolling
	void show();
	void pageUp() { selected -= page_size; show(); }
	void pageDown() { selected += page_size; show(); }
	void selectHome() { selected = 0; show(); }
	void selectEnd() { selected = filelist.size()-1; show(); }
	void selectUp() { selected--; show(); }
	void selectDown() { selected++; show(); }

	void scrollUp() { --start; draw(); }
	void scrollDown() { ++start; draw(); }

	// On click action
	void click(int x, int y); // single click
	bool onreturn(); // return key or double click
	void draw();

	// Load a directory
	void load(std::string d);
	void factoryVoices(); // load factory ROMs

	// Event handler
	bool event(XEvent e);

	// X11
	Window win;
	Context ctx;
	Atom WM_DELETE_WINDOW;
	GC gc, gc_inv;
	int ascent = 10, descent = 5;
	int text_height = ascent+descent;
	std::chrono::time_point<std::chrono::system_clock> click_timer;

	// State
	int page_size = 0;
	int start = 0;
	int end = 0;
	int selected = 0;
	bool factory = false; // Default

	// Config
	bool all = false; // like ls -a
	bool filter = true; // apply filter
	std::vector<std::string> filters = { ".SYX", ".syx" };

	// File data
	std::vector<std::string> filelist; // displayed filenames
	std::filesystem::path dir; // current directory
};

struct FileMgrPopup {
	FileMgrPopup(Context c) : ctx(c) { }
	// True if this is our event
	bool event(XEvent e) {
		if(p && p->win == e.xany.window) {
			if(p->event(e)) popdown();
			return true;
		} else return false;
	}
	// Alternate up/down
	void popup() {
		if(p==0) p = new FileMgr(ctx, path);
		else popdown();
	}

private:
	void popdown() {
		if(p) path = p->dir;
		delete p;
		p = 0;
	}

	Context ctx;
	FileMgr *p = 0;
	std::string path = ".";
};

class Cartridge : public Widget {
private:
	static const int Size = 40;
public:
	Cartridge(Context ctx, Grid g, int w=255, int h=20);
	virtual void draw();
	virtual void click(int x, int y) {
		if(x<20) {
			if(y<h/2) protect = false; else protect = true;
		} else {
			filemgr.popup();
		}
		draw();
		send();
	}
	virtual void wheel_up() { protect = false; draw(); send(); }	
	virtual void wheel_down() { protect = true; draw(); send(); }	
	virtual void send() { toSynth->protect(protect); }
	virtual void setText(const char *t) { strncpy(text, t, Size); protect = true; draw(); send(); }

	bool protect = true;
	char text[Size+1];
	FileMgrPopup filemgr;
};

extern std::string factoryROMs[8];
