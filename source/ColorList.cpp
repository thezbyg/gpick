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

#include "ColorList.h"
#include "ColorObject.h"
#include <algorithm>
ColorList *color_list_new() {
	auto *colorList = new ColorList();
	colorList->blocked = false;
	colorList->onInsert = nullptr;
	colorList->onDelete = nullptr;
	colorList->onClear = nullptr;
	colorList->onDeleteSelected = nullptr;
	colorList->onGetPositions = nullptr;
	colorList->userdata = nullptr;
	return colorList;
}
ColorList *color_list_new(ColorList *colorList) {
	ColorList *result = color_list_new();
	if (colorList)
		result->options = dynv::Map::create();
	return result;
}
void color_list_destroy(ColorList *colorList) {
	for (auto *colorObject: colorList->colors) {
		colorObject->release();
	}
	colorList->colors.clear();
	delete colorList;
}
ColorObject *color_list_new_color_object(ColorList *colorList, const Color *color) {
	return new ColorObject("", *color);
}
ColorObject *color_list_add_color(ColorList *colorList, const Color *color) {
	auto *colorObject = new ColorObject("", *color);
	int r = color_list_add_color_object(colorList, colorObject, 1);
	if (r == 0) {
		colorObject->release();
		return colorObject;
	} else {
		delete colorObject;
		return 0;
	}
}
int color_list_add_color_object(ColorList *colorList, ColorObject *colorObject, bool addToPalette) {
	colorList->colors.push_back(colorObject->reference());
	if (addToPalette && colorList->onInsert)
		colorList->onInsert(colorList, colorObject, colorList->userdata);
	colorList->changed = true;
	return 0;
}
int color_list_add_color_object(ColorList *colorList, const ColorObject &colorObject, bool addToPalette) {
	ColorObject *reference;
	colorList->colors.push_back((reference = colorObject.copy()));
	if (addToPalette && colorList->onInsert)
		colorList->onInsert(colorList, reference, colorList->userdata);
	colorList->changed = true;
	return 0;
}
int color_list_add(ColorList *colorList, ColorList *items, bool addToPalette) {
	for (auto *colorObject: items->colors) {
		colorList->colors.push_back(colorObject->reference());
		if (addToPalette && colorList->onInsert && colorObject->isVisible())
			colorList->onInsert(colorList, colorObject, colorList->userdata);
		colorList->changed = true;
	}
	return 0;
}
int color_list_remove_color_object(ColorList *colorList, ColorObject *colorObject) {
	auto i = std::find(colorList->colors.begin(), colorList->colors.end(), colorObject);
	if (i != colorList->colors.end()) {
		if (colorList->onDelete)
			colorList->onDelete(colorList, colorObject, colorList->userdata);
		colorList->colors.erase(i);
		colorObject->release();
		colorList->changed = true;
		return 0;
	} else
		return -1;
}
int color_list_remove_selected(ColorList *colorList) {
	auto i = colorList->colors.begin();
	while (i != colorList->colors.end()) {
		if ((*i)->isSelected()) {
			(*i)->release();
			i = colorList->colors.erase(i);
		} else
			++i;
	}
	colorList->onDeleteSelected(colorList, colorList->userdata);
	colorList->changed = true;
	return 0;
}
int color_list_remove_visited(ColorList *colorList) {
	auto i = colorList->colors.begin();
	while (i != colorList->colors.end()) {
		if ((*i)->isVisited()) {
			(*i)->release();
			i = colorList->colors.erase(i);
		} else
			++i;
	}
	return 0;
}
int color_list_reset_selected(ColorList *colorList) {
	for (auto *colorObject: colorList->colors)
		colorObject->setSelected(false);
	return 0;
}
int color_list_reset_all(ColorList *colorList) {
	for (auto *colorObject: colorList->colors) {
		colorObject->setSelected(false);
		colorObject->setVisited(false);
	}
	return 0;
}
int color_list_remove_all(ColorList *colorList) {
	decltype(colorList->colors)::iterator i;
	if (colorList->onClear) {
		for (auto *colorObject: colorList->colors) {
			colorObject->release();
		}
		colorList->colors.clear();
		colorList->onClear(colorList, colorList->userdata);
	} else {
		for (i = colorList->colors.begin(); i != colorList->colors.end(); ++i) {
			if (colorList->onDelete)
				colorList->onDelete(colorList, *i, colorList->userdata);
			(*i)->release();
		}
		colorList->colors.clear();
	}
	colorList->changed = true;
	return 0;
}
size_t color_list_get_count(ColorList *colorList) {
	return colorList->colors.size();
}
int color_list_get_positions(ColorList *colorList) {
	if (colorList->onGetPositions) {
		for (auto *colorObject: colorList->colors) {
			colorObject->resetPosition();
		}
		colorList->onGetPositions(colorList, colorList->userdata);
	} else {
		size_t position = 0;
		for (auto *colorObject: colorList->colors) {
			colorObject->setPosition(position++);
		}
	}
	return 0;
}
bool color_list_start_changes(ColorList *colorList) {
	if (colorList->blocked)
		return false;
	colorList->blocked = true;
	colorList->changed = false;
	return true;
}
bool color_list_end_changes(ColorList *colorList) {
	if (colorList->changed && colorList->onUpdate)
		colorList->onUpdate(colorList, colorList->userdata);
	colorList->blocked = false;
	return true;
}
