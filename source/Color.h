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

/** \file source/Color.h
 * \brief Color structure and functions to convert colors from one color space to another.
 */

/** \struct Color
 * \brief Color structure is an union of all available color spaces.
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
		}m;                        /**< General data access structure */
		float ma[4];               /**< General data access array */
	};
}Color;

/** \enum ReferenceIlluminant
 * \brief Reference illuminants.
 */
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

/** \enum ReferenceObserver
 * \brief Reference observers.
 */
enum ReferenceObserver {
	REFERENCE_OBSERVER_2 = 0,
	REFERENCE_OBSERVER_10 = 1,
};


/**
 * Initialize things needed for color conversion functions. Must be called before using any other functions.
 */
void color_init();

/**
 * Convert RGB color space to HSL color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in HSL color space.
 */
void color_rgb_to_hsl(const Color* a, Color* b);

/**
 * Convert HSL color space to RGB color space.
 * @param[in] a Source color in HSL color space.
 * @param[out] b Destination color in RGB color space.
 */
void color_hsl_to_rgb(const Color* a, Color* b);

/**
 * Convert HSL color space to HSV color space.
 * @param[in] a Source color in HSL color space.
 * @param[out] b Destination color in HSV color space.
 */
void color_hsl_to_hsv(const Color *a, Color *b);

/**
 * Convert HSV color space to HSL color space.
 * @param[in] a Source color in HSV color space.
 * @param[out] b Destination color in HSL color space.
 */
void color_hsv_to_hsl(const Color *a, Color *b);

/**
 * Convert RGB color space to HSV color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in HSV color space.
 */
void color_rgb_to_hsv(const Color* a, Color* b);

/**
 * Convert HSV color space to RGB color space.
 * @param[in] a Source color in HSV color space.
 * @param[out] b Destination color in RGB color space.
 */
void color_hsv_to_rgb(const Color* a, Color* b);

/**
 * Convert RGB color space to XYZ color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in XYZ color space.
 * @param[in] transformation Transformation matrix for RGB to XYZ conversion.
 */
void color_rgb_to_xyz(const Color* a, Color* b, const matrix3x3* transformation);

/**
 * Convert XYZ color space to RGB color space.
 * @param[in] a Source color in XYZ color space.
 * @param[out] b Destination color in RGB color space.
 * @param[in] transformation_inverted Transformation matrix for XYZ to RGB conversion.
 */
void color_xyz_to_rgb(const Color* a, Color* b, const matrix3x3* transformation_inverted);

/**
 * Convert XYZ color space to Lab color space.
 * @param[in] a Source color in XYZ color space.
 * @param[out] b Destination color in Lab color space.
 * @param[in] reference_white Reference white color values.
 */
void color_xyz_to_lab(const Color* a, Color* b, const vector3* reference_white);

/**
 * Convert Lab color space to XYZ color space.
 * @param[in] a Source color in Lab color space.
 * @param[out] b Destination color in XYZ color space.
 * @param[in] reference_white Reference white color values.
 */
void color_lab_to_xyz(const Color* a, Color* b, const vector3* reference_white);

/**
 * Convert RGB color space to Lab color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in Lab color space.
 * @param[in] reference_white Reference white color values.
 * @param[in] transformation Transformation matrix for RGB to XYZ conversion.
 * @param[in] adaptation_matrix XYZ chromatic adaptation matrix.
 */
void color_rgb_to_lab(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation, const matrix3x3* adaptation_matrix);

/**
 * Convert Lab color space to RGB color space.
 * @param[in] a Source color in Lab color space.
 * @param[out] b Destination color in RGB color space.
 * @param[in] reference_white Reference white color values.
 * @param[in] transformation_inverted Transformation matrix for XYZ to RGB conversion.
 * @param[in] adaptation_matrix_inverted Inverted XYZ chromatic adaptation matrix.
 */
void color_lab_to_rgb(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation_inverted, const matrix3x3* adaptation_matrix_inverted);

/**
 * Convert RGB color space to Lab color space with illuminant D50, observer 2, sRGB transformation matrix and D65-D50 adaptation matrix.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in Lab color space.
 */
void color_rgb_to_lab_d50(const Color* a, Color* b);

