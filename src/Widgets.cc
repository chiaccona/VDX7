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

#include <assert.h>

#include "Widgets.h"

XPixMap::XPixMap(Context ctx, const char **data) : d(ctx.d), win(ctx.p) {
	if(XpmCreateImageFromData(ctx.d, (char**)data, &image, &clp, 0))  {
		fprintf(stderr, "XpmCreateImageFromData - could not create image\n");
		throw(GUIException("pixmap"));
	}
	gc = XCreateGC(d, win, 0, 0);
	if(clp) {
		pix = XCreatePixmap(d, win, clp->width, clp->height, clp->depth);
		GC gc2 = XCreateGC(d, pix, 0, 0);
		XPutImage(d, pix, gc2, clp, 0, 0, 0, 0, clp->width, clp->height);
	}
}

void XPixMap::draw(int x, int y) {
	if(clp) {
		XSetClipMask(d, gc, pix);
		XSetClipOrigin(d, gc, x-image->width/2, y-image->height/2);
	}
	XPutImage(d, win, gc, image, 0, 0,
		x-image->width/2, y-image->height/2,
		image->width, image->height);
}

XText::XText(Context ctx) : display(ctx.d), window(ctx.p) {
	gc = XCreateGC (display, window, 0, 0);
	XSetBackground (display, gc, colBG);
	XSetForeground (display, gc, colFG);
	setFont("-*-fixed-*-r-*-*-9-*-*-*-*-*-*-*");
}

void XText::setFont(const char* fontname) {
	font = XLoadQueryFont (display, fontname);
	// If the font could not be loaded, revert to the "fixed" font
	if (! font) {
		fprintf (stderr, "unable to load font %s: using fixed\n", fontname);
		font = XLoadQueryFont (display, "fixed");
	}
	XSetFont (display, gc, font->fid);
}

void XText::draw(const char* text, int x, int y) {
	int direction, ascent, descent;
	XCharStruct overall;
	char str[256];
	strncpy(str, text, 255);
	int line=0;
	for(char *s = strtok(str, "\n"); s != 0; s = strtok(0, "\n")) {
		// Center the first line
		XTextExtents (font, s, strlen(s),
			&direction, &ascent, &descent, &overall);
		int x2 = x - overall.width / 2;
		y += (ascent - descent) / 2 + (ascent + descent)*spacing*line++;
		XDrawString(display, window, gc, x2, y, s, strlen(s));
	}
}

WidgetBase::WidgetBase(Context ctx, int x, int y, int w, int h)
	: display(ctx.d), parent(ctx.p), widgets(ctx.widgets), toSynth(ctx.toSynth),
		px(x), py(y), w(w), h(h), xtext(ctx) {
	// Do nothing
}

Widget::Widget(Context ctx, int x, int y, int w, int h)
		: WidgetBase(ctx, x, y, w, h) {
	window=XCreateSimpleWindow(display, parent, px-w/2, py-h/2, w, h, 0, 0, colBG);
	if(!window) {
		fprintf(stderr, "Cannot create window\n");
		throw(GUIException("window"));
	}
	XSelectInput(display, window,
		KeyPressMask|KeyReleaseMask
		|ButtonPressMask|ButtonReleaseMask
		|EnterWindowMask|LeaveWindowMask
		|Button1MotionMask
		|ExposureMask);
	XMapWindow(display, window);
	widgets[window] = this;

	gc = XCreateGC(display, window, 0, 0);
	XSetBackground(display, gc, colBG);
	XSetForeground(display, gc, colFG);
	XSetLineAttributes(display, gc, 4, 0, 0, 0);
}


Keyboard::Key::Key(Context ctx, int k, bool wh, int x, int y, int w, int h)
	: Widget(ctx, x, y, w, h), key(k), iswhite(wh) {
	gc = XCreateGC(display, window, 0, 0);
	XSetLineAttributes(display, gc, 1, 0, 0, 0);
}

