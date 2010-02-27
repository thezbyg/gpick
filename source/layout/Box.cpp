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

#include "Box.h"

#include <typeinfo>
#include <iostream>
using namespace std;
using namespace math;

namespace layout{

void Box::SetStyle(Style *_style){
	if (style){
		unref(style);
		style=0;
	}
	if (_style){
		style = static_cast<Style*>(_style->ref());
	}
}

void Box::Draw(cairo_t *cr, const Rect2<float>& parent_rect ){
	/*Rect2<float> draw_rect = rect;
	draw_rect.impose( parent_rect );

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, draw_rect.getX(), draw_rect.getY(), draw_rect.getWidth(), draw_rect.getHeight());
	cairo_fill(cr);*/

	DrawChildren(cr, parent_rect);
}

void Box::DrawChildren(cairo_t *cr, const math::Rect2<float>& parent_rect ){
	Rect2<float> child_rect = rect;
	child_rect.impose( parent_rect );

	for (list<Box*>::iterator i = child.begin(); i!=child.end(); i++){
		(*i)->Draw(cr, child_rect);
	}
}

void Box::AddChild(Box* box){
	child.push_back(box);
}

Box::Box(const char* _name, float x, float y, float width, float height){
	style = 0;
	name = _name;
	rect = Rect2<float>(x, y, x+width, y+height);
	helper_only = false;
}

Box::~Box(){
	SetStyle(0);
	for (list<Box*>::iterator i = child.begin(); i!=child.end(); i++){
		unref(*i);
	}
}

Box* Box::GetBoxAt(const Vec2<float>& point){
	if (rect.isInside(point.x, point.y)){
		Vec2<float> transformed_point = Vec2<float>((point.x-rect.getX()) / rect.getWidth(), (point.y-rect.getY()) / rect.getHeight());

		Box* r;
		for (list<Box*>::iterator i = child.begin(); i!=child.end(); i++){
			if ((r = (*i)->GetBoxAt(transformed_point))){
				if (!r->helper_only)
					return r;
			}
		}

		if (typeid(*this)==typeid(Box))		//do not match Box, because it is invisible
			return 0;
		else
			return this;
	}else{
		return 0;
	}
}

void Text::Draw(cairo_t *cr, const Rect2<float>& parent_rect ){
	Rect2<float> draw_rect = rect;
	draw_rect.impose( parent_rect );

	if (text!=""){
		if (helper_only){
			cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL);
		}else{
			cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		}
		if (style){
			cairo_set_font_size(cr, style->font_size * draw_rect.getHeight());
			cairo_set_source_rgb(cr, style->color.rgb.red, style->color.rgb.green, style->color.rgb.blue);
		}else{
			cairo_set_font_size(cr, draw_rect.getHeight());
			cairo_set_source_rgb(cr, 0, 0, 0);
		}

		cairo_text_extents_t extents;
		cairo_text_extents(cr, text.c_str(), &extents);

		cairo_move_to(cr, draw_rect.getX() + draw_rect.getWidth()/2 - (extents.width/2 + extents.x_bearing), draw_rect.getY() + draw_rect.getHeight()/2 - (extents.height/2 + extents.y_bearing));

		cairo_show_text(cr, text.c_str());

		if (style && style->GetBox() == this){
			cairo_rectangle(cr, draw_rect.getX()+1, draw_rect.getY()+1, draw_rect.getWidth()-2, draw_rect.getHeight()-2);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_set_line_width(cr, 2);
			cairo_stroke(cr);
		}
	}

	DrawChildren(cr, parent_rect);
}

void Fill::Draw(cairo_t *cr, const Rect2<float>& parent_rect ){
	Rect2<float> draw_rect = rect;
	draw_rect.impose( parent_rect );

	cairo_set_source_rgb(cr, style->color.rgb.red, style->color.rgb.green, style->color.rgb.blue);
	cairo_rectangle(cr, draw_rect.getX(), draw_rect.getY(), draw_rect.getWidth(), draw_rect.getHeight());
	cairo_fill(cr);

	if (style->GetBox() == this){
		cairo_rectangle(cr, draw_rect.getX()+1, draw_rect.getY()+1, draw_rect.getWidth()-2, draw_rect.getHeight()-2);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_line_width(cr, 2);
		cairo_stroke(cr);
	}

	DrawChildren(cr, parent_rect);
}




}

