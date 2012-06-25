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

#ifndef COLOR_WHEEL_TYPE_H_
#define COLOR_WHEEL_TYPE_H_

#include "Color.h"
#include <stdint.h>

/** \file source/ColorWheelType.h
 * \brief Color wheel type description structure and functions
 */

/** \struct ColorWheelType
 * \brief ColorWheelType structure contains color wheel type name and conversion functions
 */
typedef struct ColorWheelType{
	const char *name;                                   /**< Name of a color wheel */
/**
 * Callback used to convert color wheel specific hue into the color in a HSL color space
 * @param[in] hue Color wheel specific hue value
 * @param[out] hsl Result as a color in HSL color space
 */
	void (*hue_to_hsl)(double hue, Color* hsl);

/**
 * Callback used to convert HSL color space hue into color wheel specific hue
 * @param[in] rgbhue HSL color space hue value
 * @param[out] hue Color wheel specific hue value
 */
	void (*rgbhue_to_hue)(double rgbhue, double *hue);
}ColorWheelType;

/**
 * Get available color wheel types
 * @return Constant array of available color wheel types
 */
const ColorWheelType* color_wheel_types_get();

/**
 * Get the number of available color wheel types
 * @return Number of available color wheel types
 */
const uint32_t color_wheel_types_get_n();

#endif /* COLOR_WHEEL_TYPE_H_ */

