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

#include "ColorNames.h"
#include "Color.h"
#include "Paths.h"
#include "dynv/Map.h"
#include <string.h>
#include <sstream>
#include <fstream>
#include <functional>
#include <list>
#include <algorithm>
#include <map>
using namespace std;

struct ColorNameEntry
{
	std::string name;
};
struct ColorEntry
{
	Color color;
	Color original_color;
	ColorNameEntry* name;
};
const int SpaceDivisions = 8;
struct ColorNames
{
	std::list<ColorNameEntry*> names;
	std::vector<ColorEntry*> colors[SpaceDivisions][SpaceDivisions][SpaceDivisions];
};
ColorNames* color_names_new()
{
	ColorNames* color_names = new ColorNames;
	return color_names;
}
void color_names_clear(ColorNames *color_names)
{
	for (auto i = color_names->names.begin(); i != color_names->names.end(); i++){
		delete *i;
	}
	color_names->names.clear();
	for (int x = 0; x < SpaceDivisions; x++){
		for (int y = 0; y < SpaceDivisions; y++){
			for (int z = 0; z < SpaceDivisions; z++){
				for (auto i = color_names->colors[x][y][z].begin(); i != color_names->colors[x][y][z].end(); ++i){
					delete *i;
				}
				color_names->colors[x][y][z].clear();
			}
		}
	}
}
static void color_names_strip_spaces(string& string_x, const string& strip_chars)
{
	if (string_x.empty()) return;
	if (strip_chars.empty()) return;
	size_t start_index = string_x.find_first_not_of(strip_chars);
	size_t end_index = string_x.find_last_not_of(strip_chars);
	if ((start_index == string::npos) || (end_index == string::npos)){
		string_x.erase();
		return;
	}
	string_x = string_x.substr(start_index, (end_index - start_index) + 1);
}
void color_names_normalize(const Color &color, Color &out)
{
	out.xyz.x = color.xyz.x / 100.0f;
	out.xyz.y = (color.xyz.y + 86.1825f) / (86.1825f + 98.2346f);
	out.xyz.z = (color.xyz.z + 107.86f) / (107.86f + 94.478f);
}
void color_names_get_color_xyz(ColorNames* color_names, Color* c, int* x1, int* y1, int* z1, int* x2, int* y2, int* z2)
{
	*x1 = math::clamp(int(c->xyz.x / 100 * SpaceDivisions - 0.5), 0, SpaceDivisions - 1);
	*y1 = math::clamp(int((c->xyz.y + 100) / 200 * SpaceDivisions - 0.5), 0, SpaceDivisions - 1);
	*z1 = math::clamp(int((c->xyz.z + 100) / 200 * SpaceDivisions - 0.5), 0, SpaceDivisions - 1);
	*x2 = math::clamp(int(c->xyz.x / 100 * SpaceDivisions + 0.5), 0, SpaceDivisions - 1);
	*y2 = math::clamp(int((c->xyz.y + 100) / 200 * SpaceDivisions + 0.5), 0, SpaceDivisions - 1);
	*z2 = math::clamp(int((c->xyz.z + 100) / 200 * SpaceDivisions + 0.5), 0, SpaceDivisions - 1);
}
static vector<ColorEntry*>* color_names_get_color_list(ColorNames* color_names, Color* c)
{
	int x,y,z;
	x = math::clamp(int(c->xyz.x / 100 * SpaceDivisions), 0, SpaceDivisions - 1);
	y = math::clamp(int((c->xyz.y + 100) / 200 * SpaceDivisions), 0, SpaceDivisions - 1);
	z = math::clamp(int((c->xyz.z + 100) / 200 * SpaceDivisions), 0, SpaceDivisions - 1);
	return &color_names->colors[x][y][z];
}
int color_names_load_from_file(ColorNames* color_names, const std::string &filename)
{
	ifstream file(filename.c_str(), ifstream::in);
	if (file.is_open()){
		string line;
		stringstream rline (ios::in | ios::out);
		Color color;
		string name;
		while (!(file.eof())){
			getline(file, line);
			if (line.empty()) continue;
			if (line.at(0) == '!') continue;
			rline.clear();
			rline.str(line);
			rline >> color.red >> color.green >> color.blue;
			getline(rline, name);
			const string strip_chars = " \t,.\n\r";
			color_names_strip_spaces(name, strip_chars);
			string::iterator i(name.begin());
			if (i != name.end()){
				name[0] = toupper((unsigned char)name[0]);
				while(++i != name.end()){
					*i = tolower((unsigned char)*i);
				}
				color *= 1 / 255.0f;
				color.alpha = 1;
				ColorNameEntry* name_entry = new ColorNameEntry;
				name_entry->name = name;
				color_names->names.push_back(name_entry);
				ColorEntry* color_entry = new ColorEntry;
				color_entry->name = name_entry;
				color_entry->color = color.rgbToLabD50();
				color_entry->original_color = color;
				color_names_get_color_list(color_names, &color_entry->color)->push_back(color_entry);
			}
		}
		file.close();
		return 0;
	}
	return -1;
}
void color_names_destroy(ColorNames* color_names)
{
	color_names_clear(color_names);
	delete color_names;
}
static void color_names_iterate(ColorNames* color_names, const Color* color, function<bool(ColorEntry*, float)> on_color, function<bool()> on_expansion)
{
	Color c1 = color->rgbToLabD50();
	int x1, y1, z1, x2, y2, z2;
	color_names_get_color_xyz(color_names, &c1, &x1, &y1, &z1, &x2, &y2, &z2);
	char skip_mask[SpaceDivisions][SpaceDivisions][SpaceDivisions];
	memset(&skip_mask, 0, sizeof(skip_mask));
	/* Search expansion should be from 0 to SpaceDivisions, but this would only increase search time and return
	 * wrong color names when no closely matching color is found. Search expansion is only useful
	 * when color name database is very small (16 colors)
	 */
	for (int expansion = 0; expansion < SpaceDivisions - 1; ++expansion){
		int x_start = std::max(x1 - expansion, 0), x_end = std::min(x2 + expansion, SpaceDivisions - 1);
		int y_start = std::max(y1 - expansion, 0), y_end = std::min(y2 + expansion, SpaceDivisions - 1);
		int z_start = std::max(z1 - expansion, 0), z_end = std::min(z2 + expansion, SpaceDivisions - 1);
		for (int x_i = x_start; x_i <= x_end; ++x_i){
			for (int y_i = y_start; y_i <= y_end; ++y_i){
				for (int z_i = z_start; z_i <= z_end; ++z_i){
					if (skip_mask[x_i][y_i][z_i]) continue; // skip checked items
					skip_mask[x_i][y_i][z_i] = 1;
					for (auto i = color_names->colors[x_i][y_i][z_i].begin(); i != color_names->colors[x_i][y_i][z_i].end(); ++i){
						float delta = Color::distanceLch((*i)->color, c1);
						if (!on_color(*i, delta)) return;
					}
				}
			}
		}
		if (on_expansion && !on_expansion()) return;
	}
}
string color_names_get(ColorNames* color_names, const Color* color, bool imprecision_postfix)
{
	float result_delta = 1e5;
	ColorEntry* found_color_entry = nullptr;
	color_names_iterate(color_names, color, [&](ColorEntry *color_entry, float delta){
		if (delta < result_delta){
			result_delta = delta;
			found_color_entry = color_entry;
		}
		return true;
	}, [&](){
		return found_color_entry == nullptr; // stop further expansion if we have found a match
	});
	if (found_color_entry){
		stringstream s;
		s << found_color_entry->name->name;
		if (imprecision_postfix) if (result_delta > 0.1) s << " ~";
		return s.str();
	}
	return string("");
}
void color_names_load(ColorNames *color_names, const dynv::Map &params) {
	if (!params.contains("color_dictionaries.items")) {
		color_names_load_from_file(color_names, buildFilename("color_dictionary_0.txt"));
		return;
	}
	const auto items = params.getMaps("color_dictionaries.items");
	for (const auto &item: items) {
		if (!item->getBool("enable", false))
			continue;
		auto builtIn = item->getBool("built_in", false);
		auto path = item->getString("path", "");
		if (builtIn) {
			if (path == "built_in_0") {
				color_names_load_from_file(color_names, buildFilename("color_dictionary_0.txt"));
			}
		} else {
			color_names_load_from_file(color_names, path.c_str());
		}
	}
}
void color_names_find_nearest(ColorNames *color_names, const Color &color, size_t count, std::vector<std::pair<const char*, Color>> &colors)
{
	multimap<float, ColorEntry*> found_colors;
	color_names_iterate(color_names, &color, [&](ColorEntry *color_entry, float delta){
		found_colors.insert(pair<float, ColorEntry*>(delta, color_entry));
		return true;
	}, [&](){
		return found_colors.size() < count;
	});
	if (found_colors.size() >= count)
		colors.resize(count);
	else
		colors.resize(found_colors.size());
	size_t index = 0;
	for (auto &item: found_colors){
		if (index >= count) break;
		colors[index++] = pair<const char*, Color>(item.second->name->name.c_str(), item.second->original_color);
	}
}
