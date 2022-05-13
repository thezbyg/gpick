/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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
#include "Common.h"
#include "Color.h"
#include <iostream>
namespace {
const Color testColor = { 0.5f, 0.25f, 0.1f, 1.0f };
struct Initialize {
	Initialize() {
		Color::initialize();
	}
};
}
BOOST_FIXTURE_TEST_SUITE(color, Initialize)
BOOST_AUTO_TEST_CASE(basic) {
	Color empty;
	BOOST_CHECK_EQUAL(empty[0], 0.0f);
	BOOST_CHECK_EQUAL(empty[1], 0.0f);
	BOOST_CHECK_EQUAL(empty[2], 0.0f);
	BOOST_CHECK_EQUAL(empty[3], 0.0f);
	Color singleValue(1.0f);
	BOOST_CHECK_EQUAL(singleValue[0], 1.0f);
	BOOST_CHECK_EQUAL(singleValue[1], 1.0f);
	BOOST_CHECK_EQUAL(singleValue[2], 1.0f);
	BOOST_CHECK_EQUAL(singleValue[3], 1.0f);
	Color singleValueInt(255);
	BOOST_CHECK_EQUAL(singleValueInt[0], 1.0f);
	BOOST_CHECK_EQUAL(singleValueInt[1], 1.0f);
	BOOST_CHECK_EQUAL(singleValueInt[2], 1.0f);
	BOOST_CHECK_EQUAL(singleValueInt[3], 1.0f);
	Color assignment;
	assignment = testColor;
	BOOST_CHECK_EQUAL(assignment[0], testColor[0]);
	BOOST_CHECK_EQUAL(assignment[1], testColor[1]);
	BOOST_CHECK_EQUAL(assignment[2], testColor[2]);
	BOOST_CHECK_EQUAL(assignment[3], testColor[3]);
}
BOOST_AUTO_TEST_CASE(hsv) {
	Color result = testColor.rgbToHsv().hsvToRgb();
	BOOST_CHECK_EQUAL(result, testColor);
	Color inputs[] = {
		{ 0.0f, 0.5f, 0.5f },
		{ 1 / 3.0f, 0.5f, 0.5f },
		{ 2 / 3.0f, 0.5f, 0.5f },
		{ 1.0f, 0.5f, 0.5f },
		{ 0.5f, 0.0f, 0.5f },
		{ 0.5f, 0.25f, 0.5f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.75f, 0.5f },
		{ 0.5f, 1.0f, 0.5f },
		{ 0.5f, 0.5f, 0.0f },
		{ 0.5f, 0.5f, 0.25f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.5f, 0.75f },
		{ 0.5f, 0.5f, 1.0f },
	};
	Color outputs[] = {
		{ 0.5f, 0.25f, 0.25f },
		{ 0.25f, 0.5f, 0.25f },
		{ 0.25f, 0.25f, 0.5f },
		{ 0.5f, 0.25f, 0.25f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.375f, 0.5f, 0.5f },
		{ 0.25f, 0.5f, 0.5f },
		{ 0.125f, 0.5f, 0.5f },
		{ 0.0f, 0.5f, 0.5f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.125f, 0.25f, 0.25f },
		{ 0.25f, 0.5f, 0.5f },
		{ 0.375f, 0.75f, 0.75f },
		{ 0.5f, 1.0f, 1.0f },
	};
	static_assert(sizeof(inputs) == sizeof(outputs));
	for (size_t i = 0; i < sizeof(inputs) / sizeof(Color); i++) {
		BOOST_CHECK_EQUAL(inputs[i].hsvToRgb(), outputs[i]);
		if (inputs[i].hsv.hue < 1.0f && inputs[i].hsv.saturation > 0 && inputs[i].hsv.value > 0)
			BOOST_CHECK_EQUAL(outputs[i].rgbToHsv(), inputs[i]);
	}
}
BOOST_AUTO_TEST_CASE(hsl) {
	Color result = testColor.rgbToHsl().hslToRgb();
	BOOST_CHECK_EQUAL(result, testColor);
	Color inputs[] = {
		{ 0.0f, 0.5f, 0.5f },
		{ 1 / 3.0f, 0.5f, 0.5f },
		{ 2 / 3.0f, 0.5f, 0.5f },
		{ 1.0f, 0.5f, 0.5f },
		{ 0.5f, 0.0f, 0.5f },
		{ 0.5f, 0.25f, 0.5f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.75f, 0.5f },
		{ 0.5f, 1.0f, 0.5f },
		{ 0.5f, 0.5f, 0.0f },
		{ 0.5f, 0.5f, 0.25f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.5f, 0.5f, 0.75f },
		{ 0.5f, 0.5f, 1.0f },
	};
	Color outputs[] = {
		{ 0.75f, 0.25f, 0.25f },
		{ 0.25f, 0.75f, 0.25f },
		{ 0.25f, 0.25f, 0.75f },
		{ 0.75f, 0.25f, 0.25f },
		{ 0.5f, 0.5f, 0.5f },
		{ 0.375f, 0.625f, 0.625f },
		{ 0.25f, 0.75f, 0.75f },
		{ 0.125f, 0.875f, 0.875f },
		{ 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.125f, 0.375f, 0.375f },
		{ 0.25f, 0.75f, 0.75f },
		{ 0.625f, 0.875f, 0.875f },
		{ 1.0f, 1.0f, 1.0f },
	};
	static_assert(sizeof(inputs) == sizeof(outputs));
	for (size_t i = 0; i < sizeof(inputs) / sizeof(Color); i++) {
		BOOST_CHECK_EQUAL(inputs[i].hslToRgb(), outputs[i]);
		if (inputs[i].hsv.hue < 1.0f && inputs[i].hsl.saturation > 0 && inputs[i].hsl.lightness > 0 && inputs[i].hsl.lightness < 1.0f)
			BOOST_CHECK_EQUAL(outputs[i].rgbToHsl(), inputs[i]);
	}
}
BOOST_AUTO_TEST_CASE(cmy) {
	Color result = testColor.rgbToCmy().cmyToRgb();
	BOOST_CHECK_EQUAL(result, testColor);
}
BOOST_AUTO_TEST_CASE(cmyk) {
	Color result = testColor.rgbToCmyk().cmykToRgb();
	BOOST_CHECK_EQUAL(result, testColor);
}
BOOST_AUTO_TEST_CASE(xyz) {
	Color result = testColor.rgbToXyz(Color::sRGBMatrix).xyzToRgb(Color::sRGBInvertedMatrix);
	BOOST_CHECK_EQUAL(result, testColor);
}
BOOST_AUTO_TEST_CASE(lab) {
	Color result = testColor.rgbToLabD50().labToRgbD50();
	BOOST_CHECK_EQUAL(result, testColor);
}
BOOST_AUTO_TEST_CASE(lch) {
	Color result = testColor.rgbToLchD50().lchToRgbD50();
	BOOST_CHECK_EQUAL(result, testColor);
}
BOOST_AUTO_TEST_CASE(sRGBMatrix) {
	auto result = Color(Color::sRGBInvertedMatrix * (Color::sRGBMatrix * testColor.rgbVector<double>()));
	BOOST_CHECK_EQUAL(result, testColor);
}
BOOST_AUTO_TEST_SUITE_END()
