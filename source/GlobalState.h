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

#ifndef GPICK_GLOBAL_STATE_H_
#define GPICK_GLOBAL_STATE_H_
#include "dynv/MapFwd.h"
#include <memory>
#include <optional>
#include <cstdint>
struct Names;
struct Sampler;
struct ScreenReader;
struct ColorList;
struct Random;
struct Converters;
struct IColorSource;
struct EventBus;
struct IPalette;
typedef struct _GtkWidget GtkWidget;
namespace layout {
struct Layouts;
}
namespace transformation {
struct Chain;
}
namespace lua {
struct Script;
struct Callbacks;
}
struct GlobalState {
	GlobalState();
	~GlobalState();
	bool loadSettings();
	bool loadAll();
	bool writeSettings();
	Names &names();
	Sampler *getSampler();
	ScreenReader *getScreenReader();
	ColorList &colorList();
	ColorList &initializeColorList(IPalette &palette);
	dynv::Map &settings();
	lua::Script &script();
	lua::Callbacks &callbacks();
	Converters &converters();
	Random *getRandom();
	layout::Layouts &layouts();
	transformation::Chain &transformationChain();
	GtkWidget *getStatusBar();
	void setStatusBar(GtkWidget *status_bar);
	IColorSource *getCurrentColorSource();
	void setCurrentColorSource(IColorSource *color_source);
	std::optional<uint32_t> latinKeysGroup;
	EventBus &eventBus();
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
#endif /* GPICK_GLOBAL_STATE_H_ */
