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
#include "common/First.h"
#include <glib.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <boost/math/special_functions/round.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/endian/conversion.hpp>

ImportExport::ImportExport(ColorList &colorList, const char *filename, GlobalState &gs):
	m_colorList(colorList),
	m_converter(nullptr),
	m_converters(nullptr),
	m_filename(filename),
	m_itemSize(ItemSize::medium),
	m_background(Background::none),
	m_gs(gs),
	m_includeColorNames(true),
	m_lastError(Error::none) {
}
void ImportExport::fixFileExtension(const char *selected_filter) {
	using namespace std::filesystem;
	if (selected_filter && selected_filter[0] == '*') {
		std::string extension = path(m_filename).extension().string();
		bool append = false;
		if (extension.length() == 0) {
			append = true;
		} else {
			if (getFileTypeByExtension(extension.c_str()) == FileType::unknown)
				append = true;
		}
		if (append) {
			size_t length = m_filename.length();
			m_filename += &selected_filter[1];
			size_t additional_extension = m_filename.find_first_of(',', length);
			if (additional_extension != std::string::npos) {
				m_filename = m_filename.substr(0, additional_extension);
			}
		}
	}
}
void ImportExport::setConverter(Converter *converter) {
	m_converter = converter;
}
void ImportExport::setConverters(Converters *converters) {
	m_converters = converters;
}
void ImportExport::setItemSize(ItemSize itemSize) {
	m_itemSize = itemSize;
}
void ImportExport::setItemSize(const char *itemSize) {
	std::string v(itemSize);
	if (v == "small")
		m_itemSize = ItemSize::small;
	else if (v == "medium")
		m_itemSize = ItemSize::medium;
	else if (v == "big")
		m_itemSize = ItemSize::big;
	else if (v == "controllable")
		m_itemSize = ItemSize::controllable;
	else
		m_itemSize = ItemSize::medium;
}
void ImportExport::setBackground(Background background) {
	m_background = background;
}
void ImportExport::setBackground(const char *background) {
	std::string v(background);
	if (v == "none")
		m_background = Background::none;
	else if (v == "white")
		m_background = Background::white;
	else if (v == "gray")
		m_background = Background::gray;
	else if (v == "black")
		m_background = Background::black;
	else if (v == "first_color")
		m_background = Background::firstColor;
	else if (v == "last_color")
		m_background = Background::lastColor;
	else if (v == "controllable")
		m_background = Background::controllable;
	else
		m_background = Background::none;
}
void ImportExport::setIncludeColorNames(bool include_color_names) {
	m_includeColorNames = include_color_names;
}
static void gplColor(ColorObject *colorObject, std::ostream &stream) {
	using boost::math::iround;
	Color color = colorObject->getColor();
	stream
		<< iround(color.red * 255) << "\t"
		<< iround(color.green * 255) << "\t"
		<< iround(color.blue * 255) << "\t" << colorObject->getName() << '\n';
}
bool ImportExport::exportGPL() {
	std::ofstream f(m_filename, std::ios::out | std::ios::trunc);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	std::filesystem::path path(m_filename);
	f << "GIMP Palette" << '\n';
	f << "Name: " << path.filename().string() << '\n';
	f << "Columns: 1" << '\n';
	f << "#" << '\n';
	for (auto color: m_colorList) {
		gplColor(color, f);
		if (!f.good()) {
			f.close();
			m_lastError = Error::fileWriteError;
			return false;
		}
	}
	f.close();
	return true;
}
bool ImportExport::importGPL() {
	std::ifstream f(m_filename, std::ios::in);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	std::string line;
	std::getline(f, line);
	if (f.good() && line != "GIMP Palette") {
		f.close();
		m_lastError = Error::fileReadError;
		return false;
	}
	do {
		getline(f, line);
	} while (f.good() && ((line.size() < 1) || (((line[0] > '9') || (line[0] < '0')) && line[0] != ' ')));
	int r, g, b;
	Color c;
	std::string stripChars = " \t";
	for (;;) {
		if (!f.good()) break;
		stripLeadingTrailingChars(line, stripChars);
		if (line.length() > 0 && line[0] == '#') { // skip comment lines
			getline(f, line);
			continue;
		}
		std::stringstream ss(line);
		ss >> r >> g >> b;
		getline(ss, line);
		if (!f.good()) line = "";
		c.red = r / 255.0f;
		c.green = g / 255.0f;
		c.blue = b / 255.0f;
		c.alpha = 1;
		stripLeadingTrailingChars(line, stripChars);
		ColorObject colorObject(line, c);
		m_colorList.add(colorObject);
		getline(f, line);
	}
	if (!f.eof()) {
		f.close();
		m_lastError = Error::fileReadError;
		return false;
	}
	f.close();
	return true;
}
bool ImportExport::importGPA() {
	return paletteFileLoad(m_filename.c_str(), m_colorList);
}
bool ImportExport::exportGPA() {
	return paletteFileSave(m_filename.c_str(), m_colorList);
}
bool ImportExport::exportTXT() {
	std::ofstream f(m_filename.c_str(), std::ios::out | std::ios::trunc);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	ConverterSerializePosition position(m_colorList.size());
	for (auto color: m_colorList) {
		std::string line = m_converter->serialize(*color, position);
		if (m_includeColorNames) {
			f << line << " " << color->getName() << '\n';
		} else {
			f << line << '\n';
		}
		position.incrementIndex();
		if (position.index() + 1 == position.count()) {
			position.last(true);
		}
		if (position.first())
			position.first(false);
		if (!f.good()) {
			f.close();
			m_lastError = Error::fileWriteError;
			return false;
		}
	}
	f.close();
	return true;
}
bool ImportExport::importTXT() {
	std::ifstream f(m_filename.c_str(), std::ios::in);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	common::First<float, std::greater<float>, ColorObject> bestConversion;
	ColorObject colorObject;
	float quality;
	std::string line, stripChars = " \t";
	bool imported = false;
	for (;;) {
		std::getline(f, line);
		stripLeadingTrailingChars(line, stripChars);
		if (!line.empty()) {
			for (auto &converter: m_converters->allPaste()) {
				if (!converter->hasDeserialize())
					continue;
				if (converter->deserialize(line.c_str(), colorObject, quality)) {
					if (quality > 0) {
						bestConversion(quality, colorObject);
					}
				}
			}
			if (bestConversion) {
				m_colorList.add(bestConversion.data<ColorObject>());
				imported = true;
				bestConversion.reset();
			}
		}
		if (!f.good()) {
			if (f.eof())
				break;
			f.close();
			m_lastError = Error::fileReadError;
			return false;
		}
	}
	f.close();
	if (!imported) {
		m_lastError = Error::noColorsImported;
	}
	return imported;
}
static void cssColor(ColorObject *colorObject, std::ostream &stream) {
	Color color, hsl;
	color = colorObject->getColor();
	hsl = color.rgbToHsl();
	stream << " * " << colorObject->getName()
				 << ": " << HtmlHEX { color }
				 << ", " << HtmlRGBA { color }
				 << ", " << HtmlHSLA { color }
				 << '\n';
}
bool ImportExport::exportCSS() {
	std::ofstream f(m_filename.c_str(), std::ios::out | std::ios::trunc);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	f << "/**" << '\n'
		<< " * Generated by Gpick " << version::versionFull << '\n';
	for (auto color: m_colorList) {
		cssColor(color, f);
		if (!f.good()) {
			f.close();
			m_lastError = Error::fileWriteError;
			return false;
		}
	}
	f << " */" << '\n';
	if (!f.good()) {
		f.close();
		m_lastError = Error::fileWriteError;
		return false;
	}
	f.close();
	return true;
}
static void htmlColor(ColorObject *colorObject, Converter *converter, bool includeColorName, std::ostream &stream) {
	Color color, textColor;
	color = colorObject->getColor();
	textColor = color.getContrasting();
	stream << "<div style=\"background-color:" << HtmlRGBA { color } << "; color:" << HtmlRGB { textColor } << "\">";
	if (includeColorName) {
		std::string name = colorObject->getName();
		escapeHtmlInplace(name);
		if (!name.empty())
			stream << name << ":<br/>";
	}
	if (converter)
		stream << "<span>" << converter->serialize(*colorObject) << "</span></div>";
	else
		stream << "<span>" << HtmlRGB(color) << "</span></div>";
}
static std::string getHtmlColor(ColorObject *colorObject) {
	Color color = colorObject->getColor();
	std::stringstream ss;
	ss << HtmlRGBA { color };
	return ss.str();
}
static std::string getHtmlColor(const Color &color) {
	std::stringstream ss;
	ss << HtmlRGBA { color };
	return ss.str();
}
bool ImportExport::exportHTML() {
	std::ofstream f(m_filename.c_str(), std::ios::out | std::ios::trunc);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	std::filesystem::path path(m_filename);
	int itemSize = 64;
	switch (m_itemSize) {
	case ItemSize::small:
		itemSize = 32;
		break;
	case ItemSize::medium:
		itemSize = 64;
		break;
	case ItemSize::big:
		itemSize = 96;
		break;
	case ItemSize::controllable:
		break;
	}
	std::string htmlBackgroundCss = "";
	std::string htmlColorCss = "";
	switch (m_background) {
	case Background::none:
		break;
	case Background::white:
		htmlBackgroundCss = "background-color:white;";
		htmlColorCss = "color:black;";
		break;
	case Background::gray:
		htmlBackgroundCss = "background-color:gray;";
		htmlColorCss = "color:black;";
		break;
	case Background::black:
		htmlBackgroundCss = "background-color:black;";
		htmlColorCss = "color:white;";
		break;
	case Background::firstColor:
		if (!m_colorList.empty()) {
			htmlBackgroundCss = "background-color:" + getHtmlColor(m_colorList.front()) + ";";
			auto color = m_colorList.front()->getColor();
			Color textColor = color.getContrasting();
			htmlColorCss = "color:" + getHtmlColor(textColor) + ";";
		}
		break;
	case Background::lastColor:
		if (!m_colorList.empty()) {
			htmlBackgroundCss = "background-color:" + getHtmlColor(m_colorList.back()) + ";";
			auto color = m_colorList.back()->getColor();
			Color textColor = color.getContrasting();
			htmlColorCss = "color:" + getHtmlColor(textColor) + ";";
		}
		break;
	case Background::controllable:
		break;
	}
	f << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>"
		<< path.filename().string() << "</title>" << '\n'
		<< "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << '\n'
		<< "<style>" << '\n'
		<< "div#colors div{float: left; width: " << itemSize << "px; height: " << itemSize << "px; margin: 2px; text-align: center; font-size: 12px; font-family: Arial, Helvetica, sans-serif}" << '\n'
		<< "div#colors div span{font-weight: bold; cursor: pointer; display: block;}" << '\n'
		<< "div#colors div span:hover{text-decoration: underline}" << '\n'
		<< "html{" << htmlBackgroundCss << htmlColorCss << "}" << '\n'
		<< "input{margin-left: 1em;}" << '\n'
		<< "</style>"
		<< "</head>" << '\n'
		<< "<body>" << '\n';
	if (m_itemSize == ItemSize::controllable || m_background == Background::controllable) {
		f << "<form>" << '\n';
		if (m_itemSize == ItemSize::controllable) {
			f << "<div>" << _("Item size") << ":<input type=\"range\" id=\"itemSize\" min=\"16\" max=\"128\" value=\"64\" oninput=\"var elements = document.querySelectorAll('div#colors div'); for (var i = 0; i < elements.length; i++){ elements[i].style.width = this.value + 'px'; elements[i].style.height = this.value + 'px'; }\" />"
				<< "</div>" << '\n';
		}
		if (m_background == Background::controllable) {
			f << "<div>" << _("Background color") << ":<input type=\"color\" id=\"background\" oninput=\"document.body.style.backgroundColor = this.value;\" />"
				<< "</div>" << '\n';
		}
		f << "</form>" << '\n';
	}
	f << "<div id=\"colors\">" << '\n';
	std::string hexCase = m_gs.settings().getString("gpick.options.hex_case", "upper");
	if (hexCase == "upper") {
		f << std::uppercase;
	} else {
		f << std::nouppercase;
	}
	for (auto color: m_colorList) {
		htmlColor(color, m_converter, m_includeColorNames, f);
		if (!f.good()) {
			f.close();
			m_lastError = Error::fileWriteError;
			return false;
		}
	}
	f << "</div>" << '\n';
	f << "<script>" << '\n'
		<< "function selectText(element){ if (document.selection){ var range = document.body.createTextRange(); range.moveToElementText(element); range.select(); }else if (window.getSelection){ var range = document.createRange(); range.selectNode(element); window.getSelection().addRange(range); } }" << '\n'
		<< "document.getElementById('colors').addEventListener('click', function(event){ if (event.target.tagName.toLowerCase() == 'span'){ event.preventDefault(); selectText(event.target); document.execCommand('copy'); }});" << '\n'
		<< "</script>";
	f << "</body></html>" << '\n';
	if (!f.good()) {
		f.close();
		m_lastError = Error::fileWriteError;
		return false;
	}
	f.close();
	return true;
}

