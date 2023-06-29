
## What motivated you?

As a mountain climber would say, because it was there...

Truth be told, while I enjoy exploring the wide variety of voices, and find they stimulate my musical creativity, it's arguably not the world's greatest sounding synth. And I'm not a big fan of the music of the era that popularized it, though I recognize and appreciate it's cultural significance. Hendrix, Bird, and Bach didn't play them.

It was, however, a fascinating technical challenge, and seemed a natural extension of the fine recent work, particularly by ajxs and Ken Shirriff.  Note well that my code is a research project motivated by advancing the community benefit of knowledge of the inner workings of this culturally iconic musical instrument. I have no intention of distributing it commercially. 

## What are the future plans?

The project was to implement a bit-accurate reference model of the Mark I DX7. It is very close, but probably not quite there yet.  I would appreciate feedback on any technical details that may still need to be resolved. In particular, let me know if you find specific audible differences in comparison with a hardware DX7, in as detailed a manner as possible (e.g. comparing the recorded audio outputs of MIDI sequences). And if anyone should be so kind as to decap an EGS chip, that would be awesome!

I'm amenable to usability improvements - for example, I can imagine that a hi-res screen option would be useful.

I'm don't find GUI development and multiplatform coding all that fun and interesting, so things like VSTs, Mac and Windows native versions, etc. are less likely. Similarly, extensions of the DX7 concept are beyond scope (can you make it 9 ops? 27 voices? 41 algorithms? 19-bit depth? add a reverb? etc.).

As far as implementing things like the DX7II, TX816, and other such members of the family, those would require a new and extensive research project that is well beyond my headlights, though I am interested in contributing.

## How does VDX7 compare to Dexed?

- Dexed:
	- A highly refined GUI enabling easy editing of FM voices
	- Multiplatform portability
	- Several sound engine options
	- Synth DSP is architected more efficiently, though resulting in slight sound differences for some voices
	- Currently missing a few features, e.g. portamento

- VDX7:
	- A crappy 1980's era user interface, written in a ancient 1980's era GUI toolkit ;-)
	- About twice the CPU load
	- Runs native DX7 firmware, hence all features are implemented by default, including portamento, MIDI I/O, SYSEX, exact mappings of controls and patch parameters, etc.
	- Bit-level hardware emulation should be extremely close to the actual hardware, e.g. for envelope and LFO timing, oscillator amplitudes and frequencies, bit depth, etc.

## Can you use Dexed as an editor and connect to VDX7 through SYSEX, like you can with a hardware DX7?

In principle, but I haven't gotten it to work.  Dexed is looking for a hardware ALSA port to send SYSEX messages, and VDX7 is a soft synth. There is a kernel module that creates virtual hardware ALSA ports, and Dexed can see and connect to them.  But while they appear in the Jack connection list, they don't seem to pass data. Perhaps it's a bug in Jack, perhaps it's my system configuration, perhaps it's an error between keyboard and chair... if you get it to work, let me know!

## Does it run on Windows?

Not natively - it requires X11 for the GUI and Jack for the audio. It should be relatively easy to adapt to compile on Cygwin, though I haven't tried it (no longer own a Windows machine ;-). Typically Cygwin ports require a few minor changes (is M\_PI defined? is some POSIX incantation needed? different compiler flags? that kind of thing). You would need to install the Windows version of Jack, which, I recall back in the day, was practically unusable for MIDI due to high latency.  Hopefully that's been fixed - IDK. If you get it to work, let me know!

## Does it run on a Raspberry Pi?

Unfortunately not. It will actually compile and run, but the Pi (at least my 3B+) doesn't have the CPU power to run the DSP without constant xruns. Perhaps with profiling and analysis, it could be made more efficient on ARM, though I suspect it will remain marginal until the Pi's become more powerful.  In addition, at least as of this writing (June 2023), Jack on RaspOS is quite broken, and pulseaudio eats significant CPU. Jack used to work well on Raspian, but that OS is gone. Perhaps a custom OS image might help, but that's a lot of work.

## Can it run different ROM firmware?

Yes. The most interesting is the "Special Edition" update, which is more feature rich than the default version 1.8. I prefer the original flavor, though, because the SE ROM's function (orange label) defaults are not to my liking (particularly, the pitchbend defaults to 3 semitones - I prefer 2 for most voices). It does offer a visual tuning display missing in 1.8.  More broadly, you can create new ROMs using ajxs's excellent annotated disassembly and dasm. Be forewarned that writing HD6303 assembler is a bit of an acquired taste, and the code is quite convoluted. And of course, you are limited by the capabilities of the EGS and OPS, and surrounding hardware. But (unlike the hardware), you should be able to use the full 64k memory space, allowing programs larger than the native 16k of ROM.

## Is the firmware Free (i.e. Libre) software?

