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

#include <thread>
#include <string>
#include <sys/stat.h>
#include <filesystem>
#include <getopt.h>
#include "Synth.h"
#include "JackDriver.h"

#if GTKMM
#include "Gui-gtkmm.h"
#else
#include "Gui.h"
#endif


#ifndef VERSION
#define VERSION 0.00
#endif

// Stringify macro
#define XSTR(x) STR(x)
#define STR(x) #x

// Create "<config>/vdx7/" if it doesn't exist
// Use XDG_CONFIG_HOME if set, otherwise default to ~/.config
// Return config filename in fn if valid
// Return 0 if OK, >0 if fn valid but doesn't exist, and <0 if not valid
int getConfigFile(std::string &fn) {
	namespace fs = std::filesystem;
	fs::path config;
	const char *xdg_home = getenv("XDG_CONFIG_HOME");
	if(xdg_home && *xdg_home != 0) {
		config = xdg_home;
	} else {
		const char *home = getenv("HOME");
		if(home && *home != 0) {
			config = home;
			config /= ".config/";
		} else return -1; // no valid config path
	}
	config /= "vdx7/";

	try { 
		fs::create_directories(config);
	} catch(const std::exception& ex) { // can't create dir
		fprintf(stderr, "Could not create path for \"%s\" threw exception:\n%s\n", config.c_str(), ex.what());
		return -2;
	}

	auto ram = fs::path(config / "vdx7.ram");
	fn = ram;
	try {
		if(fs::exists(ram)) {
		   	if(fs::is_regular_file(ram)) return 0; // return file
			else { // it's not a valid file
				fn.clear();
				return -3;
			}
		} else return 1; // file doesn't exist
	} catch(const std::exception& ex) { // some other fs error
		fprintf(stderr, "Check for \"%s\" threw exception:\n%s\n", config.c_str(), ex.what());
		fn.clear();
		return -4;
	}
}

