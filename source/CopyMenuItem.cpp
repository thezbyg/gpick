/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#include "CopyMenuItem.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "Converters.h"
#include "Converter.h"
#include "GlobalState.h"
#include "Clipboard.h"
#include "uiUtilities.h"
struct CopyMenuItemState {
	CopyMenuItemState(Converter *converter, ColorObject *colorObject, GlobalState *gs):
		m_gs(gs),
		m_converter(converter),
		m_colorObject(colorObject),
		m_paletteWidget(nullptr) {
	}
	~CopyMenuItemState() {
		m_colorObject->release();
	}
	void setPaletteWidget(GtkWidget *paletteWidget) {
		m_paletteWidget = paletteWidget;
	}
	static void onReleaseState(CopyMenuItemState *state) {
		delete state;
	}
	static void onActivate(GtkWidget *widget, CopyMenuItemState *state) {
		if (state->m_paletteWidget)
			clipboard::set(state->m_paletteWidget, state->m_gs, state->m_converter);
		else
			clipboard::set(state->m_colorObject, state->m_gs, state->m_converter);
	}
	GlobalState *m_gs;
	Converter *m_converter;
	ColorObject *m_colorObject;
	GtkWidget *m_paletteWidget;
};
GtkWidget *CopyMenuItem::newItem(ColorObject *colorObject, GlobalState *gs, bool includeName) {
	ConverterSerializePosition position;
	auto converter = gs->converters().firstCopyOrAny();
	if (!converter)
		return nullptr;
	auto textLine = converter->serialize(colorObject, position);
	if (includeName) {
		textLine += " - ";
		textLine += colorObject->getName();
	}
	auto item = newMenuItem(textLine.c_str(), GTK_STOCK_COPY);
	auto state = new CopyMenuItemState(converter, colorObject->reference(), gs);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), state);
	g_object_set_data_full(G_OBJECT(item), "item_data", state, (GDestroyNotify)&CopyMenuItemState::onReleaseState);
	return item;
}
GtkWidget *CopyMenuItem::newItem(ColorObject *colorObject, Converter *converter, GlobalState *gs) {
	ConverterSerializePosition position;
	auto textLine = converter->serialize(colorObject, position);
	auto item = newMenuItem(textLine.c_str(), GTK_STOCK_COPY);
	auto state = new CopyMenuItemState(converter, colorObject->reference(), gs);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), state);
	g_object_set_data_full(G_OBJECT(item), "item_data", state, (GDestroyNotify)&CopyMenuItemState::onReleaseState);
	return item;
}
GtkWidget *CopyMenuItem::newItem(ColorObject *colorObject, GtkWidget *paletteWidget, Converter *converter, GlobalState *gs) {
	ConverterSerializePosition position;
	auto textLine = converter->serialize(colorObject, position);
	auto item = newMenuItem(textLine.c_str(), GTK_STOCK_COPY);
	auto state = new CopyMenuItemState(converter, colorObject->reference(), gs);
	state->setPaletteWidget(paletteWidget);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), state);
	g_object_set_data_full(G_OBJECT(item), "item_data", state, (GDestroyNotify)&CopyMenuItemState::onReleaseState);
	return item;
}
