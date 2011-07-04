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

#ifndef LAYOUT_BOX_H_
#define LAYOUT_BOX_H_

#include "../Color.h"
#include "../Rect2.h"
#include "../Vector2.h"

#include "ReferenceCounter.h"
#include "Style.h"
#include "Context.h"

#include <gtk/gtk.h>

#include <string>
#include <list>

namespace layout{

class Box: public ReferenceCounter{
public:
	std::string name;
	Style *style;
	bool helper_only;
	bool locked;

	math::Rect2<float> rect;

	std::list<Box*> child;
	virtual void Draw(Context *context, const math::Rect2<float>& parent_rect );
	void DrawChildren(Context *context, const math::Rect2<float>& parent_rect );
	void AddChild(Box* box);

	void SetStyle(Style *style);

	Box* GetBoxAt(const math::Vec2<float>& point);
	Box* GetNamedBox(const char *name);

	Box(const char* name, float x, float y, float width, float height);
	virtual ~Box();

};

class Text:public Box{
public:
	std::string text;

	virtual void Draw(Context *context, const math::Rect2<float>& parent_rect );
	Text(const char* name, float x, float y, float width, float height):Box(name,x,y,width,height){
	};
};

class Fill:public Box{
public:
	virtual void Draw(Context *context, const math::Rect2<float>& parent_rect );
	Fill(const char* name, float x, float y, float width, float height):Box(name,x,y,width,height){
	};
};



}

#endif /* LAYOUT_BOX_H_ */
