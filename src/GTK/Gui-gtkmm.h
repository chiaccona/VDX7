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
#include <string>
#include <cstdint>

#if 1
#include <gtkmm.h>
#else
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/scale.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <gtkmm/main.h>
#include <glibmm/main.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>
#endif

#include "../Message.h"
#include "../HD44780.h"

class LED : public Gtk::DrawingArea {
public:
	LED() {
		init();
		set_size_request(int(mx)+1, int(my)+1);
		set_valign(Gtk::ALIGN_CENTER);
	}
	virtual ~LED() {}
	void set(uint8_t v) { val = v; queue_draw(); }

protected:
	uint8_t val = 0x00; // bits dGFEDCBA, active low
	struct Segment { float x1, y1, x2, y2; };
	Segment segment[8] = { // initial segment skeleton
		{	0.0,	0.0,	1.0,	0.0	}, // A: top
		{	1.0,	0.0,	1.0,	0.5	}, // B: top-right
		{	1.0,	0.5,	1.0,	1.0	}, // C: bot-right
		{	0.0,	1.0,	1.0,	1.0	}, // D: bot
		{	0.0,	0.5,	0.0,	1.0	}, // E: bot-left
		{	0.0,	0.0,	0.0,	0.5	}, // F: top-left
		{	0.0,	0.5,	1.0,	0.5	}, // G: middle
		{	1.0,	1.0,	1.0,	1.0	}, // d: dot
	};
	float mx=0, my=0; // maximum for viewbox
	float linewidth=0; // segment width

	void init();
	bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
};

struct Label : public Gtk::Label {
	Label(const char *text) {
		set_text(text); 
		set_xalign(0.0);
		set_name("bg_label");
	}
};

struct Button : public Gtk::Button {
	Button() {
		set_name("teal_button");
	}
	void config(const char *left, const char *right, const int n);
	bool press(GdkEventButton *event);
	bool release(GdkEventButton *event);
	bool enter(GdkEventCrossing *event);
	bool leave(GdkEventCrossing *event);

	Gtk::Label l_label, r_label;
	Gtk::Box lbox;
	int num = 0;
	static Button *hold;
	static void release_hold();
	ToSynth *toSynth;
};

struct Slider : Gtk::VScale  {
	Slider();
	virtual ~Slider() {}
	void changed_cb();
	std::string name;
	ToSynth *toSynth;
};

struct Display : public Gtk::Box {
	Display();
	virtual ~Display() {}

	void setLCDdata(uint8_t val) { lcddriver.data(val); update(); }
	void setLCDinst(uint8_t val) { lcddriver.inst(val); update(); }
	void setLED1(const uint8_t val) { led1.set(val); }
	void setLED2(const uint8_t val) { led2.set(val); }
	void restore(uint8_t *data, uint8_t len) {
		update();
	}
	void blink(bool on);

private:
	void update();
	Gtk::Label lcd;
	char lcd_text[34];
	LED led1, led2;
	HD44780 lcddriver;
};

struct Cartridge : public Gtk::VBox {
	Cartridge() ;
	virtual ~Cartridge() {}
	void set_text(const char *s) { filename = s; name.set_text(filename); }
	void set_protect(bool v) { protect.set_active(v); }

	ToSynth *toSynth;

private:
	Gtk::HBox box;
	Gtk::Button load;
	Gtk::Switch protect;
	Gtk::Label l_protect;
	Gtk::Label name;

	void on_load_clicked();
	bool on_protect_clicked(gboolean state);
	std::string folder;
	std::string filename;
};


struct DX7GUI : public Gtk::HBox {
	DX7GUI() {}
	void init();
	virtual ~DX7GUI();
	bool idle();
	GtkHBox* widget() { return this->gobj(); }
	void on_button_file_clicked();
	void restore();

	bool onKeyRelease(GdkEventKey* event) {
    	printf("Release %d %d %d\n", event->keyval, event->hardware_keycode, event->state);
		if(event->hardware_keycode == 37) {
			Button::release_hold();
		}
    	return false;
	}

	Gtk::Grid grid;
	Button button[42];
	Slider volume, data;
	Display display;
	Cartridge cartridge;
	int blinker = 0;

	// Communication Lock-Free Queue from Synth
	ToGui *toGui;
	ToSynth *toSynth;
};

