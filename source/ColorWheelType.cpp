/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ColorWheelType.h"
#include "ColorRYB.h"
#include "MathUtil.h"
#include "Internationalisation.h"


static void rgb_hue2hue(double hue, Color* hsl){
	hsl->hsl.hue = hue;
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5;
}

static void rgb_rgbhue2hue(double rgbhue, double *hue){
	*hue = rgbhue;
}

static void ryb1_hue2hue(double hue, Color* hsl){
	Color c;
	color_rybhue_to_rgb(hue, &c);
	color_rgb_to_hsl(&c, hsl);
}

static void ryb1_rgbhue2hue(double rgbhue, double *hue){
	color_rgbhue_to_rybhue(rgbhue, hue);
}

static void ryb2_hue2hue(double hue, Color* hsl){
	hsl->hsl.hue = color_rybhue_to_rgbhue_f(hue);
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5;
}

static void ryb2_rgbhue2hue(double rgbhue, double *hue){
	color_rgbhue_to_rybhue_f(rgbhue, hue);
}

const ColorWheelType color_wheel_types[]={
	{N_("RGB"), rgb_hue2hue, rgb_rgbhue2hue},
	{N_("RYB v1"), ryb1_hue2hue, ryb1_rgbhue2hue},
	{N_("RYB v2"), ryb2_hue2hue, ryb2_rgbhue2hue},
};

const ColorWheelType* color_wheel_types_get(){
	return color_wheel_types;
}

const uint32_t color_wheel_types_get_n(){
	return sizeof(color_wheel_types)/sizeof(ColorWheelType);
}

