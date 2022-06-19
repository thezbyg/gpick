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

#include "Channels.h"
#include "I18N.h"
#include <stdexcept>
static const ChannelDescription channelDescriptions[] = {
	{ "rgb_red", N_("Red"), ColorSpace::rgb, Channel::rgbRed, ChannelFlags::none, { 0 }, 0, 1 },
	{ "rgb_green", N_("Green"), ColorSpace::rgb, Channel::rgbGreen, ChannelFlags::none, { 1 }, 0, 1 },
	{ "rgb_blue", N_("Blue"), ColorSpace::rgb, Channel::rgbBlue, ChannelFlags::none, { 2 }, 0, 1 },
	{ "hsl_hue", N_("Hue"), ColorSpace::hsl, Channel::hslHue, ChannelFlags::wrap, { 0 }, 0, 1 },
	{ "hsl_saturation", N_("Saturation"), ColorSpace::hsl, Channel::hslSaturation, ChannelFlags::none, { 1 }, 0, 1 },
	{ "hsl_lightness", N_("Lightness"), ColorSpace::hsl, Channel::hslLightness, ChannelFlags::none, { 2 }, 0, 1 },
	{ "hsv_hue", N_("Hue"), ColorSpace::hsv, Channel::hsvHue, ChannelFlags::wrap, { 0 }, 0, 1 },
	{ "hsv_saturation", N_("Saturation"), ColorSpace::hsv, Channel::hsvSaturation, ChannelFlags::none, { 1 }, 0, 1 },
	{ "hsv_value", N_("Value"), ColorSpace::hsv, Channel::hsvValue, ChannelFlags::none, { 2 }, 0, 1 },
	{ "cmyk_cyan", N_("Cyan"), ColorSpace::cmyk, Channel::cmykCyan, ChannelFlags::none, { 0 }, 0, 1 },
	{ "cmyk_magenta", N_("Magenta"), ColorSpace::cmyk, Channel::cmykMagenta, ChannelFlags::none, { 1 }, 0, 1 },
	{ "cmyk_yellow", N_("Yellow"), ColorSpace::cmyk, Channel::cmykYellow, ChannelFlags::none, { 2 }, 0, 1 },
	{ "cmyk_key", N_("Key"), ColorSpace::cmyk, Channel::cmykKey, ChannelFlags::none, { 3 }, 0, 1 },
	{ "lab_lightness", N_("Lightness"), ColorSpace::lab, Channel::labLightness, ChannelFlags::none, { 0 }, 0, 100 },
	{ "lab_a", "a", ColorSpace::lab, Channel::labA, ChannelFlags::none, { 1 }, -145, 145 },
	{ "lab_b", "b", ColorSpace::lab, Channel::labB, ChannelFlags::none, { 2 }, -145, 145 },
	{ "lch_lightness", N_("Lightness"), ColorSpace::lch, Channel::lchLightness, ChannelFlags::none, { 0 }, 0, 100 },
	{ "lch_chroma", N_("Chroma"), ColorSpace::lch, Channel::lchChroma, ChannelFlags::none, { 1 }, 0, 100 },
	{ "lch_hue", N_("Hue"), ColorSpace::lch, Channel::lchHue, ChannelFlags::wrap, { 2 }, 0, 360 },
	{ "alpha", N_("Alpha"), ColorSpace::rgb, Channel::alpha, ChannelFlags::allColorSpaces, { 3 }, 0, 1 },
};
common::Span<const ChannelDescription> channels() {
	return common::Span(channelDescriptions, sizeof(channelDescriptions) / sizeof(channelDescriptions[0]));
}
const ChannelDescription &channel(Channel channel) {
	for (auto &description: channelDescriptions) {
		if (description.type == channel)
			return description;
	}
	throw std::invalid_argument("channel");
}
bool ChannelDescription::allColorSpaces() const {
	return (flags & ChannelFlags::allColorSpaces) == ChannelFlags::allColorSpaces;
}
bool ChannelDescription::wrap() const {
	return (flags & ChannelFlags::wrap) == ChannelFlags::wrap;
}
bool ChannelDescription::useConvertTo() const {
	return (flags & ChannelFlags::useConvertTo) == ChannelFlags::useConvertTo;
}
