/*
 * Copyright (c) 2009-2015, Albertas Vyšniauskas
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

#include <memory>
struct ColorNames;
struct Sampler;
struct ScreenReader;
struct ColorList;
struct dynvSystem;
struct lua_State;
struct Random;
struct Converters;
struct ColorSource;
typedef struct _GtkWidget GtkWidget;
namespace layout {
	class Layouts;
}
namespace transformation {
	class Chain;
}
class GlobalState
{
	public:
		GlobalState();
		~GlobalState();
		bool loadSettings();
		bool loadAll();
		bool writeSettings();
		ColorNames *getColorNames();
		Sampler *getSampler();
		ScreenReader *getScreenReader();
		ColorList *getColorList();
		dynvSystem *getSettings();
		lua_State *getLua();
		Random *getRandom();
		Converters *getConverters();
		layout::Layouts *getLayouts();
		transformation::Chain *getTransformationChain();
		GtkWidget *getStatusBar();
		void setStatusBar(GtkWidget *status_bar);
		ColorSource *getCurrentColorSource();
		void setCurrentColorSource(ColorSource *color_source);
	private:
		class Impl;
		std::unique_ptr<Impl> m_impl;
};

#endif /* GPICK_GLOBAL_STATE_H_ */
