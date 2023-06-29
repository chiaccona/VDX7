
# Install

See [INSTALL.md](INSTALL.md).

# CPU Load

In aiming to be an exact emulation, the code is relatively CPU intensive from a
DSP perspective (though it has been profiled and optimized extensively). On an
Intel i5 "Sandy Bridge" CPU with 4 cores and 4G RAM, at a Jack sampling rate of
48khz and 512 frame buffer, it uses about 35% of the available CPU realtime on
a core.  Note this is about twice the utilization of Dexed, primarily because
the latter (quite reasonably) optimizes the envelope calculation to once every
64 samples, and interpolates linearly between envelope points, so the expensive
envelope calculation becomes minimal.

But here, all 96 envelopes and operators need to be computed for each audio
sample, regardless of whether they are active or not, in order to maintain bit
synchrony with the real hardware. Emulations of the HD63B03RP CPU, and the OPS
and EGS chips are interleaved at the clock cycle level, driving up cache
bandwidth. Almost all of the math is done in integers, so the parallel
pipelining of modern floating point ALUs offers no advantage.  The synth output
is rendered at the native sampling rate of the DX7 hardware, about 49,096 hz,
which ensures that pitch, LFO rates, envelope timing, etc. are bit accurate.
The output is then digitally resampled to convert to the required Jack audio
rate.

Through profiling, it can be seen that the vast majority of CPU time in the DSP
thread is spent in the EGS and OPS emulations (about 90%, and split roughly in
half between OPS and EGS). The HD6303R emulation takes relatively little CPU,
though it uses a different cache memory set than the OPS and EGS, so it
probably imposes some cache thrashing penalty felt by the OPS and EGS.

There is a compile option to force inlining of the OPS and EGS. It's generally
a bad idea to try to outsmart a compiler's inlining decisions, but it's worth
testing it each way for any given CPU architecture, I have seen it gain
efficiency.

# Architecture:

There are three separable modules:

- The X11 GUI
- The Jack audio/MIDI interface
- The core DX7 emulation

## GUI
The interface from GUI to DX7 is via a simple message passing protocol
implemented by Message.h, sending through a Lock Free Queue so that the
realtime thread never blocks.  The interface from the DX7 to the GUI is a
simple API for updating the panel LCD and LED.  The GUI is built using pure
Xlib with no dependencies except the XPM extension. This of course makes it
utterly unportable to non-Linux environments.  It should be possible to compile
under Cygwin on Windows, with the Windows version of Jack and perhaps some
minimal changes for the environment.  I haven't used Cygwin (or Windows) is
several years, but back in the day the latency on Jack MIDI made it unusable -
hopefully that's been fixed, but I don't know.

## Jack Interface
JackDriver.h and the base class Synth implement the audio/MIDI mechanism. The
internal audio sample buffer is 128 float bytes (in mono). Jack's audio
callback transfers the audio samples to the system (adapting to the Jack buffer
size), and sends and receives MIDI events to and from the system. If a
different audio/MIDI backend is required, it should implement the interface
specified by class Synth in Synth.h.

## DX7Synth Class
The class DX7Synth is derived from Synth and implements the run() callback that
Jack calls regularly.  It contains a DX7 object which encapsulates the entire
DX7 emulation.  The run callback uses libsamplerate to resample the native DX7
49.096khz rate to Jack's required rate, generating the 128 audio samples per
buffer. The "callback" interface of libsamplerate is used to drive the
resampling, calling the fillBuffer() routine, which clocks the DX7 emulation to
generate input samples for libsamplrate.  It would of course be more efficient
to adapt the DX7 emulation to the required sample rate directly as most
softsynth implementations do (e.g. Dexed) - but that would subtly change the
bit-level output.

## DX7 Class
The DX7 class contains a HD6303R object which implements the HD63B03RP MPU used
by the DX7. It also contains an EGS object which implements the Envelope
Generator chip, and which in turn contains an OPS object that implements the
Operator chip. The OPS is contained within the EGS to facilitate the transfer
of pitch and envelope data between them.