/**
 * Convert Lab color space to RGB color space with illuminant D50, observer 2, inverted sRGB transformation matrix and D50-D65 adaptation matrix.
 * @param[in] a Source color in Lab color space.
 * @param[out] b Destination color in RGB color space.
 */
void color_lab_to_rgb_d50(const Color* a, Color* b);

/**
 * Convert Lab color space to LCH color space.
 * @param[in] a Source color in Lab color space.
 * @param[out] b Destination color in LCH color space.
 */
void color_lab_to_lch(const Color* a, Color* b);

/**
 * Convert Lab color space to LCH color space.
 * @param[in] a Source color in Lab color space.
 * @param[out] b Destination color in LCH color space.
 */
void color_lch_to_lab(const Color* a, Color* b);

/**
 * Convert RGB color space to LCH color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in LCH color space.
 * @param[in] reference_white Reference white color values.
 * @param[in] transformation Transformation matrix for RGB to XYZ conversion.
 * @param[in] adaptation_matrix XYZ chromatic adaptation matrix.
 */
void color_rgb_to_lch(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation, const matrix3x3* adaptation_matrix);

/**
 * Convert LCH color space to RGB color space.
 * @param[in] a Source color in LCH color space.
 * @param[out] b Destination color in RGB color space.
 * @param[in] reference_white Reference white color values.
 * @param[in] transformation_inverted Transformation matrix for XYZ to RGB conversion.
 * @param[in] adaptation_matrix_inverted Inverted XYZ chromatic adaptation matrix.
 */
void color_lch_to_rgb(const Color* a, Color* b, const vector3* reference_white, const matrix3x3* transformation_inverted, const matrix3x3* adaptation_matrix_inverted);

/**
 * Convert RGB color space to LCH color space with illuminant D50, observer 2, sRGB transformation matrix and D65-D50 adaptation matrix.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in LCH color space.
 */
void color_rgb_to_lch_d50(const Color* a, Color* b);

/**
 * Convert LCH color space to RGB color space with illuminant D50, observer 2, inverted sRGB transformation matrix and D50-D65 adaptation matrix.
 * @param[in] a Source color in LCH color space.
 * @param[out] b Destination color in RGB color space.
 */
void color_lch_to_rgb_d50(const Color* a, Color* b);

/**
 * Convert RGB color space to CMY color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in CMY color space.
 */
void color_rgb_to_cmy(const Color* a, Color* b);

/**
 * Convert CMY color space to RGB color space.
 * @param[in] a Source color in CMY color space.
 * @param[out] b Destination color in RGB color space.
 */
void color_cmy_to_rgb(const Color* a, Color* b);

/**
 * Convert CMY color space to CMYK color space.
 * @param[in] a Source color in CMY color space.
 * @param[out] b Destination color in CMYK color space.
 */
void color_cmy_to_cmyk(const Color* a, Color* b);

/**
 * Convert CMYK color space to CMY color space.
 * @param[in] a Source color in CMYK color space.
 * @param[out] b Destination color in CMY color space.
 */
void color_cmyk_to_cmy(const Color* a, Color* b);

/**
 * Convert RGB color space to CMYK color space.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Destination color in CMYK color space.
 */
void color_rgb_to_cmyk(const Color* a, Color* b);

/**
 * Convert CMYK color space to RGB color space.
 * @param[in] a Source color in CMYK color space.
 * @param[out] b Destination color in RGB color space.
 */
void color_cmyk_to_rgb(const Color* a, Color* b);


/**
 * Normalize RGB color values.
 * @param[in,out] a Color in RGB color space.
 */
void color_rgb_normalize(Color* a);

/**
 * Check whenever the color contains invalid (out of RGB gamut) value.
 * @param[in] a Color in RGB color space.
 * @return True, when color is out of RGB gamut.
 */
bool color_is_rgb_out_of_gamut(const Color* a);

/**
 * Transform RGB color to linear RGB color.
 * @param[in] a Color in RGB color space.
 * @param[out] b Linear color in RGB color space.
 */
void color_rgb_get_linear(const Color* a, Color* b);

/**
 * Transform linear RGB color to RGB color.
 * @param[in] a Linear color in RGB color space.
 * @param[out] b Color in RGB color space.
 */
