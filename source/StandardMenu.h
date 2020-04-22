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

#ifndef GPICK_STANDARD_MENU_H_
#define GPICK_STANDARD_MENU_H_
#include "IEditableColorUI.h"
#include "IReadonlyColorUI.h"
#include <gtk/gtk.h>
#include <boost/variant.hpp>
struct ColorObject;
struct GlobalState;
struct Color;
struct Converter;
struct StandardMenu {
	static void appendMenu(GtkWidget *menu, const ColorObject &colorObject, GlobalState *gs);
	static void appendMenu(GtkWidget *menu, const Color &color, GlobalState *gs);
	static void appendMenu(GtkWidget *menu, IReadonlyColorUI *interface, GlobalState *gs);
	static void appendMenu(GtkWidget *menu);
	static GtkWidget *newMenu(const ColorObject &colorObject, GlobalState *gs);
	static GtkWidget *newMenu(const ColorObject &colorObject, IReadonlyColorUI *interface, GlobalState *gs);
	static GtkWidget *newItem(const ColorObject &colorObject, GlobalState *gs, bool includeName);
	static GtkWidget *newItem(const ColorObject &colorObject, Converter *converter, GlobalState *gs);
	static GtkWidget *newItem(const ColorObject &colorObject, IReadonlyColorUI *interface, Converter *converter, GlobalState *gs);
	static GtkWidget *newNearestColorsMenu(const ColorObject &colorObject, GlobalState *gs);
	struct Appender {
		Appender(GtkWidget *menu, GtkAccelGroup *acceleratorGroup, void *data);
		GtkWidget *appendItem(const char *label, const char *icon, guint key, GdkModifierType modifiers, GCallback callback, bool enabled = true);
		GtkWidget *appendSeparator();
	private:
		GtkWidget *menu;
		GtkAccelGroup *acceleratorGroup;
		void *data;
	};
	using Interface = boost::variant<IEditableColorUI *, IEditableColorsUI *, IReadonlyColorUI *, IReadonlyColorsUI *>;
	static void contextForColorObject(ColorObject *colorObject, GlobalState *gs, GdkEventButton *event, Interface interface);
	static void forInterface(GlobalState *gs, GdkEventButton *event, Interface interface);
};
#endif /* GPICK_STANDARD_MENU_H_ */
