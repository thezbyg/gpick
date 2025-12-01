/*
 * Copyright (c) 2009-2025, Albertas Vyšniauskas
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
#include "EventBus.h"
#include "dynv/MapFwd.h"
#include "common/Span.h"
#include <string>
#include <vector>
struct ColorList;
struct Names: public IEventHandler {
	static constexpr int spaceDivisions = 8;
	struct InternalDescription {
		const char *id;
		const char *name;
		size_t count;
	};
	static common::Span<const InternalDescription> internalNames();
	static constexpr size_t maxInternal = 4;
	Names(EventBus &eventBus, const dynv::Map &settings);
	virtual ~Names();
	std::string get(const Color &color) const;
	void findNearest(const Color &color, size_t count, std::vector<std::pair<const char *, Color>> &colors);
	bool loadFromFile(const std::string &filename);
	void loadFromList(const ColorList &colorList);
	void load();
	void clear();
private:
	struct Entry {
		std::string name;
		Color color, originalColor;
	};
	EventBus &m_eventBus;
	const dynv::Map &m_settings;
	std::vector<Entry> m_entries[spaceDivisions][spaceDivisions][spaceDivisions];
	bool m_imprecisionSuffix;
	virtual void onEvent(EventType eventType) override;
	void getXyz(const Color &color, int *x1, int *y1, int *z1, int *x2, int *y2, int *z2) const;
	std::vector<Entry> &getEntryVector(const Color &color);
	bool loadInternal(const InternalDescription &description);
	template<typename OnColor, typename OnExpansion>
	void iterate(const Color &color, OnColor onColor, OnExpansion onExpansion) const;
};
