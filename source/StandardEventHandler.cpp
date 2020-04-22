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

#include "StandardEventHandler.h"
#include "StandardMenu.h"
#include "Clipboard.h"
#include "uiColorInput.h"
#include "GlobalState.h"
#include "uiUtilities.h"
#include "ColorObject.h"
#include "common/CastToVariant.h"
#include <gdk/gdkkeysyms.h>

static gboolean onKeyPress(GtkWidget *widget, GdkEventKey *event, IReadonlyColorUI *readonlyColorUI) {
	auto *gs = reinterpret_cast<GlobalState *>(g_object_get_data(G_OBJECT(widget), "gs"));
	auto modifiers = gtk_accelerator_get_default_mod_mask();
	switch (getKeyval(*event, gs->latinKeysGroup)) {
	case GDK_KEY_c:
		if ((event->state & modifiers) == GDK_CONTROL_MASK) {
			auto *editableColorsUI = dynamic_cast<IEditableColorsUI *>(readonlyColorUI);
			if (editableColorsUI) {
				auto colors = editableColorsUI->getColors(true);
				if (colors.size() > 0)
					clipboard::set(colors, gs, Converters::Type::copy);
				return true;
			}
			auto *readonlyColorsUI = dynamic_cast<IReadonlyColorsUI*>(readonlyColorUI);
			if (readonlyColorsUI) {
				auto colors = readonlyColorsUI->getColors(true);
				if (colors.size() > 0)
					clipboard::set(colors, gs, Converters::Type::copy);
				return true;
			}
			auto &colorObject = readonlyColorUI->getColor();
			clipboard::set(colorObject, gs, Converters::Type::copy);
			return true;
		}
		return false;
	case GDK_KEY_v:
		if ((event->state & modifiers) == GDK_CONTROL_MASK) {
			auto *editableColorUI = dynamic_cast<IEditableColorUI *>(readonlyColorUI);
			if (!editableColorUI)
				return false;
			auto colorObject = clipboard::getFirst(gs);
			if (colorObject) {
				editableColorUI->setColor(*colorObject);
				colorObject->release();
			}
			return true;
		}
		return false;
	case GDK_KEY_a:
		if ((event->state & modifiers) == GDK_CONTROL_MASK) {
			auto *readonlyColorsUI = dynamic_cast<IReadonlyColorsUI *>(readonlyColorUI);
			if (!readonlyColorsUI)
				return false;
			readonlyColorsUI->addAllToPalette();
		} else {
			auto &colorObject = readonlyColorUI->getColor();
			readonlyColorUI->addToPalette(colorObject);
		}
		return true;
	case GDK_KEY_e: {
		auto *editableColorUI = dynamic_cast<IEditableColorUI *>(readonlyColorUI);
		if (!editableColorUI)
			return false;
		auto *colorObject = readonlyColorUI->getColor().copy();
		ColorObject *newColorObject;
		if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), gs, colorObject, &newColorObject) == 0) {
			editableColorUI->setColor(*newColorObject);
			newColorObject->release();
		}
		colorObject->release();
		return true;
	}
	}
	return false;
}
static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, IReadonlyColorUI *readonlyColorUI) {
	if (event->button == 3) {
		auto *gs = reinterpret_cast<GlobalState *>(g_object_get_data(G_OBJECT(widget), "gs"));
		auto interface = common::castToVariant<IReadonlyColorUI *, IEditableColorsUI *, IEditableColorUI *, IReadonlyColorsUI *>(readonlyColorUI);
		StandardMenu::forInterface(gs, event, interface);
		return true;
	}
	return false;
}
static void onPopupMenu(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	auto *gs = reinterpret_cast<GlobalState *>(g_object_get_data(G_OBJECT(widget), "gs"));
	auto interface = common::castToVariant<IReadonlyColorUI *, IEditableColorsUI *, IEditableColorUI *, IReadonlyColorsUI *>(readonlyColorUI);
	StandardMenu::forInterface(gs, nullptr, interface);
}
StandardEventHandler::Options::Options():
	m_afterEvents(true) {
}
StandardEventHandler::Options &StandardEventHandler::Options::afterEvents(bool enable) {
	m_afterEvents = enable;
	return *this;
}
void StandardEventHandler::forWidget(GtkWidget *widget, GlobalState *gs, Interface interface, Options options) {
	void *data = boost::apply_visitor([](auto *interface) -> void * {
		return interface;
	}, interface);
	if (options.m_afterEvents) {
		g_signal_connect_after(G_OBJECT(widget), "key_press_event", G_CALLBACK(onKeyPress), data);
		g_signal_connect_after(G_OBJECT(widget), "button-press-event", G_CALLBACK(onButtonPress), data);
		g_signal_connect_after(G_OBJECT(widget), "popup-menu", G_CALLBACK(onPopupMenu), data);
	} else {
		g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(onKeyPress), data);
		g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(onButtonPress), data);
		g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(onPopupMenu), data);
	}
	g_object_set_data(G_OBJECT(widget), "gs", gs);
}
