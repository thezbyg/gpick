/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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
#include <fstream>
#include <iostream>
#include <vector>
#include "parser/TextFile.h"
#include "Color.h"
using namespace text_file_parser;

struct Parser: public TextFile {
	std::istream *m_stream;
	std::vector<Color> m_colors;
	bool m_failed;
	Parser(std::istream *stream) {
		m_stream = stream;
		m_failed = false;
	}
	virtual ~Parser() {
	}
	virtual void outOfMemory() {
		m_failed = true;
	}
	virtual void syntaxError(size_t start_line, size_t start_column, size_t end_line, size_t end_colunn) {
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
	bool checkColor(size_t index, const Color &color) {
		return color_equal(&m_colors[index], &color);
	}
	void parse() {
		Configuration configuration;
		TextFile::parse(configuration);
	}
};
BOOST_AUTO_TEST_CASE(full_hex) {
	std::ifstream file("test/textImport01.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(short_hex) {
	std::ifstream file("test/textImport02.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(css_rgb) {
	std::ifstream file("test/textImport03.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(css_rgba) {
	std::ifstream file("test/textImport04.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	color.ma[3] = 0.5;
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(int_values) {
	std::ifstream file("test/textImport05.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(float_values_separated_by_comma) {
	std::ifstream file("test/textImport06.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(float_values_separated_by_space) {
	std::ifstream file("test/textImport10.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(single_line_c_comments) {
	std::ifstream file("test/textImport07.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(multi_line_c_comments) {
	std::ifstream file("test/textImport08.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(single_line_hash_comments) {
	std::ifstream file("test/textImport09.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(out_of_range_float_value) {
	std::ifstream file("test/textImport11.txt");
	BOOST_REQUIRE(file.is_open());
	Parser parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
