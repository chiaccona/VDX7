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

#include "Gui.h"
#include "panel.xpm"

#include <unistd.h>
#include <chrono>
#include <thread>
#include <X11/xpm.h>

void DX7GUI::idle() {
	if(XPending(display) == 0) {
		processMessages();
		lcd->draw();
		led1->draw();
		led2->draw();
		XFlush(display);
	} else while(XPending(display)) processEvent();
}

void DX7GUI::run() {
	while(running) {
		if(XPending(display) == 0) {
			// Set a timer to avoid blocking XNextEvent call
			struct timeval timer;
			int fd = ConnectionNumber(display);
			fd_set in_fds;
			FD_ZERO(&in_fds);
			FD_SET(fd, &in_fds);
			timer.tv_sec = 0;
			timer.tv_usec = 40000; // 0.04 sec 25fps
			// Wait until an X event becomes available, or the
			// timer expires at the given (maximum) frame rate
			if(!select(fd+1, &in_fds, 0, 0, &timer)) {
				// Timer fired
				// Receive messages from synth, Update the LED and LCD
				processMessages();
				lcd->draw();
				led1->draw();
				led2->draw();
				XFlush(display);
			}
		} else processEvent();
	}
}

DX7GUI::DX7GUI(bool showKeyboard, Window _parent) : showKeyboard(showKeyboard) {
	if(showKeyboard) height = 360; else height = 150;

	display=XOpenDisplay(NULL);
	if(!display) {
		fprintf(stderr, "Cannot open display\n");
		throw(GUIException("display"));
	}
	visual=DefaultVisual(display, 0);
	if(!visual) {
		fprintf(stderr, "Cannot open visual\n");
		throw(GUIException("visual"));
	}
	if(visual->c_class!=TrueColor) {
		fprintf(stderr, "Cannot handle non true color visual ...\n");
		throw(GUIException("truecolor"));
	}
	if(!_parent) parent = RootWindow(display, 0);
	else parent = _parent;
	panel=XCreateSimpleWindow(display, parent,
			0, 0, width, height, 0, 0, colBG);
	if(!panel) {
		fprintf(stderr, "Cannot open window\n");
		throw(GUIException("window"));
	}

	if(showKeyboard) XAutoRepeatOff(display); // long keypresses don't repeat

	gc = XCreateGC(display, panel, 0, 0);
	XSetBackground (display, gc, colBG);
	XSetForeground (display, gc, colFG);

	// Set window name
	XTextProperty wm_name;
	char *name = (char*)"VDX7";
	XStringListToTextProperty(&name, 1, &wm_name);
	XSetWMName(display, panel, &wm_name);

	// Set iconified name
	XClassHint *class_hint = XAllocClassHint();
	if (class_hint) {
		class_hint->res_name = class_hint->res_class = (char *)name;
		XSetClassHint(display, panel, class_hint);
		XFree(class_hint);
	}

	// Don't allow window resize
	XSizeHints *size_hints=XAllocSizeHints();
	if(size_hints==NULL) {
		fprintf(stderr,"XMallocSizeHints() failed\n");
		throw(GUIException("sizehints"));
	}
	size_hints->flags= PMinSize | PMaxSize | PBaseSize;
	size_hints->min_width = size_hints->max_width = size_hints->base_width = width;
	size_hints->min_height = size_hints->max_height = size_hints->base_height = height;
	XSetWMNormalHints(display, panel, size_hints);

	// Allow WM delete
	wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, panel, &wmDeleteMessage, 1);

	// Inputs
	XSelectInput(display, panel,
			KeyPressMask|KeyReleaseMask
			|ButtonPressMask|ButtonReleaseMask
			|Button1MotionMask
			|ExposureMask);

	// Background image
	Pixmap panel_bg;
	XpmCreatePixmapFromData(display, panel, (char**)&panel_xpm, &panel_bg, 0, 0);
	XSetWindowBackgroundPixmap(display, panel, panel_bg);
	XFreePixmap(display, panel_bg);

	XMapWindow(display, panel);
	Context ctx{display, panel, this, widgets, toSynth};

	// Grid spacing
	Grid::row = 60; Grid::col = 37;

	// Controls
	b_no		= new Button(ctx, { 3.5, 2}, "-1  <");
	b_yes		= new Button(ctx, { 4.5, 2}, "+1  >");

	b_w			= new Button(ctx, { 6.0, 1}, "    W"); b_w->color = colSP;
	b_x			= new Button(ctx, { 7.0, 1}, "    X");
	b_y			= new Button(ctx, { 8.0, 1}, "    Y");
	b_z			= new Button(ctx, { 9.0, 1}, "    Z"); b_z->color = colED;

	b_chr		= new Button(ctx, { 6.0, 2}, "CHAR"); b_chr->color = colED;
	b_dash		= new Button(ctx, { 7.0, 2}, "    -");
	b_dot		= new Button(ctx, { 8.0, 2}, "    .");
	b_sp		= new Button(ctx, { 9.0, 2}, "SP"); b_sp->color = colFN;

	b_1			= new Button(ctx, {17.0, 1}, "1   1");
	b_2			= new Button(ctx, {18.0, 1}, "2   2");
	b_3			= new Button(ctx, {19.0, 1}, "3   3");
	b_4			= new Button(ctx, {20.0, 1}, "4   4");
	b_5			= new Button(ctx, {21.0, 1}, "5   5");
	b_6			= new Button(ctx, {22.0, 1}, "6   6");
	b_7			= new Button(ctx, {23.0, 1}, "7   7");
	b_8			= new Button(ctx, {24.0, 1}, "8   8");
	b_9			= new Button(ctx, {25.0, 1}, "9   9");
	b_10		= new Button(ctx, {26.0, 1}, "10  0");
	b_11		= new Button(ctx, {27.0, 1}, "11  A");
	b_12		= new Button(ctx, {28.0, 1}, "12  B");
	b_13		= new Button(ctx, {29.0, 1}, "13  C");
	b_14		= new Button(ctx, {30.0, 1}, "14  D");
	b_15		= new Button(ctx, {31.0, 1}, "15  E");
	b_16		= new Button(ctx, {32.0, 1}, "16  F");

	b_17		= new Button(ctx, {17.0, 2}, "17  G");
	b_18		= new Button(ctx, {18.0, 2}, "18  H");
	b_19		= new Button(ctx, {19.0, 2}, "19  I");
	b_20		= new Button(ctx, {20.0, 2}, "20  J");
	b_21		= new Button(ctx, {21.0, 2}, "21  K");
	b_22		= new Button(ctx, {22.0, 2}, "22  L");
	b_23		= new Button(ctx, {23.0, 2}, "23  M");
	b_24		= new Button(ctx, {24.0, 2}, "24  N");
	b_25		= new Button(ctx, {25.0, 2}, "25  O");
	b_26		= new Button(ctx, {26.0, 2}, "26  P");
	b_27		= new Button(ctx, {27.0, 2}, "27  Q");
	b_28		= new Button(ctx, {28.0, 2}, "28  R");
	b_29		= new Button(ctx, {29.0, 2}, "29  S");
	b_30		= new Button(ctx, {30.0, 2}, "30  T");
	b_31		= new Button(ctx, {31.0, 2}, "31  U");
	b_32		= new Button(ctx, {32.0, 2}, "32  V");

	volume	= new Slider(ctx,       { 0.5, 1.5}); volume->set(96); // Default 75%
	data	= new Slider(ctx,       { 2.0, 1.5});

	lcd			= new LCD(ctx,      {14.0, 1.5}, 148, 56);

	led2		= new LED(ctx,      {10.5, 1.5}, 37, 56);
	led1		= new LED(ctx,      {11.5, 1.5}, 37, 56);

	cartridge	= new Cartridge(ctx, {10.0, 0.5});

	if(showKeyboard) {
		pitchbend	= new Bender(ctx,   { 0.5, 4  });
		modulate	= new ModWheel(ctx, { 2.0, 4  });
		keyboard	= new Keyboard(ctx, {17.8, 4.4});
	}

	// Message ID mappings

	// Buttons
	b_no->id = Message::CtrlID::b_no;
	b_yes->id = Message::CtrlID::b_yes;
	b_w->id = Message::CtrlID::b_w;
	b_x->id = Message::CtrlID::b_x;
	b_y->id = Message::CtrlID::b_y;
	b_z->id = Message::CtrlID::b_z;
	b_chr->id = Message::CtrlID::b_chr;
	b_dash->id = Message::CtrlID::b_dash;
	b_dot->id = Message::CtrlID::b_dot;
	b_sp->id = Message::CtrlID::b_sp;
	b_1->id = Message::CtrlID::b_1;
	b_2->id = Message::CtrlID::b_2;
	b_3->id = Message::CtrlID::b_3;
	b_4->id = Message::CtrlID::b_4;
	b_5->id = Message::CtrlID::b_5;
	b_6->id = Message::CtrlID::b_6;
	b_7->id = Message::CtrlID::b_7;
	b_8->id = Message::CtrlID::b_8;
	b_9->id = Message::CtrlID::b_9;
	b_10->id = Message::CtrlID::b_10;
	b_11->id = Message::CtrlID::b_11;
	b_12->id = Message::CtrlID::b_12;
	b_13->id = Message::CtrlID::b_13;
	b_14->id = Message::CtrlID::b_14;
	b_15->id = Message::CtrlID::b_15;
	b_16->id = Message::CtrlID::b_16;
	b_17->id = Message::CtrlID::b_17;
	b_18->id = Message::CtrlID::b_18;
	b_19->id = Message::CtrlID::b_19;
	b_20->id = Message::CtrlID::b_20;
	b_21->id = Message::CtrlID::b_21;
	b_22->id = Message::CtrlID::b_22;
	b_23->id = Message::CtrlID::b_23;
	b_24->id = Message::CtrlID::b_24;
	b_25->id = Message::CtrlID::b_25;
	b_26->id = Message::CtrlID::b_26;
	b_27->id = Message::CtrlID::b_27;
	b_28->id = Message::CtrlID::b_28;
	b_29->id = Message::CtrlID::b_29;
	b_30->id = Message::CtrlID::b_30;
	b_31->id = Message::CtrlID::b_31;
	b_32->id = Message::CtrlID::b_32;

	// Analog sources sent by GUI
	data->id = Message::CtrlID::data;
	if(showKeyboard) {
		pitchbend->id = Message::CtrlID::pitchbend;
		modulate->id = Message::CtrlID::modulate;
	}

	// Read by Synth (not sent to CPU)
	volume->id = Message::CtrlID::volume;

	// These don't send to CPU
	lcd->id = Message::CtrlID::none;
	led1->id = Message::CtrlID::none;
	led2->id = Message::CtrlID::none;
	cartridge->id = Message::CtrlID::none;
}

