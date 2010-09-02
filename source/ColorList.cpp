/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#include <algorithm>
using namespace std;

struct ColorList* color_list_new(struct dynvHandlerMap* handler_map){
	struct ColorList* color_list=new struct ColorList;
	if (handler_map){
		color_list->params=dynv_system_create(handler_map);
	}else{
		color_list->params=NULL;
	}
	color_list->on_insert=NULL;
	color_list->on_change=NULL;
	color_list->on_delete=NULL;
	color_list->on_clear=NULL;
	color_list->on_delete_selected=NULL;
	color_list->on_get_positions=NULL;
	color_list->userdata=NULL;


	return color_list;
}

void color_list_destroy(struct ColorList* color_list){
	ColorList::iter i;
	for (i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
		color_object_release(*i);
	}
	color_list->colors.clear();
	if (color_list->params) dynv_system_release(color_list->params);
	delete color_list;
}

struct ColorObject* color_list_new_color_object(struct ColorList* color_list, Color* color){
	struct dynvHandlerMap* handler_map;
	if (color_list->params){
		handler_map=dynv_system_get_handler_map(color_list->params);
	}else{
		handler_map=NULL;
	}
	struct ColorObject *color_object=color_object_new(handler_map);
	if (handler_map) dynv_handler_map_release(handler_map);
	color_object_set_color(color_object, color);

	return color_object;
}

struct ColorObject* color_list_add_color(struct ColorList* color_list, Color* color){
	struct dynvHandlerMap* handler_map;
	if (color_list->params){
		handler_map=dynv_system_get_handler_map(color_list->params);
	}else{
		handler_map=NULL;
	}
	struct ColorObject *color_object=color_object_new(handler_map);
	if (handler_map) dynv_handler_map_release(handler_map);
	color_object_set_color(color_object, color);
	int r= color_list_add_color_object(color_list, color_object, 1);
	if (r==0){
		color_object_release(color_object);
		return color_object;
	}else{
		delete color_object;
		return 0;
	}
}

int color_list_add_color_object(struct ColorList* color_list, struct ColorObject* color_object, int add_to_palette){
	color_list->colors.push_back(color_object_ref(color_object));
	if (add_to_palette) if (color_list->on_insert) color_list->on_insert(color_list, color_object);
	return 0;
}

int color_list_remove_color_object(struct ColorList* color_list, struct ColorObject* color_object){
	list<struct ColorObject*>::iterator i = std::find(color_list->colors.begin(), color_list->colors.end(), color_object);
  if (i != color_list->colors.end()){
		if (color_list->on_delete) color_list->on_delete(color_list, color_object);
		color_list->colors.erase(i);
		color_object_release(color_object);
		return 0;
	}else return -1;
}

int color_list_remove_selected(struct ColorList* color_list){
	ColorList::iter i=color_list->colors.begin();
	while (i!=color_list->colors.end()){
		if ((*i)->selected){
			color_object_release(*i);
			i=color_list->colors.erase(i);
		}else ++i;
	}
	color_list->on_delete_selected(color_list);
	return 0;
}

int color_list_remove_all(struct ColorList* color_list){
	ColorList::iter i;
	if (color_list->on_clear){
		color_list->on_clear(color_list);
		for (i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
			color_object_release(*i);
		}
	}else{
		for (i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
			if (color_list->on_delete) color_list->on_delete(color_list, *i);
			color_object_release(*i);
		}
	}

	color_list->colors.clear();
	return 0;
}

unsigned long color_list_get_count(struct ColorList* color_list){
	return color_list->colors.size();
}

int color_list_get_positions(struct ColorList* color_list){
	ColorList::iter i;
	for (i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
		(*i)->position=~(uint32_t)0;
	}
	if (color_list->on_get_positions) color_list->on_get_positions(color_list);
	return 0;
}


