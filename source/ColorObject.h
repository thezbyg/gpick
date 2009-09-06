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

#ifndef COLOROBJECT_H_
#define COLOROBJECT_H_

#include "Color.h"
#include "ColorAction.h"
#include "dynv/DynvSystem.h"
#include <list>

struct ColorObject;

struct ColorObject{
	uint32_t refcnt;
	struct dynvSystem* params;
	struct ColorList* childs;	//color objects depending on current object
	uint32_t position;

	//Color color;
	struct ColorAction* action;
	int recalculate;
	int selected;
	int visited;
};

struct ColorObject* color_object_new(struct dynvHandlerMap* handler_map);
int color_object_release(struct ColorObject* color_object);
struct ColorObject* color_object_ref(struct ColorObject* color_object);

int color_object_get_color(struct ColorObject* color_object, Color* color);
int color_object_set_color(struct ColorObject* color_object, Color* color);



#endif /* COLOROBJECT_H_ */
