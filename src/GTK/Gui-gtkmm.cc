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

#include "Gui-gtkmm.h"

#include "labels.h"
#include "style.h"

void LED::init() {
	// Display size
	float h = 7.0; // char height mm
	float w = 4.85; // char width mm
	linewidth = 0.85; // segment thickness mm
	float incline = 10; // degrees inclined
	//float scale = 4.4; // approx px/mm
	float scale = 4.0; // approx px/mm

	float slope = tan(incline*M_PI/180);
	w *= cos(incline*M_PI/180); // width is measured on bias

	// scale to dpi
	h *= scale;
	w *= scale;
	linewidth *= scale;

	for(int i=0; i<8; i++) {
		Segment &s = segment[i];
		// Scale h and w
		s.x1 *= w; s.x2 *= w; s.y1 *= h; s.y2 *= h;

		if(i != 7) { // don't move the dot
			// Shorten segments
			if(s.y1 == s.y2) { // vertical
				s.x1 += linewidth; s.x2 -= linewidth;
			} else if(s.x1 == s.x2) { // horizontal
				s.y1 += linewidth; s.y2 -= linewidth;
			}
			// Skew (x coord moves left as y increases)
			s.x1 -= slope * s.y1; s.x2 -= slope * s.y2;
		}

		// Shift in viewbox
		s.x1 += linewidth/2 + h*slope;
		s.x2 += linewidth/2 + h*slope;
		s.y1 += linewidth/2;
		s.y2 += linewidth/2;
	}
	// Viewbox size (max x and y) - extend by half line width
	mx = linewidth/2 + (segment[0].x2 > segment[7].x2 ? segment[0].x2 : segment[7].x2);
	my = linewidth/2 + segment[7].y2;
}

bool LED::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
	cr->set_source_rgb(0, 0, 0); // black background
	cr->rectangle(0, 0, get_width(), get_height());
	cr->fill();
	
	cr->set_line_width(linewidth);
	cr->set_line_join(Cairo::LINE_JOIN_ROUND); /*_MITER _BEVEL*/
	cr->set_line_cap(Cairo::LINE_CAP_ROUND); /*_MITER _SQUARE*/
	
	cr->set_source_rgb(0.8, 0.0, 0.0); // red LED
	for(int i=0; i<8; i++) {
		if(!((val>>i)&1)) { // active low
			Segment &s = segment[i];
			cr->move_to (s.x1, s.y1);
			cr->line_to (s.x2, s.y2);
		}
	}
	cr->stroke();
	return true;
}

void Button::config(const char *left, const char *right, const int n) {
	num = n;
	set_size_request(32, 20);
	add(lbox);
	if(left) {
		l_label.set_text(left); 
		l_label.set_xalign(0.0);
		l_label.set_name("l_label");
		lbox.add(l_label);
	}
	r_label.set_text(right);
	r_label.set_xalign(1.0);
	r_label.set_hexpand(true);
	r_label.set_halign(Gtk::ALIGN_FILL);
	r_label.set_name("r_label");
	lbox.add(r_label);
	signal_button_release_event().connect(sigc::mem_fun(*this, &Button::release), false);
	signal_button_press_event().connect(sigc::mem_fun(*this, &Button::press), false);
	signal_leave_notify_event().connect(sigc::mem_fun(*this, &Button::leave), false);
	signal_enter_notify_event().connect(sigc::mem_fun(*this, &Button::enter), false);
}

Button *Button::hold = 0;

bool Button::leave(GdkEventCrossing *event) {
	if(hold==this) {
		set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE, false);
		return true;
	}
	return false;
}

bool Button::enter(GdkEventCrossing *event) {
	if(hold==this) {
		set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE, false);
		return true;
	}
	return false;
}

bool Button::press(GdkEventButton *event) {
if(!toSynth) {
fprintf(stderr, "Button::press() toSynth=0\n");
return false;
}
	if(event->type == GDK_BUTTON_PRESS && event->button == 1) {
		if(event->state & GDK_CONTROL_MASK) {
			if(!hold) {
				hold = this;
				set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE, false);
				toSynth->buttondown(Message::CtrlID(num));
				return true;
			}
		}
		if(hold!=this) toSynth->buttondown(Message::CtrlID(num));
	}
	return false;
}

bool Button::release(GdkEventButton *event) {
if(!toSynth) {
fprintf(stderr, "Button::release() toSynth=0\n");
return false;
}
	if(event->type == GDK_BUTTON_RELEASE && event->button == 1) {
		if(event->state & GDK_CONTROL_MASK) {
			if(hold==this) {
				set_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE, false);
				return true;
			}
		}
		toSynth->buttonup(Message::CtrlID(num));
	}
	return false;
}

void Button::release_hold() {
	if(hold) {
if(!hold->toSynth) {
fprintf(stderr, "Button::release_hold() toSynth=0\n");
return;
}
		hold->unset_state_flags(Gtk::StateFlags::STATE_FLAG_ACTIVE);
		hold->toSynth->buttonup(Message::CtrlID(hold->num));
	}
	hold = 0;
}

