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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <cstdint>
#include <fcntl.h>
#include <cctype>
#include <cmath>
#include <sys/time.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>

#include "JackDriver.h"

void JackDriver::close_jack() {
	if (j_client) {
		jack_deactivate(j_client);
		jack_client_close(j_client);
	}
	j_client=0;
}

// when jack shuts down...
void JackDriver::jack_shutdown_callback(void *arg) {
	fprintf(stderr,"jack server shut us down.\n");
	((JackDriver*)arg)->j_client=0;
}

int JackDriver::jack_srate_callback(jack_nframes_t nframes, void *arg) {
	JackDriver *jp = (JackDriver*)arg;
	jp->synth->setSampleRate(nframes);
	return(0);
}

int JackDriver::callback(jack_nframes_t nframes) {
	// Audio
	jack_default_audio_sample_t *out = (jack_default_audio_sample_t*)
		jack_port_get_buffer(jack_audio_out_port, nframes);
	if (!synth_ready) {
		memset(out, 0, nframes*sizeof(jack_default_audio_sample_t));
		return (0);
	}

	// MIDI
	jack_nframes_t midi_tme_proc = 0;
	void *jack_midi_in_buf = jack_port_get_buffer(jack_midi_in_port, nframes);
	int midi_events = jack_midi_get_event_count(jack_midi_in_buf);
	void *jack_midi_out_buf = jack_port_get_buffer(jack_midi_out_port, nframes);
	jack_midi_clear_buffer(jack_midi_out_buf);

	// Loop
	int midi_event_count = 0;
	static int boffset = BufSize;
	jack_nframes_t written = 0;
	while (written < nframes) {
		int nremain = nframes - written;

		if (boffset >= BufSize)  {
			// Receive MIDI
			for (midi_event_count=0; midi_event_count<midi_events; midi_event_count++) {
				jack_midi_event_t ev;
				jack_midi_event_get(&ev, jack_midi_in_buf, midi_event_count);
				if (ev.time >= written && ev.time < (written+BufSize)) synth->queueMidiRx(ev.size, ev.buffer);
			}
			midi_tme_proc = written + BufSize;

			// Send MIDI
			uint32_t size=0;
			uint8_t* buffer=0;
			while(synth->queueMidiTx(size, buffer)) {
				jack_midi_event_write(jack_midi_out_buf, written, buffer, size);
			}

			// Process Audio - get BufSize samples
			synth->run();
			boffset = 0;
		}

		int nread = (nremain < (BufSize - boffset) ? nremain : (BufSize - boffset));

		memcpy(out+written, synth->outputBuffer+boffset, nread*sizeof(float));
		written+=nread;
		boffset+=nread;
	}

	// process remaining MIDI events
	// IF nframes < BufSize OR nframes != N * BufSize
	for ( ; midi_event_count<midi_events; midi_event_count++) {
		jack_midi_event_t ev;
		jack_midi_event_get(&ev, jack_midi_in_buf, midi_event_count);
		if (ev.time >= midi_tme_proc) synth->queueMidiRx(ev.size, ev.buffer);
	}
	return(0);
}

int JackDriver::jack_audio_callback(jack_nframes_t nframes, void *arg) {
	JackDriver *jp = (JackDriver*)arg;
	jp->callback(nframes);
	return(0);
}

