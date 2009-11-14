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

#include <string>
#include <iostream>
using namespace std;
using namespace math;

namespace layout{

	
Style::Style(const char* _name, Color* _color, float _font_size){
	string name = string(_name);
	
	size_t pos = name.find(":");
	if (pos != string::npos){
		ident_name = name.substr(0, pos);
		human_name = name.substr(pos+1);
	}else{
		ident_name = name;
		human_name = name;
	}
	
	style_type = TYPE_UNKNOWN;
	
	if ((pos = ident_name.rfind("_")) != string::npos){
		string flags = ident_name.substr(pos);
		
		if (flags.find("t") != string::npos){
			style_type = TYPE_COLOR;
		}else if (flags.find("b") != string::npos){
			style_type = TYPE_BACKGROUND;
		}
	}
		
	color_copy(_color, &color);
	font_size = _font_size;
	
	dirty = true;
	highlight = false;
	selected_box = 0;
}
	
Style::~Style(){

}

bool Style::IsDirty(){
	return dirty;
}

void Style::SetDirty(bool _dirty){
	dirty = _dirty;
}

bool Style::GetHighlight(){
	return highlight;
}

Box* Style::GetBox(){
	return selected_box;
}

void Style::SetState(bool _highlight, Box *box){
	selected_box = box;
	highlight = _highlight;
}




}

