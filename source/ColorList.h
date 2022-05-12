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

#ifndef GPICK_COLOR_LIST_H_
#define GPICK_COLOR_LIST_H_
#include "Color.h"
#include "dynv/Map.h"
#include <list>
#include <cstddef>
struct ColorObject;
struct ColorList {
	std::list<ColorObject *> colors;
	dynv::Ref options;
	bool blocked, changed;
	int (*onInsert)(ColorList *colorList, ColorObject *colorObject, void *userdata);
	int (*onDelete)(ColorList *colorList, ColorObject *colorObject, void *userdata);
	int (*onDeleteSelected)(ColorList *colorList, void *userdata);
	int (*onClear)(ColorList *colorList, void *userdata);
	int (*onGetPositions)(ColorList *colorList, void *userdata);
	int (*onUpdate)(ColorList *colorList, void *userdata);
	void *userdata;
};
ColorList *color_list_new();
ColorList *color_list_new(ColorList *colorList);
void color_list_destroy(ColorList *colorList);
ColorObject *color_list_new_color_object(ColorList *colorList, const Color *color);
ColorObject *color_list_add_color(ColorList *colorList, const Color *color);
int color_list_add_color_object(ColorList *colorList, ColorObject *colorObject, bool addToPalette);
int color_list_add_color_object(ColorList *colorList, const ColorObject &colorObject, bool addToPalette);
int color_list_add(ColorList *colorList, ColorList *items, bool addToPalette);
int color_list_remove_color_object(ColorList *colorList, ColorObject *colorObject);
int color_list_remove_selected(ColorList *colorList);
int color_list_remove_visited(ColorList *colorList);
int color_list_reset_selected(ColorList *colorList);
int color_list_reset_all(ColorList *colorList);
int color_list_remove_all(ColorList *colorList);
bool color_list_start_changes(ColorList *colorList);
bool color_list_end_changes(ColorList *colorList);
size_t color_list_get_count(ColorList *colorList);
int color_list_get_positions(ColorList *colorList);
#endif /* GPICK_COLOR_LIST_H_ */