void Keyboard::Key::draw() {
	if(iswhite) {
		if (on) {
			XSetForeground(display, gc, colON);
			XFillRectangle(display, window, gc, 0, 0, w, h);
		} else {
			XSetForeground(display, gc, colWK);
			XFillRectangle(display, window, gc, 0, 0, w, h);
			XSetForeground(display, gc, colBK);
			XDrawRectangle(display, window, gc, 0, 0, w, h);
		}
	} else {
		if (on) XSetForeground(display, gc, colON);
			else XSetForeground(display, gc, colBK);
		XFillRectangle(display, window, gc, 0, 0, w, h);
	}
}

void Keyboard::Key::click(int x, int y) {
	XUngrabPointer(display, CurrentTime);
	on = true; velocity = 127*y/h; send();
}

void Keyboard::Key::enter(unsigned state, int x, int y) {
	if(! (state&Button1Mask)) return;
	on = true; velocity = 127*y/h; send();
}

void Keyboard::Key::leave(unsigned state) {
	if(! (state&Button1Mask)) return;
	on = false; velocity = 0; send(); 
}

void Keyboard::Key::send() {
	if(on) toSynth->key_on(key, velocity);
	else toSynth->key_off(key);
}

constexpr const Keyboard::InverseKeymap Keyboard::inverseKeymap[] = {
	{'q',  1}, {'2',  2},
	{'w',  3}, {'3',  4},
	{'e',  5},
	{'r',  6}, {'5',  7},
	{'t',  8}, {'6',  9},
	{'y', 10}, {'7', 11},
	{'u', 12},
	{'i', 13}, {'9', 14},
	{'o', 15}, {'0', 16},
	{'p', 17},
	{'[', 18}, {'=', 19},
	{']', 20},
	{'\\', 22},

	{'z', 13}, {'s', 14},
	{'x', 15}, {'d', 16},
	{'c', 17},
	{'v', 18}, {'g', 19},
	{'b', 20}, {'h', 21},
	{'n', 22}, {'j', 23},
	{'m', 24},
	{',', 25}, {'l', 26},
	{'.', 27}, {';', 28},
	{'/', 29},
};

Keyboard::Keyboard(Context ctx, Grid g)
	: Widget(ctx, g, keywid*(octaves*7+1), keyhgt) {

	for(auto ik : inverseKeymap) keymap[int(ik.c)] = ik.n;

	Context keyctx = ctx;
	keyctx.p = window; // make keyboard the parent

	for(int k=0; k<numkeys; k++) {
		int note = k%12;
		if(note>=5) note++;
		int oct = k/12;
		bool iswhite = true;
		int kx = (note+1)*keywid/2 + oct*keywid*7;
		int kw = keywid;
		int kh = keyhgt;
		if(note&0x1) { // black key
			iswhite = false;
			kw = kw*6/10;
			kh = kh*65/100;
		}
		int ky = kh/2; // undo widget centering
		// adjust black key spacing +/-7% 
		int delta = 7*keywid * 7/1200 + 1;
		if(note==1 || note==7) kx -= delta;
		else if(note==3 || note==11) kx += delta;
		keys[k] = new Key(keyctx, k, iswhite, kx, ky, kw, kh);
	}
	for(int k=0; k<numkeys; k++) if(! keys[k]->isWhite()) keys[k]->raise();
}
	
void Keyboard::keypress(unsigned k) {
	if(k<256 && keymap[k]) {
		keys[keymap[k]-1]->click(0, keyhgt);
		keys[keymap[k]-1]->draw();
	}
}

void Keyboard::keyrelease(unsigned k) {
	if(k<256 && keymap[k]) {
		keys[keymap[k]-1]->release(0, 0);
		keys[keymap[k]-1]->draw();
	}
}

Button::Button(Context ctx, Grid g, const char *l, int w, int h)
	: WidgetBase(ctx, g, w, h) {
	// "Invisible" window
	XSetWindowAttributes attr;
	attr.background_pixmap = ParentRelative;
	attr.event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask;
	window = XCreateWindow(display, parent,
		px-w/2, py-h/2, w, h, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWBackPixmap|CWEventMask, &attr);
	if(!window) {
		fprintf(stderr, "Cannot create window\n");
		throw(GUIException("window"));
	}
	XGCValues gcv;
	gcv.function = GXand;
	gc = XCreateGC(display, window, GCFunction, &gcv);
	XSetForeground(display, gc, 0x808080);
	XMapWindow(display, window);
	widgets[window] = this;
}

