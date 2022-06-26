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
void ColorList::add(ColorObject *colorObject) {
	m_colors.push_back(colorObject->reference());
	m_palette.add(*this, colorObject);
	m_changed = true;
}
void ColorList::add(ColorObject *colorObject, size_t position, bool updatePalette) {
	m_colors.insert(m_colors.begin() + position, colorObject->reference());
	if (updatePalette)
		m_palette.add(*this, colorObject);
	m_changed = true;
}
void ColorList::add(const ColorObject &colorObject) {
	auto *copy = colorObject.copy().unwrap();
	m_colors.push_back(copy);
	m_palette.add(*this, copy);
	m_changed = true;
}
void ColorList::add(ColorList &colorList) {
	common::Guard colorListGuard = changeGuard();
	for (auto *colorObject: colorList) {
		m_colors.push_back(colorObject->reference());
		m_palette.add(*this, colorObject);
		m_changed = true;
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
ColorObject *&ColorList::back() {
	return m_colors.back();
}
common::Guard<void (*)(ColorList *), ColorList *> ColorList::changeGuard() {
	return common::Guard(startChanges(), ColorList::onEndChanges, this);
}
void ColorList::onEndChanges(ColorList *colorList) {
	colorList->endChanges();
}
void ColorList::releaseItem(ColorObject *colorObject) {
	colorObject->release();
}
void ColorList::paletteRemoveSelected() {
	m_palette.removeSelected(*this);
}
void ColorList::paletteRemove(ColorObject *colorObject) {
	m_palette.remove(*this, colorObject);
}
