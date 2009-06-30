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

#include "Color.h"
#include <math.h>
#include "MathUtil.h"

void
color_rgb_to_hsv(Color* a, Color* b)
{
	float min, max, delta;

	max = max_float_3(a->rgb.red, a->rgb.green, a->rgb.blue);
	min = min_float_3(a->rgb.red, a->rgb.green, a->rgb.blue);
	delta = max - min;

	b->hsv.value = max;

	if (max != 0.0f)
		b->hsv.saturation = delta / max;
	else
		b->hsv.saturation = 0.0f;

	if (b->hsv.saturation == 0.0f) {
		b->hsv.hue = 0.0f;
	}
	else {
		if (a->rgb.red == max)
			b->hsv.hue = (a->rgb.green - a->rgb.blue) / delta;
		else if (a->rgb.green == max)
			b->hsv.hue = 2.0f + (a->rgb.blue - a->rgb.red) / delta;
		else if (a->rgb.blue == max)
			b->hsv.hue = 4.0f + (a->rgb.red - a->rgb.green) / delta;

		b->hsv.hue /= 6.0f;

		if (b->hsv.hue < 0.0f)
			b->hsv.hue += 1.0f;
		if (b->hsv.hue >= 1.0f)
			b->hsv.hue -= 1.0f;
	}
}


void
color_hsv_to_rgb(Color* a, Color* b)
{
	float h,v, f, x, y, z;
	int i;

	v = a->hsv.value;

	if (a->hsv.saturation == 0.0f) {
		b->rgb.red = b->rgb.green = b->rgb.blue = a->hsv.value;
	} else {

		h = fmodf(a->hsv.hue, 1.0f) * 6.0f;

		i = int(h);
		f = h - floor(h);

		x = v * (1.0f - a->hsv.saturation) ;
		y = v * (1.0f - (a->hsv.saturation * f));
		z = v * (1.0f - (a->hsv.saturation * (1.0f - f)));

		if (i==0){
			b->rgb.red = v;
			b->rgb.green = z;
			b->rgb.blue = x;
		}else if (i==1){
			b->rgb.red = y;
			b->rgb.green = v;
			b->rgb.blue = x;
		}else if (i==2){
			b->rgb.red = x;
			b->rgb.green = v;
			b->rgb.blue = z;
		}else if (i==3){
			b->rgb.red = x;
			b->rgb.green = y;
			b->rgb.blue = v;
		}else if (i==4){
			b->rgb.red = z;
			b->rgb.green = x;
			b->rgb.blue = v;
		}else{
			b->rgb.red = v;
			b->rgb.green = x;
			b->rgb.blue = y;
		}
	}
}

void
color_rgb_to_xyz(Color* a, Color* b)
{
	float R=a->rgb.red, G=a->rgb.green, B=a->rgb.blue;

	if (R>0.04045){
		R=pow(((R+0.055)/1.055),2.4);
	}else{
		R=R/12.92;
	}

	if (G>0.04045){
		G=pow(((G+0.055)/1.055),2.4);
	}else{
		G=G/12.92;
	}

	if (B>0.04045){
		B=pow(((B+0.055)/1.055),2.4);
	}else{
		B=B/12.92;
	}

	b->xyz.x = R * 0.4124 + G * 0.3576 + B * 0.1805;
	b->xyz.y = R * 0.2126 + G * 0.7152 + B * 0.0722;
	b->xyz.z = R * 0.0193 + G * 0.1192 + B * 0.9505;
}

void
color_xyz_to_rgb(Color* a, Color* b)
{
	float R,G,B;

	R = a->xyz.x *  3.2406 + a->xyz.y * -1.5372 + a->xyz.z * -0.4986;
	G = a->xyz.x * -0.9689 + a->xyz.y *  1.8758 + a->xyz.z *  0.0415;
	B = a->xyz.x *  0.0557 + a->xyz.y * -0.2040 + a->xyz.z *  1.0570;

	if (R>0.0031308){
		R=1.055*(pow(R,1/2.4))-0.055;
	}else{
		R=12.92*R;
	}
	if (G>0.0031308){
		G=1.055*(pow(G,1/2.4))-0.055;
	}else{
		G=12.92*G;
	}
	if(B>0.0031308){
		B=1.055*(pow(B,1/2.4))-0.055;
	}else{
		B=12.92*B;
	}

	b->rgb.red=R;
	b->rgb.green=G;
	b->rgb.blue=B;
}

void
color_xyz_to_lab(Color* a, Color* b)
{
	float X,Y,Z;

	X = a->xyz.x / 95.047f;
	Y = a->xyz.y / 100.000f;
	Z = a->xyz.z / 108.883f;

	if (X>0.008856){
		X=pow(X,1.0/3.0);
	}else{
		X=(7.787*X)+(16.0/116.0);
	}
	if (Y>0.008856){
		Y=pow(Y,1.0/3.0);
	}else{
		Y=(7.787*Y)+(16.0/116.0);
	}
	if (Z>0.008856){
		Z=pow(Z,1.0/3.0);
	}else{
		Z=(7.787*Z)+(16.0/116.0);
	}

	b->lab.L=(116*Y)-16;
	b->lab.a=500*(X-Y);
	b->lab.b=200*(Y-Z);

}

void
color_rgb_to_lab(Color* a, Color* b)
{
	Color c;
	color_rgb_to_xyz(a, &c);
	color_xyz_to_lab(&c, b);
}

void
color_lab_to_rgb(Color* a, Color* b)
{
	Color c;
	color_lab_to_xyz(a, &c);
	color_xyz_to_rgb(&c, b);
}

