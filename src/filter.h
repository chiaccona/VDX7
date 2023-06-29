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
#include <cmath>

// Single pole lowpass to filter MIDI volume
struct LP1 {
	LP1(double d = 1.) { set (d); y1 = 0.; }
	void reset() { y1 = 0.; }
	void set_f(float fc) { set (1 - exp(-2*M_PI*fc)); }
	void set(float d) { a0 = d; b1 = 1 - d; }
	float operate(float x) { return y1 = a0*x + b1*y1; }
	float a0, b1, y1;
};

// First order lowpass section
struct LP {
	LP() { }
	void reset() { x1 = y1 = 0.; }
	float operate(float s) {
		y1 = s + b1*x1 - a0*y1;
		x1 = s;
		return y1;
	}
	float a0=0.0, b1=0.0, y1=0.0, x1=0.0;
};

// Second Order Section filter
struct SOS_Coeff { float b1, b2, a1, a2; };
struct SOS {
	SOS_Coeff coeff;
	int h=0;
	float x[2]={0}, y[2]={0}; // history 

	SOS() = default;
	SOS(const SOS_Coeff& x) : coeff(x) {}

	// DF-I implementation
	float operate(float s) {
		float r = s;
		r += coeff.b1 * x[h]; r -= coeff.a1 * y[h];
		h ^= 1;
		r += coeff.b2 * x[h]; r -= coeff.a2 * y[h];
		y[h] = r; x[h] = s;
		return r;
	}
};

// DX7 5th order Sallen-Key topology decimation filter
// Fixed SR at 16*49.096khz
struct Filter {
	LP lp;
	SOS sos1;
	SOS sos2;
	float gain;

	Filter() {
		lp.b1 = 1.0000065695182569;
		lp.a0 = -0.9471494282369527;
		sos1.coeff.b1 = 1.9999934304817428;
		sos1.coeff.b2 = 0.9999934305249014;
		sos1.coeff.a1 = -1.9047157177069487;
		sos1.coeff.a2 = 0.9129212928486624;
		sos2.coeff.b1 = 2.0000000000000000;
		sos2.coeff.b2 = 1.0000000000000000;
		sos2.coeff.a1 = -1.9531729648773684;
		sos2.coeff.a2 = 0.9694025617460298;
		gain = 2.1994620400553497e-07;
	}

	float operate(float s) {
		return gain * sos2.operate( sos1.operate( lp.operate(s) ) );
	}
};

