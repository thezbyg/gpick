/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#include "ColorList.h"
#include "ColorObject.h"
#include "IPalette.h"
#include <algorithm>
namespace {
struct NullPalette: public IPalette {
	virtual ~NullPalette() {
	}
	virtual void add(ColorList &colorList, ColorObject *colorObject) override {
	}
	virtual void remove(ColorList &colorList, ColorObject *colorObject) override {
	}
	virtual void removeSelected(ColorList &colorList) override {
	}
	virtual void clear(ColorList &colorList) override {
	}
	virtual void getPositions(ColorList &colorList) override {
	}
	virtual void update(ColorList &colorList) override {
	}
};
static NullPalette nullPalette = {};
}
ColorList::ColorList():
	m_palette(nullPalette),
	m_blocked(false),
	m_changed(false) {
}
ColorList::ColorList(IPalette &palette):
	m_palette(palette),
	m_blocked(false),
	m_changed(false) {
}
ColorList::~ColorList() {
	for (auto *colorObject: m_colors) {
		colorObject->release();
	}
	m_colors.clear();
}
ColorList::ColorList(ColorList &&colorList):
	Counter(std::move(colorList)),
	m_colors(std::move(colorList.m_colors)),
	m_palette(colorList.m_palette),
	m_blocked(colorList.m_blocked),
	m_changed(colorList.m_changed) {
}
[[nodiscard]] common::Ref<ColorList> ColorList::newList() {
	return common::Ref<ColorList>(new ColorList());
}
[[nodiscard]] common::Ref<ColorList> ColorList::newList(IPalette &palette) {
	return common::Ref<ColorList>(new ColorList(palette));
}
void ColorList::add(ColorObject *colorObject, bool addToPalette) {
	m_colors.push_back(colorObject->reference());
	if (addToPalette)
		m_palette.add(*this, colorObject);
	m_changed = true;
}
void ColorList::add(const ColorObject &colorObject, bool addToPalette) {
	auto *copy = colorObject.copy().unwrap();
	m_colors.push_back(copy);
	if (addToPalette)
		m_palette.add(*this, copy);
	m_changed = true;
}
void ColorList::add(ColorList &colorList, bool addToPalette) {
	common::Guard colorListGuard = changeGuard();
	for (auto *colorObject: colorList) {
		m_colors.push_back(colorObject->reference());
		if (addToPalette && colorObject->isVisible())
			m_palette.add(*this, colorObject);
		m_changed = true;
	}
}
void ColorList::getPositions() {
	if (&m_palette != &nullPalette) {
		for (auto *colorObject: m_colors) {
			colorObject->resetPosition();
		}
		m_palette.getPositions(*this);
	} else {
		size_t position = 0;
		for (auto *colorObject: m_colors) {
			colorObject->setPosition(position++);
		}
	}
}
bool ColorList::startChanges() {
	if (m_blocked)
		return false;
	m_blocked = true;
	m_changed = false;
	return true;
}
bool ColorList::endChanges() {
	if (m_changed)
		m_palette.update(*this);
	m_blocked = false;
	return true;
}
bool ColorList::blocked() const {
	return m_blocked;
}
bool ColorList::changed() const {
	return m_changed;
}
size_t ColorList::size() const {
	return m_colors.size();
}
bool ColorList::empty() const {
	return m_colors.empty();
}
void ColorList::removeSelected() {
	auto i = m_colors.begin();
	while (i != m_colors.end()) {
		if ((*i)->isSelected()) {
			(*i)->release();
			i = m_colors.erase(i);
		} else
			++i;
	}
	m_palette.removeSelected(*this);
	m_changed = true;
}
void ColorList::removeVisited() {
	auto i = m_colors.begin();
	while (i != m_colors.end()) {
		if ((*i)->isVisited()) {
			(*i)->release();
			i = m_colors.erase(i);
		} else
			++i;
	}
}
void ColorList::resetSelected() {
	for (auto *colorObject: m_colors)
		colorObject->setSelected(false);
}
void ColorList::resetAll() {
	for (auto *colorObject: m_colors) {
		colorObject->setSelected(false);
		colorObject->setVisited(false);
	}
}
void ColorList::removeAll() {
	for (auto *colorObject: m_colors) {
		colorObject->release();
	}
	m_colors.clear();
	m_palette.clear(*this);
	m_changed = true;
}
std::vector<ColorObject *>::iterator ColorList::begin() {
	return m_colors.begin();
}
std::vector<ColorObject *>::iterator ColorList::end() {
	return m_colors.end();
}
std::vector<ColorObject *>::const_iterator ColorList::begin() const {
	return m_colors.begin();
}
std::vector<ColorObject *>::const_iterator ColorList::end() const {
	return m_colors.end();
}
std::vector<ColorObject *>::reverse_iterator ColorList::rbegin() {
	return m_colors.rbegin();
}
std::vector<ColorObject *>::reverse_iterator ColorList::rend() {
	return m_colors.rend();
}
std::vector<ColorObject *>::const_reverse_iterator ColorList::rbegin() const {
	return m_colors.rbegin();
}
std::vector<ColorObject *>::const_reverse_iterator ColorList::rend() const {
	return m_colors.rend();
}
ColorObject *&ColorList::front() {
	return m_colors.front();
}
common::Guard<void (*)(ColorList *), ColorList *> ColorList::changeGuard() {
	return common::Guard(startChanges(), ColorList::onEndChanges, this);
}
void ColorList::onEndChanges(ColorList *colorList) {
	colorList->endChanges();
}