void
color_copy(Color* a, Color* b)
{
	b->m.m1=a->m.m1;
	b->m.m2=a->m.m2;
	b->m.m3=a->m.m3;
	b->m.m4=a->m.m4;
}

void
color_add(Color* a, Color* b)
{
	a->m.m1+=b->m.m1;
	a->m.m2+=b->m.m2;
	a->m.m3+=b->m.m3;
	a->m.m4+=b->m.m4;
}

void
color_multiply(Color* a, float b)
{
	a->m.m1*=b;
	a->m.m2*=b;
	a->m.m3*=b;
	a->m.m4*=b;
}

void
color_zero(Color* a)
{
	a->m.m1=0;
	a->m.m2=0;
	a->m.m3=0;
	a->m.m4=0;
}

void
color_get_contrasting(Color* a, Color* b)
{
	Color t;
	color_rgb_to_lab(a, &t);

	if (t.lab.L>3){
		t.hsv.value=0;
	}else{
		t.hsv.value=1;
	}

	t.hsv.saturation=0;
	t.hsv.hue=0;

	color_hsv_to_rgb(&t, b);
}

void
color_set(Color* a, float value)
{
	a->rgb.red = a->rgb.green = a->rgb.blue = value;
}

Color* color_new() {
	return new Color;
}

void color_destroy(Color *a) {
	delete a;
}

void color_rgb_to_hsl(Color* a, Color* b) {
	float min, max, delta;

	min = min_float_3(a->rgb.red, a->rgb.green, a->rgb.blue);
	max = max_float_3(a->rgb.red, a->rgb.green, a->rgb.blue);
	delta = max - min;

	b->hsl.lightness = (max + min) / 2;

	if (delta == 0) {
		b->hsl.hue = 0;
		b->hsl.saturation = 0;
	} else {
		if (b->hsl.lightness < 0.5) {
			b->hsl.saturation = delta / (max + min);
		} else {
			b->hsl.saturation = delta / (2 - max - min);
		}

		if (a->rgb.red == max)
			b->hsv.hue = (a->rgb.green - a->rgb.blue) / delta;
		else if (a->rgb.green == max)
			b->hsv.hue = 2.0f + (a->rgb.blue - a->rgb.red) / delta;
		else if (a->rgb.blue == max)
			b->hsv.hue = 4.0f + (a->rgb.red - a->rgb.green) / delta;

		b->hsv.hue /= 6.0f;

		if (b->hsv.hue < 0.0f)
			b->hsv.hue += 1.0f;
		if (b->hsv.hue >= 1.0f)
			b->hsv.hue -= 1.0f;

	}
}


void color_hsl_to_rgb(Color* a, Color* b) {
	if (a->hsl.saturation == 0) {
		b->rgb.red = b->rgb.green = b->rgb.blue = a->hsl.lightness;
	} else {
		float q, p, R, G, B;

		if (a->hsl.lightness < 0.5)
			q = a->hsl.lightness * (1.0 + a->hsl.saturation);
		else
			q = a->hsl.lightness + a->hsl.saturation - a->hsl.lightness * a->hsl.saturation;

		p = 2 * a->hsl.lightness - q;

		R = a->hsl.hue+1/3.0;
		G = a->hsl.hue;
		B = a->hsl.hue-1/3.0;

		if (R>1) R-=1;
		if (B<0) B+=1;

		if (6.0 * R < 1)
			b->rgb.red = p + (q - p) * 6.0 * R;
		else if (2.0 * R < 1)
			b->rgb.red = q;
		else if (3.0 * R < 2)
			b->rgb.red = p + (q - p) * ((2.0 / 3.0) - R) * 6.0;
		else
			b->rgb.red = p;

		if (6.0 * G < 1)
			b->rgb.green = p + (q - p) * 6.0 * G;
		else if (2.0 * G < 1)
			b->rgb.green = q;
		else if (3.0 * G < 2)
			b->rgb.green = p + (q - p) * ((2.0 / 3.0) - G) * 6.0;
		else
			b->rgb.green = p;

		if (6.0 * B < 1)
			b->rgb.blue = p + (q - p) * 6.0 * B;
		else if (2.0 * B < 1)
			b->rgb.blue = q;
		else if (3.0 * B < 2)
			b->rgb.blue = p + (q - p) * ((2.0 / 3.0) - B) * 6.0;
		else
			b->rgb.blue = p;


	}
}

void color_lab_to_lch(Color* a, Color* b) {
	float H;
	H = atan2(a->lab.b, a->lab.a);

	if (H > 0)
		H /= PI;
	else
		H = (2*PI - abs_float(H))/PI;

	b->lch.L = a->lab.L;
	b->lch.C = sqrt(a->lab.b * a->lab.b + a->lab.a * a->lab.a);
	b->lch.h = H;
}

void color_rgb_to_lch(Color* a, Color* b){
	Color c;
	color_rgb_to_lab(a, &c);
	color_lab_to_lch(&c, b);
}

void color_lab_to_xyz(Color* a, Color* b) {
	float x, y, z;

	float fy = (a->lab.L + 16) / 116;
	float fx = a->lab.a / 500 + fy;
	float fz = fy - a->lab.b / 200;

	float e=0.008856;
	float K=903.3;

	if (pow(fx, 3)>e){
		x=pow(fx, 3);
	}else{
		x=(116*fx-16)/K;
	}

	if (a->lab.L > K*e){
		y=pow((a->lab.L+16)/116, 3);
	}else{
		y=a->lab.L/K;
	}

	if (pow(fz, 3)>e){
		z=pow(fz, 3);
	}else{
		z=(116*fz-16)/K;
	}

	b->xyz.x = x * 95.047f;
	b->xyz.y = y * 100.000f;
	b->xyz.z = z * 108.883f;
}