Slider::Slider() {
	set_draw_value(false);
	set_inverted(true);
	set_range(0, 127);
	set_increments(1, 1);
	set_value(0);
	signal_value_changed().connect(sigc::mem_fun(*this, &Slider::changed_cb));
}

void Slider::changed_cb() {
if(!toSynth) {
fprintf(stderr, "Slider::changed_cb() toSynth=0\n");
return;
}
	if(name == "volume")
		toSynth->analog(Message::CtrlID::volume, uint8_t(get_value()));
	else toSynth->analog(Message::CtrlID::data, uint8_t(get_value()));
}

Display::Display() {
	add(led2); add(led1);
	add(lcd);
	set_name("display");
	lcd.set_max_width_chars(16);
	lcd.set_name("lcd");
	set_valign(Gtk::ALIGN_CENTER);
}

void Display::update() {
	memcpy(lcd_text, lcddriver.line1, 16);
	memcpy(lcd_text+17, lcddriver.line2, 16);
	lcd_text[16] = '\n';
	lcd_text[33] = 0;
	lcd.set_label(lcd_text);
}

void Display::blink(bool on) {
	// NB this widget doesn't indicate a solid cursor position,
	// (i.e. cursor_on==true). It will only respond to the "blink" instruction
	// (cursor_blink==true, swapping the character with '_'), which is OK for
	// the DX7 display. A true reversed blit character is kludgier in Gtk than I
	// care to implement.
	if(lcddriver.cursor_blink && lcddriver.cursor_pos < 16) {
		char *p = lcd_text+lcddriver.cursor_pos;
		if(lcddriver.cursor_line == 2)  p += 17;
		if(on) {
			update(); // restore original char
		} else {
			*p = '_'; // replace with blink char '_'
			lcd.set_label(lcd_text);
		}
	}
}

void DX7GUI::init() {
	grid.set_column_homogeneous(true);
	grid.set_column_spacing(2);
	add(grid);

	grid.attach(cartridge, 10, 1, 4, 4);
	cartridge.toSynth = toSynth;

	// Rows containing the buttons
	int btn_row1 = 3;
	int btn_row2 = 10;

	// Sliders
	volume.toSynth = toSynth;
	data.toSynth = toSynth;
	volume.name = "volume";
	data.name = "data";
	volume.set_value(0.75*127);
	grid.attach(volume, 1, 2, 1, 10);
	grid.attach(data,   2, 2, 1, 10);

	// Left button rows
	button[40].config("-1", "<", 40);
	grid.attach(button[40], 3, btn_row2, 1, 1);

	button[41].config("+1", ">", 41);
	grid.attach(button[41], 4, btn_row2, 1, 1);

	button[36].config(0, "CHARACTER", 36);
	button[36].set_name("blue_button");
	grid.attach(button[36], 6, btn_row2, 1, 1);

	button[37].config(0, "-", 37);
	grid.attach(button[37], 7, btn_row2, 1, 1);

	button[38].config(0, ".", 38);
	grid.attach(button[38], 8, btn_row2, 1, 1);

	button[39].config(0, "SPACE", 39);
	button[39].set_name("brown_button");
	grid.attach(button[39], 9, btn_row2, 1, 1);

	button[32].config(0, "W", 32);
	button[32].set_name("red_button");
	grid.attach(button[32], 6, btn_row1, 1, 1);

	button[33].config(0, "X", 33);
	grid.attach(button[33], 7, btn_row1, 1, 1);

	button[34].config(0, "Y", 34);
	grid.attach(button[34], 8, btn_row1, 1, 1);

	button[35].config(0, "Z", 35);
	button[35].set_name("blue_button");
	grid.attach(button[35], 9, btn_row1, 1, 1);

	// Display
	grid.attach(display, 10, btn_row1+1, 4, 7);

	// Main button rows
	for(int i=0; i<32; i++) {
		char c = i<10 ? (i+1)%10 +0x30 : i-10+0x41;
		char left[20];
		sprintf(left, "%-2d", i+1);
		char right[20];
		sprintf(right, "%c", c);
		button[i].config(left, right, i);
		if(i<16) grid.attach(button[i], i+15,    btn_row1, 1, 1);
			else grid.attach(button[i], i+15-16, btn_row2, 1, 1);
	}

	for(int i=0; i<42; i++) button[i].toSynth = toSynth;

	// Labels
	for(label& l : labels) {
		l.label = new Label(l.text);
		l.label->set_name(l.name);
		if(!strcmp(l.name, "l_reverse")) l.label->set_xalign(0.5);
		if(l.col_offset==0) l.label->set_xalign(0.5);
		grid.attach(*l.label, l.col+l.col_offset, l.row, l.colspan, l.rowspan);
	}

	this->signal_key_release_event().connect(sigc::mem_fun(*this, &DX7GUI::onKeyRelease), false);


	auto css = Gtk::CssProvider::create();
#if 0
	if(not css->load_from_path("style.css")) {
#else
	if(not css->load_from_data(style)) {
#endif
		fprintf(stderr, "Failed to load style.css\n");
		std::exit(1);
	}

	auto screen = Gdk::Screen::get_default();
	auto sc = Gtk::StyleContext::create();
	sc->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_USER);

	// 40 ms refresh rate
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &DX7GUI::idle), 40);
}
DX7GUI::~DX7GUI() {
	for(label& l : labels) if(l.label) delete l.label;
}