void Button::draw() {
	if(down) XFillRectangle(display, window, gc, 1, 1, w-2, h-2);
		else XClearArea(display, window, 0, 0, w, h, 0);
}

void Button::send() { if(down) toSynth->buttondown(id); else toSynth->buttonup(id); }

Slider::Slider(Context ctx, Grid g, int w, int h, int maxvalue)
	: Widget(ctx, g, w, h), maxvalue(maxvalue) {
}	

void Slider::draw() {
	XSetForeground(display, gc, 0x000000);
	XFillRectangle(display, window, gc, 0, 0, w, h);

	XSetForeground(display, gc, 0x101010);
	XSetLineAttributes(display, gc, 5, 0, 0, 0);
	XDrawLine(display, window, gc, w/2, 0, w/2, h);

	const int sh=16, sw=w-2;
	int posY = (maxvalue-value)*(h-sh)/maxvalue+sh/2;

	XSetForeground(display, gc, 0x444444);
	XFillRectangle(display, window, gc, (w-sw)/2, posY-sh/2, sw, sh);
	XSetLineAttributes(display, gc, 2, 0, 0, 0);
	XSetForeground(display, gc, 0xCBCBCB);
	XDrawLine(display, window, gc, 1, posY, sw, posY);
}

void Slider::send() { toSynth->analog(id, value); }

void Slider::set(int v) {
	value = (v<0 ? 0 : (v>maxvalue ? maxvalue : v));
}

Bender::Bender(Context ctx, Grid g, int w, int h, int maxvalue)
	: ModWheel(ctx, g, w, h, maxvalue) { value = maxvalue/2; }


ModWheel::ModWheel(Context ctx, Grid g, int w, int h, int maxvalue)
	: Widget(ctx, g, w, h), maxvalue(maxvalue) {
	XSetLineAttributes(display, gc, 16, 0, 0, 0);
}	

void ModWheel::draw() {
	XClearArea(display, window, 0, 0, w, h, 0);
	XSetForeground(display, gc, colFG);

	XDrawLine(display, window, gc, w/2, 8, w/2, h-8);
	XFillArc(display, window, gc, w/2-8, 0, 16, 16, 0, 360*64);
	XFillArc(display, window, gc, w/2-8, h-16, 16, 16, 0, 360*64);

	XSetForeground(display, gc, colHL);
	XFillArc(display, window, gc, (w/2-8)+2, (maxvalue-value)*(h-16)/maxvalue+2, 12, 12, 0, 360*64);
}

void ModWheel::send() { toSynth->analog(id, value); }

void ModWheel::set(int v) {
	value = (v<0 ? 0 : (v>maxvalue ? maxvalue : v));
}

LCD::LCD(Context ctx, Grid g, int w, int h)
	: WidgetBase(ctx, g, w, h) {

	// Should be fixed 7x14 bold
	const char *fontname = "-misc-fixed-bold-r-*-*-14-*-*-*-*-*-*-*";
	//const char *fontname = "-misc-hd44780-regular-r-normal--10-100-75-75-c-60-misc-1";
	font = XLoadQueryFont (display, fontname);
	// If the font could not be loaded, revert to the "fixed" font
	// FIX depending on font, this may not fit window
	if (!font) {
		fprintf (stderr, "unable to load font %s: using fixed\n", fontname);
		font = XLoadQueryFont (display, "fixed");
	}

	cwidth = font->max_bounds.width;
	cheight = font->max_bounds.ascent + font->max_bounds.descent;
	descent = font->max_bounds.descent;
	bg_w = cwidth*16 + 2*pad;
	bg_h = cheight*2 + 2*pad;
	bg_x = (w - bg_w)/2;
	bg_y = (h - bg_h)/2;
	baseline1 = bg_y+cheight;
	baseline2 = bg_y+2*cheight;
	window=XCreateSimpleWindow(display, parent, px-w/2, py-h/2, w, h, 0, 0, colFG);
	if(!window) {
		fprintf(stderr, "Cannot create window\n");
		throw(("window"));
	}
	XSelectInput(display, window, ButtonPressMask|ButtonReleaseMask|ExposureMask);
	XMapWindow(display, window);

	gc = XCreateGC(display, window, 0, 0);
	XSetLineAttributes(display, gc, 0, LineSolid, 0, 0);
	XSetFont(display, gc, font->fid);
	XClearArea(display, window, 0, 0, w, h, 0);
	XSetForeground(display, gc, colLCD_BG);
	XFillRectangle(display, window, gc, bg_x, bg_y, bg_w, bg_h);
	XFlush(display);
}

