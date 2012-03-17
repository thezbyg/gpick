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

#include "Color.h"
#include <math.h>
#include "MathUtil.h"

#include <iostream>
using namespace std;

// Constant used for lab->xyz transform. Should be calculated with maximum accuracy possible.
#define EPSILON (216.0 / 24389.0)

static vector3 references[][2] = {
	{{{{109.850, 100.000,  35.585}}}, {{{111.144, 100.000,  35.200}}}},
	{{{{ 98.074, 100.000, 118.232}}}, {{{ 97.285, 100.000, 116.145}}}},
	{{{{ 96.422, 100.000,  82.521}}}, {{{ 96.720, 100.000,  81.427}}}},
	{{{{ 95.682, 100.000,  92.149}}}, {{{ 95.799, 100.000,  90.926}}}},
	{{{{ 95.047, 100.000, 108.883}}}, {{{ 94.811, 100.000, 107.304}}}},
	{{{{ 94.972, 100.000, 122.638}}}, {{{ 94.416, 100.000, 120.641}}}},
	{{{{ 99.187, 100.000,  67.395}}}, {{{103.280, 100.000,  69.026}}}},
	{{{{ 95.044, 100.000, 108.755}}}, {{{ 95.792, 100.000, 107.687}}}},
	{{{{100.966, 100.000,  64.370}}}, {{{103.866, 100.000,  65.627}}}},
};

static matrix3x3 sRGB_transformation;
static matrix3x3 sRGB_transformation_inverted;

static matrix3x3 d65_d50_adaptation_matrix;
static matrix3x3 d50_d65_adaptation_matrix;


void color_init()
{
	// constants used below are sRGB working space red, green and blue primaries for D65 reference white
	color_get_working_space_matrix(0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), &sRGB_transformation);
	matrix3x3_inverse(&sRGB_transformation, &sRGB_transformation_inverted);

	color_get_chromatic_adaptation_matrix(color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), &d65_d50_adaptation_matrix);
	color_get_chromatic_adaptation_matrix(color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), &d50_d65_adaptation_matrix);

}


void color_rgb_to_hsv(const Color* a, Color* b)
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


void color_hsv_to_rgb(const Color* a, Color* b)
{
	float h,v, f, x, y, z;
	int i;

	v = a->hsv.value;

	if (a->hsv.saturation == 0.0f) {
		b->rgb.red = b->rgb.green = b->rgb.blue = a->hsv.value;
	} else {

		h = (a->hsv.hue - floor(a->hsv.hue)) * 6.0f;

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

void color_rgb_to_xyz(const Color* a, Color* b, const matrix3x3* transformation)
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

	vector3 rgb;
	rgb.x = R;
	rgb.y = G;
	rgb.z = B;

	vector3_multiply_matrix3x3(&rgb, transformation, &rgb);

	b->xyz.x = rgb.x;
	b->xyz.y = rgb.y;
	b->xyz.z = rgb.z;
}

void color_xyz_to_rgb(const Color* a, Color* b, const matrix3x3* transformation_inverted)
{
	vector3 rgb;
	float R,G,B;

	vector3_multiply_matrix3x3((vector3*)a, transformation_inverted, &rgb);
	R=rgb.x;
	G=rgb.y;
	B=rgb.z;

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



void color_rgb_to_lab(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation, const matrix3x3* adaptation_matrix)
{
	Color c;
	color_rgb_to_xyz(a, &c, transformation);
	color_xyz_chromatic_adaptation(&c, &c, adaptation_matrix);
	color_xyz_to_lab(&c, b, reference_white);
}

void color_lab_to_rgb(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation_inverted, const matrix3x3* adaptation_matrix_inverted)
{
	Color c;
	color_lab_to_xyz(a, &c, reference_white);
	color_xyz_chromatic_adaptation(&c, &c, adaptation_matrix_inverted);
	color_xyz_to_rgb(&c, b, transformation_inverted);
}

void color_copy(const Color* a, Color* b)
{
	b->m.m1 = a->m.m1;
	b->m.m2 = a->m.m2;
	b->m.m3 = a->m.m3;
	b->m.m4 = a->m.m4;
}

void color_add(Color* a, const Color* b)
{
	a->m.m1 += b->m.m1;
	a->m.m2 += b->m.m2;
	a->m.m3 += b->m.m3;
	a->m.m4 += b->m.m4;
}

void color_multiply(Color* a, float b)
{
	a->m.m1 *= b;
	a->m.m2 *= b;
	a->m.m3 *= b;
	a->m.m4 *= b;
}

void color_zero(Color* a)
{
	a->m.m1 = 0;
	a->m.m2 = 0;
	a->m.m3 = 0;
	a->m.m4 = 0;
}

void
color_get_contrasting(const Color* a, Color* b)
{
	Color t;
	color_rgb_to_lab(a, &t, color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), &sRGB_transformation, &d65_d50_adaptation_matrix);

	if (t.lab.L > 50){
		t.hsv.value=0;
	}else{
		t.hsv.value=1;
	}

	t.hsv.saturation=0;
	t.hsv.hue=0;

	color_hsv_to_rgb(&t, b);
}

void color_set(Color* a, float value)
{
	a->rgb.red = a->rgb.green = a->rgb.blue = value;
}

Color* color_new()
{
	return new Color;
}

void color_destroy(Color *a)
{
	delete a;
}

void color_rgb_to_hsl(const Color* a, Color* b)
{
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


void color_hsl_to_rgb(const Color* a, Color* b) {
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

void color_lab_to_lch(const Color* a, Color* b) {
	double H;
	if (a->lab.a == 0 && a->lab.b == 0){
		H = 0;
	}else{
		H = atan2(a->lab.b, a->lab.a);
	}

	H *= 180.0 / PI;

	if (H < 0) H += 360;
	if (H >= 360) H -= 360;

	b->lch.L = a->lab.L;
	b->lch.C = sqrt(a->lab.a * a->lab.a + a->lab.b * a->lab.b);
	b->lch.h = H;
}

void color_lch_to_lab(const Color* a, Color* b) {
	b->lab.L = a->lch.L;
	b->lab.a = a->lch.C * cos(a->lch.h * PI / 180.0);
	b->lab.b = a->lch.C * sin(a->lch.h * PI / 180.0);
}

void color_rgb_to_lch(const Color* a, Color* b){
	Color c;
	color_rgb_to_lab(a, &c, color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), &sRGB_transformation, &d65_d50_adaptation_matrix);
	color_lab_to_lch(&c, b);
}

void color_lch_to_rgb(const Color* a, Color* b){
	Color c;
	color_lch_to_lab(a, &c);
	color_lab_to_rgb(&c, b, color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), &sRGB_transformation_inverted, &d50_d65_adaptation_matrix);
}

void color_rgb_to_lab_d50(const Color* a, Color* b){
	color_rgb_to_lab(a, b, color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), &sRGB_transformation, &d65_d50_adaptation_matrix);
}

