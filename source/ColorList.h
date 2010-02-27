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

#ifndef COLORLIST_H_
#define COLORLIST_H_

#include "ColorObject.h"
#include "dynv/DynvSystem.h"
#include <list>

struct ColorList{
	std::list<struct ColorObject*> colors;
	typedef std::list<struct ColorObject*>::iterator iter;
	struct dynvSystem* params;

	int (*on_insert)(struct ColorList* color_list, struct ColorObject* color_object);
	int (*on_delete)(struct ColorList* color_list, struct ColorObject* color_object);
	int (*on_delete_selected)(struct ColorList* color_list);
	int (*on_change)(struct ColorList* color_list, struct ColorObject* color_object);
	int (*on_clear)(struct ColorList* color_list);

	int (*on_get_positions)(struct ColorList* color_list);

	void* userdata;
};

struct ColorList* color_list_new(struct dynvHandlerMap* handler_map);
void color_list_destroy(struct ColorList* color_list);
struct ColorObject* color_list_new_color_object(struct ColorList* color_list, Color* color);
struct ColorObject* color_list_add_color(struct ColorList* color_list, Color* color);
int color_list_add_color_object(struct ColorList* color_list, struct ColorObject* color_object, int add_to_palette);
int color_list_remove_color_object(struct ColorList* color_list, struct ColorObject* color_object);
int color_list_remove_selected(struct ColorList* color_list);
int color_list_remove_all(struct ColorList* color_list);
unsigned long color_list_get_count(struct ColorList* color_list);
int color_list_get_positions(struct ColorList* color_list);

#endif /* COLORLIST_H_ */