void color_linear_get_rgb(const Color* a, Color* b);

/**
 * Copy color.
 * @param[in] a Source color in any color space.
 * @param[out] b Destination color.
 */
void color_copy(const Color* a, Color* b);

/**
 * Add color values.
 * @param[in,out] a Source color in any color space.
 * @param[in] b Color values.
 */
void color_add(Color* a, const Color* b);

/**
 * Multiply color values by specified amount.
 * @param[in,out] a Source color in any color space.
 * @param[in] b Multiplier.
 */
void color_multiply(Color* a, float b);

/**
 * Set all color values to zero.
 * @param[in,out] a Color to be set.
 */
void color_zero(Color* a);

/**
 * Create new Color structure.
 * @return Color structure with unspecified values.
 */
Color* color_new();

/**
 * Free memory associated with Color structure.
 * @param[in] a Color to be freed.
 */
void color_destroy(Color* a);

/**
 * Set all color values to specified value.
 * @param[in,out] a Color to be set.
 * @param[in] value Value which is used.
 */
void color_set(Color* a, float value);

/**
 * Get either black or white color depending on which has more contrast with specified color.
 * @param[in] a Source color in RGB color space.
 * @param[out] b Color with most contrast in RGB color space.
 */
void color_get_contrasting(const Color* a, Color* b);

/**
 * Calculate working space matrix.
 * @param[in] xr Red primary in x channel.
 * @param[in] yr Red primary in y channel.
 * @param[in] xg Green primary in x channel.
 * @param[in] yg Green primary in y channel.
 * @param[in] xb Blue primary in x channel.
 * @param[in] yb Blue primary in y channel.
 * @param[in] reference_white Reference white vector.
 * @param[out] result Calculated working space matrix.
 */
void color_get_working_space_matrix(float xr, float yr, float xg, float yg, float xb, float yb, const vector3* reference_white, matrix3x3* result);

/**
 * Calculate chromatic adaptation matrix from source and destination reference white vectors.
 * @param[in] source_reference_white Source reference white vector.
 * @param[in] destination_reference_white Destination reference white vector.
 * @param[out] result Calculated chromatic adaptation matrix.
 */
void color_get_chromatic_adaptation_matrix(const vector3* source_reference_white, const vector3* destination_reference_white, matrix3x3* result);

/**
 * Apply chromatic adaptation matrix to the XYZ color.
 * @param[in] a Source color in XYZ color space.
 * @param[out] result Pointer to a Color structure where the result is stored in XYZ color space.
 * @param[in] adaptation Chromatic adaptation matrix.
 * @see color_get_chromatic_adaptation_matrix.
 */
void color_xyz_chromatic_adaptation(const Color* a, Color* result, const matrix3x3* adaptation);

/**
 * Get working space matrix for sRGB.
 * @return Constant reference to sRGB working space matrix.
 */
const matrix3x3* color_get_sRGB_transformation_matrix();

/**
 * Get inverted working space matrix for sRGB.
 * @return Constant reference to inverted sRGB working space matrix.
 */
const matrix3x3* color_get_inverted_sRGB_transformation_matrix();

/**
 * Get D65 to D50 chromatic adaptation matrix.
 * @return Constant reference to chromatic adaptation matrix.
 */
const matrix3x3* color_get_d65_d50_adaptation_matrix();

/**
 * Get D50 to D65 chromatic adaptation matrix.
 * @return Constant reference to chromatic adaptation matrix.
 */
const matrix3x3* color_get_d50_d65_adaptation_matrix();

/**
 * Get reference white vector for specified illuminant and observer.
 * @param[in] illuminant Illuminant.
 * @param[in] observer Observer.
 * @return Reference white vector.
 */
const vector3* color_get_reference(ReferenceIlluminant illuminant, ReferenceObserver observer);

/**
 * Get illuminant by name.
 * @param[in] illuminant Illuminant name.
 * @return Reference illuminant.
 */
const ReferenceIlluminant color_get_illuminant(const char *illuminant);

/**
 * Get observer by name.
 * @param[in] observer Observer name.
 * @return Reference observer.
 */
const ReferenceObserver color_get_observer(const char *observer);

#endif /* COLOR_H_ */