void color_lab_to_rgb_d50(const Color* a, Color* b){
	color_lab_to_rgb(a, b, color_get_reference(REFERENCE_ILLUMINANT_D50, REFERENCE_OBSERVER_2), &sRGB_transformation_inverted, &d50_d65_adaptation_matrix);
}

#define Kk (24389.0 / 27.0)

void color_xyz_to_lab(const Color* a, Color* b, const vector3* reference_white){
	float X,Y,Z;

	X = a->xyz.x / reference_white->x; //95.047f;
	Y = a->xyz.y / reference_white->y; //100.000f;
	Z = a->xyz.z / reference_white->z; //108.883f;

	if (X>EPSILON){
		X=pow(X,1.0/3.0);
	}else{
		X=(Kk*X+16.0)/116.0;
	}
	if (Y>EPSILON){
		Y=pow(Y,1.0/3.0);
	}else{
		Y=(Kk*Y+16.0)/116.0;
	}
	if (Z>EPSILON){
		Z=pow(Z,1.0/3.0);
	}else{
		Z=(Kk*Z+16.0)/116.0;
	}

	b->lab.L=(116*Y)-16;
	b->lab.a=500*(X-Y);
	b->lab.b=200*(Y-Z);

}

void color_lab_to_xyz(const Color* a, Color* b, const vector3* reference_white) {
	float x, y, z;

	float fy = (a->lab.L + 16) / 116;
	float fx = a->lab.a / 500 + fy;
	float fz = fy - a->lab.b / 200;

	const double K=(24389.0 / 27.0);

	if (pow(fx, 3)>EPSILON){
		x=pow(fx, 3);
	}else{
		x=(116*fx-16)/K;
	}

	if (a->lab.L > K*EPSILON){
		y=pow((a->lab.L+16)/116, 3);
	}else{
		y=a->lab.L/K;
	}

	if (pow(fz, 3)>EPSILON){
		z=pow(fz, 3);
	}else{
		z=(116*fz-16)/K;
	}

	b->xyz.x = x * reference_white->x; //95.047f;
	b->xyz.y = y * reference_white->y; //100.000f;
	b->xyz.z = z * reference_white->z; //108.883f;
}

