/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#include "ColorObject.h"
ColorObject::ColorObject():
	m_name(),
	m_color(),
	m_position(0),
	m_positionSet(false),
	m_selected(false),
	m_visited(false),
	m_visible(true) {
}
ColorObject::ColorObject(const Color &color):
	m_color(color),
	m_position(0),
	m_positionSet(false),
	m_selected(false),
	m_visited(false),
	m_visible(true) {
}
ColorObject::ColorObject(std::string_view name, const Color &color):
	m_name(name),
	m_color(color),
	m_position(0),
	m_positionSet(false),
	m_selected(false),
	m_visited(false),
	m_visible(true) {
}
ColorObject::ColorObject(const ColorObject &colorObject):
	m_name(colorObject.m_name),
	m_color(colorObject.m_color),
	m_position(colorObject.m_position),
	m_positionSet(colorObject.m_positionSet),
	m_selected(colorObject.m_selected),
	m_visited(colorObject.m_visited),
	m_visible(colorObject.m_visible) {
}
ColorObject &ColorObject::operator=(const ColorObject &colorObject) {
	m_name = colorObject.m_name;
	m_color = colorObject.m_color;
	m_position = colorObject.m_position;
	m_positionSet = colorObject.m_positionSet;
	m_selected = colorObject.m_selected;
	m_visited = colorObject.m_visited;
	m_visible = colorObject.m_visible;
	return *this;
}
const Color &ColorObject::getColor() const {
	return m_color;
}
void ColorObject::setColor(const Color &color) {
	m_color = color;
}
const std::string &ColorObject::getName() const {
	return m_name;
}
void ColorObject::setName(const std::string &name) {
	m_name = name;
}
[[nodiscard]] common::Ref<ColorObject> ColorObject::copy() const {
	return common::Ref(new ColorObject(*this));
}
bool ColorObject::isSelected() const {
	return m_selected;
}
bool ColorObject::isVisited() const {
	return m_visited;
}
size_t ColorObject::getPosition() const {
	return m_position;
}
bool ColorObject::isPositionSet() const {
	return m_positionSet;
}
bool ColorObject::isVisible() const {
	return m_visible;
}
void ColorObject::setPosition(size_t position) {
	m_position = position;
	m_positionSet = true;
}
void ColorObject::resetPosition() {
	m_positionSet = false;
}
void ColorObject::setSelected(bool selected) {
	m_selected = selected;
}
void ColorObject::setVisited(bool visited) {
	m_visited = visited;
}
void ColorObject::setVisible(bool visible) {
	m_visible = visible;
}