## HD6303R Class
The HD63B03RP emulator is complete enough to run the DX7 firmware. Note, the
"B" just indicates the clock speed, and the "P" is the package, a 40-pin DIP.
The "R" indicates the specific peripheral architecture, so HD6303R is the
designation being emulated. The entire instruction set is implemented (even
instructions not used by the DX7).  There are some modes of the timer,
register file, and serial interface that are not implemented since they are not
used in the DX7, implementing them would have required a price in efficiency,
and they are not likely to be needed for any "alternative" DX7 ROMs.  The
hardware environment of the DX7, apart from the OPS and EGS, is emulated in the
DX7::run() function. In particular, the serial TX and RX work through the
register file at 0x000\*, the memory mapping of the EGS is 256 bytes at located
at 0x30\*\*, the peripherals LCD, LED, pedals, OPS, and sub-CPU handoff protocol
are 16 bytes at 0x280\*, and the cartridge is 4K bytes at 0x4\*\*\*. The main
external RAM, which has the battery backup, is at 0x1000-0x2800, and the 128
byte volatile internal RAM is at 0x80.

The default firmware is V1.8, the last official factory ROM. The synth may also
be compiled with the newer "Special Edition" ROM. It will probably run on other
custom ROMs, but depends on the original memory mapping for RAM, EGS, OPS,
cartridge, the hardware 0x280\* space, and the location of the master tune and
MIDI RX channel variables.

The code is fairly compact.  The CPU emulator uses a table of lamdas to select
each instruction. The lambdas are very brief, intended to fit on one line,
though there are few calls for more complex functions, which generally get
inlined.  So the main overhead is the lambda call itself. This is certainly
more efficient than a large switch that does not optimize to a jump-table (and
who knows what GCC would do). There is a more efficient design option using
GCC's labeled goto's, but that is not portable. The main memory is implemented
as a monolithic block. There are some read-only and write-only locations in the
register file - the ones that the DX7 microcode depends on are "patched" after a
read or write instruction to ensure that they comply.  A complete general
HD6303 emulator should trap on all low memory accesses and redirect to a true
register file implementation, but that would have an efficiency cost for the DX7.