DX7GUI::~DX7GUI() {
	lcd->saveState();
	XUnmapWindow(display, panel);
	if(showKeyboard) XAutoRepeatOn(display);
	XSync(display, false);
}

// Receive and act on messages sent from synth
void DX7GUI::processMessages() {
	Message msg;
	while(toGui->pop(msg)) {
		switch(Message::CtrlID(msg.byte1)) {
			case Message::CtrlID::lcd_inst: lcd->inst(msg.byte2); break;
			case Message::CtrlID::lcd_data: lcd->data(msg.byte2); break;
			case Message::CtrlID::led1_setval: led1->setval(msg.byte2); break;
			case Message::CtrlID::led2_setval: led2->setval(msg.byte2); break;
			case Message::CtrlID::cartridge_num: cartridge->setText(factoryROMs[msg.byte2&0x7].c_str()); break;
			case Message::CtrlID::cartridge_name: {
					int len = msg.byte2;
					uint8_t data[len+1];
					toGui->getBinary(data, len);
					data[len] = 0;
					cartridge->setText((const char*)data);
					break;
				}
			case Message::CtrlID::lcd_state: {
					int len = msg.byte2;
					uint8_t data[len+1];
					toGui->getBinary(data, len);
					data[len] = 0;
					lcd->restoreState(data, len);
					break;
				}
			default: break;
		}
	}
}

