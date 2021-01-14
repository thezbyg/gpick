/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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
namespace layout {
Style::Style(const char *_name, Color *_color, float _font_size) {
	std::string name = std::string(_name);
	size_t pos = name.find(":");
	if (pos != std::string::npos) {
		ident_name = name.substr(0, pos);
		label = name.substr(pos + 1);
	} else {
		ident_name = name;
		label = name;
	}
	styleType = Type::unknown;
	if ((pos = ident_name.rfind("_")) != std::string::npos) {
		std::string flags = ident_name.substr(pos);
		if (flags.find("t") != std::string::npos) {
			styleType = Type::color;
		} else if (flags.find("b") != std::string::npos) {
			styleType = Type::background;
		}
	}
	color = *_color;
	font_size = _font_size;
	dirty = true;
	highlight = false;
	selected_box = 0;
}
Style::~Style() {
}
bool Style::isDirty() {
	return dirty;
}
void Style::setDirty(bool _dirty) {
	dirty = _dirty;
}
bool Style::getHighlight() {
	return highlight;
}
Box *Style::getBox() {
	return selected_box;
}
void Style::setState(bool _highlight, Box *box) {
	selected_box = box;
	highlight = _highlight;
}
}
