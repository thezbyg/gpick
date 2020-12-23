/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#include "ImportExport.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "FileFormat.h"
#include "Converters.h"
#include "Converter.h"
#include "I18N.h"
#include "StringUtils.h"
#include "HtmlUtils.h"
#include "GlobalState.h"
#include "dynv/Map.h"
#include "version/Version.h"
#include "parser/TextFile.h"
#include <glib.h>
#include <fstream>
#include <string>
#include <sstream>
#include <boost/math/special_functions/round.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/endian/conversion.hpp>
using namespace std;

static bool getOrderedColors(ColorList *color_list, vector<ColorObject*> &ordered)
{
	color_list_get_positions(color_list);
	size_t max_index = 0;
	bool index_set = false;
	for (auto color: color_list->colors){
		if (color->isPositionSet()){
			if (!index_set){
				max_index = color->getPosition();
				index_set = true;
			}else if (color->getPosition() > max_index){
				max_index = color->getPosition();
			}
		}
	}
	if (!index_set){
		return false;
	}else{
		ordered.resize(max_index + 1);
		for (auto color: color_list->colors){
			if (color->isPositionSet()){
				ordered[color->getPosition()] = color;
			}
		}
		return true;
	}
}
ImportExport::ImportExport(ColorList *color_list, const char* filename, GlobalState *gs):
	m_color_list(color_list),
	m_converter(nullptr),
	m_converters(nullptr),
	m_filename(filename),
	m_item_size(ItemSize::medium),
	m_background(Background::none),
	m_gs(gs),
	m_include_color_names(true),
	m_last_error(Error::none)
{
}
void ImportExport::fixFileExtension(const char *selected_filter)
{
	using namespace boost::filesystem;
	if (selected_filter && selected_filter[0] == '*'){
		string extension = path(m_filename).extension().string();
		bool append = false;
		if (extension.length() == 0){
			append = true;
		}else{
			if (getFileTypeByExtension(extension.c_str()) == FileType::unknown)
				append = true;
		}
		if (append){
			size_t length = m_filename.length();
			m_filename += &selected_filter[1];
			size_t additional_extension = m_filename.find_first_of(',', length);
			if (additional_extension != string::npos){
				m_filename = m_filename.substr(0, additional_extension);
			}
		}
	}
}
void ImportExport::setConverter(Converter *converter)
{
	m_converter = converter;
}
void ImportExport::setConverters(Converters *converters)
{
	m_converters = converters;
}
void ImportExport::setItemSize(ItemSize item_size)
{
	m_item_size = item_size;
}
void ImportExport::setItemSize(const char *item_size)
{
	string v(item_size);
	if (v == "small")
		m_item_size = ItemSize::small;
	else if (v == "medium")
		m_item_size = ItemSize::medium;
	else if (v == "big")
		m_item_size = ItemSize::big;
	else if (v == "controllable")
		m_item_size = ItemSize::controllable;
	else
		m_item_size = ItemSize::medium;
}
void ImportExport::setBackground(Background background)
{
	m_background = background;
}
void ImportExport::setBackground(const char *background)
{
	string v(background);
	if (v == "none")
		m_background = Background::none;
	else if (v == "white")
		m_background = Background::white;
	else if (v == "gray")
		m_background = Background::gray;
	else if (v == "black")
		m_background = Background::black;
	else if (v == "first_color")
		m_background = Background::first_color;
	else if (v == "last_color")
		m_background = Background::last_color;
	else if (v == "controllable")
		m_background = Background::controllable;
	else
		m_background = Background::none;
}
void ImportExport::setIncludeColorNames(bool include_color_names)
{
	m_include_color_names = include_color_names;
}
static void gplColor(ColorObject* color_object, ostream &stream)
{
	using boost::math::iround;
	Color color = color_object->getColor();
	stream
		<< iround(color.rgb.red * 255) << "\t"
		<< iround(color.rgb.green * 255) << "\t"
		<< iround(color.rgb.blue * 255) << "\t" << color_object->getName() << endl;
}
bool ImportExport::exportGPL()
{
	ofstream f(m_filename, ios::out | ios::trunc);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	boost::filesystem::path path(m_filename);
	f << "GIMP Palette" << endl;
	f << "Name: " << path.filename().string() << endl;
	f << "Columns: 1" << endl;
	f << "#" << endl;
	vector<ColorObject*> ordered;
	getOrderedColors(m_color_list, ordered);
	for (auto color: ordered){
		gplColor(color, f);
		if (!f.good()){
			f.close();
			m_last_error = Error::file_write_error;
			return false;
		}
	}
	f.close();
	return true;
}
bool ImportExport::importGPL()
{
	ifstream f(m_filename, ios::in);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	string line;
	getline(f, line);
	if (f.good() && line != "GIMP Palette"){
		f.close();
		m_last_error = Error::file_read_error;
		return false;
	}
	do{
		getline(f, line);
	}while (f.good() && ((line.size() < 1) || (((line[0] > '9') || (line[0] < '0')) && line[0] != ' ')));
	int r, g, b;
	Color c;
	ColorObject* color_object;
	string strip_chars = " \t";
	for(;;){
		if (!f.good()) break;
		stripLeadingTrailingChars(line, strip_chars);
		if (line.length() > 0 && line[0] == '#'){ // skip comment lines
			getline(f, line);
			continue;
		}
		stringstream ss(line);
		ss >> r >> g >> b;
		getline(ss, line);
		if (!f.good()) line = "";
		c.rgb.red = r / 255.0;
		c.rgb.green = g / 255.0;
		c.rgb.blue = b / 255.0;
		color_object = color_list_new_color_object(m_color_list, &c);
		stripLeadingTrailingChars(line, strip_chars);
		color_object->setName(line);
		color_list_add_color_object(m_color_list, color_object, true);
		color_object->release();
		getline(f, line);
	}
	if (!f.eof()) {
		f.close();
		m_last_error = Error::file_read_error;
		return false;
	}
	f.close();
	return true;
}
bool ImportExport::importGPA() {
	return paletteFileLoad(m_filename.c_str(), m_color_list);
}
bool ImportExport::exportGPA() {
	return paletteFileSave(m_filename.c_str(), m_color_list);
}
bool ImportExport::exportTXT()
{
	ofstream f(m_filename.c_str(), ios::out | ios::trunc);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	vector<ColorObject*> ordered;
	getOrderedColors(m_color_list, ordered);
	ConverterSerializePosition position(ordered.size());
	for (auto color: ordered){
		string line = m_converter->serialize(color, position);
		if (m_include_color_names) {
			f << line << " " << color->getName() << endl;
		} else {
			f << line << endl;
		}
		position.incrementIndex();
		if (position.index() + 1 == position.count()){
			position.last(true);
		}
		if (position.first())
			position.first(false);
		if (!f.good()){
			f.close();
			m_last_error = Error::file_write_error;
			return false;
		}
	}
	f.close();
	return true;
}
bool ImportExport::importTXT()
{
	ifstream f(m_filename.c_str(), ios::in);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	ColorObject *color_object;
	Color dummy_color;
	multimap<float, ColorObject*, greater<float>> valid_converters;
	string line;
	string strip_chars = " \t";
	bool imported = false;
	for(;;){
		getline(f, line);
		stripLeadingTrailingChars(line, strip_chars);
		if (!line.empty()){
			for (auto &converter: m_converters->allPaste()){
				if (!converter->hasDeserialize())
					continue;
				color_object = color_list_new_color_object(m_color_list, &dummy_color);
				float quality;
				if (converter->deserialize(line.c_str(), color_object, quality)){
					if (quality > 0){
						valid_converters.insert(make_pair(quality, color_object));
					}else{
						color_object->release();
					}
				}else{
					color_object->release();
				}
			}
			bool first = true;
			for (auto result: valid_converters){
				if (first){
					first = false;
					color_list_add_color_object(m_color_list, result.second, true);
					imported = true;
				}
				result.second->release();
			}
			valid_converters.clear();
		}
		if (!f.good()) {
			if (f.eof()) break;
			f.close();
			m_last_error = Error::file_read_error;
			return false;
		}
	}
	f.close();
	if (!imported){
		m_last_error = Error::no_colors_imported;
	}
	return imported;
}
static void cssColor(ColorObject* color_object, ostream &stream)
{
	Color color, hsl;
	color = color_object->getColor();
	color_rgb_to_hsl(&color, &hsl);
	stream << " * " << color_object->getName()
		<< ": " << HtmlHEX{&color}
		<< ", " << HtmlRGB{&color}
		<< ", " << HtmlHSL{&color}
		<< endl;
}
bool ImportExport::exportCSS()
{
	ofstream f(m_filename.c_str(), ios::out | ios::trunc);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	f << "/**" << endl << " * Generated by Gpick " << gpick_build_version << endl;
	vector<ColorObject*> ordered;
	getOrderedColors(m_color_list, ordered);
	for (auto color: ordered){
		cssColor(color, f);
		if (!f.good()){
			f.close();
			m_last_error = Error::file_write_error;
			return false;
		}
	}
	f << " */" << endl;
	if (!f.good()){
		f.close();
		m_last_error = Error::file_write_error;
		return false;
	}
	f.close();
	return true;
}
static void htmlColor(ColorObject* color_object, bool include_color_name, ostream &stream)
{
	Color color, text_color;
	color = color_object->getColor();
	color_get_contrasting(&color, &text_color);
	stream << "<div style=\"background-color:" << HtmlRGB{&color} << "; color:" << HtmlRGB{&text_color} << "\">";
	if (include_color_name){
		string name = color_object->getName();
		escapeHtmlInplace(name);
		if (!name.empty())
			stream << name << ":<br/>";
	}
	stream << "<span>" << HtmlHEX{&color} << "</span>" << "</div>";
}
static string getHtmlColor(ColorObject* color_object)
{
	Color color = color_object->getColor();
	stringstream ss;
	ss << HtmlRGB{&color};
	return ss.str();
}
bool ImportExport::exportHTML()
{
	ofstream f(m_filename.c_str(), ios::out | ios::trunc);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	boost::filesystem::path path(m_filename);
	vector<ColorObject*> ordered;
	getOrderedColors(m_color_list, ordered);
	int item_size = 64;
	switch (m_item_size){
		case ItemSize::small:
			item_size = 32;
			break;
		case ItemSize::medium:
			item_size = 64;
			break;
		case ItemSize::big:
			item_size = 96;
			break;
		case ItemSize::controllable:
			break;
	}
	string background = "";
	switch (m_background){
		case Background::none:
			break;
		case Background::white:
			background = "background-color:white;";
			break;
		case Background::gray:
			background = "background-color:gray;";
			break;
		case Background::black:
			background = "background-color:black;";
			break;
		case Background::first_color:
			if (!ordered.empty())
				background = "background-color:" + getHtmlColor(ordered.front()) + ";";
			break;
		case Background::last_color:
			if (!ordered.empty())
				background = "background-color:" + getHtmlColor(ordered.back()) + ";";
			break;
		case Background::controllable:
			break;
	}
	f << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>"
		<< path.filename().string() << "</title>" << endl
		<< "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << endl
		<< "<style>" << endl
		<< "div#colors div{float: left; width: " << item_size << "px; height: " << item_size << "px; margin: 2px; text-align: center; font-size: 12px; font-family: Arial, Helvetica, sans-serif}" << endl
		<< "div#colors div span{font-weight: bold; cursor: pointer}" << endl
		<< "div#colors div span:hover{text-decoration: underline}" << endl
		<< "html{" << background << "}" << endl
		<< "input{margin-left: 1em;}" << endl
		<< "</style>"
		<< "</head>" << endl
		<< "<body>" << endl;
	if (m_item_size == ItemSize::controllable || m_background == Background::controllable){
		f << "<form>" << endl;
		if (m_item_size == ItemSize::controllable){
			f << "<div>" << _("Item size") << ":<input type=\"range\" id=\"item_size\" min=\"16\" max=\"128\" value=\"64\" oninput=\"var elements = document.querySelectorAll('div#colors div'); for (var i = 0; i < elements.length; i++){ elements[i].style.width = this.value + 'px'; elements[i].style.height = this.value + 'px'; }\" />" << "</div>" << endl;
		}
		if (m_background == Background::controllable){
			f << "<div>" << _("Background color") << ":<input type=\"color\" id=\"background\" oninput=\"document.body.style.backgroundColor = this.value;\" />" << "</div>" << endl;
		}
		f << "</form>" << endl;
	}
	f << "<div id=\"colors\">" << endl;
	string hex_case = m_gs->settings().getString("gpick.options.hex_case", "upper");
	if (hex_case == "upper"){
		f << uppercase;
	}else{
		f << nouppercase;
	}
	for (auto color: ordered){
		htmlColor(color, m_include_color_names, f);
		if (!f.good()){
			f.close();
			m_last_error = Error::file_write_error;
			return false;
		}
	}
	f << "</div>" << endl;
	f << "<script>" << endl
		<< "function selectText(element){ if (document.selection){ var range = document.body.createTextRange(); range.moveToElementText(element); range.select(); }else if (window.getSelection){ var range = document.createRange(); range.selectNode(element); window.getSelection().addRange(range); } }" << endl
		<< "document.getElementById('colors').addEventListener('click', function(event){ if (event.target.tagName.toLowerCase() == 'span'){ event.preventDefault(); selectText(event.target); document.execCommand('copy'); }});" << endl
		<< "</script>";
	f << "</body></html>" << endl;
	if (!f.good()){
		f.close();
		m_last_error = Error::file_write_error;
		return false;
	}
	f.close();
	return true;
}

