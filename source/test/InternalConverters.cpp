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

#include <boost/test/unit_test.hpp>
#include "Converter.h"
#include "Converters.h"
#include "InternalConverters.h"
#include "ColorObject.h"
#include "Common.h"
BOOST_AUTO_TEST_SUITE(internalConverters)
BOOST_AUTO_TEST_CASE(webHex) {
	Converter::Options options = {};
	Converters converters;
	addInternalConverters(converters, options);
	auto *converter = converters.byName("color_web_hex");
	BOOST_REQUIRE(converter != nullptr);
	ColorObject colorObject;
	float quality;
	const struct {
		const char *text;
		bool good;
		Color color;
	} colors[] = {
		{ "", false, { } },
		{ "#", false, { } },
		{ "#204", false, { } },
		{ "#20408", false, { } },
		{ "# 204080", false, { } },
		{ "#204080", true, { 32, 64, 128, 255 } },
		{ " #204080", true, { 32, 64, 128, 255 } },
		{ " #204080 ", true, { 32, 64, 128, 255 } },
		{ "##204080", true, { 32, 64, 128, 255 } },
	};
	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
		bool good = converter->deserialize(colors[i].text, colorObject, quality);
		BOOST_CHECK_MESSAGE(good == colors[i].good, "wrong result at index " << i);
		if (good == colors[i].good)
			BOOST_CHECK_MESSAGE(colorObject.getColor() == colors[i].color, "wrong color at index " << i << ", " << colorObject.getColor() << " != " << colors[i].color);
	}
}
BOOST_AUTO_TEST_CASE(webHexWithAlpha) {
	Converter::Options options = {};
	Converters converters;
	addInternalConverters(converters, options);
	auto *converter = converters.byName("color_web_hex_with_alpha");
	BOOST_REQUIRE(converter != nullptr);
	ColorObject colorObject;
	float quality;
	const struct {
		const char *text;
		bool good;
		Color color;
	} colors[] = {
		{ "", false, { } },
		{ "#", false, { } },
		{ "#204", false, { } },
		{ "#20408", false, { } },
		{ "# 20408010", false, { } },
		{ "#20408010", true, { 32, 64, 128, 16 } },
		{ " #20408010", true, { 32, 64, 128, 16 } },
		{ " #20408010 ", true, { 32, 64, 128, 16 } },
		{ "##20408010", true, { 32, 64, 128, 16 } },
	};
	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
		bool good = converter->deserialize(colors[i].text, colorObject, quality);
		BOOST_CHECK_MESSAGE(good == colors[i].good, "wrong result at index " << i);
		if (good == colors[i].good)
			BOOST_CHECK_MESSAGE(colorObject.getColor() == colors[i].color, "wrong color at index " << i << ", " << colorObject.getColor() << " != " << colors[i].color);
	}
}
BOOST_AUTO_TEST_CASE(cssRgb) {
	Converter::Options options = {};
	Converters converters;
	addInternalConverters(converters, options);
	auto *converter = converters.byName("color_css_rgb");
	BOOST_REQUIRE(converter != nullptr);
	ColorObject colorObject;
	float quality;
	const struct {
		const char *text;
		bool good;
		Color color;
	} colors[] = {
		{ "", false, { } },
		{ "rgb", false, { } },
		{ "rgb()", false, { } },
		{ "rgb(1, 1)", false, { } },
		{ "rgb(1, 1, 1", false, { } },
		{ "rgb(32, 64, 128)", true, { 32, 64, 128, 255 } },
		{ " rgb(32, 64, 128)", true, { 32, 64, 128, 255 } },
		{ "rgb(32, 64, 128) ", true, { 32, 64, 128, 255 } },
		{ "rgb(rgb(32, 64, 128)", true, { 32, 64, 128, 255 } },
		{ "rgb(12.5%, 25%, 50%)", true, { 0.125f, 0.25f, 0.5f, 1.0f } },
		{ "rgb(50%, rgb(32, 64, 128)", true, { 32, 64, 128, 255 } },
	};
	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
		bool good = converter->deserialize(colors[i].text, colorObject, quality);
		BOOST_CHECK_MESSAGE(good == colors[i].good, "wrong result at index " << i);
		if (good == colors[i].good)
			BOOST_CHECK_MESSAGE(colorObject.getColor() == colors[i].color, "wrong color at index " << i << ", " << colorObject.getColor() << " != " << colors[i].color);
	}
}
BOOST_AUTO_TEST_CASE(cssRgba) {
	Converter::Options options = {};
	Converters converters;
	addInternalConverters(converters, options);
	auto *converter = converters.byName("color_css_rgba");
	BOOST_REQUIRE(converter != nullptr);
	ColorObject colorObject;
	float quality;
	const struct {
		const char *text;
		bool good;
		Color color;
	} colors[] = {
		{ "", false, { } },
		{ "rgba", false, { } },
		{ "rgba()", false, { } },
		{ "rgba(1, 1)", false, { } },
		{ "rgba(1, 1, 1", false, { } },
		{ "rgba(32, 64, 128, 1)", true, { 32, 64, 128, 255 } },
		{ " rgba(32, 64, 128, 1)", true, { 32, 64, 128, 255 } },
		{ "rgba(32, 64, 128, 1) ", true, { 32, 64, 128, 255 } },
		{ "rgba(rgba(32, 64, 128, 1)", true, { 32, 64, 128, 255 } },
		{ "rgba(12.5%, 25%, 50%, 75%)", true, { 0.125f, 0.25f, 0.5f, 0.75f } },
		{ "rgba(50%, rgba(32, 64, 128, 1)", true, { 32, 64, 128, 255 } },
	};
	for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
		bool good = converter->deserialize(colors[i].text, colorObject, quality);
		BOOST_CHECK_MESSAGE(good == colors[i].good, "wrong result at index " << i);
		if (good == colors[i].good)
			BOOST_CHECK_MESSAGE(colorObject.getColor() == colors[i].color, "wrong color at index " << i << ", " << colorObject.getColor() << " != " << colors[i].color);
	}
}
BOOST_AUTO_TEST_SUITE_END()
