#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include "parser/TextFile.h"
#include "Color.h"
using namespace std;

struct TextFile: public text_file_parser::TextFile
{
	public:
		istream *m_stream;
		vector<Color> m_colors;
		bool m_failed;
		TextFile(istream *stream)
		{
			m_stream = stream;
			m_failed = false;
		}
		virtual ~TextFile()
		{
		}
		virtual void outOfMemory()
		{
			m_failed = true;
		}
		virtual void syntaxError(size_t start_line, size_t start_column, size_t end_line, size_t end_colunn)
		{
			m_failed = true;
		}
		virtual size_t read(char *buffer, size_t length)
		{
			m_stream->read(buffer, length);
			size_t bytes = m_stream->gcount();
			if (bytes > 0) return bytes;
			if (m_stream->eof()) return 0;
			if (!m_stream->good()){
				m_failed = true;
			}
			return 0;
		}
		virtual void addColor(const Color &color)
		{
			m_colors.push_back(color);
		}
		size_t count()
		{
			return m_colors.size();
		}
		bool checkColor(size_t index, const Color &color)
		{
			return color_equal(&m_colors[index], &color);
		}
		void parse()
		{
			text_file_parser::Configuration configuration;
			text_file_parser::TextFile::parse(configuration);
		}
};

BOOST_AUTO_TEST_CASE(full_hex)
{
	ifstream file("test/textImport01.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(short_hex)
{
	ifstream file("test/textImport02.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(css_rgb)
{
	ifstream file("test/textImport03.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(css_rgba)
{
	ifstream file("test/textImport04.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	color.ma[3] = 0.5;
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(int_values)
{
	ifstream file("test/textImport05.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(float_values)
{
	ifstream file("test/textImport06.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(single_line_c_comments)
{
	ifstream file("test/textImport07.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(multi_line_c_comments)
{
	ifstream file("test/textImport08.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
BOOST_AUTO_TEST_CASE(single_line_hash_comments)
{
	ifstream file("test/textImport09.txt");
	BOOST_REQUIRE(file.is_open());
	TextFile parser(&file);
	parser.parse();
	BOOST_CHECK(parser.count() == 1);
	Color color;
	color_set(&color, 0xaa, 0xbb, 0xcc);
	BOOST_CHECK(parser.checkColor(0, color));
	file.close();
}
