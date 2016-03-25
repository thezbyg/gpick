#include "ImportExport.h"
#include "FileFormat.h"
#include "Converter.h"
#include "Endian.h"
#include "Internationalisation.h"
#include "StringUtils.h"
#include "version/Version.h"
#include <glib.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/math/special_functions/round.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;

static bool getOrderedColors(ColorList *color_list, vector<ColorObject*> &ordered)
{
	color_list_get_positions(color_list);
	size_t max_index = 0;
	bool index_set = false;
	for (auto color: color_list->colors){
		if (color->position_set){
			if (!index_set){
				max_index = color->position;
				index_set = true;
			}else if (color->position > max_index){
				max_index = color->position;
			}
		}
	}
	if (!index_set){
		return false;
	}else{
		ordered.resize(max_index + 1);
		for (auto color: color_list->colors){
			if (color->position_set){
				ordered[color->position] = color;
			}
		}
		return true;
	}
}
ImportExport::ImportExport(ColorList *color_list, const char* filename):
	m_color_list(color_list),
	m_converter(nullptr),
	m_converters(nullptr),
	m_filename(filename)
{
}
void ImportExport::setConverter(Converter *converter)
{
	m_converter = converter;
}
void ImportExport::setConverters(Converters *converters)
{
	m_converters = converters;
}
static void gplColor(ColorObject* color_object, ostream &stream)
{
	using boost::math::iround;
	Color color;
	color_object_get_color(color_object, &color);
	const char* name = color_object_get_name(color_object);
	stream
		<< iround(color.rgb.red * 255) << "\t"
		<< iround(color.rgb.green * 255) << "\t"
		<< iround(color.rgb.blue * 255) << "\t" << name << endl;
}
bool ImportExport::exportGPL()
{
	ofstream f(m_filename, ios::out | ios::trunc);
	if (f.is_open()){
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
				return false;
			}
		}
		f.close();
		return true;
	}
	return false;
}
bool ImportExport::importGPL()
{
	ifstream f(m_filename, ios::in);
	if (!f.is_open()){
		return false;
	}
	string line;
	getline(f, line);
	if (f.good() && line != "GIMP Palette"){
		f.close();
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
		color_object_set_name(color_object, line.c_str());
		color_list_add_color_object(m_color_list, color_object, true);
		color_object_release(color_object);
		getline(f, line);
	}
	if (!f.eof()) {
		f.close();
		return false;
	}
	f.close();
	return true;
}
bool ImportExport::importGPA()
{
	return palette_file_load(m_filename, m_color_list);
}
bool ImportExport::exportGPA()
{
	return palette_file_save(m_filename, m_color_list);
}
bool ImportExport::exportTXT()
{
	ofstream f(m_filename, ios::out | ios::trunc);
	if (f.is_open()){
		vector<ColorObject*> ordered;
		getOrderedColors(m_color_list, ordered);
		ConverterSerializePosition position;
		position.index = 0;
		position.count = ordered.size();
		position.first = true;
		position.last = position.count <= 1;
		for (auto color: ordered){
			string line;
			converters_color_serialize(m_converter, color, position, line);
			f << line << endl;
			position.index++;
			if (position.index + 1 == position.count){
				position.last = true;
			}
			if (position.first)
				position.first = false;
			if (!f.good()){
				f.close();
				return false;
			}
		}
		f.close();
		return true;
	}
	return false;
}
bool ImportExport::importTXT()
{
	ifstream f(m_filename, ios::in);
	if (!f.is_open()){
		return false;
	}
	uint32_t table_size;
	Converter *converter = nullptr;
	Converter **converter_table = converters_get_all_type(m_converters, CONVERTERS_ARRAY_TYPE_PASTE, &table_size);
	ColorObject* color_object;
	Color dummy_color;
	typedef multimap<float, ColorObject*, greater<float>> ValidConverters;
	ValidConverters valid_converters;
	string line;
	string strip_chars = " \t";
	for(;;){
		getline(f, line);
		stripLeadingTrailingChars(line, strip_chars);
		if (!line.empty()){
			for (size_t i = 0; i != table_size; ++i){
				converter = converter_table[i];
				if (!converter->deserialize_available) continue;
				color_object = color_list_new_color_object(m_color_list, &dummy_color);
				float quality;
				if (converters_color_deserialize(m_converters, converter->function_name, line.c_str(), color_object, &quality) == 0){
					if (quality > 0){
						valid_converters.insert(make_pair(quality, color_object));
					}else{
						color_object_release(color_object);
					}
				}else{
					color_object_release(color_object);
				}
			}
			bool first = true;
			for (auto result: valid_converters){
				if (first){
					first = false;
					color_list_add_color_object(m_color_list, result.second, true);
				}
				color_object_release(result.second);
			}
			valid_converters.clear();
		}
		if (!f.good()) {
			if (f.eof()) break;
			f.close();
			return false;
		}
	}
	f.close();
	return true;
}
static void cssColor(ColorObject* color_object, ostream &stream)
{
	using boost::math::iround;
	Color color, hsl;
	color_object_get_color(color_object, &color);
	const char* name = color_object_get_name(color_object);
	int r, g, b, h, s, l;
	r = iround(color.rgb.red * 255);
	g = iround(color.rgb.green * 255);
	b = iround(color.rgb.blue * 255);
	color_rgb_to_hsl(&color, &hsl);
	h = iround(hsl.hsl.hue * 360);
	s = iround(hsl.hsl.saturation * 100);
	l = iround(hsl.hsl.lightness * 100);
	stream << " * " << name
		<< ": #" << hex << r << g << b
		<< ", rgb(" << dec << r << ", " << g << ", " << b
		<< "), hsl(" << dec << h << ", " << s << "%, " << l << "%)" << endl;
}
bool ImportExport::exportCSS()
{
	ofstream f(m_filename, ios::out | ios::trunc);
	if (f.is_open()){
		f << "/**" << endl << " * Generated by Gpick " << gpick_build_version << endl;
		vector<ColorObject*> ordered;
		getOrderedColors(m_color_list, ordered);
		for (auto color: ordered){
			cssColor(color, f);
			if (!f.good()){
				f.close();
				return false;
			}
		}
		f << " */" << endl;
		if (!f.good()){
			f.close();
			return false;
		}
		f.close();
		return true;
	}
	return false;
}
static void htmlColor(ColorObject* color_object, ostream &stream)
{
	using boost::math::iround;
	Color color, text_color;
	color_object_get_color(color_object, &color);
	color_get_contrasting(&color, &text_color);
	const char *name = color_object_get_name(color_object);
	int r, g, b, tr, tg, tb;
	r = iround(color.rgb.red * 255);
	g = iround(color.rgb.green * 255);
	b = iround(color.rgb.blue * 255);
	tr = iround(text_color.rgb.red * 255);
	tg = iround(text_color.rgb.green * 255);
	tb = iround(text_color.rgb.blue * 255);
	stream << "<div style=\"background-color:rgb(" << dec << r << ", " << g << ", " << b << "); color:rgb(" << dec << tr << ", " << tg << ", " << tb << ")\">" << name << "</div>";
}
bool ImportExport::exportHTML()
{
	ofstream f(m_filename, ios::out | ios::trunc);
	if (f.is_open()){
		boost::filesystem::path path(m_filename);
		vector<ColorObject*> ordered;
		getOrderedColors(m_color_list, ordered);
		f << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><title>"
			<< path.filename().string() << "</title>"
			<< "<style>div{float: left; width: 64px; height: 64px; margin: 2px; text-align: center; font-size: 12px; font-family: Arial, Helvetica, sans-serif}</style>"
			<< "</head>"
			<< "<body>" << endl;
		for (auto color: ordered){
			htmlColor(color, f);
			if (!f.good()){
				f.close();
				return false;
			}
		}
		f << "</body></html>" << endl;
		if (!f.good()){
			f.close();
			return false;
		}
		f.close();
		return true;
	}
	return false;
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
		case FileType::unknown:
			return false;
	}
	return false;
}
static void mtlColor(ColorObject* color_object, ostream &stream)
{
	Color color;
	color_object_get_color(color_object, &color);
	const char* name = color_object_get_name(color_object);
	stream << "newmtl " << name << endl;
	stream << "Ns 90.000000" << endl;
	stream << "Ka 0.000000 0.000000 0.000000" << endl;
	stream << "Kd " << color.rgb.red << " " << color.rgb.green << " " << color.rgb.blue << endl;
	stream << "Ks 0.500000 0.500000 0.500000" << endl << endl;
}
bool ImportExport::exportMTL()
{
	ofstream f(m_filename, ios::out | ios::trunc);
	if (f.is_open()){
		vector<ColorObject*> ordered;
		getOrderedColors(m_color_list, ordered);
		for (auto color: ordered){
			mtlColor(color, f);
			if (!f.good()){
				f.close();
				return false;
			}
		}
		f.close();
		return true;
	}
	return false;
}
typedef union FloatInt
{
	float f;
	uint32_t i;
}FloatInt;
static void aseColor(ColorObject* color_object, ostream &stream)
{
	Color color;
	color_object_get_color(color_object, &color);
	const char* name = color_object_get_name(color_object);
	glong name_u16_len = 0;
	gunichar2 *name_u16 = g_utf8_to_utf16(name, -1, 0, &name_u16_len, 0);
	for (glong i = 0; i < name_u16_len; ++i){
		name_u16[i] = UINT16_TO_BE(name_u16[i]);
	}
	uint16_t color_entry = UINT16_TO_BE(0x0001);
	stream.write((char*)&color_entry, 2);
	int32_t block_size = 2 + (name_u16_len + 1) * 2 + 4 + (3 * 4) + 2; //name length + name (zero terminated and 2 bytes per char wide) + color name + 3 float values + color type
	block_size = UINT32_TO_BE(block_size);
	stream.write((char*)&block_size, 4);
	uint16_t name_length = UINT16_TO_BE(uint16_t(name_u16_len + 1));
	stream.write((char*)&name_length, 2);
	stream.write((char*)name_u16, (name_u16_len + 1) * 2);
	stream << "RGB ";
	FloatInt r, g, b;
	r.f = color.rgb.red;
	g.f = color.rgb.green;
	b.f = color.rgb.blue;
	r.i = UINT32_TO_BE(r.i);
	g.i = UINT32_TO_BE(g.i);
	b.i = UINT32_TO_BE(b.i);
	stream.write((char*)&r, 4);
	stream.write((char*)&g, 4);
	stream.write((char*)&b, 4);
	int16_t color_type = UINT16_TO_BE(0);
	stream.write((char*)&color_type, 2);
	g_free(name_u16);
}
bool ImportExport::exportASE()
{
	ofstream f(m_filename, ios::out | ios::trunc | ios::binary);
	if (f.is_open()){
		f << "ASEF"; //magic header
		uint32_t version = UINT32_TO_BE(0x00010000);
		f.write((char*)&version, 4);
		vector<ColorObject*> ordered;
		getOrderedColors(m_color_list, ordered);
		uint32_t blocks = ordered.size();
		blocks = UINT32_TO_BE(blocks);
		f.write((char*)&blocks, 4);
		for (auto color: ordered){
			aseColor(color, f);
			if (!f.good()){
				f.close();
				return false;
			}
		}
		f.close();
		return true;
	}
	return false;
}
bool ImportExport::importASE()
{
	ifstream f(m_filename, ios::binary);
	if (f.is_open()){
		char magic[4];
		f.read(magic, 4);
		if (memcmp(magic, "ASEF", 4) != 0){
			f.close();
			return false;
		}
		uint32_t version;
		f.read((char*)&version, 4);
		version = UINT32_FROM_BE(version);
		uint32_t blocks;
		f.read((char*)&blocks, 4);
		blocks = UINT32_FROM_BE(blocks);
		uint16_t block_type;
		uint32_t block_size;
		int color_supported;
		for (uint32_t i = 0; i < blocks; ++i){
			f.read((char*)&block_type, 2);
			block_type = UINT16_FROM_BE(block_type);
			f.read((char*)&block_size, 4);
			block_size = UINT32_FROM_BE(block_size);
			switch (block_type){
			case 0x0001: //color block
				{
					uint16_t name_length;
					f.read((char*)&name_length, 2);
					name_length = UINT16_FROM_BE(name_length);

					gunichar2 *name_u16 = (gunichar2*)g_malloc(name_length*2);
					f.read((char*)name_u16, name_length*2);
					for (uint32_t j = 0; j < name_length; ++j){
						name_u16[j] = UINT16_FROM_BE(name_u16[j]);
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
						rgb[0].i = UINT32_FROM_BE(rgb[0].i);
						rgb[1].i = UINT32_FROM_BE(rgb[1].i);
						rgb[2].i = UINT32_FROM_BE(rgb[2].i);
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
						cmyk[0].i = UINT32_FROM_BE(cmyk[0].i);
						cmyk[1].i = UINT32_FROM_BE(cmyk[1].i);
						cmyk[2].i = UINT32_FROM_BE(cmyk[2].i);
						cmyk[3].i = UINT32_FROM_BE(cmyk[3].i);
						c2.cmyk.c = cmyk[0].f;
						c2.cmyk.m = cmyk[1].f;
						c2.cmyk.y = cmyk[2].f;
						c2.cmyk.k = cmyk[3].f;
						color_cmyk_to_rgb(&c2, &c);
						color_supported = 1;
					}else if (memcmp(color_space, "Gray", 4) == 0){
						FloatInt gray;
						f.read((char*)&gray, 4);
						gray.i = UINT32_FROM_BE(gray.i);
						c.rgb.red = c.rgb.green = c.rgb.blue = gray.f;
						color_supported = 1;
					}else if (memcmp(color_space, "LAB ", 4) == 0){
						Color c2;
						FloatInt lab[3];
						f.read((char*)&lab[0], 4);
						f.read((char*)&lab[1], 4);
						f.read((char*)&lab[2], 4);
						lab[0].i = UINT32_FROM_BE(lab[0].i);
						lab[1].i = UINT32_FROM_BE(lab[1].i);
						lab[2].i = UINT32_FROM_BE(lab[2].i);
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
						color_object_set_name(color_object, name);
						color_list_add_color_object(m_color_list, color_object, true);
						color_object_release(color_object);
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
	return false;
}

FileType ImportExport::getFileType(const char *filename)
{
	const struct{
		FileType type;
		const char *extension;
	}extensions[] = {
		{FileType::gpa, ".gpa"},
		{FileType::gpl, ".gpl"},
		{FileType::ase, ".ase"},
		{FileType::txt, ".txt"},
		{FileType::mtl, ".mtl"},
		{FileType::css, ".css"},
		{FileType::html, ".html"},
		{FileType::html, ".htm"},
		{FileType::unknown, nullptr},
	};
	boost::filesystem::path path(filename);
	string extension = path.extension().string();
	if (extension.length() == 0)
		return FileType::unknown;
	boost::algorithm::to_lower(extension);
	for (size_t i = 0; extensions[i].type != FileType::unknown; ++i){
		if (extension == extensions[i].extension){
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
		case FileType::mtl:
		case FileType::css:
		case FileType::html:
		case FileType::unknown:
			return false;
	}
	return false;
}