// Event loop body
void DX7GUI::processEvent() {

	// Have X event to process
	XEvent ev;
	XNextEvent(display, &ev);

	// Run popup event loop
	if(cartridge->filemgr.event(ev)) return;

	switch(ev.type) {
	case Expose: {
			auto it = widgets.find(ev.xexpose.window);
			if(it != widgets.end()) it->second->draw();
		}
		break;

	case ButtonPress:
		switch(ev.xbutton.button) {

		case 1: {
				auto it = widgets.find(ev.xany.window);
				if(it != widgets.end()) {
					it->second->click(ev.xbutton.x, ev.xbutton.y);
					it->second->draw();
					if(fineAdjust && !hold) hold = it->second;
				}
			}
			break;
		case 2: break;
		case 3: break;
		case 4: {
				auto it = widgets.find(ev.xany.window);
				if(it != widgets.end()) {
					if(fineAdjust) it->second->incr();
					else it->second->wheel_up();
					it->second->draw();
				}
			}
			break;
		case 5:{
				auto it = widgets.find(ev.xany.window);
				if(it != widgets.end()) {
					if(fineAdjust) it->second->decr();
					else it->second->wheel_down();
					it->second->draw();
				}
			}
			break;
		}
		break;

	case ButtonRelease: {
			if(ev.xbutton.button != 1) break;
			auto it = widgets.find(ev.xany.window);
			if(it != widgets.end()) {
				if(it->second != hold) {
					it->second->release(ev.xbutton.x, ev.xbutton.y);
					it->second->draw();
				}
			}
		}
		break;

	case EnterNotify: {
			auto it = widgets.find(ev.xany.window);
			if(it != widgets.end()) {
				if(ev.xcrossing.mode == 0) { // Don't trigger on ungrabs
					it->second->enter(ev.xcrossing.state, ev.xcrossing.x, ev.xcrossing.y);
					it->second->draw();
				}
			}
		}
		break;

	case LeaveNotify: {
			auto it = widgets.find(ev.xcrossing.window);
			if(it != widgets.end()) {
				it->second->leave(ev.xcrossing.state);
				it->second->draw();
			}
		}
		break;

	case MotionNotify: {
			auto it = widgets.find(ev.xany.window);
			if(it != widgets.end()) {
				it->second->drag(ev.xbutton.x, ev.xbutton.y);
				it->second->draw();
			}
		}
		break;

	case KeyPress: {
			KeySym k = XkbKeycodeToKeysym(display, ev.xkey.keycode, 0, 0);
			switch(k) {
			case XK_Down: {
					auto it = widgets.find(ev.xany.window);
					if(it != widgets.end()) {
						it->second->decr();
						it->second->draw();
					}
				}
				break;
			case XK_Up: {
					auto it = widgets.find(ev.xany.window);
					if(it != widgets.end()) {
						it->second->incr();
						it->second->draw();
					}
				}
				break;
			case XK_Control_L: case XK_Control_R:
				fineAdjust = true;
				break;
			case XK_Escape: // Shutdown
				running = false;
				break;
			case XK_space: // sustain pedal (or porta) is space bar
				toSynth->sustain(true);
				//toSynth->porta(true);
				break;
			default:
				if(showKeyboard) keyboard->keypress(k);
				break; 
			}
		}
		break;
	case KeyRelease: {
			KeySym k = XkbKeycodeToKeysym(display, ev.xkey.keycode, 0, 0);
			switch(k) {
			// Ctrl key enables both fine adjust of slider value, and holds
			// the next button press down to allow a second "simultaneous" button
			// to be pressed, for certain edit operations on the DX7. Held button
			// is released when Ctrl key is released.
			case XK_Control_L: case XK_Control_R:
				if(fineAdjust && hold) {
					hold->release(0, 0);
					hold->draw();
					hold = 0;
				}
				fineAdjust = false;
				break;
			case XK_space:
				toSynth->sustain(false);
				//toSynth->porta(false);
				break;
			default:
				if(showKeyboard) keyboard->keyrelease(k);
				break;
			}
		}
	case ClientMessage:
		if (ev.xclient.data.l[0] == (unsigned)wmDeleteMessage) running = false;
	}
}