No. The firmware blob, which is widely available online, is, to my knowledge, likely to still be proprietary. My code is GPL V3 and is a unique creative work independent of any proprietary software. It happens to be capable of loading (any) HD6303 firmware as a binary blob and may perhaps do something useful. This is much the same as, for example, the Linux kernel itself, which routinely loads proprietary driver blobs. My understanding is that you as a user generally have the right to combine such software for personal use, though it may be problematic if you chose to commercially distribute the combination.

## Why doesn't voice XXX sound quite right?

Possibly there is something not quite right in the OPS or EGS implementation - report a bug.  But be aware that on voices that are velocity sensitive, velocity matters - big time. Your MIDI controller likely sends different MIDI velocities than the native DX7 keyboard for exactly the same key press, so sounds will be different.  If you find that a voice sounds too dull, set a velocity curve to increase it.  Conversely, if it sounds too saturated, reduce the velocity curve. You can typically do that with a parameter in your keyboard, with a MIDI filter in Jack, or using the velocity curve options on the VDX7 command line.

## Speaking of velocity, I heard the DX7's MIDI range is restricted. Whatup?

Yep, incoming and outgoing MIDI are quantized by the firmware to 32 discrete values, rather than the full MIDI 127. The maximum output value is 118 (evidently if you _really_ hammer the key), and the minimum is 16. There are curves in the firmware, and for some reason they are in fact different for incoming and outgoing messages. A MIDI sequence recorded from a DX7 and then played back will sound slightly different.

The firmware does handle the full 127 range internally (different from MIDI, though - 0 is still note off, but all note-ons are inverted i.e. 127 is minimum velocity and 1 is maximum). The sub-CPU interface that hands off key events between the native keyboard and the HD6303 supports 0-127, but I don't know if the native keyboard actually uses the full range. Someone might experiment on a DX7 by setting the Init patch to produce a sine and making it velocity sensitive (Init is not by default), then playing with a wide range of dynamics and analyzing the recorded output to see if it clusters around 127 or 32 (or some other) discrete amplitudes.  I suspect the internal keyboard uses the full range, and the quantization is specific to MIDI I/O.

VDX7 by default uses the full MIDI velocity range, working under the assumption that the native DX7 keyboard does as well, and emulates the sub-CPU handoff. But there is a command line option "-m" that instead feeds incoming MIDI through the DX7 serial MIDI interface, as would be the case if you sent your keyboard to a hardware DX7.  In that case, the 32-level quantization is used. 

By the way, you would want to use the "-m" option if you are comparing VDX7 to a real DX7 (and sending your keyboard MIDI to the hardware DX7), rather than using the DX7's MIDI output.  That way both synth's firmware see exactly the same velocity.

## Does it simulate the crunchy DAC in the Mark I DX7?

Yes. The bit depth is modified by a 2 bit "exponent" and level shifter (see Ken Shirrif's description of the output stage). The 16 voices are then mixed by send through a Sallen-Key decimation filter, resulting in some degree of aliasing and quantization noise (there is a "clean" option that bypasses this). The hardware's 16khz lowpass analog decimation filter is modeled with quite high accuracy by a 5th order IIR digital filter designed from the hardware circuit.

A possible future modification is to simulate variations in the resistors used in the level shifter. As of now they are "perfect", whereas in the hardware they have 0.1% tolerance, resulting in some slight distortion that would vary between individual DX7s. I don't know whether this effect would be all that noticeable, and there is some potential risk of introducing unintended aliasing noise (though it would be mitigated by the 16x oversampling).

## Does it do that weird envelope delay "bug"?

Yes. See Howard Massey's "The Complete DX7", page 117 for a good description of the behavior.  It's not a bug, it was designed that way, and used in several patches, including WATERGDN and CHIMES.

The implementation of the basic delay timing should be bit accurate.  There is an open question as to how the "compressed" (significantly shortened) delay behavior works. My speculation is that compression only applies to the first stage of the envelope (whereas the delay feature also works on the second stage, but never compressed), and depends on whether the previous cycle of that envelope completed the first stage or not. Thus the seemingly "random" behavior Massey refers to would stem from this algorithm.

From the code:
```
		// Set up delay when env level and prev level >= 40 (i.e. env
		// level param <20).  Small levels are inaudible, hence
		// experienced as a delay. Magic delay constant is 479 decaying
		// envelope steps, which, in conjunction with qrate, creates
		// the required delay time.
		// FIX: speculative implementation of "bug" behavior (if my
		// speculation is correct this would actually have to have been designed
		// behavior in the DX7). Delays when in stage 0 are compressed (shortened)
		// by a factor of 32 if the previous stage 0 delay did not fully
		// complete, otherwise it reverts to the full length delay.
		// Stage 1 delays are always full length. No delays in stage 2 and 3
		// (because because they only exit on key events).
```

I would be interested in input from actual DX7s to validate and refine this behavior.

