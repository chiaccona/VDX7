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

const char* style = R"(
frame border {
	/*border: 0px white none;*/
}

label {
/*
	border-width: 0;
	padding: 0;
	font-size: 10pt;
	font-weight: bold;
*/
}

#cart_label {
	color: #ffffff;
	background: #000000;
	font-size: 7pt;
	border-width: 0;
	padding: 0;
	margin: 0;
	border-width: 0;
}

switch slider {
	border: 0px white none;
	border-bottom-left-radius: 0;
	border-bottom-right-radius: 0;
	border-top-left-radius: 0;
	border-top-right-radius: 0;
	font-size: 4px;
	min-height: 4px;
	min-width: 4px;
	padding: 0;
	margin: 0;
	color: #0000FF;
	background-color: #000000;
	background-image: none;
}

switch {
	border: 0px white none;
	border-bottom-left-radius: 0;
	border-bottom-right-radius: 0;
	border-top-left-radius: 0;
	border-top-right-radius: 0;
	font-size: 4px;
	min-height: 4px;
	min-width: 10px;
	padding: 0;
	margin: 0;
	color: #0000FF;
	background-color: #808080;
	background-image: none;
}


#bg_label {
	color: #1394E3;
	font-size: 5pt;
	border-width: 0;
	padding: 0;
	margin: 0;
	border-width: 0;
}

#l_blue, #l_blue_ul, #l_orange, #l_orange_ul, #l_white, #l_white_ul, #l_reverse {
	border-width: 0;
	padding: 0;
	font-size: 5pt;
	font-weight: bold;
}

#l_blue, #l_blue_ul {
	color: #1394E3;
}

#l_blue_ul {
	border-bottom: 1px #1394E3 solid;
}

#l_orange, #l_orange_ul {
	color: #be7e50;
}

#l_orange_ul {
	border-top: 1px #be7e50 solid;
}

#l_white, #l_white_ul {
	color: #ffffff;
}

#l_white_ul {
	border-bottom: 1px #ffffff solid;
}

#l_reverse {
	color: #463130;
	background: #ffffff;
}


#l_label {
	color: white;
	font-size: 7pt;
}

#r_label {
	color: black;
	font-size: 5pt;
	font-weight: bold;
	font-stretch: ultra-condensed;
}


grid {
	background: #463130;
}

#teal_button, #brown_button, #red_button, #blue_button {
	background: #00BBBE;
	border-width: 0px;
	padding: 0;
	margin: 0;
	text-shadow: none;
}

#teal_button { background: #00BBBE; }
#brown_button { background: #BE7E50; }
#red_button { background: #FF6D5E; }
#blue_button { background: #1394E3; }

#file_button {
	background: #808080;
	border-width: 1px;
	min-height: 5px;
	padding: 0;
	margin: 0;
	text-shadow: none;
	font-size: 7pt;
	font-weight: bold;
}

#led {
	background: black;
	color: red;
	font: 24px monospace;
}
#lcd {
	background: #74B097;
	color: #0A0A0A;
	font: 12px monospace;
	margin-top: 10px;
	margin-bottom: 10px;
	border-style: inset;
	border-width: 3px;
}
#display {
	background:black;
}

scale trough {
	border-style: inset;
	border-radius: 0px;
	background: black;
	min-width: 25px;
}
scale highlight {
	background: black;
}

scale slider {
	border-radius: 0px;
	background-color: gray;
	min-height: 5px;
	min-width: 25px;
	/*background-image: url("fader.png");*/
	background-image: url("resource:///image/fader.png");
	background-size: 100% 100%;
	border-width: 1px;
	border-style: outset;
}
scale {
	/*background:gray;*/
}
)";

#if 0
const char* style = R"(

frame border {
	/*border: 0px white none;*/
}

label {
/*
	border-width: 0;
	padding: 0;
	font-size: 10pt;
	font-weight: bold;
*/
}

#cart_label {
	color: #ffffff;
	background: #000000;
	font-size: 7pt;
	border-width: 0;
	padding: 0;
	margin: 0;
	border-width: 0;
}

switch, slider {
	/*
	border: 0px white none;
	border-bottom-left-radius: 0;
	border-bottom-right-radius: 0;
	border-top-left-radius: 0;
	border-top-right-radius: 0;
	*/
	font-size: 4px;
	min-height: 4px;
	min-width: 10px;
	padding: 0;
	margin: 0;
	color: #FF0000;
	background-color: #ffffff;
	background-image: none;
}


#bg_label {
	color: #1394E3;
	font-size: 5pt;
	border-width: 0;
	padding: 0;
	margin: 0;
	border-width: 0;
}

#l_blue, #l_blue_ul, #l_orange, #l_orange_ul, #l_white, #l_white_ul, #l_reverse {
	border-width: 0;
	padding: 0;
	font-size: 5pt;
	font-weight: bold;
}

#l_blue, #l_blue_ul {
	color: #1394E3;
}

#l_blue_ul {
	border-bottom: 1px #1394E3 solid;
}

#l_orange, #l_orange_ul {
	color: #be7e50;
}

#l_orange_ul {
	border-top: 1px #be7e50 solid;
}

#l_white, #l_white_ul {
	color: #ffffff;
}

#l_white_ul {
	border-bottom: 1px #ffffff solid;
}

#l_reverse {
	color: #463130;
	background: #ffffff;
}


#l_label {
	color: white;
	font-size: 7pt;
}

#r_label {
	color: black;
	font-size: 5pt;
	font-weight: bold;
	font-stretch: ultra-condensed;
}


grid {
	background: #463130;
}

#teal_button, #brown_button, #red_button, #blue_button {
	background: #00BBBE;
	border-width: 0px;
	padding: 0;
	margin: 0;
	text-shadow: none;
}

#teal_button { background: #00BBBE; }
#brown_button { background: #BE7E50; }
#red_button { background: #FF6D5E; }
#blue_button { background: #1394E3; }

#file_button {
	background: #ffffff;
	border-width: 0px;
	min-height: 5px;
	padding: 0;
	margin: 0;
	text-shadow: none;
	font-size: 7pt;
	font-weight: bold;
}

#led {
	background: black;
	color: red;
	font: 24px monospace;
}
#lcd {
	background: #74B097;
	color: #0A0A0A;
	font: 12px monospace;
	margin-top: 10px;
	margin-bottom: 10px;
	border-style: inset;
	border-width: 3px;
}
#display {
	background:black;
}

scale trough {
	border-style: inset;
	border-radius: 0px;
	background: black;
	min-width: 25px;
}
scale highlight {
	background: black;
}

scale slider {
	border-radius: 0px;
	background-color: gray;
	min-height: 5px;
	min-width: 25px;
	/*background-image: url("fader.png");*/
	background-image: url("resource:///image/fader.png");
	background-size: 100% 100%;
	border-width: 1px;
	border-style: outset;
}
scale {
	/*background:gray;*/
}

)";
#endif