void color_get_working_space_matrix(float xr, float yr, float xg, float yg, float xb, float yb, const vector3* reference_white, matrix3x3* result){
	float Xr,Yr,Zr;
	float Xg,Yg,Zg;
	float Xb,Yb,Zb;

	Xr=xr/yr;
	Yr=1;
	Zr=(1-xr-yr)/yr;

	Xg=xg/yg;
	Yg=1;
	Zg=(1-xg-yg)/yg;

	Xb=xb/yb;
	Yb=1;
	Zb=(1-xb-yb)/yb;

	vector3 v;
	v.x=reference_white->x;
	v.y=reference_white->y;
	v.z=reference_white->z;

	matrix3x3 m_inv;
	m_inv.m[0][0]=Xr; m_inv.m[1][0]=Yr; m_inv.m[2][0]=Zr;
	m_inv.m[0][1]=Xg; m_inv.m[1][1]=Yg; m_inv.m[2][1]=Zg;
	m_inv.m[0][2]=Xb; m_inv.m[1][2]=Yb; m_inv.m[2][2]=Zb;

	matrix3x3_inverse(&m_inv, &m_inv);

	vector3_multiply_matrix3x3(&v, &m_inv, &v);

	result->m[0][0]=Xr*v.x;	result->m[1][0]=Yr*v.x;	result->m[2][0]=Zr*v.x;
	result->m[0][1]=Xg*v.y;	result->m[1][1]=Yg*v.y;	result->m[2][1]=Zg*v.y;
	result->m[0][2]=Xb*v.z;	result->m[1][2]=Yb*v.z;	result->m[2][2]=Zb*v.z;
}

void color_get_chromatic_adaptation_matrix(const vector3* source_reference_white, const vector3* destination_reference_white, matrix3x3* result){

	matrix3x3 Ma;
	//Bradford matrix
	Ma.m[0][0]= 0.8951; Ma.m[1][0]=-0.7502; Ma.m[2][0]= 0.0389;
	Ma.m[0][1]= 0.2664; Ma.m[1][1]= 1.7135; Ma.m[2][1]=-0.0685;
	Ma.m[0][2]=-0.1614; Ma.m[1][2]= 0.0367; Ma.m[2][2]= 1.0296;

	matrix3x3 Ma_inv;
	//Bradford inverted matrix
	Ma_inv.m[0][0]= 0.986993; Ma_inv.m[1][0]= 0.432305; Ma_inv.m[2][0]=-0.008529;
	Ma_inv.m[0][1]=-0.147054; Ma_inv.m[1][1]= 0.518360; Ma_inv.m[2][1]= 0.040043;
	Ma_inv.m[0][2]= 0.159963; Ma_inv.m[1][2]= 0.049291; Ma_inv.m[2][2]= 0.968487;

	vector3 Vs, Vd;
	vector3_multiply_matrix3x3(source_reference_white, &Ma, &Vs);
	vector3_multiply_matrix3x3(destination_reference_white, &Ma, &Vd);

	matrix3x3 M;
	matrix3x3_identity(&M);
	M.m[0][0]=Vd.x / Vs.x;
	M.m[1][1]=Vd.y / Vs.y;
	M.m[2][2]=Vd.z / Vs.z;

	matrix3x3_multiply(&Ma, &M, &M);
	matrix3x3_multiply(&M, &Ma_inv, result);
}

void color_xyz_chromatic_adaptation(const Color* a, Color* result, const matrix3x3* adaptation)
{
	vector3 x;
	x.x=a->xyz.x;
	x.y=a->xyz.y;
	x.z=a->xyz.z;
	vector3_multiply_matrix3x3(&x, adaptation, &x);
	result->xyz.x=x.x;
	result->xyz.y=x.y;
	result->xyz.z=x.z;
}

void color_rgb_to_cmy(const Color* a, Color* b)
{
	b->cmy.c = 1 - a->rgb.red;
	b->cmy.m = 1 - a->rgb.green;
	b->cmy.y = 1 - a->rgb.blue;
}

void color_cmy_to_rgb(const Color* a, Color* b)
{
	b->rgb.red = 1 - a->cmy.c;
	b->rgb.green = 1 - a->cmy.m;
	b->rgb.blue = 1 - a->cmy.y;
}

void color_cmy_to_cmyk(const Color* a, Color* b)
{
	float k = 1;

	if (a->cmy.c < k) k = a->cmy.c;
	if (a->cmy.m < k) k = a->cmy.m;
	if (a->cmy.y < k) k = a->cmy.y;

	if (k == 1){
		b->cmyk.c = b->cmyk.m = b->cmyk.y = 0;
	}else{
		b->cmyk.c = (a->cmy.c - k) / (1 - k);
		b->cmyk.m = (a->cmy.m - k) / (1 - k);
		b->cmyk.y = (a->cmy.y - k) / (1 - k);
	}
	b->cmyk.k = k;
}

