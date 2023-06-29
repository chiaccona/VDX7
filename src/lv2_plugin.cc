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

#include <cstdbool>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "Synth.h"
#include "uris.h"

struct DX7Plugin {
	// Port buffers
	const LV2_Atom_Sequence* midi_in = 0;
	LV2_Atom_Sequence* midi_out = 0;
	LV2_Atom_Sequence* control_out = 0;
	float* out = 0;
	double rate = 1;

	// Features
	LV2_URID_Map* map = 0;
	LV2_Log_Logger logger;
	URIs uris;

	// State
	std::string rampath;
	DX7Synth dx7;
	bool running = false;
	int boffset = Synth::BufSize;

	// Communication interface
	LV2_ToGui *toGui = 0;
	App_ToSynth toSynth;

	DX7Plugin(double rate, const LV2_Feature* const* features)
		: rate(rate)
	{
		// Scan host features for URID map
		const char* missing = lv2_features_query(
			features,
			LV2_LOG__log,  &logger.log, false,
			LV2_URID__map, &map,        true,
			NULL);

		lv2_log_logger_set_map(&logger, map);
		if (missing) {
			lv2_log_error(&logger, "Missing feature <%s>\n", missing);
		}
		map_uris(map, &uris);
		toGui = new LV2_ToGui(map, &uris);
		dx7.setSampleRate(rate);

		dx7.toSynth = &toSynth;
		dx7.toGui = toGui;
	}
	~DX7Plugin() {
		if(toGui) delete toGui;
		toGui = 0;
	}

	void connectPort (const uint32_t port, void* data) {
		switch (port) {
			case 0: midi_in = (const LV2_Atom_Sequence*)data; break;
			case 1: midi_out = (LV2_Atom_Sequence*)data; break;
			case 2: control_out = (LV2_Atom_Sequence*)data; break;
			case 3: out = (float*)data; break;
			default: break;
		}
	}

	void activate () { dx7.start(); }

	void run(uint32_t nframes) {
		if(!running) return;

		int midi_events = 0;

		struct MIDINoteEvent {
			LV2_Atom_Event event;
			uint8_t msg[3];
		};

		LV2_ATOM_SEQUENCE_FOREACH (midi_in, ev) {
			if (ev->body.type == uris.midi_Event) {

				uint8_t* msg = (uint8_t*)(ev + 1);
				switch (lv2_midi_message_type(msg)) {
					case LV2_MIDI_MSG_NOTE_ON: dx7.queueMidiRx(3, msg); break;
					case LV2_MIDI_MSG_NOTE_OFF: dx7.queueMidiRx(3, msg); break;
					case LV2_MIDI_MSG_CONTROLLER: dx7.queueMidiRx(3, msg); break;
					case LV2_MIDI_MSG_PGM_CHANGE: dx7.queueMidiRx(2, msg); break;
					case LV2_MIDI_MSG_CHANNEL_PRESSURE: dx7.queueMidiRx(2, msg); break;
					case LV2_MIDI_MSG_BENDER: dx7.queueMidiRx(3, msg); break;
					default: break;
				}
			} else if (ev->body.type == uris.atom_Int) {
				int m = (reinterpret_cast<const LV2_Atom_Int*>(&ev->body))->body;
				if(dx7.toSynth) dx7.toSynth->push(uint16_t(m));
			}
		}

		// Loop
		if(toGui) toGui->setup(control_out);
		const uint32_t out_capacity = midi_out->atom.size;
		lv2_atom_sequence_clear(midi_out);
		midi_out->atom.type = uris.atom_Sequence;

		int midi_event_count = 0;
		uint32_t written = 0;
		while (written < nframes) {
			int nremain = nframes - written;

			if (boffset >= Synth::BufSize)  {
				// Send MIDI
				uint32_t size=0;
				uint8_t* buffer=0;
				while(dx7.queueMidiTx(size, buffer)) {
					MIDINoteEvent ev_out;
					ev_out.event.time.frames = 0; // ev->time;
					ev_out.event.body.type = uris.midi_Event;
					ev_out.event.body.size = size;
					ev_out.msg[0] = buffer[0];
					ev_out.msg[1] = buffer[1];
					ev_out.msg[2] = buffer[2];
					lv2_atom_sequence_append_event(midi_out, out_capacity, &ev_out.event);
				}

				// Process Audio - get BufSize samples
				dx7.run();
				boffset = 0;
			}

			int nread = (nremain < (Synth::BufSize - boffset) ? nremain : (Synth::BufSize - boffset));

			memcpy(out+written, dx7.outputBuffer+boffset, nread*sizeof(float));
			written+=nread;
			boffset+=nread;
		}

		// process remaining MIDI events
		// IF nframes < BufSize OR nframes != N * BufSize
		for ( ; midi_event_count<midi_events; midi_event_count++) {
			printf("leftover event?\n");
		}
	}
};


//
// LV2 core interface
//
static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
		double rate, const char* bundle_path,
		const LV2_Feature* const* features) {
	DX7Plugin* self = new DX7Plugin(rate, features);
	return (LV2_Handle)self;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
	DX7Plugin* self = (DX7Plugin*)instance;
	if(self) self->connectPort(port, data);
}

static void activate(LV2_Handle instance) {
	DX7Plugin* self = (DX7Plugin*)instance;
	if(self) self->activate();
}

static void run(LV2_Handle instance, uint32_t sample_count) {
	DX7Plugin* self = (DX7Plugin*)instance;
	if(self) self->run(sample_count);
}

static void deactivate(LV2_Handle instance) { /* no action */ }

static void cleanup(LV2_Handle instance) {
	DX7Plugin* self = (DX7Plugin*)instance;
	if(self) delete self;
}

static LV2_State_Status save(
	LV2_Handle                instance,
	LV2_State_Store_Function  store,
	LV2_State_Handle          handle,
	uint32_t                  flags,
	const LV2_Feature* const* features)
{
	DX7Plugin* self = (DX7Plugin*)instance;

	LV2_State_Map_Path* map_path = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_STATE__mapPath)) {
			map_path = (LV2_State_Map_Path*)features[i]->data;
		}
	}
	// FIX how to get path we're saving to?  or a new path?  this is just the existing RAM file
	char* apath = map_path->abstract_path(map_path->handle, self->rampath.c_str());

	store(handle,
		self->uris.dx7ram,
		apath,
		strlen(self->rampath.c_str()) + 1,
		self->uris.atom_Path,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	free(apath);
	return LV2_STATE_SUCCESS;
}

static LV2_State_Status restore(
	LV2_Handle                  instance,
	LV2_State_Retrieve_Function retrieve,
	LV2_State_Handle            handle,
	uint32_t                    flags,
	const LV2_Feature* const*   features)
{
	DX7Plugin* self = (DX7Plugin*)instance;

	size_t   size;
	uint32_t type;
	uint32_t valflags;

	const void* value = retrieve( handle, self->uris.dx7ram, &size, &type, &valflags);

	if (value) {
		const char* path = (const char*)value;
		lv2_log_trace(&self->logger, "LV2: Restoring file %s\n", path);
		self->rampath = path;
		self->dx7.dx7.ramfile = self->rampath.c_str();
		self->dx7.dx7.restoreRAM(self->dx7.dx7.ramfile);
		self->running = true;
	}
	return LV2_STATE_SUCCESS;
}

static const void* extension_data(const char* uri) {
	static const LV2_State_Interface  state  = { save, restore };
	if (!strcmp(uri, LV2_STATE__interface)) return &state;
	return NULL;
}

static const LV2_Descriptor descriptor = {
	DX7_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
	return index == 0 ? &descriptor : NULL;
}


