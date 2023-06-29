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

#include <unistd.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "Synth.h"

class JackDriver {
public:
	JackDriver(Synth *s) : synth(s), BufSize(Synth::BufSize)  { }
	void init();
	int initMidi(const char *in_port_name, const char *out_port_name);
	int initPCM(const char *dev_name);
	virtual ~JackDriver() { close_jack(); }

private:
	int open_jack(void);
	void close_jack();
	static void jack_shutdown_callback(void *arg);
	static int jack_srate_callback(jack_nframes_t nframes, void *arg);
	static int jack_audio_callback(jack_nframes_t nframes, void *arg);
	int callback(jack_nframes_t nframes);

	bool synth_ready = true;

	Synth *synth = 0;
	int BufSize;  

	// Port names advertised
	const char *audio_out_port_name = "out";
	const char *midi_in_port_name = "midi_in";
	const char *midi_out_port_name = "midi_out";

	// Default regex search to find ports to connect with
	const char *audio_out_port_regex = "system:playback_";
	const char *midi_in_port_regex = "system:midi_capture_";
	const char *midi_out_port_regex = "system:midi_playback_";

	// Client name
	const char *jack_client_name = "VDX7";

	// Client handle
	jack_client_t *j_client = 0;

	// Ports
	jack_port_t *jack_audio_out_port = 0;
	jack_port_t *jack_midi_in_port = 0;
	jack_port_t *jack_midi_out_port = 0;

};