int main(int argc, char* argv[]) {
	int err;

	const char* opts = "c:n:b:B:s:t:V:i:o:p:r:aqhvm";
	const struct option long_opts[] = {
		{"cart",		required_argument,	0,	'c'},
		{"new",			required_argument,	0,	'n'},
		{"bank",		required_argument,	0,	'b'},
		{"Bank",		required_argument,	0,	'B'},
		{"save",		required_argument,	0,	's'},
		{"tuning",		required_argument,	0,	't'},
		{"Velocity",	required_argument,	0,	'V'},
		{"midi-in",		required_argument,	0,	'i'},
		{"midi-out",	required_argument,	0,	'o'},
		{"port",		required_argument,	0,	'p'},
		{"rom",			required_argument,	0,	'r'},
		{"noauto",		no_argument,		0,	'a'},
		{"quiet",		no_argument,		0,	'q'},
		{"serial",		no_argument,		0,	'm'},
		{"help",		no_argument,		0,	'h'},
		{"version",		no_argument,		0,	'v'},
		{0, 0, 0, 0},
	};

	bool quiet = false; // no tty output
	bool noauto = false; // no jack autoconnect
	bool newcart = false; // create a new cartrdige file
	char *cartridge = 0; // cartridge file
	int bank = -1; // load cartridge to internal memory
	int Bank = -1; // load cartridge to cartridge memory
	const char *romfile = 0; // Firmware ROM
	const char *savefile = 0; // RAM memory "battery backup"
	const char *midi_in = 0; // jack regex
	const char *midi_out = 0; // jack regex
	const char *port = 0; // jack regex
	bool tune = false; // set tuning
	int tuning = 0; // tuning value to set 
	bool serial = false; // send midi directly to synth
	char *velArg = 0; // velocity map

	int c;
	while((c = getopt_long(argc, argv, opts, long_opts, 0)) != -1) {
		switch (c) {
		case 'n':
			newcart = true;
			cartridge = optarg;
			break;
		case 'c':
			// Ignore if already asked for a new cart
			if(!newcart) cartridge = optarg;
			break;
		case 'b': 
			bank = atoi(optarg);
			if(bank<0 || bank>7) {
				fprintf(stderr, "-b arg must be 0 to 7 (factory cartirdge ROM1A to ROM4B)\n");
				bank = -1;
			}
			break;
		case 'B': 
			Bank = atoi(optarg);
			if(Bank<0 || Bank>7) {
				fprintf(stderr, "-B arg must be 0 to 7 (factory cartirdge ROM1A to ROM4B)\n");
				Bank = -1;
			}
			break;
		case 's': savefile = optarg; break;
		case 't': tuning = atoi(optarg); tune = true; break;
		case 'V': velArg = optarg; break;
		case 'i': midi_in = optarg; break;
		case 'o': midi_out = optarg; break;
		case 'p': port = optarg; break;
		case 'r': romfile = optarg; break;

		case 'v':
			fprintf(stderr, "Version: %s " XSTR(VERSION) "\n", argv[0]);
			exit(-1);
			break;
		case 'a': noauto = true; break;
		case 'm': serial = true; break;
		case 'q': quiet = true; break;
		case 'h':
		default:
			fprintf(stderr,
				"Usage: %s \n"
				"	-h (this help)\n"
				"	-v (version)\n"
				"	-q quiet (no terminal stdout)\n"
				"	-a don't autoconnect Jack midi\n"
				"	-m send MIDI directly to DX7 serial interface\n"
				"	-c filename (sysex cartridge file)\n"
				"	-n filename (create new sysex cartridge file)\n"
				"	-r filename (load a firmware ROM)\n"
				"	-b [0-7] (bank number: load factory voice cartridge into internal memory)\n"
				"	-B [0-7] (bank number: load factory voice cartridge into cartridge memory)\n"
				"	-s filename (save/restore RAM memory file, default ~/.config/vdx7/vdx7.ram)\n"
				"	-t master tuning (+/-256 steps, ~.3 cents/step, default 0=A440)\n"
				"	-V MIDI velocity curve (.25 to 4.0, default 0.4, 1.0=linear)\n"
				"	-p port (jack audio port)\n"
				"	-i midi-in (jack midi in port regex)\n"
				"	-o midi-out (jack midi out port regex)\n",
				argv[0]);
			exit(-1);
		}
	}

	// Redirect stdout
	if(quiet) if(!freopen("/dev/null", "a", stdout)) {
		fprintf(stderr, "Couldn't redirect stdout (%d)\n", errno);
	}

	// Set up ram image file
	bool loadDefault = false;
	std::string filename;
	if(!savefile) {
		int err = getConfigFile(filename);
		if(err) loadDefault = true; // no existing ram file
		if(err>=0) savefile = filename.c_str();
	}

	// Interthread communication
	App_ToSynth toSynth;
	App_ToGui toGui;
	
	DX7Synth synth(savefile);
	synth.toGui = &toGui;
	synth.toSynth = &toSynth;
	if(romfile) synth.dx7.loadROM(romfile);
	synth.start();

	// Load cartridge
	fprintf(stderr, "Cartridge: %s\n", cartridge ? cartridge : "(none)");
	if(newcart) { // New blank cartridge
		synth.dx7.cartFile = cartridge;
		synth.dx7.cartPresent(true);
		synth.dx7.cartWriteProtect(false);
	} else {
		if(cartridge && (err=synth.dx7.cartLoad(cartridge))) {
			fprintf(stderr, "Couldn't load cartridge (%d)\n", err);
		}
	}

	// Load a default voice bank (the real synth doesn't do this)
	if(bank != -1) synth.dx7.setBank(bank);
	else if(loadDefault) synth.dx7.setBank(4); // Default to ROM3A if no init file

	// Load a factory cartridge (-B option)
	if(Bank != -1) synth.dx7.setBank(Bank, true);

	if(tune) synth.dx7.tune(tuning);
	if(serial) synth.useSerialMidi(true);
	if(velArg) synth.parseMidiVelocityArgs(velArg);

	// Set up I/O
	JackDriver jack(&synth);
	jack.init();

	// Regex for port connections
	if(noauto) midi_in = midi_out = "";
	if(!midi_in && !noauto) midi_in = "system:midi_capture_";
	if(!midi_out && !noauto) midi_out = "system:midi_playback_[^1]"; // avoid looping
	if(!port) port = "system:playback_*";

	if((err=jack.initMidi(midi_in, midi_out))) {
		fprintf(stderr, "Can't open %s or %s\n", midi_in, midi_out);
		return(err);
	}

	if((err=jack.initPCM(port))) {
		fprintf(stderr, "Can't open %s\n", port);
		return(err);
	}

	// Set up GUI
#if GTKMM
	auto app = Gtk::Application::create();
	struct App : public Gtk::Window {
		App() {}
		void init() {
			gui_widget.init();
			add(gui_widget);
			show_all_children();
		}
		DX7GUI gui_widget;
	};
	App gui;
	gui.gui_widget.toGui = &toGui;
	gui.gui_widget.toSynth = &toSynth;
	gui.init();
	return app->run(gui); // Run GUI
#else
	// X11 version
	DX7GUI gui;
	gui.toGui = &toGui;
	gui.toSynth = &toSynth;

	// Launch GUI thread
	std::thread tgui(&DX7GUI::run, &gui);
	tgui.join(); // Clean up
#endif
}
