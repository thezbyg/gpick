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
#include "Common.h"
#include "FileFormat.h"
#include "ColorList.h"
#include "ColorObject.h"
#include <string_view>
#include <sstream>
#include <fstream>
BOOST_AUTO_TEST_SUITE(fileFormat)
const struct ColorAndName {
	Color color;
	std::string_view name;
} savedColors[] = {
	{ { 0.0f, 0.0f, 0.0f, 1.0f }, "Black" },
	{ { 0.5f, 0.0f, 0.0f, 1.0f }, "Indian red" },
	{ { 0.5f, 0.5f, 0.5f, 1.0f }, "Medium grey" },
	{ { 1.0f, 0.0f, 0.0f, 1.0f }, "Fire engine red" },
	{ { 1.0f, 0.5f, 0.5f, 1.0f }, "Salmon pink" },
	{ { 1.0f, 0.5f, 0.0f, 1.0f }, "Pumpkin orange" },
	{ { 0.5f, 0.5f, 0.0f, 1.0f }, "Shit green" },
	{ { 1.0f, 1.0f, 0.0f, 1.0f }, "Yellow" },
	{ { 1.0f, 1.0f, 0.5f, 1.0f }, "Butter" },
	{ { 0.5f, 1.0f, 0.0f, 1.0f }, "Bright lime" },
	{ { 0.0f, 0.5f, 0.0f, 1.0f }, "Emerald green" },
	{ { 0.0f, 1.0f, 0.0f, 1.0f }, "Fluro green" },
	{ { 0.5f, 1.0f, 0.5f, 1.0f }, "Lightgreen" },
	{ { 0.0f, 1.0f, 0.5f, 0.5f }, "Minty green" },
	{ { 0.0f, 0.5f, 0.5f, 0.0f }, "" },
	{ { 0.0f, 1.0f, 1.0f, 0.5f }, "Cyan" },
	{ { 0.5f, 1.0f, 1.0f, 1.0f }, "Robin's egg" },
	{ { 0.0f, 0.5f, 1.0f, 1.0f }, "Clear blue" },
	{ { 0.0f, 0.0f, 0.5f, 1.0f }, "Royal" },
	{ { 0.0f, 0.0f, 1.0f, 1.0f }, "Primary blue" },
	{ { 0.5f, 0.5f, 1.0f, 1.0f }, "Periwinkle" },
	{ { 0.5f, 0.0f, 1.0f, 1.0f }, "Blue/purple" },
	{ { 0.5f, 0.0f, 0.5f, 1.0f }, "Darkish purple" },
	{ { 1.0f, 0.0f, 1.0f, 1.0f }, "Pink/purple" },
	{ { 1.0f, 0.5f, 1.0f, 1.0f }, "Purply pink" },
	{ { 1.0f, 0.0f, 0.5f, 1.0f }, "Strong pink" },
	{ { 1.0f, 1.0f, 1.0f, 1.0f }, "White" },
};
BOOST_AUTO_TEST_CASE(load_0_2_6) {
	ColorList colors;
	auto result = paletteFileLoad("test/palette-0.2.6.gpa", colors);
	BOOST_REQUIRE_MESSAGE(result, result.error());
	BOOST_REQUIRE_EQUAL(colors.size(), 27);
	std::vector<ColorAndName> loaded;
	loaded.reserve(27);
	for (auto *colorObject: colors) {
		loaded.emplace_back(ColorAndName { colorObject->getColor(), colorObject->getName() });
	}
	for (size_t i = 0; i < loaded.size(); ++i) {
		auto withoutAlpha = savedColors[i].color;
		withoutAlpha.alpha = 1.0f;
		BOOST_CHECK_MESSAGE(loaded[i].color == withoutAlpha, "loaded wrong color at index " << i << ", " << loaded[i].color << " != " << withoutAlpha);
		BOOST_CHECK_MESSAGE(loaded[i].name == savedColors[i].name, "loaded wrong name at index " << i << ", " << loaded[i].name << " != " << savedColors[i].name);
	}
}
BOOST_AUTO_TEST_CASE(save) {
	ColorList colors;
	for (size_t i = 0; i < sizeof(savedColors) / sizeof(savedColors[0]); ++i) {
		colors.add(ColorObject(std::string(savedColors[i].name), savedColors[i].color), false);
	}
	std::stringstream output(std::ios::out | std::ios::binary);
	auto result = paletteStreamSave(output, colors);
	BOOST_REQUIRE_MESSAGE(result, result.error());
	auto outputData = output.str();
	auto length = outputData.length();
	std::ifstream goodFile("test/palette-0.3.gpa", std::ios::binary);
	BOOST_REQUIRE(goodFile.is_open());
	goodFile.seekg(0, std::ios::end);
	size_t size = goodFile.tellg();
	goodFile.seekg(0, std::ios::beg);
	BOOST_REQUIRE_EQUAL(length, size);
	std::vector<char> fileData;
	fileData.resize(size);
	goodFile.read(fileData.data(), size);
	BOOST_CHECK(std::memcmp(outputData.data(), fileData.data(), size) == 0);
}
BOOST_AUTO_TEST_CASE(load) {
	ColorList colors;
	auto result = paletteFileLoad("test/palette-0.3.gpa", colors);
	BOOST_REQUIRE_MESSAGE(result, result.error());
	BOOST_REQUIRE_EQUAL(colors.size(), 27);
	std::vector<ColorAndName> loaded;
	loaded.reserve(27);
	for (auto *colorObject: colors) {
		loaded.emplace_back(ColorAndName { colorObject->getColor(), colorObject->getName() });
	}
	for (size_t i = 0; i < loaded.size(); ++i) {
		BOOST_CHECK_MESSAGE(loaded[i].color == savedColors[i].color, "loaded wrong color at index " << i << ", " << loaded[i].color << " != " << savedColors[i].color);
		BOOST_CHECK_MESSAGE(loaded[i].name == savedColors[i].name, "loaded wrong name at index " << i << ", " << loaded[i].name << " != " << savedColors[i].name);
	}
}
BOOST_AUTO_TEST_SUITE_END()
