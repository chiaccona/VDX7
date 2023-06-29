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

#if GTKMM
#include "Gui-gtkmm.h"
#else
#include "Gui.h"
#endif

struct DX7PluginGUI {
	// Port buffers
	const LV2_Atom_Sequence* control = 0;
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	// Features
	LV2_URID_Map* map = 0;
	LV2_Log_Logger logger;
	URIs uris;

	// State
	DX7GUI *gui = 0;

	// Communication interface
	LV2_ToSynth *toSynth = 0;
	App_ToGui toGui;

	DX7PluginGUI(const LV2_Feature* const* features,
		LV2UI_Write_Function w, LV2UI_Controller c)
		: write_function(w), controller(c)
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

		toSynth = new LV2_ToSynth(write_function, controller, &uris);
#if GTKMM
		Gtk::Main::init_gtkmm_internals(); // gtkmm only
		gui = new DX7GUI();
		gui->toSynth = toSynth;
		gui->toGui = &toGui;
		gui->init();
#else
		// X11 version
		Window parentXwindow = 0;
		for (int i = 0; features[i]; ++i) {
			if (!strcmp(features[i]->URI, LV2_UI__parent)) {
				parentXwindow = (Window)features[i]->data;
			}
		}
		gui = new DX7GUI(false, parentXwindow); // don't show keyboard
		gui->toSynth = toSynth;
		gui->toGui = &toGui;
#endif
	}
	~DX7PluginGUI() {
		delete gui;
		if(toSynth) delete(toSynth);
		toSynth = 0;
	}

	void connectPort (const uint32_t port, void* data) {
		switch (port) {
			case 0: control = (const LV2_Atom_Sequence*)data; break;
			default: break;
		}
	}
};

// LV2 core interface

static LV2UI_Handle instantiateUI(const LV2UI_Descriptor* descriptor,
		const char* plugin_uri,
		const char* bundle_path,
		LV2UI_Write_Function write_function,
		LV2UI_Controller controller,
		LV2UI_Widget* _widget,
		const LV2_Feature* const* features) {

	if (strcmp(plugin_uri, DX7_URI) != 0) {
		fprintf(stderr, "LV2 error: this GUI does not support plugin with URI %s\n", plugin_uri);
		return NULL;
	}

	DX7PluginGUI* self = new DX7PluginGUI(features, write_function, controller);
	if (self == NULL) return NULL;
	*_widget = (LV2UI_Widget)self->gui->widget();

	self->gui->restore();

	return (LV2UI_Handle)self;
}

static void cleanupUI(LV2_Handle instance) {
	DX7PluginGUI* self = (DX7PluginGUI*)instance;
	if(self) delete self;
}

static int ui_idle(LV2UI_Handle instance) {
	DX7PluginGUI* self = (DX7PluginGUI*)instance;
	if(self) self->gui->idle();
	return 0;
}


static void port_eventUI(LV2UI_Handle ui,
	uint32_t port_index,
	uint32_t buffer_size,
	uint32_t format,
	const void * buffer)
{
	if(port_index!=2) return;

	DX7PluginGUI *self = static_cast<DX7PluginGUI*>(ui);

	const LV2_Atom *msg = static_cast<const LV2_Atom*>(buffer);

	if(msg->type == self->uris.atom_Int) {
		int m = (static_cast<const LV2_Atom_Int*>(buffer))->body;
		if(self && self->gui && self->gui->toGui) self->gui->toGui->push(uint16_t(m));

	} else
		lv2_log_trace(&self->logger, "synth to gui msg not recognized: %d\n", msg->type);

	return;
}

static const void* extension_dataUI(const char* uri) {
	static const LV2UI_Idle_Interface idle = {ui_idle};
	if (!strcmp(uri, LV2_UI__idleInterface)) return &idle;
	return NULL;
}

static LV2UI_Descriptor descriptorUI = {
		DX7_URI "#gui",
		instantiateUI,
		cleanupUI,
		port_eventUI,
		extension_dataUI
};

const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
	return index == 0 ? &descriptorUI : NULL;
}

