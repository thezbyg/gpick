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
#include "version/Version.h"
#include <cstddef>
#include <algorithm>
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace common;
using namespace common::ops;
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
static int toDegrees(float value) {
	return std::max(std::min(static_cast<int>(value * 361), 360), 0);
}
static int toShortInteger(float value) {
	return std::max(std::min(static_cast<int>(value * 16), 15), 0);
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
	return sequence(save(percentage, false), save(number, value), optional(sequence(single('%'), save(percentage))));
}
inline auto percentage(std::string_view &value) {
	return sequence(save(number, value), single('%'));
}
const auto valueSeparator = oneOrMore(single({',', ';', '\t', ' '}));
static std::string webHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[8];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "#%02X%02X%02X", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	} else {
		std::snprintf(result, sizeof(result), "#%02x%02x%02x", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	}
	return result;
}
static bool webHexDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith('#', save(save(count(hex, 6), matched), start, end))))
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
static std::string webHexWithAlphaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[10];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "#%02X%02X%02X%02X", toInteger(c.red), toInteger(c.green), toInteger(c.blue), toInteger(c.alpha));
	} else {
		std::snprintf(result, sizeof(result), "#%02x%02x%02x%02x", toInteger(c.red), toInteger(c.green), toInteger(c.blue), toInteger(c.alpha));
	}
	return result;
}
static bool webHexWithAlphaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith('#', save(save(count(hex, 8), matched), start, end))))
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
static std::string webHexNoHashSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[7];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "%02X%02X%02X", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	} else {
		std::snprintf(result, sizeof(result), "%02x%02x%02x", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	}
	return result;
}
static bool webHexNoHashDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(save(count(hex, 6), matched), start, end))))
		return false;
	Color c;
	c.red = (fromHex(matched[0]) << 4 | fromHex(matched[1])) / 255.0f;
	c.green = (fromHex(matched[2]) << 4 | fromHex(matched[3])) / 255.0f;
	c.blue = (fromHex(matched[4]) << 4 | fromHex(matched[5])) / 255.0f;
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string webHexShortSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[5];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "#%X%X%X", toShortInteger(c.red), toShortInteger(c.green), toShortInteger(c.blue));
	} else {
		std::snprintf(result, sizeof(result), "#%x%x%x", toShortInteger(c.red), toShortInteger(c.green), toShortInteger(c.blue));
	}
	return result;
}
static bool webHexShortDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith('#', save(save(count(hex, 3), matched), start, end))))
		return false;
	Color c;
	c.red = fromHex(matched[0]) / 15.0f;
	c.green = fromHex(matched[1]) / 15.0f;
	c.blue = fromHex(matched[2]) / 15.0f;
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start - 1, end, std::string_view(value).length());
	return true;
}
static std::string webHexShortWithAlphaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[6];
	auto &c = colorObject.getColor();
	if (options.upperCaseHex) {
		std::snprintf(result, sizeof(result), "#%X%X%X%X", toShortInteger(c.red), toShortInteger(c.green), toShortInteger(c.blue), toShortInteger(c.alpha));
	} else {
		std::snprintf(result, sizeof(result), "#%x%x%x%x", toShortInteger(c.red), toShortInteger(c.green), toShortInteger(c.blue), toShortInteger(c.alpha));
	}
	return result;
}
static bool webHexShortWithAlphaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view matched;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith('#', save(save(count(hex, 4), matched), start, end))))
		return false;
	Color c;
	c.red = fromHex(matched[0]) / 15.0f;
	c.green = fromHex(matched[1]) / 15.0f;
	c.blue = fromHex(matched[2]) / 15.0f;
	c.alpha = fromHex(matched[3]) / 15.0f;
	colorObject.setColor(c);
	quality = toQuality(start - 1, end, std::string_view(value).length());
	return true;
}
static std::string cssRgbSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[22];
	auto &c = colorObject.getColor();
	if (options.cssPercentages) {
		std::snprintf(result, sizeof(result), "rgb(%d%%, %d%%, %d%%)", toPercentage(c.red), toPercentage(c.green), toPercentage(c.blue));
	} else {
		std::snprintf(result, sizeof(result), "rgb(%d, %d, %d)", toInteger(c.red), toInteger(c.green), toInteger(c.blue));
	}
	return result;
}
static bool cssRgbDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue;
	bool redPercentage = false, greenPercentage = false, bluePercentage = false;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith("rgb("sv, save(sequence(maybeSpace, numberOrPercentage(red, redPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(green, greenPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(blue, bluePercentage), maybeSpace, single(')')), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f) / (redPercentage ? 100.0f : 255.0f));
	c.green = math::clamp(convert<float>(green, 0.0f) / (greenPercentage ? 100.0f : 255.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f) / (bluePercentage ? 100.0f : 255.0f));
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start - 4, end, std::string_view(value).length());
	return true;
}
static std::string cssRgbaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
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
static bool cssRgbaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue, alpha;
	bool redPercentage = false, greenPercentage = false, bluePercentage = false, alphaPercentage = false;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith("rgba("sv, save(sequence(maybeSpace, numberOrPercentage(red, redPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(green, greenPercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(blue, bluePercentage), maybeSpace, single(','), maybeSpace, numberOrPercentage(alpha, alphaPercentage), maybeSpace, single(')')), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f) / (redPercentage ? 100.0f : 255.0f));
	c.green = math::clamp(convert<float>(green, 0.0f) / (greenPercentage ? 100.0f : 255.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f) / (bluePercentage ? 100.0f : 255.0f));
	c.alpha = math::clamp(convert<float>(alpha, 0.0f) / (alphaPercentage ? 100.0f : 1.0f));
	colorObject.setColor(c);
	quality = toQuality(start - 5, end, std::string_view(value).length());
	return true;
}
static std::string cssHslSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[21];
	auto c = colorObject.getColor().rgbToHsl();
	std::snprintf(result, sizeof(result), "hsl(%d, %d%%, %d%%)", toDegrees(c.hsl.hue), toPercentage(c.hsl.saturation), toPercentage(c.hsl.lightness));
	return result;
}
static bool cssHslDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view hue, saturation, lightness;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith("hsl("sv, save(sequence(maybeSpace, save(number, hue), maybeSpace, single(','), maybeSpace, percentage(saturation), maybeSpace, single(','), maybeSpace, percentage(lightness), maybeSpace, single(')')), start, end))))
		return false;
	Color c;
	c.hsl.hue = math::clamp(convert<float>(hue, 0.0f) / 360.0f);
	c.hsl.saturation = math::clamp(convert<float>(saturation, 0.0f) / 100.0f);
	c.hsl.lightness = math::clamp(convert<float>(lightness, 0.0f) / 100.0f);
	c.alpha = 1.0f;
	colorObject.setColor(c.hslToRgb());
	quality = toQuality(start - 4, end, std::string_view(value).length());
	return true;
}
static std::string cssHslaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[29];
	auto c = colorObject.getColor().rgbToHsl();
	std::snprintf(result, sizeof(result), "hsla(%d, %d%%, %d%%, %0.3f)", toDegrees(c.hsl.hue), toPercentage(c.hsl.saturation), toPercentage(c.hsl.lightness), c.alpha);
	return result;
}
static bool cssHslaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view hue, saturation, lightness, alpha;
	bool alphaPercentage;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith("hsla("sv, save(sequence(maybeSpace, save(number, hue), maybeSpace, single(','), maybeSpace, percentage(saturation), maybeSpace, single(','), maybeSpace, percentage(lightness), maybeSpace, single(','), maybeSpace, numberOrPercentage(alpha, alphaPercentage), maybeSpace, single(')')), start, end))))
		return false;
	Color c;
	c.hsl.hue = math::clamp(convert<float>(hue, 0.0f) / 360.0f);
	c.hsl.saturation = math::clamp(convert<float>(saturation, 0.0f) / 100.0f);
	c.hsl.lightness = math::clamp(convert<float>(lightness, 0.0f) / 100.0f);
	c.alpha = math::clamp(convert<float>(alpha, 0.0f) / (alphaPercentage ? 100.0f : 1.0f));
	colorObject.setColor(c.hslToRgb());
	quality = toQuality(start - 5, end, std::string_view(value).length());
	return true;
}
static std::string csvRgbSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[18];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f,%0.3f,%0.3f", c.red, c.green, c.blue);
	return result;
}
static bool csvRgbDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), maybeSpace, single(','), maybeSpace, save(number, green), maybeSpace, single(','), maybeSpace, save(number, blue)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string csvRgbTabSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[18];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f\t%0.3f\t%0.3f", c.red, c.green, c.blue);
	return result;
}
static bool csvRgbTabDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), maybeSpaceStrict, single('\t'), maybeSpaceStrict, save(number, green), maybeSpaceStrict, single('\t'), maybeSpaceStrict, save(number, blue)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string csvRgbSemicolonSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[18];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f;%0.3f;%0.3f", c.red, c.green, c.blue);
	return result;
}
static bool csvRgbSemicolonDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), maybeSpace, single(';'), maybeSpace, save(number, green), maybeSpace, single(';'), maybeSpace, save(number, blue)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string csvRgbaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[24];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f,%0.3f,%0.3f,%0.3f", c.red, c.green, c.blue, c.alpha);
	return result;
}
static bool csvRgbaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue, alpha;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), maybeSpace, single(','), maybeSpace, save(number, green), maybeSpace, single(','), maybeSpace, save(number, blue), maybeSpace, single(','), maybeSpace, save(number, alpha)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = math::clamp(convert<float>(alpha, 0.0f));
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string csvRgbaTabSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[24];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f\t%0.3f\t%0.3f\t%0.3f", c.red, c.green, c.blue, c.alpha);
	return result;
}
static bool csvRgbaTabDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue, alpha;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), maybeSpaceStrict, single('\t'), maybeSpaceStrict, save(number, green), maybeSpaceStrict, single('\t'), maybeSpaceStrict, save(number, blue), maybeSpaceStrict, single('\t'), maybeSpaceStrict, save(number, alpha)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = math::clamp(convert<float>(alpha, 0.0f));
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string csvRgbaSemicolonSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[24];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f;%0.3f;%0.3f;%0.3f", c.red, c.green, c.blue, c.alpha);
	return result;
}
static bool csvRgbaSemicolonDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue, alpha;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), maybeSpace, single(';'), maybeSpace, save(number, green), maybeSpace, single(';'), maybeSpace, save(number, blue), maybeSpace, single(';'), maybeSpace, save(number, alpha)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = math::clamp(convert<float>(alpha, 0.0f));
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string valueRgbSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[20];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f, %0.3f, %0.3f", c.red, c.green, c.blue);
	return result;
}
static bool valueRgbDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), valueSeparator, save(number, green), valueSeparator, save(number, blue)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = 1.0f;
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string valueRgbaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	char result[27];
	auto &c = colorObject.getColor();
	std::snprintf(result, sizeof(result), "%0.3f, %0.3f, %0.3f, %0.3f", c.red, c.green, c.blue, c.alpha);
	return result;
}
static bool valueRgbaDeserialize(const char *value, ColorObject &colorObject, float &quality, const Options &options) {
	std::string_view red, green, blue, alpha;
	size_t start, end;
	if (!matchPattern(std::string_view(value), startWith(save(sequence(save(number, red), valueSeparator, save(number, green), valueSeparator, save(number, blue), valueSeparator, save(number, alpha)), start, end))))
		return false;
	Color c;
	c.red = math::clamp(convert<float>(red, 0.0f));
	c.green = math::clamp(convert<float>(green, 0.0f));
	c.blue = math::clamp(convert<float>(blue, 0.0f));
	c.alpha = math::clamp(convert<float>(alpha, 0.0f));
	colorObject.setColor(c);
	quality = toQuality(start, end, std::string_view(value).length());
	return true;
}
static std::string cssColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBackgroundColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "background-color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBorderColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "border-color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBorderTopColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "border-top-color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBorderRightColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "border-right-color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBorderBottomColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "border-bottom-color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBorderLeftColorHexSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	return "border-left-color: " + webHexSerialize(colorObject, position, options);
}
static std::string cssBlockSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	std::string result;
	result.reserve(100);
	if (position.first()) {
		result += "/**\n * Generated by Gpick ";
		result += version::versionFull;
		result += '\n';
	}
	result += " * ";
	result += colorObject.getName();
	result += ": ";
	result += webHexSerialize(colorObject, position, options);
	result += ", ";
	result += cssRgbSerialize(colorObject, position, options);
	result += ", ";
	result += cssHslSerialize(colorObject, position, options);
	if (position.last()) {
		result += "\n */";
	}
	return result;
}
static std::string cssBlockWithAlphaSerialize(const ColorObject &colorObject, const ConverterSerializePosition &position, const Options &options) {
	std::string result;
	result.reserve(100);
	if (position.first()) {
		result += "/**\n * Generated by Gpick ";
		result += version::versionFull;
		result += '\n';
	}
	result += " * ";
	result += colorObject.getName();
	result += ": ";
	result += webHexWithAlphaSerialize(colorObject, position, options);
	result += ", ";
	result += cssRgbaSerialize(colorObject, position, options);
	result += ", ";
	result += cssHslaSerialize(colorObject, position, options);
	if (position.last()) {
		result += "\n */";
	}
	return result;
}
}
void addInternalConverters(Converters &converters, Converter::Options &options) {
	converters.add("color_web_hex", _("Web: hex code"), Serialize(webHexSerialize, options), Deserialize(webHexDeserialize, options));
	converters.add("color_web_hex_with_alpha", _("Web: hex code with alpha"), Serialize(webHexWithAlphaSerialize, options), Deserialize(webHexWithAlphaDeserialize, options));
	converters.add("color_web_hex_no_hash", _("Web: hex code (no hash symbol)"), Serialize(webHexNoHashSerialize, options), Deserialize(webHexNoHashDeserialize, options));
	converters.add("color_web_hex_short", _("Web: short hex code"), Serialize(webHexShortSerialize, options), Deserialize(webHexShortDeserialize, options));
	converters.add("color_web_hex_short_with_alpha", _("Web: short hex code with alpha"), Serialize(webHexShortWithAlphaSerialize, options), Deserialize(webHexShortWithAlphaDeserialize, options));
	converters.add("color_css_rgb", _("CSS: red green blue"), Serialize(cssRgbSerialize, options), Deserialize(cssRgbDeserialize, options));
	converters.add("color_css_rgba", _("CSS: red green blue alpha"), Serialize(cssRgbaSerialize, options), Deserialize(cssRgbaDeserialize, options));
	converters.add("color_css_hsl", _("CSS: hue saturation lightness"), Serialize(cssHslSerialize, options), Deserialize(cssHslDeserialize, options));
	converters.add("color_css_hsla", _("CSS: hue saturation lightness alpha"), Serialize(cssHslaSerialize, options), Deserialize(cssHslaDeserialize, options));
	converters.add("css_color_hex", "CSS(color)", Serialize(cssColorHexSerialize, options), Deserialize());
	converters.add("css_background_color_hex", "CSS(background-color)", Serialize(cssBackgroundColorHexSerialize, options), Deserialize());
	converters.add("css_border_color_hex", "CSS(border-color)", Serialize(cssBorderColorHexSerialize, options), Deserialize());
	converters.add("css_border_top_color_hex", "CSS(border-top-color)", Serialize(cssBorderTopColorHexSerialize, options), Deserialize());
	converters.add("css_border_right_color_hex", "CSS(border-right-color)", Serialize(cssBorderRightColorHexSerialize, options), Deserialize());
	converters.add("css_border_bottom_color_hex", "CSS(border-bottom-color)", Serialize(cssBorderBottomColorHexSerialize, options), Deserialize());
	converters.add("css_border_left_hex", "CSS(border-left-color)", Serialize(cssBorderLeftColorHexSerialize, options), Deserialize());
	converters.add("color_css_block", _("CSS block"), Serialize(cssBlockSerialize, options), Deserialize());
	converters.add("color_css_block_with_alpha", _("CSS block with alpha"), Serialize(cssBlockWithAlphaSerialize, options), Deserialize());
	converters.add("csv_rgb", "CSV RGB", Serialize(csvRgbSerialize, options), Deserialize(csvRgbDeserialize, options));
	converters.add("csv_rgb_tab", "CSV RGB "s + _("(tab separator)"), Serialize(csvRgbTabSerialize, options), Deserialize(csvRgbTabDeserialize, options));
	converters.add("csv_rgb_semicolon", "CSV RGB "s + _("(semicolon separator)"), Serialize(csvRgbSemicolonSerialize, options), Deserialize(csvRgbSemicolonDeserialize, options));
	converters.add("csv_rgba", "CSV RGBA", Serialize(csvRgbaSerialize, options), Deserialize(csvRgbaDeserialize, options));
	converters.add("csv_rgba_tab", "CSV RGBA "s + _("(tab separator)"), Serialize(csvRgbaTabSerialize, options), Deserialize(csvRgbaTabDeserialize, options));
	converters.add("csv_rgba_semicolon", "CSV RGBA "s + _("(semicolon separator)"), Serialize(csvRgbaSemicolonSerialize, options), Deserialize(csvRgbaSemicolonDeserialize, options));
	converters.add("value_rgb", _("RGB values"), Serialize(valueRgbSerialize, options), Deserialize(valueRgbDeserialize, options));
	converters.add("value_rgba", _("RGBA values"), Serialize(valueRgbaSerialize, options), Deserialize(valueRgbaDeserialize, options));
}