## OPS Class
The OPS is implemented based on Ken Shirriff's analysis of the chip. For
efficiency, logsin and exptab tables are expanded to their full sizes rather
than try to copy the elaborate compression and encoding schemes used in the
chip, but the end result is precisely bit-perfect (simply making a different
time/space tradeoff - we can afford the 64k and 4k tables but badly need the
lookup speed, whereas the DX7 designers had plenty of nanoseconds for a few
layers of decoding logic, but no silicon space for large tables). The only
questionable areas are the required extension of the 12 bit envelope to 14 bits
- I assumed that they simply shifted up 2 bits, rather than try to implement a
more linear extrapolation in hardware. I also assume the feedback is an
inverted down shift (i.e. >> (1 + (7-feedback)).  I believe anything else would
be clearly audible in many patches. The hardware output circuit analog level
shifter, which was designed to mitigate the loss of 3 bits of resolution due to
the 12 bit DAC, is implemented by reducing bit resolution for larger
signals and preserving for small.

## EGS Class
The EGS required a bit of guesswork, but based on the public research for MSFA
and Dexed, the manifest interface generated by the microcode, and the
silicon limitations, the possibilities are quite narrow. The envelope is based
on the MSFA analysis, and the assumption of a single master clock (as used in
the OPL3 chip). The output should match the MSFA Java code, except
incorporating the "jump" to 1716, and the master clock.  Also, the envelope is
inverted (i.e. it represents a 12-bit attenuation, with 0xFFF being the minimum
possible amplitude). Bits change according to the clocking pattern documented for MSFA.

The DX7 has an envelope "delay" feature which is used on a few voices including
WATERGDN and CHIMES.  Pascal Gauthier measured delay times for use in Dexed,
and based on that data I was able to determine how the EGS actually implements
the delay time (see comments in the code). The delay also has either a feature
or bug wherein delay times are shortened significantly under certain conditions.
I designed implementation of that behavior that could have very plausibly been
implemented in the hardware (and I believe it was intended by design rather
than bug in the original EGS). The delay compression still needs to be validated.

The 256 byte memory map from the HD6303 drives the pitch and envelope
calculations, which in turn are passed to the OPS as a 14 bit pitch and 12 bit
level. Key events both trigger the envelopes, and are passed to the OPS for
operator sync. The pitch calculation starts with operator pitch (downshifted 2
bits to recover the value), and the detune is added. This value is used for a
"fixed" pitch operator. For a "ratio" pitch, the Voice Pitch and pitch
modulation are added. Finally the pitch value clamped to its 14 bit range.

One questionable area is the Rate Scaling calculation. To match the graph shown
on page 39 of the Operation Manual, it appears the correct calculation for the
scaling value added to the envelope rate needs to be:

	rs = int (min(27,max(0,(voicePitch[voice]>>8)-16))*rateScaling/7.0 + 0.5);
	This is added to qrate, then qrate is clamped to 0-63.

The voicePitch value needs to be used because that is the representation of the
keyboard note that the EGS has to work with. While it's possible that the rate
could change as the voice pitch modulates (due to, for example, a pitch EG
modulation), the MSFA analysis indicates that it is only set at key-on, so
that's what we do. This calculation of rate scaling ends up somewhat different
than that implemented in Dexed. Note that the calculation is somewhat
complicated and requires a division, so it is virtually certain that the chip
designers implemented it as a lookup in a small ROM (much as was done in the
OPS).

Before handing off to the OPS, the EGS adds in the Amplitude Modulation for
each operator. There is no modulation when ampModSens parameter is 0. When it
is 1 to 3, the 8-bit AmpMod value (which is logarithmic) generated by the LFO
is shifted left by 1 to 3 bits as given by the ampModSens value, and added to
the envelope.  Because both values are logarithmic, and the envelope is
inverted, this effectively divides the envelope by the scaled AmpMod.

## Filtering
The DX7 hardware's audio output runs at 16x49.096khz, and smoothes and
decimates the 16 channel output using a pair of sample/hold op amps, and a pair
of Sallen-Key filters. By the way, my guess is that the S/H was needed due to
the op amp's limited slew rate causing distortion - by reducing the rail to
rail slamming of the signal from voice to voice. Since the order of the voices
output by the OPS are shuffled, that interleaving might serve to mitigate the
potential signal swings, at least on average.

Anyway, the S/H is a very gentle filter that would have no audible effect in a
DSP emulation.  The S-K filters have a bit of complication - there is a
one-pole lowpass in front of the first S-K that loads it down. This turns out
to be clever - the 2 cascaded S-K's alone have a prominent resonance of about
12 dB around the 16khz cutoff frequency.  The passive LP loads down the first
S-K just enough to nearly completely remove the resonance.  So we end up with a
5th order LP at about 16k that is nearly flat in the passband.  Nevertheless,
with the Nyquist at 24.5khz, the 30 dB rolloff should still generate some
aliasing in the output. This synth's output stage emulates the 5th order filter
and decimation, making it deliberately noisier to match the real hardware.  In
addition, as previously mentioned, up to 3 bits of resolution are dropped to
emulate the level shifter.  All in all, it seems daft to increase CPU load in
order to create more quantization and aliasing noise, all in the name of
fidelity to the hardware, but it can be switched on or off through the
egs.clean(bool) function (e.g.  via MIDI). ;-)

# LV2
The LV2 specification and API is in general a hot mess...  sorry, I had to
say it... now I'll resist going into a rant.

Anyway, I'll call the LV2 implementation "experimental". It seems to work
properly in testing on Ardour 6.9 and JALV (and only on Linux, of course). The
code is ugly and who knows what may or may not work on other hosts - it seems
that's the nature of the beast. OK, I'll stop ranting...

The fact that the GUI is raw X11 seems to be fortunate, it is the
least common denominator for LV2 GUIs on Linux. An experimental GTK3 GUI works
on JALV (the GTK version, of course), but not on Ardour, which is GTK2-based.
So platform independence is a long way off, if ever, and "big" toolkits
inevitably have a limited half-life.

LV2 required some hacks - the message passing protocol between GUI and DSP
needs to be rerouted through the host, and the state of the display (the DX7's
LED and LCD) needs to be saved and restored because the host can terminate and
restart the GUI arbitrarily. The DX7 firmware updates its LCD controller
incrementally, so the display state would be lost if the GUI shuts down. And of
course the LV2 GUI and DSP modules should not share memory (though in these
specific hosts, they in fact do).

