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

#include "InternalConverters.h"
#include "Converters.h"
#include "ColorObject.h"
#include "I18N.h"
#include "common/MatchPattern.h"
#include "common/Convert.h"
#include "math/Algorithms.h"
#include <cstddef>
#include <algorithm>
using namespace std::string_view_literals;
namespace {
using Options = Converter::Options;
using Serialize = Converter::Callback<Converter::Serialize>;
using Deserialize = Converter::Callback<Converter::Deserialize>;
static int toInteger(float value) {
	return std::max(std::min(static_cast<int>(value * 256), 255), 0);
}
static int toPercentage(float value) {
	return std::max(std::min(static_cast<int>(value * 101), 100), 0);
}
static int fromHex(char value) {
	if (value >= '0' && value <= '9')
		return value - '0';
	else if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	else if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	else
		return 0;
}
static float toQuality(size_t start, size_t end, size_t length) {
	return 1.0f - static_cast<float>(std::atan(start) / math::PI) - static_cast<float>(std::atan(length - end) / math::PI);
}
inline auto numberOrPercentage(std::string_view &value, bool &percentage) {
	using namespace common::ops;
	return sequence(save(number, value), optional(sequence(single('%'), save(percentage))));
}
std::string webHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[8];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "#%02X%02X%02X", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	} else {
		std::snprintf(result, sizeof(result), "#%02x%02x%02x", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	}
	return result;
}
bool webHexDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	using namespace common;
	using namespace common::ops;
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), findSingle('#'), save(save(count(hex, 6), matched), start, end)))
		return false;
	Color c;
	c.red = (fromHex(matched[0]) << 4 | fromHex(matched[1])) / 255.0f;
	c.green = (fromHex(matched[2]) << 4 | fromHex(matched[3])) / 255.0f;
	c.blue = (fromHex(matched[4]) << 4 | fromHex(matched[5])) / 255.0f;
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start - 1, end, std::string_view(value).length());
	return true;
}
std::string webHexWithAlphaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[10];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "#%02X%02X%02X%02X", toInteger(c.red), toInteger(c.green), toInteger(c.blue), toInteger(c.alpha));
	} else {
		std::snprintf(result, sizeof(result), "#%02x%02x%02x%02x", toInteger(c.red), toInteger(c.green), toInteger(c.blue), toInteger(c.alpha));
	}
	return result;
}
bool webHexWithAlphaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	using namespace common;
	using namespace common::ops;
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), findSingle('#'), save(save(count(hex, 8), matched), start, end)))
		return false;
	Color c;
	c.red = (fromHex(matched[0]) << 4 | fromHex(matched[1])) / 255.0f;
	c.green = (fromHex(matched[2]) << 4 | fromHex(matched[3])) / 255.0f;
	c.blue = (fromHex(matched[4]) << 4 | fromHex(matched[5])) / 255.0f;
	c.alpha = (fromHex(matched[6]) << 4 | fromHex(matched[7])) / 255.0f;
	colorObject.setColor(c);
	quality = toQuality(start - 1, end, std::string_view(value).length());
	return true;
}
std::string cssRgbSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[22];
	auto &c = colorObject.getColor();
	if (options.cssPercentages) {
		std::snprintf(result, sizeof(result), "rgb(%d%%, %d%%, %d%%)", toPercentage(c.red), toPercentage(c.green), toPercentage(c.blue));
	} else {
		std::snprintf(result, sizeof(result), "rgb(%d, %d, %d)", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	}
	return result;
}
bool cssRgbDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	using namespace common;
	using namespace common::ops;
	std::string_view red, green, blue;
	bool redPercentage = false, greenPercentage = false, bluePercentage = false;
	size_t start, end;
	if (!matchPattern(std::string_view(value), find("rgb("sv), save(sequence(maybeSpace, numberOrPercentage(red, redPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(green, greenPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(blue, bluePercentage), maybeSpace, single(')')), start, end)))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f) / (redPercentage ? 100.0f : 255.0f));
	c.green = math::clamp(convert<float>(green, 0.0f) / (greenPercentage ? 100.0f : 255.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f) / (bluePercentage ? 100.0f : 255.0f));
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start - 1, end, std::string_view(value).length());
	return true;
}
std::string cssRgbaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[30];
	auto &c = colorObject.getColor();
	if (options.cssAlphaPercentage) {
		if (options.cssPercentages) {
			std::snprintf(result, sizeof(result), "rgba(%d%%, %d%%, %d%%, %d%%)", toPercentage(c.red), toPercentage(c.green), toPercentage(c.blue), toPercentage(c.alpha));
		} else {
			std::snprintf(result, sizeof(result), "rgba(%d, %d, %d, %d%%)", toInteger(c.red), toInteger(c.green), toInteger(c.blue), toPercentage(c.alpha));
		}
	} else {
		if (options.cssPercentages) {
			std::snprintf(result, sizeof(result), "rgba(%d%%, %d%%, %d%%, %0.3f)", toPercentage(c.red), toPercentage(c.green), toPercentage(c.blue), c.alpha);
		} else {
			std::snprintf(result, sizeof(result), "rgba(%d, %d, %d, %0.3f)", toInteger(c.red), toInteger(c.green), toInteger(c.blue), c.alpha);
		}
	}
	return result;
}
bool cssRgbaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	using namespace common;
	using namespace common::ops;
	std::string_view red, green, blue, alpha;
	bool redPercentage = false, greenPercentage = false, bluePercentage = false, alphaPercentage = false;
	size_t start, end;
	if (!matchPattern(std::string_view(value), find("rgba("sv), save(sequence(maybeSpace, numberOrPercentage(red, redPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(green, greenPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(blue, bluePercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(alpha, alphaPercentage), maybeSpace, single(')')), start, end)))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f) / (redPercentage ? 100.0f : 255.0f));
	c.green = math::clamp(convert<float>(green, 0.0f) / (greenPercentage ? 100.0f : 255.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f) / (bluePercentage ? 100.0f : 255.0f));
	c.alpha = math::clamp(convert<float>(alpha, 0.0f) / (alphaPercentage ? 100.0f : 1.0f));
	colorObject.setColor(c);
	quality = toQuality(start - 1, end, std::string_view(value).length());
	return true;
}
}
void addInternalConverters(Converters &converters, Converter::Options &options) {
	converters.add("color_web_hex", _("Web: hex code"), Serialize(webHexSerialize, options), Deserialize(webHexDeserialize, options));
	converters.add("color_web_hex_with_alpha", _("Web: hex code with alpha"), Serialize(webHexWithAlphaSerialize, options), Deserialize(webHexWithAlphaDeserialize, options));
	converters.add("color_css_rgb", _("CSS: red green blue"), Serialize(cssRgbSerialize, options), Deserialize(cssRgbDeserialize, options));
	converters.add("color_css_rgba", _("CSS: red green blue alpha"), Serialize(cssRgbaSerialize, options), Deserialize(cssRgbaDeserialize, options));
}
