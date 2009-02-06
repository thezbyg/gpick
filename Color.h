/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#ifndef COLOR_H_
#define COLOR_H_

typedef struct Color{
	union{
		struct{
			float red;
			float green;
			float blue;
		}rgb;
		struct{
			float hue;
			float saturation;
			float value;
		}hsv;
		struct{
			float hue;
			float saturation;
			float lightness;
		}hsl;
		struct{
			float x;
			float y;
			float z;
		}xyz;
		struct{
			float L;
			float a;
			float b;
		}lab;
	};
}Color;

void
color_rgb_to_hsl(Color* a, Color* b);

void
color_rgb_to_hsv(Color* a, Color* b);

void
color_hsv_to_rgb(Color* a, Color* b);

void
color_rgb_to_xyz(Color* a, Color* b);

void
color_xyz_to_rgb(Color* a, Color* b);

void
color_xyz_to_lab(Color* a, Color* b);

void
color_rgb_to_lab(Color* a, Color* b);


void
color_copy(Color* a, Color* b);

void
color_add(Color* a, Color* b);

void
color_multiply(Color* a, float b);

void
color_zero(Color* a);

Color*
color_new();

void
color_destroy(Color* a);

void
color_set(Color* a, float value);

void
color_get_contrasting(Color* a, Color* b);

#endif /* COLOR_H_ */
