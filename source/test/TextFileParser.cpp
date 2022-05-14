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
#include "parser/TextFile.h"
#include "Color.h"
#include "Common.h"
#include <iostream>
#include <vector>
#include <string_view>
using namespace text_file_parser;

namespace {
struct Buffer: public std::streambuf {
	using Base = std::streambuf;
	Buffer(std::string_view value) {
		auto *p = const_cast<char *>(value.data());
		Base::setg(p, p, p + value.length());
	}
};
struct Parser: public TextFile {
	std::istream *m_stream;
	std::vector<Color> m_colors;
	bool m_failed;
	Buffer m_buffer;
	Parser(std::istream *stream):
		m_stream(stream),
		m_failed(false),
		m_buffer("") {
	}
	Parser(std::string_view text):
		m_stream(nullptr),
		m_failed(false),
		m_buffer(text) {
		std::istream stream(&m_buffer);
		m_stream = &stream;
		parse();
	}
	virtual ~Parser() {
	}
	virtual void outOfMemory() {
		m_failed = true;
	}
	virtual void syntaxError(size_t startLine, size_t startColumn, size_t endLine, size_t endEolunn) {
		m_failed = true;
	}
	virtual size_t read(char *buffer, size_t length) {
		m_stream->read(buffer, length);
		size_t bytes = m_stream->gcount();
		if (bytes > 0) return bytes;
		if (m_stream->eof()) return 0;
		if (!m_stream->good()) {
			m_failed = true;
		}
		return 0;
	}
	virtual void addColor(const Color &color) {
		m_colors.push_back(color);
	}
	size_t count() {
		return m_colors.size();
	}
	const Color &operator[](size_t index) const {
		return m_colors[index];
	}
	void parse() {
		Configuration configuration;
		TextFile::parse(configuration);
	}
};
}
BOOST_AUTO_TEST_SUITE(textFileParser)
BOOST_AUTO_TEST_CASE(fullHex) {
	Parser parser("color: #aabbcc");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(fullHexWithAlpha) {
	Parser parser("color: #aabbccdd");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc, 0xdd };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(shortHex) {
	Parser parser("color: #abc");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(shortHexWithAlpha) {
	Parser parser("color: #abcd");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc, 0xdd };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(cssRgb) {
	Parser parser("color: rgb(170, 187, 204)\r\ncolor: rgb(170 187 204)\r\ncolor: rgb(66.6667% 73.3333% 80%)");
	BOOST_REQUIRE_EQUAL(parser.count(), 3);
	Color color = { 0xaa, 0xbb, 0xcc };
	for (size_t i = 0; i < 3; ++i) {
		BOOST_CHECK_MESSAGE(parser[i] == color, "color " << (i + 1) << " incorrect, " << parser[i] << " != " << color);
	}
}
BOOST_AUTO_TEST_CASE(cssRgba) {
	Parser parser("color: rgba(170, 187, 204, 0.5)\r\ncolor: rgba(170 187 204 / 0.5)\r\ncolor: rgba(170, 187, 204, 50%)\r\ncolor: rgba(170 187 204 / 50%)\r\ncolor: rgba(66.6667% 73.3333% 80% / 50%)");
	BOOST_REQUIRE_EQUAL(parser.count(), 5);
	Color color = { 0xaa, 0xbb, 0xcc };
	color[3] = 0.5f;
	for (size_t i = 0; i < 5; ++i) {
		BOOST_CHECK_MESSAGE(parser[i] == color, "color " << (i + 1) << " incorrect, " << parser[i] << " != " << color);
	}
}
BOOST_AUTO_TEST_CASE(cssHsl) {
	Parser parser("color: hsl(180, 75%, 50%)\r\ncolor: hsl(180 75% 50%)\r\ncolor: hsl(180deg 75% 50%)\r\ncolor: hsl(0.5turn 75% 50%)\r\ncolor: hsl(200grad 75% 50%)\r\ncolor: hsl(3.141592rad 75% 50%)\r\n");
	BOOST_REQUIRE_EQUAL(parser.count(), 6);
	Color color = { 0.125f, 0.875f, 0.875f, 1.0f };
	for (size_t i = 0; i < 6; ++i) {
		BOOST_CHECK_MESSAGE(parser[i] == color, "color " << (i + 1) << " incorrect, " << parser[i] << " != " << color);
	}
}
BOOST_AUTO_TEST_CASE(cssHsla) {
	Parser parser("color: hsla(180, 75%, 50%, 0.5)\r\ncolor: hsla(180 75% 50% / 0.5)\r\ncolor: hsla(180, 75%, 50%, 50%)\r\ncolor: hsla(180 75% 50% / 50%)\r\ncolor: hsla(180deg 75% 50% / 50%)\r\ncolor: hsla(0.5turn 75% 50% / 50%)\r\ncolor: hsla(200grad 75% 50% / 50%)\r\ncolor: hsla(3.141592rad 75% 50% / 50%)");
	BOOST_REQUIRE_EQUAL(parser.count(), 8);
	Color color = { 0.125f, 0.875f, 0.875f, 0.5f };
	for (size_t i = 0; i < 8; ++i) {
		BOOST_CHECK_MESSAGE(parser[i] == color, "color " << (i + 1) << " incorrect, " << parser[i] << " != " << color);
	}
}
BOOST_AUTO_TEST_CASE(intValues) {
	Parser parser("color: 170, 187, 204");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(intValuesWithAlpha) {
	Parser parser("color: 170, 187, 204, 221");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc, 0xdd };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(floatValuesSeparatedByComma) {
	Parser parser("color: 0.666667, 0.733333, 0.8");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(floatValuesSeparatedBySpace) {
	Parser parser("color: 0.666667 0.733333 0.8");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(singleLineCComments) {
	Parser parser("//color: #ccbbaa\r\ncolor: #aabbcc");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(multiLineCComments) {
	Parser parser("/*color: #ccbbaa\r\n*/\r\ncolor: #aabbcc");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(singleLineHashComments) {
	Parser parser("#color: #ccbbaa\r\ncolor: #aabbcc");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0xcc };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_CASE(outOfRangeFloatValue) {
	Parser parser("color: 0.666667 0.733333 1e1000");
	BOOST_REQUIRE_EQUAL(parser.count(), 1);
	Color color = { 0xaa, 0xbb, 0 };
	BOOST_CHECK_MESSAGE(parser[0] == color, parser[0] << " != " << color);
}
BOOST_AUTO_TEST_SUITE_END()
