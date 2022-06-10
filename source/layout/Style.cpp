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

#include "Style.h"
#include "Box.h"
#include <string>
namespace layout {
Style::Style(std::string_view name, Color color, float fontSize) {
	size_t position = name.find(":");
	if (position != std::string_view::npos) {
		m_name = std::string(name.substr(0, position));
		m_label = std::string(name.substr(position + 1));
	} else {
		m_name = m_label = std::string(name);
	}
	m_type = Type::unknown;
	if ((position = m_name.rfind("_")) != std::string::npos) {
		std::string flags = m_name.substr(position);
		if (flags.find("t") != std::string::npos) {
			m_type = Type::color;
		} else if (flags.find("b") != std::string::npos) {
			m_type = Type::background;
		}
	}
	m_color = color;
	m_fontSize = fontSize;
	m_dirty = true;
	m_textOffset = math::Vector2f(0, 0);
}
Style::~Style() {
}
bool Style::dirty() const {
	return m_dirty;
}
Style &Style::setDirty(bool dirty) {
	m_dirty = dirty;
	return *this;
}
Color Style::color() const {
	return m_color;
}
Style &Style::setColor(Color color) {
	m_color = color;
	return *this;
}
const std::string &Style::name() const {
	return m_name;
}
const std::string &Style::label() const {
	return m_label;
}
Style &Style::setLabel(std::string_view label) {
	m_label = label;
	return *this;
}
Style::Type Style::type() const {
	return m_type;
}
float Style::fontSize() const {
	return m_fontSize;
}
math::Vector2f Style::textOffset() const {
	return m_textOffset;
}
Style &Style::setTextOffset(math::Vector2f textOffset) {
	m_textOffset = textOffset;
	return *this;
}
}