int JackDriver::open_jack(void) {
	j_client = jack_client_open (jack_client_name, JackNullOption, 0);

	if (!j_client) {
		fprintf(stderr, "could not connect to jack.\n");
		return(1);
	}

	jack_on_shutdown(j_client, jack_shutdown_callback, this);
	jack_set_process_callback(j_client,jack_audio_callback, this);
	jack_set_sample_rate_callback(j_client, jack_srate_callback, this);

	jack_audio_out_port = jack_port_register(j_client, audio_out_port_name,
			JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	if (!jack_audio_out_port) {
		fprintf(stderr, "no more jack ports available.\n");
		jack_client_close(j_client);
		return(1);
	}

	fprintf(stderr, "jack-midi-in-port = %s\n", midi_in_port_name);
	jack_midi_in_port = jack_port_register(j_client,
			midi_in_port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput , 0);
	if (jack_midi_in_port == 0) {
		fprintf(stderr, "can't register jack-midi-in-port\n");
		jack_client_close(j_client);
		return(1);
	}

	fprintf(stderr, "jack-midi-out-port = %s\n", midi_out_port_name);
	jack_midi_out_port = jack_port_register(j_client,
			midi_out_port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput , 0);
	if (jack_midi_out_port == 0) {
		fprintf(stderr, "can't register jack-midi-out-port\n");
		jack_client_close(j_client);
		return(1);
	}

	// force getting samplerate
	jack_srate_callback(jack_get_sample_rate(j_client),this);

	return jack_activate(j_client);
}

int JackDriver::initPCM(const char *dev_name) {
	if(dev_name) audio_out_port_regex = dev_name;

	if (audio_out_port_regex && strlen(audio_out_port_regex)>0) {
		// Match port name regexp
		const char **found_ports = jack_get_ports(j_client, audio_out_port_regex,
				JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
		if (found_ports) {
			for (int i = 0; found_ports[i]; ++i) {
				if (jack_connect(j_client, jack_port_name(jack_audio_out_port), found_ports[i])) {
					fprintf(stderr, "JACK: cannot connect port %s to %s\n",
							jack_port_name(jack_audio_out_port), found_ports[i]);
				}
			}
			jack_free(found_ports);
		}
	}
	return 0;
}

int JackDriver::initMidi(const char *in_port_rx, const char *out_port_rx) {
	midi_in_port_regex = in_port_rx;
	midi_out_port_regex = out_port_rx;

	// Midi input port connections
	if (midi_in_port_regex && strlen(midi_in_port_regex)>0) {
		fprintf(stderr, "midi_in_port_regex = %s\n", midi_in_port_regex);
		const char **found_ports = jack_get_ports(j_client, midi_in_port_regex,
				JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
		if (found_ports) {
			for (int i = 0; found_ports[i]; ++i) {
				// Don't connect to ourself
				if(!strcmp(found_ports[i], jack_port_name(jack_midi_out_port))) continue;
				// Connect to found port
				fprintf(stderr, "connecting  %s = %s\n",
						found_ports[i], jack_port_name(jack_midi_in_port));
				if (jack_connect(j_client, found_ports[i],
							jack_port_name(jack_midi_in_port))) {
					fprintf(stderr, "JACK: cannot connect port %s to %s\n",
							found_ports[i], jack_port_name(jack_midi_in_port));
				}
			}
			jack_free(found_ports);
		}
	}

	// Midi output port connections
	if (midi_out_port_regex && strlen(midi_out_port_regex)>0) {
		fprintf(stderr, "midi_out_port_regex = %s\n", midi_out_port_regex);
		const char **found_ports = jack_get_ports(j_client, midi_out_port_regex,
				JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
		if (found_ports) {
			for (int i = 0; found_ports[i]; ++i) {
				// Don't connect to ourself
				if(!strcmp(found_ports[i], jack_port_name(jack_midi_in_port))) continue;
				// Connect to found port
				fprintf(stderr, "connecting  %s = %s\n",
						found_ports[i], jack_port_name(jack_midi_out_port));
				if (jack_connect(j_client, jack_port_name(jack_midi_out_port), found_ports[i])) {
					fprintf(stderr, "JACK: cannot connect port %s to %s\n",
							found_ports[i], jack_port_name(jack_midi_out_port));
				}
			}
			jack_free(found_ports);
		}
	}

	return 0;
}

void JackDriver::init() {
	// Open the JACK connection and get samplerate
	if (open_jack()) {
		perror ("could not connect to JACK.");
		exit(1);
	}
}