void LCD::draw() {
	WidgetBase::draw();
	// Clear
	XSetForeground(display, gc, colLCD_BG);
	XFillRectangle(display, window, gc, bg_x, bg_y, bg_w, bg_h);
	// Draw text
	XSetForeground(display, gc, colLCD);

	XDrawString(display, window, gc, bg_x+pad, baseline1, driver.line1, 16);
	if(driver.lines) XDrawString(display, window, gc, bg_x+pad, baseline2, driver.line2, 16);

	if(driver.cursor_pos >= 16) return;
	int x = bg_x+driver.cursor_pos*cwidth + pad;
	int y = baseline1+descent;
	if(driver.lines && driver.cursor_line==2) y = baseline2+descent;
	if(driver.cursor_blink) { // Blink cursor position
		// shift>>4 implies blinking at 1/16 of refresh rate
		if(!((blinkOn>>0)&4)) XClearArea(display, window, x, y-cheight, cwidth, cheight, 0);
		blinkOn++;
	} else blinkOn = -1;
	// Cursor
	if(driver.cursor_on) XFillRectangle(display, window, gc, x, y, cwidth, 1);
}

LED::LED(Context ctx, Grid g, int w, int h)
	: Widget(ctx, g, w, h) {
	XSetForeground(display, gc, colFG);
	XSetLineAttributes(display, gc, 3, LineSolid, 0, 0);
	XFillRectangle(display, window, gc, 0, 0, w, h);
}

void LED::draw() {
	Widget::draw();
	XSetForeground(display, gc, colFG);
	XFillRectangle(display, window, gc, 0, 0, w, h);
	XSetForeground(display, gc, colLD);
	uint8_t top = 12;
	uint8_t mid = h/2;
	uint8_t bot = h-12;
	uint8_t left = 8;
	uint8_t right = w-8;
	if(~value&(1<<0)) XDrawLine(display, window, gc, left+2+2, top, right-2+2, top); // A
	if(~value&(1<<1)) XDrawLine(display, window, gc, right+2, top+2, right, mid-2); // B
	if(~value&(1<<2)) XDrawLine(display, window, gc, right, mid+2, right-2, bot-2); // C
	if(~value&(1<<3)) XDrawLine(display, window, gc, left+2-2, bot, right-2-2, bot); // D
	if(~value&(1<<4)) XDrawLine(display, window, gc, left, mid+2, left-2, bot-2); // E
	if(~value&(1<<5)) XDrawLine(display, window, gc, left+2, top+2, left, mid-2); // F
	if(~value&(1<<6)) XDrawLine(display, window, gc, left+2, mid, right-2, mid); // G
	if(~value&(1<<7)) XDrawLine(display, window, gc, right+2, bot, right+5, bot); // Dot
}

