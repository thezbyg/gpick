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
#include "I18N.h"
#include <boost/math/special_functions/round.hpp>
#include <iostream>
static const ColorSpaceDescription colorSpaceDescriptions[] = {
	{ "rgb", "RGB", ColorSpace::rgb, ColorSpaceFlags::none, &Color::linearRgb, &Color::nonLinearRgb, 4, {
		{ Channel::rgbRed, "rgb_red", N_("Red"), "R", 255, 0, 255, 0.01 },
		{ Channel::rgbGreen, "rgb_green", N_("Green"), "G", 255, 0, 255, 0.01 },
		{ Channel::rgbBlue, "rgb_blue", N_("Blue"), "B", 255, 0, 255, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "hsl", "HSL", ColorSpace::hsl, ColorSpaceFlags::none, &Color::rgbToHsl, &Color::hslToRgb, 4, {
		{ Channel::hslHue, "hsl_hue", N_("Hue"), "H", 360, 0, 360, 0.01 },
		{ Channel::hslSaturation, "hsl_saturation", N_("Saturation"), "S", 100, 0, 100, 0.01 },
		{ Channel::hslLightness, "hsl_lightness", N_("Lightness"), "L", 100, 0, 100, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "hsv", "HSV", ColorSpace::hsv, ColorSpaceFlags::none, &Color::rgbToHsv, &Color::hsvToRgb, 4, {
		{ Channel::hsvHue, "hsv_hue", N_("Hue"), "H", 360, 0, 360, 0.01 },
		{ Channel::hsvSaturation, "hsv_saturation", N_("Saturation"), "S", 100, 0, 100, 0.01 },
		{ Channel::hsvValue, "hsv_value", N_("Value"), "V", 100, 0, 100, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "cmyk", "CMYK", ColorSpace::cmyk, ColorSpaceFlags::externalAlpha, &Color::rgbToCmyk, &Color::cmykToRgb, 5, {
		{ Channel::cmykCyan, "cmyk_cyan", N_("Cyan"), "C", 255, 0, 255, 0.01 },
		{ Channel::cmykMagenta, "cmyk_magenta", N_("Magenta"), "M", 255, 0, 255, 0.01 },
		{ Channel::cmykYellow, "cmyk_yellow", N_("Yellow"), "Y", 255, 0, 255, 0.01 },
		{ Channel::cmykKey, "cmyk_key", N_("Key"), "K", 255, 0, 255, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "lab", "LAB", ColorSpace::lab, ColorSpaceFlags::none, &Color::rgbToLabD50, &Color::labToRgbD50, 4, {
		{ Channel::labLightness, "lab_lightness", N_("Lightness"), "L", 1, 0, 100, 0.01 },
		{ Channel::labA, "lab_a", "a", "a", 1, -145, 145, 0.01 },
		{ Channel::labB, "lab_b", "b", "b", 1, -145, 145, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "lch", "LCH", ColorSpace::lch, ColorSpaceFlags::none, &Color::rgbToLchD50, &Color::lchToRgbD50, 4, {
		{ Channel::lchLightness, "lch_lightness", N_("Lightness"), "L", 1, 0, 100, 0.01 },
		{ Channel::lchChroma, "lch_chroma", N_("Chroma"), "C", 1, 0, 150, 0.01 },
		{ Channel::lchHue, "lch_hue", N_("Hue"), "H", 1, 0, 360, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "oklab", "OKLAB", ColorSpace::oklab, ColorSpaceFlags::none, &Color::rgbToOklab, &Color::oklabToRgb, 4, {
		{ Channel::oklabLightness, "oklab_lightness", N_("Lightness"), "L", 1, 0, 1, 0.0001 },
		{ Channel::oklabA, "oklab_a", "a", "a", 1, -0.4, 0.4, 0.0001 },
		{ Channel::oklabB, "oklab_b", "b", "b", 1, -0.4, 0.4, 0.0001 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
	{ "oklch", "OKLCH", ColorSpace::oklch, ColorSpaceFlags::none, &Color::rgbToOklch, &Color::oklchToRgb, 4, {
		{ Channel::oklchLightness, "oklch_lightness", N_("Lightness"), "L", 1, 0, 1, 0.0001 },
		{ Channel::oklchChroma, "oklch_chroma", N_("Chroma"), "C", 1, 0, 0.37, 0.0001 },
		{ Channel::oklchHue, "oklch_hue", N_("Hue"), "H", 1, 0, 360, 0.01 },
		{ Channel::alpha, "alpha", N_("Alpha"), "A", 100, 0, 100, 0.01 },
	} },
};
static_assert(sizeof(colorSpaceDescriptions) / sizeof(colorSpaceDescriptions[0]) <= maxColorSpaces);
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
std::vector<std::string> toTexts(ColorSpace colorSpace, const Color &color, float alpha) {
	using namespace boost::math;
	std::vector<std::string> result;
	const auto &description = ::colorSpace(colorSpace);
	for (int i = 0; i < description.channelCount; ++i) {
		const auto &channel = description.channels[i];
		float value;
		if (channel.type == Channel::alpha) {
			if (description.externalAlpha()) {
				value = alpha;
			} else {
				value = color.alpha;
			}
		} else {
			value = color.data[i];
		}
		std::stringstream ss;
		if (channel.maxValue - channel.minValue > 1) {
			ss << round(value * channel.rawScale);
		} else {
			ss << std::fixed << std::setprecision(3) << round(value * channel.rawScale * 1000) / 1000;
		}
		result.emplace_back(std::move(ss).str());
	}
	return result;
}
