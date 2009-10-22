/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#include "ColorObject.h"

using namespace std;


struct ColorObject* color_object_new(struct dynvHandlerMap* handler_map){
	struct ColorObject* color_object=new struct ColorObject;
	color_object->action=NULL;
	color_object->refcnt=0;
	color_object->selected=0;
	color_object->position=~(uint32_t)0;
	color_object->recalculate=1;
	if (handler_map){
		color_object->params=dynv_system_create(handler_map);
	}else{
		color_object->params=NULL;
	}
	return color_object;
}

int color_object_release(struct ColorObject* color_object){
	if (color_object->refcnt){
		color_object->refcnt--;
		return -1;
	}else{
		if (color_object->params) dynv_system_release(color_object->params);
		delete color_object;
		return 0;
	}
}

struct ColorObject* color_object_ref(struct ColorObject* color_object){
	color_object->refcnt++;
	return color_object;
}

int color_object_get_color(struct ColorObject* color_object, Color* color){
	if (!color_object->action){	
		Color* c=(Color*)dynv_system_get(color_object->params, "color", "color");
		if (c){
			color_copy(c, color);
			return 0;
		}
		return -1;
	}else{
		//action
	}
	return -1;
}

int color_object_set_color(struct ColorObject* color_object, Color* color){
	if (!color_object->action){	
		//color_copy(color, &color_object->color);
		dynv_system_set(color_object->params, "color", "color", color);
		return 0;
	}else{
		//action
	}
	return -1;
}

struct ColorObject* color_object_copy(struct ColorObject* color_object){
	struct ColorObject* new_color_object = color_object_new(0);
	new_color_object->params = dynv_system_copy(color_object->params);
	
	new_color_object->recalculate = color_object->recalculate;
	new_color_object->selected = color_object->selected;
	new_color_object->visited = color_object->visited;
	
	return new_color_object;
}