Cartridge::Cartridge(Context ctx, Grid g, int w, int h)
	: Widget(ctx, g.x()+w/2-20, g.y(), w, h), filemgr(ctx) {
	XSetWindowBorderWidth(ctx.d, window, 2);
	XSetWindowBorder(ctx.d, window, 0x000080);

	const char *fontname = "-*-fixed-*-r-*-*-9-*-*-*-*-*-*-*";
	XFontStruct *font = XLoadQueryFont (display, fontname);
	// If the font could not be loaded, revert to the "fixed" font
	if (! font) {
		fprintf (stderr, "unable to load font %s: using fixed\n", fontname);
		font = XLoadQueryFont (display, "fixed");
	}
	XSetFont (display, gc, font->fid);
	strncpy(text, "<No Cartridge>", Size);
}

void Cartridge::draw() {
	XSetForeground(display, gc, colFG);
	XFillRectangle(display, window, gc, 0, 0, w, h);
	XSetForeground(display, gc, colBG);
	XFillRectangle(display, window, gc, 0, 0, 10, h);
	XSetForeground(display, gc, colHL);
	XFillRectangle(display, window, gc, 0, protect*h/2, 10, h/2);
	if(text[0]) XDrawString(display, window, gc, 12, h/2+2, text, strlen(text));
}

int Grid::row = 80;
int Grid::col = 70;