bool ImportExport::exportType(FileType type) {
	switch (type) {
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
static void mtlColor(ColorObject *colorObject, std::ostream &stream) {
	Color color = colorObject->getColor();
	stream << "newmtl " << colorObject->getName() << '\n';
	stream << "Ns 90.000000" << '\n';
	stream << "Ka 0.000000 0.000000 0.000000" << '\n';
	stream << "Kd " << color.red << " " << color.green << " " << color.blue << '\n';
	stream << "Ks 0.500000 0.500000 0.500000" << '\n'
				 << '\n';
}
bool ImportExport::exportMTL() {
	std::ofstream f(m_filename.c_str(), std::ios::out | std::ios::trunc);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	for (auto color: m_colorList) {
		mtlColor(color, f);
		if (!f.good()) {
			f.close();
			m_lastError = Error::fileWriteError;
			return false;
		}
	}
	f.close();
	return true;
}
union FloatInt {
	float f;
	uint32_t i;
};
static void aseColor(ColorObject *colorObject, std::ostream &stream) {
	Color color = colorObject->getColor();
	std::string name = colorObject->getName();
	glong name_u16_len = 0;
	gunichar2 *name_u16 = g_utf8_to_utf16(name.c_str(), -1, 0, &name_u16_len, 0);
	for (glong i = 0; i < name_u16_len; ++i) {
		name_u16[i] = boost::endian::native_to_big<uint16_t>(name_u16[i]);
	}
	uint16_t color_entry = boost::endian::native_to_big<uint16_t>(0x0001);
	stream.write((char *)&color_entry, 2);
	int32_t block_size = 2 + (name_u16_len + 1) * 2 + 4 + (3 * 4) + 2; //name length + name (zero terminated and 2 bytes per char wide) + color name + 3 float values + color type
	block_size = boost::endian::native_to_big<uint32_t>(block_size);
	stream.write((char *)&block_size, 4);
	uint16_t name_length = boost::endian::native_to_big<uint16_t>(uint16_t(name_u16_len + 1));
	stream.write((char *)&name_length, 2);
	stream.write((char *)name_u16, (name_u16_len + 1) * 2);
	stream << "RGB ";
	FloatInt r, g, b;
	r.f = color.red;
	g.f = color.green;
	b.f = color.blue;
	r.i = boost::endian::native_to_big<uint32_t>(r.i);
	g.i = boost::endian::native_to_big<uint32_t>(g.i);
	b.i = boost::endian::native_to_big<uint32_t>(b.i);
	stream.write((char *)&r, 4);
	stream.write((char *)&g, 4);
	stream.write((char *)&b, 4);
	int16_t color_type = boost::endian::native_to_big<uint16_t>(0);
	stream.write((char *)&color_type, 2);
	g_free(name_u16);
}
bool ImportExport::exportASE() {
	std::ofstream f(m_filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	f << "ASEF"; //magic header
	uint32_t version = boost::endian::native_to_big<uint32_t>(0x00010000);
	f.write((char *)&version, 4);
	uint32_t blocks = m_colorList.size();
	blocks = boost::endian::native_to_big<uint32_t>(blocks);
	f.write((char *)&blocks, 4);
	for (auto color: m_colorList) {
		aseColor(color, f);
		if (!f.good()) {
			f.close();
			m_lastError = Error::fileWriteError;
			return false;
		}
	}
	f.close();
	return true;
}
bool ImportExport::importASE() {
	std::ifstream f(m_filename.c_str(), std::ios::binary);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	char magic[4];
	f.read(magic, 4);
	if (memcmp(magic, "ASEF", 4) != 0) {
		f.close();
		m_lastError = Error::fileReadError;
		return false;
	}
	uint32_t version;
	f.read((char *)&version, 4);
	version = boost::endian::big_to_native<uint32_t>(version);
	uint32_t blocks;
	f.read((char *)&blocks, 4);
	blocks = boost::endian::big_to_native<uint32_t>(blocks);
	uint16_t block_type;
	uint32_t block_size;
	int color_supported;
	for (uint32_t i = 0; i < blocks; ++i) {
		f.read((char *)&block_type, 2);
		block_type = boost::endian::big_to_native<uint16_t>(block_type);
		f.read((char *)&block_size, 4);
		block_size = boost::endian::big_to_native<uint32_t>(block_size);
		switch (block_type) {
		case 0x0001: //color block
		{
			uint16_t name_length;
			f.read((char *)&name_length, 2);
			name_length = boost::endian::big_to_native<uint16_t>(name_length);

			gunichar2 *name_u16 = (gunichar2 *)g_malloc(name_length * 2);
			f.read((char *)name_u16, name_length * 2);
			for (uint32_t j = 0; j < name_length; ++j) {
				name_u16[j] = boost::endian::big_to_native<uint16_t>(name_u16[j]);
			}
			gchar *name = g_utf16_to_utf8(name_u16, name_length, 0, 0, 0);
			g_free(name_u16);
			Color c;
			char color_space[4];
			f.read(color_space, 4);
			color_supported = 0;
			if (memcmp(color_space, "RGB ", 4) == 0) {
				FloatInt rgb[3];
				f.read((char *)&rgb[0], 4);
				f.read((char *)&rgb[1], 4);
				f.read((char *)&rgb[2], 4);
				rgb[0].i = boost::endian::big_to_native<uint32_t>(rgb[0].i);
				rgb[1].i = boost::endian::big_to_native<uint32_t>(rgb[1].i);
				rgb[2].i = boost::endian::big_to_native<uint32_t>(rgb[2].i);
				c.red = rgb[0].f;
				c.green = rgb[1].f;
				c.blue = rgb[2].f;
				c.alpha = 1;
				color_supported = 1;
			} else if (memcmp(color_space, "CMYK", 4) == 0) {
				Color c2;
				FloatInt cmyk[4];
				f.read((char *)&cmyk[0], 4);
				f.read((char *)&cmyk[1], 4);
				f.read((char *)&cmyk[2], 4);
				f.read((char *)&cmyk[3], 4);
				cmyk[0].i = boost::endian::big_to_native<uint32_t>(cmyk[0].i);
				cmyk[1].i = boost::endian::big_to_native<uint32_t>(cmyk[1].i);
				cmyk[2].i = boost::endian::big_to_native<uint32_t>(cmyk[2].i);
				cmyk[3].i = boost::endian::big_to_native<uint32_t>(cmyk[3].i);
				c2.cmyk.c = cmyk[0].f;
				c2.cmyk.m = cmyk[1].f;
				c2.cmyk.y = cmyk[2].f;
				c2.cmyk.k = cmyk[3].f;
				c = c2.cmykToRgb();
				c.alpha = 1;
				color_supported = 1;
			} else if (memcmp(color_space, "Gray", 4) == 0) {
				FloatInt gray;
				f.read((char *)&gray, 4);
				gray.i = boost::endian::big_to_native<uint32_t>(gray.i);
				c.red = c.green = c.blue = gray.f;
				c.alpha = 1;
				color_supported = 1;
			} else if (memcmp(color_space, "LAB ", 4) == 0) {
				Color c2;
				FloatInt lab[3];
				f.read((char *)&lab[0], 4);
				f.read((char *)&lab[1], 4);
				f.read((char *)&lab[2], 4);
				lab[0].i = boost::endian::big_to_native<uint32_t>(lab[0].i);
				lab[1].i = boost::endian::big_to_native<uint32_t>(lab[1].i);
				lab[2].i = boost::endian::big_to_native<uint32_t>(lab[2].i);
				c2.lab.L = lab[0].f * 100;
				c2.lab.a = lab[1].f;
				c2.lab.b = lab[2].f;
				c = c2.labToRgbD50();
				c.red = math::clamp(c.red, 0.0f, 1.0f);
				c.green = math::clamp(c.green, 0.0f, 1.0f);
				c.blue = math::clamp(c.blue, 0.0f, 1.0f);
				c.alpha = 1;
				color_supported = 1;
			}
			if (color_supported) {
				ColorObject colorObject(name, c);
				m_colorList.add(colorObject);
			}
			uint16_t color_type;
			f.read((char *)&color_type, 2);
			g_free(name);
		} break;
		default:
			f.seekg(block_size, std::ios::cur);
		}
	}
	f.close();
	return true;
}
static std::string::size_type rfind_first_of_not(std::string const &str, std::string::size_type const pos, std::string const &chars) {
	auto start = str.rend() - pos - 1;
	auto found = std::find_first_of(start, str.rend(), chars.begin(), chars.end());
	return found == str.rend() ? std::string::npos : pos - (found - start);
}
static int hexToInt(char hex) {
	if (hex >= '0' && hex <= '9') return hex - '0';
	if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
	if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
	return 0;
}
static int hexPairToInt(const char *hex_pair) {
	return hexToInt(hex_pair[0]) << 4 | hexToInt(hex_pair[1]);
}
bool ImportExport::importRGBTXT() {
	std::ifstream f(m_filename.c_str(), std::ios::in);
	if (!f.is_open()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	std::string line;
	Color c;
	std::string stripChars = " \t";
	for (;;) {
		getline(f, line);
		if (!f.good()) break;
		stripLeadingTrailingChars(line, stripChars);
		if (line.length() > 0 && line[0] == '#') { // skip comment lines
			continue;
		}
		size_t hashPosition = line.find('#');
		if (hashPosition != std::string::npos) {
			size_t lastNonSpace = rfind_first_of_not(line, hashPosition, " \t");

			c.red = hexPairToInt(&line.at(hashPosition + 1)) / 255.0f;
			c.green = hexPairToInt(&line.at(hashPosition + 3)) / 255.0f;
			c.blue = hexPairToInt(&line.at(hashPosition + 5)) / 255.0f;
			c.alpha = 1;

			ColorObject colorObject(c);
			if (lastNonSpace != std::string::npos) {
				colorObject.setName(line.substr(0, lastNonSpace));
			}
			m_colorList.add(colorObject);
		}
	}
	if (!f.eof()) {
		f.close();
		m_lastError = Error::fileReadError;
		return false;
	}
	f.close();
	return true;
}
static bool compareChunkType(const char *chunk_type, const char *data) {
	int i;
	for (i = 0; i < 16; i++) {
		if (chunk_type[i] != data[i]) return false;
		if (chunk_type[i] == 0) break;
	}
	for (; i < 16; i++) {
		if (data[i] != 0) return false;
	}
	return true;
}
FileType ImportExport::getFileTypeByContent(const char *filename) {
	std::ifstream f(filename, std::ios::in);
	if (!f.is_open())
		return FileType::unknown;
	char data[64];
	f.read(data, 64);
	size_t have = f.gcount();
	if (have >= 16) {
		if (compareChunkType("GPA version", data))
			return FileType::gpa;
	}
	if (have >= 13) {
		if (strncmp("GIMP Palette", data, 12) == 0 && (data[12] == '\r' || data[12] == '\n'))
			return FileType::gpl;
	}
	return FileType::unknown;
}
static struct {
	FileType type;
	const char *extension;
	bool fullName;
} extensions[] = {
	{ FileType::gpa, ".gpa", false },
	{ FileType::gpl, ".gpl", false },
	{ FileType::ase, ".ase", false },
	{ FileType::txt, ".txt", false },
	{ FileType::mtl, ".mtl", false },
	{ FileType::css, ".css", false },
	{ FileType::html, ".html", false },
	{ FileType::html, ".htm", false },
	{ FileType::rgbtxt, "rgb.txt", true },
	{ FileType::unknown, nullptr, false },
};
FileType ImportExport::getFileType(const char *filename) {
	std::filesystem::path path(filename);
	std::string name = path.filename().string();
	boost::algorithm::to_lower(name);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i) {
		if (extensions[i].fullName && name == extensions[i].extension) {
			return extensions[i].type;
		}
	}
	std::string extension = path.extension().string();
	if (extension.length() == 0)
		return getFileTypeByContent(filename);
	boost::algorithm::to_lower(extension);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i) {
		if (!extensions[i].fullName && extension == extensions[i].extension) {
			return extensions[i].type;
		}
	}
	return getFileTypeByContent(filename);
}
FileType ImportExport::getFileTypeByExtension(const char *extension) {
	std::string extension_lowercase = extension;
	boost::algorithm::to_lower(extension_lowercase);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i) {
		if (!extensions[i].fullName && extension_lowercase == extensions[i].extension) {
			return extensions[i].type;
		}
	}
	return FileType::unknown;
}
bool ImportExport::importType(FileType type) {
	switch (type) {
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
ImportExport::Error ImportExport::getLastError() const {
	return m_lastError;
}
struct ImportTextFile: public text_file_parser::TextFile {
	std::ifstream m_file;
	std::vector<Color> m_colors;
	bool m_failed;
	ImportTextFile(const std::string &filename) {
		m_failed = false;
		m_file.open(filename, std::ios::in);
	}
	bool isOpen() {
		return m_file.is_open();
	}
	virtual ~ImportTextFile() {
		m_file.close();
	}
	virtual void outOfMemory() {
		m_failed = true;
	}
	virtual void syntaxError(size_t start_line, size_t start_column, size_t end_line, size_t end_colunn) {
		m_failed = true;
	}
	virtual size_t read(char *buffer, size_t length) {
		m_file.read(buffer, length);
		size_t bytes = m_file.gcount();
		if (bytes > 0) return bytes;
		if (m_file.eof()) return 0;
		if (!m_file.good()) {
			m_failed = true;
		}
		return 0;
	}
	virtual void addColor(const Color &color) {
		m_colors.push_back(color);
	}
};
bool ImportExport::importTextFile(const text_file_parser::Configuration &configuration) {
	ImportTextFile importTextFile(m_filename);
	if (!importTextFile.isOpen()) {
		m_lastError = Error::couldNotOpenFile;
		return false;
	}
	if (!importTextFile.parse(configuration)) {
		m_lastError = Error::parsingFailed;
		return false;
	}
	if (importTextFile.m_failed) {
		m_lastError = Error::parsingFailed;
		return false;
	}
	if (importTextFile.m_colors.size() == 0) {
		m_lastError = Error::noColorsImported;
		return false;
	}
	for (auto color: importTextFile.m_colors) {
		m_colorList.add(ColorObject("", color));
	}
	return true;
}
const std::string &ImportExport::getFilename() const {
	return m_filename;
}