void color_cmyk_to_cmy(const Color* a, Color* b)
{
	b->cmy.c = (a->cmyk.c * (1 - a->cmyk.k) + a->cmyk.k);
	b->cmy.m = (a->cmyk.m * (1 - a->cmyk.k) + a->cmyk.k);
	b->cmy.y = (a->cmyk.y * (1 - a->cmyk.k) + a->cmyk.k);
}

void color_rgb_to_cmyk(const Color* a, Color* b)
{
	Color c;
	color_rgb_to_cmy(a, &c);
	color_cmy_to_cmyk(&c, b);
}

void color_cmyk_to_rgb(const Color* a, Color* b)
{
	Color c;
	color_cmyk_to_cmy(a, &c);
	color_cmy_to_rgb(&c, b);
}


void color_rgb_normalize(Color* a)
{
	a->rgb.red = clamp_float(a->rgb.red, 0, 1);
	a->rgb.green = clamp_float(a->rgb.green, 0, 1);
	a->rgb.blue = clamp_float(a->rgb.blue, 0, 1);
}

void color_hsl_to_hsv(const Color *a, Color *b)
{
	float l = a->hsl.lightness * 2.0;
	float s = a->hsl.saturation * ((l <= 1.0) ? (l) : (2.0 - l));

	b->hsv.hue = a->hsl.hue;
	if (l + s == 0){
		b->hsv.value = 0;
		b->hsv.saturation = 1;
	}else{
		b->hsv.value = (l + s) / 2.0;
		b->hsv.saturation = (2.0 * s) / (l + s);
	}
}

void color_hsv_to_hsl(const Color *a, Color *b)
{
	float l = (2.0 - a->hsv.saturation) * a->hsv.value;
	float s = (a->hsv.saturation * a->hsv.value) / ((l <= 1.0) ? (l) : (2 - l));
    if (l == 0) s = 0;

	b->hsl.hue = a->hsv.hue;
	b->hsl.saturation = s;
	b->hsl.lightness = l / 2.0;
}

void color_rgb_get_linear(const Color* a, Color* b)
{
	b->rgb.red = pow(a->rgb.red, 1.0 / 2.1);
	b->rgb.green = pow(a->rgb.green, 1.0 / 2.0);
	b->rgb.blue = pow(a->rgb.blue, 1.0 / 2.1);
}

void color_linear_get_rgb(const Color* a, Color* b)
{
	b->rgb.red = pow(a->rgb.red, 2.1);
	b->rgb.green = pow(a->rgb.green, 2.0);
	b->rgb.blue = pow(a->rgb.blue, 2.1);
}

const matrix3x3* color_get_sRGB_transformation_matrix()
{
	return &sRGB_transformation;
}

const matrix3x3* color_get_inverted_sRGB_transformation_matrix()
{
	return &sRGB_transformation_inverted;
}

const matrix3x3* color_get_d65_d50_adaptation_matrix()
{
	return &d65_d50_adaptation_matrix;
}

const matrix3x3* color_get_d50_d65_adaptation_matrix()
{
	return &d50_d65_adaptation_matrix;
}

const vector3* color_get_reference(ReferenceIlluminant illuminant, ReferenceObserver observer)
{
	return &references[illuminant][observer];
}

const ReferenceIlluminant color_get_illuminant(const char *illuminant)
{
	const struct{
		const char *label;
		ReferenceIlluminant illuminant;
	}illuminants[] = {
		{"A", REFERENCE_ILLUMINANT_A},
		{"C", REFERENCE_ILLUMINANT_C},
		{"D50", REFERENCE_ILLUMINANT_D50},
		{"D55", REFERENCE_ILLUMINANT_D55},
		{"D65", REFERENCE_ILLUMINANT_D65},
		{"D75", REFERENCE_ILLUMINANT_D75},
		{"F2", REFERENCE_ILLUMINANT_F2},
		{"F7", REFERENCE_ILLUMINANT_F7},
		{"F11", REFERENCE_ILLUMINANT_F11},
		{0, REFERENCE_ILLUMINANT_D50},
	};
	for (int i = 0; illuminants[i].label; i++){
		if (string(illuminants[i].label).compare(illuminant) == 0){
			return illuminants[i].illuminant;
		}
	}
	return REFERENCE_ILLUMINANT_D50;
};

const ReferenceObserver color_get_observer(const char *observer)
{
	const struct{
		const char *label;
		ReferenceObserver observer;
	}observers[] = {
		{"2", REFERENCE_OBSERVER_2},
		{"10", REFERENCE_OBSERVER_10},
		{0, REFERENCE_OBSERVER_2},
	};
	for (int i = 0; observers[i].label; i++){
		if (string(observers[i].label).compare(observer) == 0){
			return observers[i].observer;
		}
	}
	return REFERENCE_OBSERVER_2;
}