#include <filesystem>
#include <algorithm>
FileMgr::FileMgr(Context c, std::string path) : ctx(c) {
	win = XCreateSimpleWindow(ctx.d, RootWindow(ctx.d,0), 10, 10, 200, 160, 0, colWK, colBG);
	XSelectInput(ctx.d, win, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask);
	XMapWindow(ctx.d, win);

	WM_DELETE_WINDOW = XInternAtom(ctx.d, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(ctx.d, win, &WM_DELETE_WINDOW, 1);

	// Create a normal gc and inverted gc for selection
	XGCValues values;
	unsigned long valuemask = GCForeground | GCBackground;
	values.foreground = colWK;
	values.background = colBG;
	gc = XCreateGC(ctx.d, win, valuemask, &values);
	values.foreground = colBG;
	values.background = colWK;
	gc_inv = XCreateGC(ctx.d, win, valuemask, &values);

	// Use default font in GC
	XFontStruct* xfs = XQueryFont(ctx.d, XGContextFromGC(gc));
	if(xfs) {
		ascent = xfs->ascent;
		descent = xfs->descent;
		text_height = ascent+descent;
	}

	load(path);
}

// Set display window
void FileMgr::show() {
	if(selected<0) selected = 0;
	if(selected>=(int)filelist.size()) selected = filelist.size()-1;
	if(selected < start) start = selected;
	if(selected >= end) start = selected-page_size+1;
	draw();
}

// On click action
void FileMgr::click(int x, int y) {
	selected = start + y/text_height;
	if(selected >= (int)filelist.size()) selected = filelist.size() - 1;
	// Directory
	if(filelist[selected].back() == '/') {
		if(factory) load(std::string(dir)); // exit factory mode
		else load(std::string(dir)+"/"+filelist[selected]); // chdir
	}
	draw();
}

void FileMgr::draw() {
	XWindowAttributes wa;
	XGetWindowAttributes(ctx.d, win, &wa);
	page_size = wa.height/text_height;

	long fs = filelist.size(); // needs to be signed
	if(start > fs-page_size) start = fs-page_size;
	if(start<0) start = 0;
	end = start + page_size;
	if(end > fs) end = fs;

	XClearArea(ctx.d, win, 0, 0, 0, 0, 0);
	int y_offset = ascent;
	for(int i = start; i<end; i++) {
		if(i==selected) {
			XDrawImageString(ctx.d, win,gc_inv, 10, y_offset,
				filelist[i].c_str(), filelist[i].size());
		} else {
			XDrawImageString(ctx.d, win, gc, 10, y_offset,
				filelist[i].c_str(), filelist[i].size());
		}
		y_offset += text_height;
	}
}

// Load factory ROM list
inline void FileMgr::factoryVoices() {
	XStoreName(ctx.d, win, "Factory Voices");

	// Reset lists
	filelist.clear();

	// Create displayed listing
	filelist.push_back("../");

	for (auto & rom : factoryROMs) filelist.push_back(rom);
	start = end = 0;
	selected = 0;
	factory = true;
	draw();
}

// Load a directory
void FileMgr::load(std::string d) {
	filelist.clear();
	filelist.push_back("../");
	try {
		dir = std::filesystem::canonical(d);
		if(!is_directory(dir)) dir = dir.parent_path(); // strip off filename, if any
		for (auto const& dir_entry : std::filesystem::directory_iterator{dir}) {
			auto path = dir_entry.path();
			if(!all) if(path.filename().string()[0] == '.') continue; // Skip hidden files
			if(is_regular_file(path)) {
				if(filter) { // apply file extension filters
					for(auto &s : filters) {
						if(s == path.extension()) {
							filelist.push_back(path.filename());
							break;
						}
					}
				} else filelist.push_back(path.filename());
			}
			else if(is_directory(path)) filelist.push_back(path.filename().string() + "/");
		}
	} catch(const std::exception& ex) { // can't read dir
		fprintf(stderr, "Could not read \"%s\":\n%s\n", d.c_str(), ex.what());
	}
	// Sort files
	std::sort(filelist.begin(), filelist.end());
	start = end = selected = 0;
	factory = false;
	XStoreName(ctx.d, win, dir.c_str()); // display in title bar
	draw();
}

bool FileMgr::onreturn() {
	if(factory) {
		int rom = selected-1;
		printf("selected ROM: %s\n", factoryROMs[rom].c_str());
		ctx.toSynth->load_cartridge_num(rom);
		return true;
	} else {
		std::string filename = std::string(dir)+"/"+filelist[selected];
		if(filename.back() == '/') { // Directory
			load(filename);
			return false;
		} else { // File
			if(filename.length()<=255)
				ctx.toSynth->cartridge_file((uint8_t*)(filename.c_str()), filename.length());
			else fprintf(stderr, "File path too long (%ld>255): %s\n", filename.length(), filename.c_str());
			return true;
		}
	}
}

// Event handler
bool FileMgr::event(XEvent e) {
	switch(e.type) {
		case Expose: draw(); break;

		case KeyPress: {
			KeySym keysym = XLookupKeysym(&e.xkey, e.xkey.state&ShiftMask);
			switch(keysym) {
				case XK_Home: selectHome(); break;
				case XK_End: selectEnd(); break;
				case XK_Page_Up: pageUp(); break;
				case XK_Page_Down: pageDown(); break;
				case XK_Up: selectUp(); break;
				case XK_Down: selectDown(); break;
				case XK_f: 
					if(!factory) factoryVoices();
					else load(std::string(dir)); // exit factory mode
					break;
				case XK_asterisk: filter = !filter; load(dir); break;
				case XK_Return: if(onreturn()) return true; else break;
				case XK_Escape: return true; // quit
				default: break;
			}
			break;
		}

		case ButtonPress:
			switch(e.xbutton.button) {
				case 1: click(e.xbutton.x, e.xbutton.y); break;
				case 4: scrollUp(); break;
				case 5: scrollDown(); break;
			}
			break;

		case ButtonRelease:
			switch(e.xbutton.button) {
				case 1: { // double click detection
        			auto now = std::chrono::system_clock::now();
        			double diff_ms = std::chrono::duration <double, std::milli> (now - click_timer).count();
        			click_timer = now;
        			if(diff_ms<500) { // Double click
						if(onreturn()) return true; else break;
					}
					break;
				}
				default: break;
			}
			break;

		case ClientMessage: 
			if ((unsigned)(e.xclient.data.l[0]) == WM_DELETE_WINDOW) return true; // quit

		default: break;
	}
	return false;
}

// Display names for the standard factory voice banks
std::string factoryROMs[8] = {
	"ROM1A Master (EU/JP)",
	"ROM1B Keyboard & Plucked (EU/JP)",
	"ROM2A Orchestral & Percussive (EU/JP)",
	"ROM2B Synth, Complex & Effects (EU/JP)",
	"ROM3A Master (US)",
	"ROM3B Keyboard & Plucked (US)",
	"ROM4A Orchestral & Percussive (US)",
	"ROM4B Synth, Complex & Effects (US)",
};

