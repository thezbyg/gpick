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

#include "Box.h"

using namespace std;
using namespace math;

namespace layout{

	
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
	group = 0;
	name = _name;
	rect = Rect2<float>(x, y, x+width, y+height);
	refcnt = 0;
}

Box::~Box(){
	for (list<Box*>::iterator i = child.begin(); i!=child.end(); i++){
		delete (*i);
	}
}

Box* Box::ref(){
	refcnt++;
	return this;
}

bool Box::unref(){
	if (refcnt){
		refcnt--;
		return false;
	}else{
		return true;
	}
}

void Text::Draw(cairo_t *cr, const Rect2<float>& parent_rect ){
	Rect2<float> draw_rect = rect;
	draw_rect.impose( parent_rect );
	
	if (text!=""){
		//cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, font_size * draw_rect.getHeight());
	
		cairo_text_extents_t extents;
		cairo_text_extents(cr, text.c_str(), &extents);
	
		cairo_set_source_rgb(cr, text_color.rgb.red, text_color.rgb.green, text_color.rgb.blue);

		cairo_move_to(cr, draw_rect.getX() + draw_rect.getWidth()/2 - (extents.width/2 + extents.x_bearing), draw_rect.getY() + draw_rect.getHeight()/2 - (extents.height/2 + extents.y_bearing));

		cairo_show_text(cr, text.c_str());
	}
	
	DrawChildren(cr, parent_rect);
}

void Fill::Draw(cairo_t *cr, const Rect2<float>& parent_rect ){
	Rect2<float> draw_rect = rect;
	draw_rect.impose( parent_rect );
	
	cairo_set_source_rgb(cr, background_color.rgb.red, background_color.rgb.green, background_color.rgb.blue);
	cairo_rectangle(cr, draw_rect.getX(), draw_rect.getY(), draw_rect.getWidth(), draw_rect.getHeight());
	cairo_fill(cr);
	
	DrawChildren(cr, parent_rect);
}




}

