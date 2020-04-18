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
#include "dynv/DynvSystem.h"
#include <algorithm>
using namespace std;

ColorList* color_list_new()
{
	ColorList* color_list = new ColorList;
	color_list->params = nullptr;
	color_list->on_insert = nullptr;
	color_list->on_change = nullptr;
	color_list->on_delete = nullptr;
	color_list->on_clear = nullptr;
	color_list->on_delete_selected = nullptr;
	color_list->on_get_positions = nullptr;
	color_list->userdata = nullptr;
	return color_list;
}
ColorList* color_list_new(ColorList *color_list)
{
	ColorList *result = color_list_new();
	if (color_list){
		dynvHandlerMap *handler_map = dynv_system_get_handler_map(color_list->params);
		result->params = dynv_system_create(handler_map);
		dynv_handler_map_release(handler_map);
	}else{
		result->params = nullptr;
	}
	return result;
}
ColorList* color_list_new(struct dynvHandlerMap* handler_map)
{
	ColorList *result = color_list_new();
	if (handler_map){
		result->params = dynv_system_create(handler_map);
	}else{
		result->params = nullptr;
	}
	return result;
}
ColorList* color_list_new_with_one_color(ColorList *template_color_list, const Color *color)
{
	ColorList *color_list = color_list_new();
	ColorObject *color_object = new ColorObject("", *color);
	color_list_add_color_object(color_list, color_object, 1);
	return color_list;
}
void color_list_destroy(ColorList* color_list)
{
	for (auto color_object: color_list->colors){
		color_object->release();
	}
	color_list->colors.clear();
	if (color_list->params) dynv_system_release(color_list->params);
	delete color_list;
}
ColorObject* color_list_new_color_object(ColorList* color_list, const Color *color)
{
	return new ColorObject("", *color);
}
ColorObject* color_list_add_color(ColorList *color_list, const Color *color)
{
	ColorObject *color_object = new ColorObject("", *color);
	int r = color_list_add_color_object(color_list, color_object, 1);
	if (r == 0){
		color_object->release();
		return color_object;
	}else{
		delete color_object;
		return 0;
	}
}
int color_list_add_color_object(ColorList *color_list, ColorObject *color_object, bool add_to_palette)
{
	color_list->colors.push_back(color_object->reference());
	if (add_to_palette && color_list->on_insert)
		color_list->on_insert(color_list, color_object);
	return 0;
}
int color_list_add(ColorList *color_list, ColorList *items, bool add_to_palette)
{
	for (auto color_object: items->colors){
		color_list->colors.push_back(color_object->reference());
		if (add_to_palette && color_list->on_insert && color_object->isVisible())
			color_list->on_insert(color_list, color_object);
	}
	return 0;
}
int color_list_remove_color_object(ColorList *color_list, ColorObject *color_object)
{
	list<ColorObject*>::iterator i = std::find(color_list->colors.begin(), color_list->colors.end(), color_object);
	if (i != color_list->colors.end()){
		if (color_list->on_delete) color_list->on_delete(color_list, color_object);
		color_list->colors.erase(i);
		color_object->release();
		return 0;
	}else return -1;
}
int color_list_remove_selected(ColorList *color_list)
{
	ColorList::iter i=color_list->colors.begin();
	while (i != color_list->colors.end()){
		if ((*i)->isSelected()){
			(*i)->release();
			i = color_list->colors.erase(i);
		}else ++i;
	}
	color_list->on_delete_selected(color_list);
	return 0;
}
int color_list_set_selected(ColorList *color_list, bool selected) {
	for (auto &color : color_list->colors)
		color->setSelected(false);
	return 0;
}
int color_list_remove_all(ColorList *color_list)
{
	ColorList::iter i;
	if (color_list->on_clear){
		color_list->on_clear(color_list);
		for (i = color_list->colors.begin(); i != color_list->colors.end(); ++i){
			(*i)->release();
		}
	}else{
		for (i = color_list->colors.begin(); i != color_list->colors.end(); ++i){
			if (color_list->on_delete) color_list->on_delete(color_list, *i);
			(*i)->release();
		}
	}
	color_list->colors.clear();
	return 0;
}
size_t color_list_get_count(ColorList *color_list)
{
	return color_list->colors.size();
}
int color_list_get_positions(ColorList *color_list)
{
	if (color_list->on_get_positions){
		for (auto color: color_list->colors){
			color->resetPosition();
		}
		color_list->on_get_positions(color_list);
	}else{
		size_t position = 0;
		for (auto color: color_list->colors){
			color->setPosition(position++);
		}
	}
	return 0;
}
