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
#include <stdio.h>

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/parameters/parameters.h"
#include "lv2/state/state.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"

#define DX7_URI "urn:chiaccona:vdx7" 

struct URIs {
	LV2_URID atom_Int;
	LV2_URID atom_String;
	LV2_URID atom_Path;
	LV2_URID atom_Sequence;
	LV2_URID atom_URID;
	LV2_URID atom_eventTransfer;
	LV2_URID midi_Event;
	LV2_URID param_gain;
	LV2_URID dx7ram;
};

static inline void
map_uris(LV2_URID_Map* map, URIs* uris) {
	uris->atom_Int           = map->map(map->handle, LV2_ATOM__Int);
	uris->atom_String        = map->map(map->handle, LV2_ATOM__String);
	uris->atom_Path          = map->map(map->handle, LV2_ATOM__Path);
	uris->atom_Sequence      = map->map(map->handle, LV2_ATOM__Sequence);
	uris->atom_URID          = map->map(map->handle, LV2_ATOM__URID);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->midi_Event         = map->map(map->handle, LV2_MIDI__MidiEvent);
	uris->param_gain         = map->map(map->handle, LV2_PARAMETERS__gain);
	uris->dx7ram             = map->map(map->handle, DX7_URI "#vdx7ram");
}



#include "Message.h"

// Interface to send Synth-to-Gui comm through LV2 host
struct LV2_ToGui : public ToGui {
	LV2_ToGui(LV2_URID_Map* map, URIs* uris) : uris(uris) {
		lv2_atom_forge_init(&forge, map);
	}
	virtual ~LV2_ToGui() {}
	virtual void push(Message m) { send(m); }
	virtual int pop(Message &m) { return 0; } // Should not be called

	void setup(LV2_Atom_Sequence* port) {
		capacity = port->atom.size;
		lv2_atom_forge_set_buffer(&forge, (uint8_t*)port, capacity);
		lv2_atom_forge_sequence_head(&forge, &frame, 0);
	}

	void send(int m) {
		lv2_atom_forge_frame_time(&forge, 0);
		lv2_atom_forge_int(&forge, m);
	}

	URIs* uris;

	uint32_t capacity;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
};

// Interface to send Gui-to-Synth comm through LV2 host
struct LV2_ToSynth : public ToSynth {
	LV2_ToSynth(LV2UI_Write_Function w, LV2UI_Controller c, URIs* uris)
		: uris(uris), write_function(w), controller(c) {
	}
	virtual ~LV2_ToSynth() {}
	virtual void push(Message m) { send(m); }
	virtual int pop(Message &m) { return 0; } // Should not be called

	void send(int m) {
		LV2_Atom_Int msg { sizeof(int), uris->atom_Int, m };
		write_function(controller, control_index, sizeof(LV2_Atom_Int), uris->atom_eventTransfer, &msg);
	}

	URIs* uris;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	int control_index = 0;
};