// Display names for the standard factory voice banks
static const char *factoryROMs[8] = {
	"ROM1A Master (EU/JP)",
	"ROM1B Keyboard & Plucked (EU/JP)",
	"ROM2A Orchestral & Percussive (EU/JP)",
	"ROM2B Synth, Complex & Effects (EU/JP)",
	"ROM3A Master (US)",
	"ROM3B Keyboard & Plucked (US)",
	"ROM4A Orchestral & Percussive (US)",
	"ROM4B Synth, Complex & Effects (US)",
};

void DX7GUI::restore() {
if(!toSynth) {
fprintf(stderr, "DX7GUI::restore() toSynth=0\n");
return ;
}
	toSynth->requestState();
}

bool DX7GUI::idle() {
	// Blink timer
	++blinker &= 0x1F;
	if(blinker==0x00) display.blink(true);
	else if(blinker==0x10) display.blink(false);
#if 1
	Message msg;
	while(toGui->pop(msg)) {
		switch(Message::CtrlID(msg.byte1)) {
			case Message::CtrlID::lcd_inst: display.setLCDinst(msg.byte2); break;
			case Message::CtrlID::lcd_data: display.setLCDdata(msg.byte2); break;
			case Message::CtrlID::led1_setval: display.setLED1(msg.byte2); break;
			case Message::CtrlID::led2_setval: display.setLED2(msg.byte2); break;
			case Message::CtrlID::cartridge_num:
				cartridge.set_text(factoryROMs[msg.byte2&0x7]);
				break;
			case Message::CtrlID::cartridge_name: {
					int len = msg.byte2;
					uint8_t data[len+1];
					toGui->getBinary(data, len);
					data[len] = 0;
					printf("cartridge: %s\n", data);
					cartridge.set_text((const char*)data);
					break;
				}
			case Message::CtrlID::lcd_state: {
					int len = msg.byte2;
					uint8_t data[len+1];
					toGui->getBinary(data, len);
					data[len] = 0;
					printf("lcd_state: %d\n", len);
					display.restore(data, len);
					break;
				}
			default: break;
		}
	}
#endif
	return true;
}


Cartridge::Cartridge() 
	: load("Cartridge") {

	load.set_name("file_button");
	load.signal_clicked().connect(sigc::mem_fun(*this, &Cartridge::on_load_clicked));

	set_protect(true);
	protect.signal_state_set().connect(sigc::mem_fun(*this, &Cartridge::on_protect_clicked),0);

	name.set_name("cart_label");
	name.set_line_wrap(true);
	name.set_line_wrap_mode(Pango::WrapMode::WRAP_CHAR);
	//name.set_max_width_chars(24);
	filename = "No Cartridge";
	name.set_text(filename);

	pack_start(box, Gtk::PACK_SHRINK);
	pack_start(name, Gtk::PACK_EXPAND_WIDGET);
	box.pack_start(load, Gtk::PACK_SHRINK);
	box.pack_end(protect, Gtk::PACK_SHRINK);
	box.pack_end(l_protect, Gtk::PACK_SHRINK);

	l_protect.set_label("PROTECT");
	l_protect.set_name("l_white");
	name.set_vexpand(true);
}

bool Cartridge::on_protect_clicked(gboolean state) {
	printf("protect: %d\n", state);
	toSynth->protect(state);
	return false;
}

void Cartridge::on_load_clicked() {
	Gtk::FileChooserDialog dialog("Choose Cartridge File",
		Gtk::FILE_CHOOSER_ACTION_OPEN);

	//Add response buttons the the dialog:
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("_Open", Gtk::RESPONSE_OK);

	//Add filters, so that only certain file types can be selected:

	auto filter_syx = Gtk::FileFilter::create();
	filter_syx->set_name("SYX files");
	filter_syx->add_pattern("*.syx");
	filter_syx->add_pattern("*.SYX");
	dialog.add_filter(filter_syx);

	auto filter_any = Gtk::FileFilter::create();
	filter_any->set_name("Any files");
	filter_any->add_pattern("*");
	dialog.add_filter(filter_any);

	if(!folder.empty()) dialog.set_current_folder(folder);

	//Show the dialog and wait for a user response:
	int result = dialog.run();

	//Handle the response:
	switch(result) {
	case(Gtk::RESPONSE_OK): {
		filename = dialog.get_filename();
		if(filename.length()<=255)
			toSynth->cartridge_file((uint8_t*)(filename.c_str()), filename.length());
			else fprintf(stderr, "File path too long (%ld>255): %s\n", filename.length(), filename.c_str());
		folder = dialog.get_current_folder();
		break;
	}
	case(Gtk::RESPONSE_CANCEL): break;
	default: break;
	}
}