void DX7Headless::run() {
	fprintf(stderr, "Headless mode: \n"
		"	q         quit\n"
		"	b [1-42]  button press and release\n"
		"	h [1-42]  button hold\n"
		"	r [1-42]  button release hold\n"
		"	d [0-127] data slider\n"
		"Buttons:\n"
		" 1-32 voices,edit,func\n"
		"   st/w=33  int/x=34 cart/y=35    op/z=36\n"
		" ed/chr=37  int/-=38 cart/.=39 func/sp=40\n"
		"  no/-1=41 yes/+1=42\n"
	);

	char *line = 0;
	size_t len = 0;
	ssize_t nread;
	char c;
	int val;
	while ((nread = getline(&line, &len, stdin)) != -1) {
		int n = sscanf(line, "%c %d\n", &c, &val);
		if(n==1 && c=='q') break;
		if(n==2) {
			switch(c) {
				case 'b':
					if(val>=1 && val <=42) {
						Message::CtrlID id = Message::CtrlID(val-1);
						toSynth->buttondown(id);
						// Have to delay button release for some functions
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						toSynth->buttonup(id);
					}
					break;
				case 'h':
					if(val>=1 && val <=42) {
						Message::CtrlID id = Message::CtrlID(val-1);
						toSynth->buttondown(id);
					}
					break;
				case 'r':
					if(val>=1 && val <=42) {
						Message::CtrlID id = Message::CtrlID(val-1);
						toSynth->buttonup(id);
					}
					break;
				case 'd':
					if(val>=0 && val <=127) {
						toSynth->analog(Message::CtrlID::data, val);
						// Don't really need delay, just waiting for LCD to update
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					break;
				default: break;
			}
		}
		// Update LCD
		processMessages();
		printf("%s\n%s\n", lcd.line1, lcd.line2);
	}
	free(line);
	stop();
}

void DX7Headless::processMessages() {
	Message msg;
	while(toGui->pop(msg)) {
		switch(Message::CtrlID(msg.byte1)) {
			case Message::CtrlID::lcd_inst: lcd.inst(msg.byte2); break;
			case Message::CtrlID::lcd_data: lcd.data(msg.byte2); break;
			//case Message::CtrlID::led1_setval: break;
			//case Message::CtrlID::led2_setval: break;
			//case Message::CtrlID::cartridge_num: break;
			//case Message::CtrlID::cartridge_name: break;
			//case Message::CtrlID::lcd_state: break;
			default: break;
		}
	}
}

