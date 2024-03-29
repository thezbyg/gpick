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
#include "Style.h"
#include "Box.h"
namespace layout {
System::System():
	m_selectable(true) {
}
System::~System() {
}
void System::draw(Context &context, const math::Rectanglef &parentRect) {
	if (!m_box)
		return;
	m_box->draw(context, parentRect);
}
void System::addStyle(common::Ref<Style> style) {
	m_styles.push_back(style);
}
std::vector<common::Ref<Style>> &System::styles() {
	return m_styles;
}
const std::vector<common::Ref<Style>> &System::styles() const {
	return m_styles;
}
common::Ref<Box> System::box() {
	return m_box;
}
void System::setBox(common::Ref<Box> box) {
	m_box = box;
}
common::Ref<Box> System::getBoxAt(const math::Vector2f &point) {
	if (m_box)
		return m_box->getBoxAt(point);
	else
		return common::nullRef;
}
common::Ref<Box> System::getNamedBox(std::string_view name) {
	if (m_box)
		return m_box->getNamedBox(name);
	else
		return common::nullRef;
}
void System::setSelected(common::Ref<Box> box) {
	m_selectedBox = box;
}
const common::Ref<Box> &System::selectedBox() const {
	return m_selectedBox;
}
const bool System::selectable() const {
	return m_selectable;
}
void System::setSelectable(bool selectable) {
	m_selectable = selectable;
}
}
