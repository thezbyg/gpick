/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#include "ColorSpaces.h"
#include "Color.h"
#include <stdexcept>
static const ColorSpaceDescription colorSpaceDescriptions[] = {
	{ "rgb", "RGB", ColorSpace::rgb, ColorSpaceFlags::none, &Color::linearRgb, &Color::nonLinearRgb },
	{ "hsl", "HSL", ColorSpace::hsl, ColorSpaceFlags::none, &Color::rgbToHsl, &Color::hslToRgb },
	{ "hsv", "HSV", ColorSpace::hsv, ColorSpaceFlags::none, &Color::rgbToHsv, &Color::hsvToRgb },
	{ "cmyk", "CMYK", ColorSpace::cmyk, ColorSpaceFlags::externalAlpha, &Color::rgbToCmyk, &Color::cmykToRgb },
	{ "lab", "LAB", ColorSpace::lab, ColorSpaceFlags::none, &Color::rgbToLabD50, &Color::labToRgbD50 },
	{ "lch", "LCH", ColorSpace::lch, ColorSpaceFlags::none, &Color::rgbToLchD50, &Color::lchToRgbD50 },
};
common::Span<const ColorSpaceDescription> colorSpaces() {
	return common::Span(colorSpaceDescriptions, sizeof(colorSpaceDescriptions) / sizeof(colorSpaceDescriptions[0]));
}
const ColorSpaceDescription &colorSpace(ColorSpace colorSpace) {
	for (auto &description: colorSpaceDescriptions) {
		if (description.type == colorSpace)
			return description;
	}
	throw std::invalid_argument("colorSpace");
}
bool ColorSpaceDescription::externalAlpha() const {
	return (flags & ColorSpaceFlags::externalAlpha) == ColorSpaceFlags::externalAlpha;
}
