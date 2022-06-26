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

#pragma once
#include "Color.h"
#include "common/Ref.h"
#include "common/Guard.h"
#include <vector>
#include <cstddef>
struct ColorObject;
struct IPalette;
struct ColorList: public common::Ref<ColorList>::Counter {
	using value_type = ColorList *;
	using iterator = std::vector<ColorObject *>::iterator;
	ColorList();
	ColorList(IPalette &palette);
	ColorList(const ColorList &) = delete;
	ColorList(ColorList &&colorList);
	ColorList &operator=(const ColorList &) = delete;
	virtual ~ColorList();
	size_t size() const;
	bool empty() const;
	void add(const ColorObject &colorObject);
	void add(ColorObject *colorObject);
	void add(ColorObject *colorObject, size_t position, bool updatePalette = false);
	void add(ColorList &colorList);
	template<typename Callback>
	void remove(Callback &&callback, bool selected, bool updatePalette) {
		auto i = m_colors.begin();
		while (i != m_colors.end()) {
			if (callback(*i)) {
				if (updatePalette && !selected) {
					paletteRemove(*i);
				}
				releaseItem(*i);
				i = m_colors.erase(i);
			} else
				++i;
		}
		if (updatePalette && selected)
			paletteRemoveSelected();
		m_changed = true;
	}
	void removeAll();
	bool startChanges();
	bool endChanges();
	bool blocked() const;
	bool changed() const;
	std::vector<ColorObject *>::iterator begin();
	std::vector<ColorObject *>::iterator end();
	std::vector<ColorObject *>::const_iterator begin() const;
	std::vector<ColorObject *>::const_iterator end() const;
	std::vector<ColorObject *>::reverse_iterator rbegin();
	std::vector<ColorObject *>::reverse_iterator rend();
	std::vector<ColorObject *>::const_reverse_iterator rbegin() const;
	std::vector<ColorObject *>::const_reverse_iterator rend() const;
	ColorObject *&front();
	ColorObject *&back();
	common::Guard<void (*)(ColorList *), ColorList *> changeGuard();
	[[nodiscard]] static common::Ref<ColorList> newList();
	[[nodiscard]] static common::Ref<ColorList> newList(IPalette &palette);
private:
	std::vector<ColorObject *> m_colors;
	IPalette &m_palette;
	bool m_blocked, m_changed;
	static void onEndChanges(ColorList *colorList);
	void releaseItem(ColorObject *colorObject);
	void paletteRemoveSelected();
	void paletteRemove(ColorObject *colorObject);
};
