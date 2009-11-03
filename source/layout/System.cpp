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

#include "System.h"

using namespace std;

namespace layout{
	
System::System(){
	box = 0;
}

System::~System(){
	for (list<Style*>::iterator i=styles.begin(); i!=styles.end(); i++){
		Style::unref(*i);
	}
	styles.clear();
	Box::unref(box);
}

void System::Draw(cairo_t *cr, const math::Rect2<float>& parent_rect ){
	if (!box) return;
	box->Draw(cr, parent_rect);	
}

void System::AddStyle(Style *_style){
	styles.push_back(static_cast<Style*>(_style->ref()));	
}

void System::SetBox(Box *_box){
	if (box){
		Box::unref(box);
		box = 0;
	}
	box = static_cast<Box*>(_box->ref());
}

Box* System::GetBoxAt(const math::Vec2<float>& point){
	if (box)
		return box->GetBoxAt(point);
	else
		return 0;
}

}