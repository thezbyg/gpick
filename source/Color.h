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

#ifndef COLOR_H_
#define COLOR_H_

#include "MathUtil.h"

/** \file Color.h
 * \brief Color structure and functions to convert colors from one colorspace to another
 */

/** \struct Color
 * \brief Color structure is an union of all available colorspaces
 */
typedef struct Color{
	union{
		struct{
			float red;               /**< Red component */
			float green;             /**< Green component */
			float blue;              /**< Blue component */
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
		struct{
			float L;
			float C;
			float h;
		}lch;
		struct{
			float c;
			float m;
			float y;
		}cmy;
		struct{
			float c;
			float m;
			float y;
			float k;
		}cmyk;
		struct{
			float m1;
			float m2;
			float m3;
			float m4;
		}m;
		float ma[4];
	};
}Color;

enum ReferenceIlluminant {
	REFERENCE_ILLUMINANT_A = 0,
	REFERENCE_ILLUMINANT_C = 1,
	REFERENCE_ILLUMINANT_D50 = 2,
	REFERENCE_ILLUMINANT_D55 = 3,
	REFERENCE_ILLUMINANT_D65 = 4,
	REFERENCE_ILLUMINANT_D75 = 5,
	REFERENCE_ILLUMINANT_F2 = 6,
	REFERENCE_ILLUMINANT_F7 = 7,
	REFERENCE_ILLUMINANT_F11 = 8,
};

enum ReferenceObserver {
	REFERENCE_OBSERVER_2 = 0,
	REFERENCE_OBSERVER_10 = 1,
};


/**
 * Initialize things needed for color conversion functions
 */
void color_init();

/**
 * Convert RGB colorspace to HSL colorspace
 * @param[in] a Source color in RGB colorspace
 * @param[out] b Destination color in HSL colorspace
 */
void color_rgb_to_hsl(const Color* a, Color* b);

/**
 * Convert HSL colorspace to RGB colorspace
 * @param[in] a Source color in HSL colorspace
 * @param[out] b Destination color in RGB colorspace
 */
void color_hsl_to_rgb(const Color* a, Color* b);

/**
 * Convert HSL colorspace to HSV colorspace
 * @param[in] a Source color in HSL colorspace
 * @param[out] b Destination color in HSV colorspace
 */
void color_hsl_to_hsv(const Color *a, Color *b);

/**
 * Convert HSV colorspace to HSL colorspace
 * @param[in] a Source color in HSV colorspace
 * @param[out] b Destination color in HSL colorspace
 */
void color_hsv_to_hsl(const Color *a, Color *b);

/**
 * Convert RGB colorspace to HSV colorspace
 * @param[in] a Source color in RGB colorspace
 * @param[out] b Destination color in HSV colorspace
 */
void color_rgb_to_hsv(const Color* a, Color* b);

/**
 * Convert HSV colorspace to RGB colorspace
 * @param[in] a Source color in HSV colorspace
 * @param[out] b Destination color in RGB colorspace
 */
void color_hsv_to_rgb(const Color* a, Color* b);

void color_rgb_to_xyz(const Color* a, Color* b, const matrix3x3* transformation);
void color_xyz_to_rgb(const Color* a, Color* b, const matrix3x3* transformation_inverted);

void color_xyz_to_lab(const Color* a, Color* b, const vector3* reference_white);
void color_lab_to_xyz(const Color* a, Color* b, const vector3* reference_white);

void color_rgb_to_lab(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation, const matrix3x3* adaptation_matrix);
void color_lab_to_rgb(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation_inverted, const matrix3x3* adaptation_matrix_inverted);

void color_rgb_to_lab_d50(const Color* a, Color* b);
void color_lab_to_rgb_d50(const Color* a, Color* b);

void color_lab_to_lch(const Color* a, Color* b);
void color_lch_to_lab(const Color* a, Color* b);

void color_rgb_to_lch(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation, const matrix3x3* adaptation_matrix);
void color_lch_to_rgb(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation_inverted, const matrix3x3* adaptation_matrix_inverted);

void color_rgb_to_lch_d50(const Color* a, Color* b);
void color_lch_to_rgb_d50(const Color* a, Color* b);

void color_rgb_to_cmy(const Color* a, Color* b);
void color_cmy_to_rgb(const Color* a, Color* b);

void color_cmy_to_cmyk(const Color* a, Color* b);
void color_cmyk_to_cmy(const Color* a, Color* b);

void color_rgb_to_cmyk(const Color* a, Color* b);
void color_cmyk_to_rgb(const Color* a, Color* b);


void color_rgb_normalize(Color* a);
bool color_is_rgb_out_of_gamut(const Color* a);

void color_rgb_get_linear(const Color* a, Color* b);
void color_linear_get_rgb(const Color* a, Color* b);

/**
 * Copy color
 * @param[in] a Source color in any colorspace
 * @param[out] b Destination color
 */
void color_copy(const Color* a, Color* b);

void color_add(Color* a, const Color* b);

void color_multiply(Color* a, float b);

void color_zero(Color* a);

Color* color_new();

void color_destroy(Color* a);

void color_set(Color* a, float value);

void color_get_contrasting(const Color* a, Color* b);

void color_get_working_space_matrix(float xr, float yr, float xg, float yg, float xb, float yb, const vector3* reference_white, matrix3x3* result);
void color_get_chromatic_adaptation_matrix(const vector3* source_reference_white, const vector3* destination_reference_white, matrix3x3* result);
void color_xyz_chromatic_adaptation(const Color* a, Color* result, const matrix3x3* adaptation );

const matrix3x3* color_get_sRGB_transformation_matrix();
const matrix3x3* color_get_inverted_sRGB_transformation_matrix();
const matrix3x3* color_get_d65_d50_adaptation_matrix();
const matrix3x3* color_get_d50_d65_adaptation_matrix();

const vector3* color_get_reference(ReferenceIlluminant illuminant, ReferenceObserver observer);

const ReferenceIlluminant color_get_illuminant(const char *illuminant);
const ReferenceObserver color_get_observer(const char *observer);

#endif /* COLOR_H_ */

