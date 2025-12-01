/*
 * Copyright (c) 2009-2025, Albertas Vyšniauskas
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

#include "Names.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "Color.h"
#include "I18N.h"
#include "Paths.h"
#include "dynv/Map.h"
#include "common/MatchPattern.h"
#include <algorithm>
#include <fstream>
#include <vector>
using namespace std::string_literals;
namespace {
static int fromHex(char value) {
	if (value >= '0' && value <= '9')
		return value - '0';
	else if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	else if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	else
		return 0;
}
const Names::InternalDescription descriptions[] = {
	{ "meodai-best", N_("Meodai Best of Names"), 4753 },
	{ "meodai-short", N_("Meodai Short Names"), 2987 },
	{ "meodai-all", N_("Meodai All Names"), 30042 },
	{ "xkcd", N_("XKCD Color Name Survey"), 949 },
};
static_assert(sizeof(descriptions) / sizeof(descriptions[0]) <= Names::maxInternal);
}
common::Span<const Names::InternalDescription> Names::internalNames() {
	return common::Span(descriptions, sizeof(descriptions) / sizeof(descriptions[0]));
}
Names::Names(EventBus &eventBus, const dynv::Map &settings):
	m_eventBus(eventBus),
	m_settings(settings),
	m_imprecisionSuffix(false) {
	m_eventBus.subscribe(EventType::optionsUpdate, *this);
}
Names::~Names() {
	m_eventBus.unsubscribe(*this);
	clear();
}
template<typename OnColor, typename OnExpansion>
void Names::iterate(const Color &color, OnColor onColor, OnExpansion onExpansion) const {
	Color c1 = color.rgbToLabD50();
	int x1, y1, z1, x2, y2, z2;
	getXyz(c1, &x1, &y1, &z1, &x2, &y2, &z2);
	char skipMask[spaceDivisions][spaceDivisions][spaceDivisions] = {};
	/* Search expansion should be from 0 to spaceDivisions, but this would only increase search time and return
	 * wrong color names when no closely matching color is found. Search expansion is only useful
	 * when color name database is very small (16 colors)
	 */
	for (int expansion = 0; expansion < spaceDivisions - 1; ++expansion) {
		int xStart = std::max(x1 - expansion, 0), xEnd = std::min(x2 + expansion, spaceDivisions - 1);
		int yStart = std::max(y1 - expansion, 0), yEnd = std::min(y2 + expansion, spaceDivisions - 1);
		int zStart = std::max(z1 - expansion, 0), zEnd = std::min(z2 + expansion, spaceDivisions - 1);
		for (int x = xStart; x <= xEnd; ++x) {
			for (int y = yStart; y <= yEnd; ++y) {
				for (int z = zStart; z <= zEnd; ++z) {
					if (skipMask[x][y][z])
						continue; // skip checked items
					skipMask[x][y][z] = 1;
					for (auto &entry: m_entries[x][y][z]) {
						float delta = Color::distanceLch(entry.color, c1);
						onColor(entry, delta);
					}
				}
			}
		}
		if (!onExpansion())
			return;
	}
}
std::string Names::get(const Color &color) const {
	float resultDelta = 1e5;
	const Entry *found = nullptr;
	iterate(color, [&](const Entry &entry, float delta) {
		if (delta < resultDelta) {
			resultDelta = delta;
			found = &entry;
		}
	}, [&]() {
		return found == nullptr; // stop further expansion if we have found a match
	});
	if (!found)
		return std::string();
	if (m_imprecisionSuffix && resultDelta > 0.1)
		return found->name + " ~";
	return found->name;
}
void Names::findNearest(const Color &color, size_t count, std::vector<std::pair<const char *, Color>> &colors) {
	std::vector<std::pair<float, const Entry *>> found;
	found.reserve(count);
	iterate(color, [&](const Entry &entry, float delta) {
		found.emplace_back(delta, &entry);
	}, [&]() {
		return found.size() < count;
	});
	std::sort(found.begin(), found.end(), [](auto &left, auto &right) {
		return left.first < right.first;
	});
	colors.clear();
	size_t index = 0;
	for (auto &item: found) {
		if (index >= count)
			break;
		colors.emplace_back(item.second->name.c_str(), item.second->originalColor);
		++index;
	}
}
bool Names::loadFromFile(const std::string &filename) {
	using namespace common::ops;
	std::ifstream file(filename.c_str(), std::ifstream::in);
	if (!file.is_open())
		return false;
	std::string line;
	while (!(file.eof())) {
		std::getline(file, line);
		if (line.empty() || line[0] == '!' || line[0] == '#')
			continue;
		std::string_view matched;
		size_t separatorStart, nameStart;
		if (!common::matchPattern(std::string_view(line), save(count(hex, 6), matched), save(space, separatorStart, nameStart)))
			continue;
		Color color;
		color.red = (fromHex(matched[0]) << 4 | fromHex(matched[1])) * (1 / 255.0f);
		color.green = (fromHex(matched[2]) << 4 | fromHex(matched[3])) * (1 / 255.0f);
		color.blue = (fromHex(matched[4]) << 4 | fromHex(matched[5])) * (1 / 255.0f);
		color.alpha = 1.0f;
		auto xyz = color.rgbToLabD50();
		getEntryVector(xyz).emplace_back(Entry { line.substr(nameStart), xyz, color });
	}
	file.close();
	return true;
}
void Names::loadFromList(const ColorList &colorList) {
	for (auto *colorObject: colorList) {
		auto color = colorObject->getColor();
		auto xyz = color.rgbToLabD50();
		getEntryVector(xyz).emplace_back(Entry { colorObject->getName(), xyz, color });
	}
}
bool Names::loadInternal(const InternalDescription &description) {
	auto path = "names-"s + description.id + ".txt";
	return loadFromFile(buildFilename(path.c_str()));
}
void Names::load() {
	clear();
	m_imprecisionSuffix = m_settings.getBool("gpick.color_names.imprecision_postfix", false);
	if (!m_settings.contains("gpick.color_dictionaries.items")) {
		loadInternal(internalNames()[0]);
		return;
	}
	const auto items = m_settings.getMaps("gpick.color_dictionaries.items");
	for (const auto &item: items) {
		if (!item->getBool("enable", false))
			continue;
		auto internal = item->getBool("built_in", false);
		auto path = item->getString("path", "");
		if (internal && path == "built_in_0") {
			loadInternal(internalNames()[0]);
			return;
		} else if (internal) {
			for (const auto &description: internalNames()) {
				if (path == description.id) {
					if (loadInternal(description))
						break;
				}
			}
		} else {
			loadFromFile(path.c_str());
		}
	}
}
void Names::clear() {
	for (int x = 0; x < spaceDivisions; x++) {
		for (int y = 0; y < spaceDivisions; y++) {
			for (int z = 0; z < spaceDivisions; z++) {
				m_entries[x][y][z].clear();
			}
		}
	}
}
void Names::onEvent(EventType eventType) {
	switch (eventType) {
	case EventType::optionsUpdate:
		m_imprecisionSuffix = m_settings.getBool("gpick.color_names.imprecision_postfix", false);
		break;
	case EventType::convertersUpdate:
	case EventType::displayFiltersUpdate:
	case EventType::colorDictionaryUpdate:
	case EventType::paletteChanged:
		break;
	}
}
void Names::getXyz(const Color &color, int *x1, int *y1, int *z1, int *x2, int *y2, int *z2) const {
	*x1 = math::clamp(int(color.xyz.x / 100 * spaceDivisions - 0.5), 0, spaceDivisions - 1);
	*y1 = math::clamp(int((color.xyz.y + 100) / 200 * spaceDivisions - 0.5), 0, spaceDivisions - 1);
	*z1 = math::clamp(int((color.xyz.z + 100) / 200 * spaceDivisions - 0.5), 0, spaceDivisions - 1);
	*x2 = math::clamp(int(color.xyz.x / 100 * spaceDivisions + 0.5), 0, spaceDivisions - 1);
	*y2 = math::clamp(int((color.xyz.y + 100) / 200 * spaceDivisions + 0.5), 0, spaceDivisions - 1);
	*z2 = math::clamp(int((color.xyz.z + 100) / 200 * spaceDivisions + 0.5), 0, spaceDivisions - 1);
}
std::vector<Names::Entry> &Names::getEntryVector(const Color &color) {
	int x, y, z;
	x = math::clamp(int(color.xyz.x / 100 * spaceDivisions), 0, spaceDivisions - 1);
	y = math::clamp(int((color.xyz.y + 100) / 200 * spaceDivisions), 0, spaceDivisions - 1);
	z = math::clamp(int((color.xyz.z + 100) / 200 * spaceDivisions), 0, spaceDivisions - 1);
	return m_entries[x][y][z];
}
