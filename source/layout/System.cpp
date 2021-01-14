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

#include "System.h"
namespace layout {
System::System() {
	m_box = nullptr;
}
System::~System() {
	for (auto i = m_styles.begin(); i != m_styles.end(); i++) {
		Style::unref(*i);
	}
	m_styles.clear();
	Box::unref(m_box);
}
void System::draw(Context *context, const math::Rectangle<float> &parentRect) {
	if (!m_box) return;
	m_box->draw(context, parentRect);
}
void System::addStyle(Style *style) {
	m_styles.push_back(static_cast<Style *>(style->ref()));
}
const std::vector<Style *> &System::styles() const {
	return m_styles;
}
Box *System::box() {
	return m_box;
}
void System::setBox(Box *box) {
	if (m_box) {
		Box::unref(m_box);
		m_box = nullptr;
	}
	m_box = static_cast<Box *>(box->ref());
}
Box *System::getBoxAt(const math::Vector2f &point) {
	if (m_box)
		return m_box->getBoxAt(point);
	else
		return nullptr;
}
Box *System::getNamedBox(const char *name) {
	if (m_box)
		return m_box->getNamedBox(name);
	else
		return nullptr;
}
Box *System::getNamedBox(const std::string &name) {
	if (m_box)
		return m_box->getNamedBox(name.c_str());
	else
		return nullptr;
}
}
