#!/bin/bash
EXE="jalv.gtk3  "
while getopts sdgq flag
do
	case "${flag}" in
		s) EXE="jalv -s ";;
		d) EXE="jalv.gtk ";;
		g) EXE="jalv.gtk3 ";;
		q) EXE="jalv.qt5 ";;
	esac
done

export LV2_PATH=$(pwd)/lv2
lv2ls

${EXEPATH}${EXE} urn:chiaccona:vdx7 &
sleep 2
jack_connect VDX7:out system:playback_1
jack_connect VDX7:out system:playback_2
jack_connect system:midi_capture_1 VDX7:midi_in
jack_connect system:midi_capture_2 VDX7:midi_in
wait