bool ImportExport::exportType(FileType type)
{
	switch (type){
		case FileType::gpa:
			return exportGPA();
		case FileType::gpl:
			return exportGPL();
		case FileType::ase:
			return exportASE();
		case FileType::txt:
			return exportTXT();
		case FileType::mtl:
			return exportMTL();
		case FileType::css:
			return exportCSS();
		case FileType::html:
			return exportHTML();
		case FileType::rgbtxt:
		case FileType::unknown:
			return false;
	}
	return false;
}
static void mtlColor(ColorObject* color_object, ostream &stream)
{
	Color color = color_object->getColor();
	stream << "newmtl " << color_object->getName() << endl;
	stream << "Ns 90.000000" << endl;
	stream << "Ka 0.000000 0.000000 0.000000" << endl;
	stream << "Kd " << color.rgb.red << " " << color.rgb.green << " " << color.rgb.blue << endl;
	stream << "Ks 0.500000 0.500000 0.500000" << endl << endl;
}
bool ImportExport::exportMTL()
{
	ofstream f(m_filename.c_str(), ios::out | ios::trunc);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	vector<ColorObject*> ordered;
	getOrderedColors(m_color_list, ordered);
	for (auto color: ordered){
		mtlColor(color, f);
		if (!f.good()){
			f.close();
			m_last_error = Error::file_write_error;
			return false;
		}
	}
	f.close();
	return true;
}
typedef union FloatInt
{
	float f;
	uint32_t i;
}FloatInt;
static void aseColor(ColorObject* color_object, ostream &stream)
{
	Color color = color_object->getColor();
	string name = color_object->getName();
	glong name_u16_len = 0;
	gunichar2 *name_u16 = g_utf8_to_utf16(name.c_str(), -1, 0, &name_u16_len, 0);
	for (glong i = 0; i < name_u16_len; ++i){
		name_u16[i] = boost::endian::native_to_big<uint16_t>(name_u16[i]);
	}
	uint16_t color_entry = boost::endian::native_to_big<uint16_t>(0x0001);
	stream.write((char*)&color_entry, 2);
	int32_t block_size = 2 + (name_u16_len + 1) * 2 + 4 + (3 * 4) + 2; //name length + name (zero terminated and 2 bytes per char wide) + color name + 3 float values + color type
	block_size = boost::endian::native_to_big<uint32_t>(block_size);
	stream.write((char*)&block_size, 4);
	uint16_t name_length = boost::endian::native_to_big<uint16_t>(uint16_t(name_u16_len + 1));
	stream.write((char*)&name_length, 2);
	stream.write((char*)name_u16, (name_u16_len + 1) * 2);
	stream << "RGB ";
	FloatInt r, g, b;
	r.f = color.rgb.red;
	g.f = color.rgb.green;
	b.f = color.rgb.blue;
	r.i = boost::endian::native_to_big<uint32_t>(r.i);
	g.i = boost::endian::native_to_big<uint32_t>(g.i);
	b.i = boost::endian::native_to_big<uint32_t>(b.i);
	stream.write((char*)&r, 4);
	stream.write((char*)&g, 4);
	stream.write((char*)&b, 4);
	int16_t color_type = boost::endian::native_to_big<uint16_t>(0);
	stream.write((char*)&color_type, 2);
	g_free(name_u16);
}
bool ImportExport::exportASE()
{
	ofstream f(m_filename.c_str(), ios::out | ios::trunc | ios::binary);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	f << "ASEF"; //magic header
	uint32_t version = boost::endian::native_to_big<uint32_t>(0x00010000);
	f.write((char*)&version, 4);
	vector<ColorObject*> ordered;
	getOrderedColors(m_color_list, ordered);
	uint32_t blocks = ordered.size();
	blocks = boost::endian::native_to_big<uint32_t>(blocks);
	f.write((char*)&blocks, 4);
	for (auto color: ordered){
		aseColor(color, f);
		if (!f.good()){
			f.close();
			m_last_error = Error::file_write_error;
			return false;
		}
	}
	f.close();
	return true;
}
bool ImportExport::importASE()
{
	ifstream f(m_filename.c_str(), ios::binary);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	char magic[4];
	f.read(magic, 4);
	if (memcmp(magic, "ASEF", 4) != 0){
		f.close();
		m_last_error = Error::file_read_error;
		return false;
	}
	uint32_t version;
	f.read((char*)&version, 4);
	version = boost::endian::big_to_native<uint32_t>(version);
	uint32_t blocks;
	f.read((char*)&blocks, 4);
	blocks = boost::endian::big_to_native<uint32_t>(blocks);
	uint16_t block_type;
	uint32_t block_size;
	int color_supported;
	for (uint32_t i = 0; i < blocks; ++i){
		f.read((char*)&block_type, 2);
		block_type = boost::endian::big_to_native<uint16_t>(block_type);
		f.read((char*)&block_size, 4);
		block_size = boost::endian::big_to_native<uint32_t>(block_size);
		switch (block_type){
		case 0x0001: //color block
			{
				uint16_t name_length;
				f.read((char*)&name_length, 2);
				name_length = boost::endian::big_to_native<uint16_t>(name_length);

				gunichar2 *name_u16 = (gunichar2*)g_malloc(name_length*2);
				f.read((char*)name_u16, name_length*2);
				for (uint32_t j = 0; j < name_length; ++j){
					name_u16[j] = boost::endian::big_to_native<uint16_t>(name_u16[j]);
				}
				gchar *name = g_utf16_to_utf8(name_u16, name_length, 0, 0, 0);
				g_free(name_u16);
				Color c;
				char color_space[4];
				f.read(color_space, 4);
				color_supported = 0;
				if (memcmp(color_space, "RGB ", 4) == 0){
					FloatInt rgb[3];
					f.read((char*)&rgb[0], 4);
					f.read((char*)&rgb[1], 4);
					f.read((char*)&rgb[2], 4);
					rgb[0].i = boost::endian::big_to_native<uint32_t>(rgb[0].i);
					rgb[1].i = boost::endian::big_to_native<uint32_t>(rgb[1].i);
					rgb[2].i = boost::endian::big_to_native<uint32_t>(rgb[2].i);
					c.rgb.red = rgb[0].f;
					c.rgb.green = rgb[1].f;
					c.rgb.blue = rgb[2].f;
					color_supported = 1;
				}else if (memcmp(color_space, "CMYK", 4) == 0){
					Color c2;
					FloatInt cmyk[4];
					f.read((char*)&cmyk[0], 4);
					f.read((char*)&cmyk[1], 4);
					f.read((char*)&cmyk[2], 4);
					f.read((char*)&cmyk[3], 4);
					cmyk[0].i = boost::endian::big_to_native<uint32_t>(cmyk[0].i);
					cmyk[1].i = boost::endian::big_to_native<uint32_t>(cmyk[1].i);
					cmyk[2].i = boost::endian::big_to_native<uint32_t>(cmyk[2].i);
					cmyk[3].i = boost::endian::big_to_native<uint32_t>(cmyk[3].i);
					c2.cmyk.c = cmyk[0].f;
					c2.cmyk.m = cmyk[1].f;
					c2.cmyk.y = cmyk[2].f;
					c2.cmyk.k = cmyk[3].f;
					color_cmyk_to_rgb(&c2, &c);
					color_supported = 1;
				}else if (memcmp(color_space, "Gray", 4) == 0){
					FloatInt gray;
					f.read((char*)&gray, 4);
					gray.i = boost::endian::big_to_native<uint32_t>(gray.i);
					c.rgb.red = c.rgb.green = c.rgb.blue = gray.f;
					color_supported = 1;
				}else if (memcmp(color_space, "LAB ", 4) == 0){
					Color c2;
					FloatInt lab[3];
					f.read((char*)&lab[0], 4);
					f.read((char*)&lab[1], 4);
					f.read((char*)&lab[2], 4);
					lab[0].i = boost::endian::big_to_native<uint32_t>(lab[0].i);
					lab[1].i = boost::endian::big_to_native<uint32_t>(lab[1].i);
					lab[2].i = boost::endian::big_to_native<uint32_t>(lab[2].i);
					c2.lab.L = lab[0].f*100;
					c2.lab.a = lab[1].f;
					c2.lab.b = lab[2].f;
					color_lab_to_rgb_d50(&c2, &c);
					c.rgb.red = clamp_float(c.rgb.red, 0, 1);
					c.rgb.green = clamp_float(c.rgb.green, 0, 1);
					c.rgb.blue = clamp_float(c.rgb.blue, 0, 1);
					color_supported = 1;
				}
				if (color_supported){
					ColorObject* color_object;
					color_object = color_list_new_color_object(m_color_list, &c);
					color_object->setName(name);
					color_list_add_color_object(m_color_list, color_object, true);
					color_object->release();
				}
				uint16_t color_type;
				f.read((char*)&color_type, 2);
				g_free(name);
			}
			break;
		default:
			f.seekg(block_size, ios::cur);
		}
	}
	f.close();
	return true;
}
static string::size_type rfind_first_of_not(string const& str, string::size_type const pos, string const& chars)
{
	auto start = str.rend() - pos - 1;
	auto found = std::find_first_of(start, str.rend(), chars.begin(), chars.end());
	return found == str.rend() ? string::npos : pos - (found - start);
}
static int hexToInt(char hex)
{
	if (hex >= '0' && hex <= '9') return hex - '0';
	if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
	if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
	return 0;
}
static int hexPairToInt(const char *hex_pair)
{
	return hexToInt(hex_pair[0]) << 4 | hexToInt(hex_pair[1]);
}
bool ImportExport::importRGBTXT()
{
	ifstream f(m_filename.c_str(), ios::in);
	if (!f.is_open()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	string line;
	Color c;
	ColorObject* color_object;
	string strip_chars = " \t";
	for(;;){
		getline(f, line);
		if (!f.good()) break;
		stripLeadingTrailingChars(line, strip_chars);
		if (line.length() > 0 && line[0] == '#'){ // skip comment lines
			continue;
		}
		size_t hash_position = line.find('#');
		if (hash_position != string::npos){
			size_t last_non_space = rfind_first_of_not(line, hash_position, " \t");

			c.rgb.red = hexPairToInt(&line.at(hash_position + 1)) / 255.0;
			c.rgb.green = hexPairToInt(&line.at(hash_position + 3)) / 255.0;
			c.rgb.blue = hexPairToInt(&line.at(hash_position + 5)) / 255.0;

			color_object = color_list_new_color_object(m_color_list, &c);
			if (last_non_space != string::npos){
				color_object->setName(line.substr(0, last_non_space));
			}
			color_list_add_color_object(m_color_list, color_object, true);
			color_object->release();
		}
	}
	if (!f.eof()) {
		f.close();
		m_last_error = Error::file_read_error;
		return false;
	}
	f.close();
	return true;
}
static bool compareChunkType(const char *chunk_type, const char *data)
{
	int i;
	for (i = 0; i < 16; i++){
		if (chunk_type[i] != data[i]) return false;
		if (chunk_type[i] == 0) break;
	}
	for (; i < 16; i++){
		if (data[i] != 0) return false;
	}
	return true;
}
FileType ImportExport::getFileTypeByContent(const char *filename)
{
	ifstream f(filename, ios::in);
	if (!f.is_open())
		return FileType::unknown;
	char data[64];
	f.read(data, 64);
	size_t have = f.gcount();
	if (have >= 16){
		if (compareChunkType("GPA version", data))
			return FileType::gpa;
	}
	if (have >= 13){
		if (strncmp("GIMP Palette", data, 12) == 0 && (data[12] == '\r' || data[12] == '\n'))
			return FileType::gpl;
	}
	return FileType::unknown;
}
static struct{
	FileType type;
	const char *extension;
	bool full_name;
}extensions[] = {
	{FileType::gpa, ".gpa", false},
	{FileType::gpl, ".gpl", false},
	{FileType::ase, ".ase", false},
	{FileType::txt, ".txt", false},
	{FileType::mtl, ".mtl", false},
	{FileType::css, ".css", false},
	{FileType::html, ".html", false},
	{FileType::html, ".htm", false},
	{FileType::rgbtxt, "rgb.txt", true},
	{FileType::unknown, nullptr, false},
};
FileType ImportExport::getFileType(const char *filename)
{
	boost::filesystem::path path(filename);
	string name = path.filename().string();
	boost::algorithm::to_lower(name);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i){
		if (extensions[i].full_name && name == extensions[i].extension){
			return extensions[i].type;
		}
	}
	string extension = path.extension().string();
	if (extension.length() == 0)
		return getFileTypeByContent(filename);
	boost::algorithm::to_lower(extension);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i){
		if (!extensions[i].full_name && extension == extensions[i].extension){
			return extensions[i].type;
		}
	}
	return getFileTypeByContent(filename);
}
FileType ImportExport::getFileTypeByExtension(const char *extension)
{
	string extension_lowercase = extension;
	boost::algorithm::to_lower(extension_lowercase);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i){
		if (!extensions[i].full_name && extension_lowercase == extensions[i].extension){
			return extensions[i].type;
		}
	}
	return FileType::unknown;
}
bool ImportExport::importType(FileType type)
{
	switch (type){
		case FileType::gpa:
			return importGPA();
		case FileType::gpl:
			return importGPL();
		case FileType::ase:
			return importASE();
		case FileType::txt:
			return importTXT();
		case FileType::rgbtxt:
			return importRGBTXT();
		case FileType::mtl:
		case FileType::css:
		case FileType::html:
		case FileType::unknown:
			return false;
	}
	return false;
}
ImportExport::Error ImportExport::getLastError() const
{
	return m_last_error;
}
struct ImportTextFile: public text_file_parser::TextFile
{
	ifstream m_file;
	list<Color> m_colors;
	bool m_failed;
	ImportTextFile(const string &filename)
	{
		m_failed = false;
		m_file.open(filename, ios::in);
	}
	bool isOpen()
	{
		return m_file.is_open();
	}
	virtual ~ImportTextFile()
	{
		m_file.close();
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
		m_file.read(buffer, length);
		size_t bytes = m_file.gcount();
		if (bytes > 0) return bytes;
		if (m_file.eof()) return 0;
		if (!m_file.good()){
			m_failed = true;
		}
		return 0;
	}
	virtual void addColor(const Color &color)
	{
		m_colors.push_back(color);
	}
};
bool ImportExport::importTextFile(const text_file_parser::Configuration &configuration)
{
	ImportTextFile import_text_file(m_filename);
	if (!import_text_file.isOpen()){
		m_last_error = Error::could_not_open_file;
		return false;
	}
	if (!import_text_file.parse(configuration)){
		m_last_error = Error::parsing_failed;
		return false;
	}
	if (import_text_file.m_failed){
		m_last_error = Error::parsing_failed;
		return false;
	}
	if (import_text_file.m_colors.size() == 0){
		m_last_error = Error::no_colors_imported;
		return false;
	}
	for (auto color: import_text_file.m_colors){
		auto color_object = new ColorObject("" ,color);
		color_list_add_color_object(m_color_list, color_object, true);
		color_object->release();
	}
	return true;
}
const std::string &ImportExport::getFilename() const
{
	return m_filename;
}
