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

#include "StandardMenu.h"
#include "CopyMenu.h"
#include "ColorObject.h"
#include "NearestColorsMenu.h"
#include "Clipboard.h"
#include "uiUtilities.h"
#include "uiColorInput.h"
#include "I18N.h"
#include <gdk/gdkkeysyms.h>

static void buildMenu(GtkWidget *menu, GtkWidget **copyToClipboard, GtkWidget **nearestFromPalette) {
	GtkAccelGroup *accel_group = gtk_menu_get_accel_group(GTK_MENU(menu));
	GtkWidget *item = *copyToClipboard = gtk_menu_item_new_with_mnemonic(_("_Copy to clipboard"));
	if (accel_group)
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_c, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	item = *nearestFromPalette = gtk_menu_item_new_with_mnemonic(_("_Nearest from palette"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}
void StandardMenu::appendMenu(GtkWidget *menu, ColorObject *colorObject, GtkWidget *paletteWidget, GlobalState *gs) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), CopyMenu::newMenu(colorObject, paletteWidget, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), NearestColorsMenu::newMenu(colorObject, gs));
}
void StandardMenu::appendMenu(GtkWidget *menu, ColorObject *colorObject, GlobalState *gs) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), CopyMenu::newMenu(colorObject, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), NearestColorsMenu::newMenu(colorObject, gs));
}
void StandardMenu::appendMenu(GtkWidget *menu, const Color &color, GlobalState *gs) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	auto colorObject = new ColorObject("", color);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), CopyMenu::newMenu(colorObject, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), NearestColorsMenu::newMenu(colorObject, gs));
	colorObject->release();
}
void StandardMenu::appendMenu(GtkWidget *menu) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	gtk_widget_set_sensitive(copyToClipboard, false);
	gtk_widget_set_sensitive(nearestFromPalette, false);
}
StandardMenu::Appender::Appender(GtkWidget *menu, GtkAccelGroup *acceleratorGroup, void *data):
	menu(menu),
	acceleratorGroup(acceleratorGroup),
	data(data) {
}
GtkWidget *StandardMenu::Appender::appendItem(const char *label, const char *icon, guint key, GdkModifierType modifiers, GCallback callback, bool enabled) {
	auto item = newMenuItem(label, icon);
	gtk_widget_add_accelerator(item, "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), key, modifiers, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(callback), data);
	gtk_widget_set_sensitive(item, enabled);
	return item;
}
GtkWidget *StandardMenu::Appender::appendSeparator() {
	GtkWidget *item;
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item = gtk_separator_menu_item_new());
	return item;
}
static void onEditableColorAdd(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	auto *colorObject = reinterpret_cast<ColorObject *>(g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "color"));
	readonlyColorUI->addToPalette(*colorObject);
}
static void onEditableColorAddAll(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	dynamic_cast<IReadonlyColorsUI *>(readonlyColorUI)->addAllToPalette();
}
static void onEditableColorEdit(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	auto *colorObject = reinterpret_cast<ColorObject *>(g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "color"));
	auto *gs = reinterpret_cast<GlobalState *>(g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "gs"));
	ColorObject *newColorObject;
	if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), gs, colorObject, &newColorObject) == 0) {
		dynamic_cast<IEditableColorUI *>(readonlyColorUI)->setColor(*newColorObject);
		newColorObject->release();
	}
}
static void onEditableColorPaste(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	auto *gs = reinterpret_cast<GlobalState *>(g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "gs"));
	auto colorObject = clipboard::getFirst(gs);
	if (colorObject) {
		dynamic_cast<IEditableColorUI *>(readonlyColorUI)->setColor(*colorObject);
		colorObject->release();
	}
}
static void releaseColorObject(ColorObject *colorObject) {
	colorObject->release();
}
void StandardMenu::contextForColorObject(ColorObject *colorObject, GlobalState *gs, GdkEventButton *event, Interface interface) {
	auto menu = gtk_menu_new();
	auto acceleratorGroup = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), acceleratorGroup);
	bool editable = false, multiple = false;
	struct InterfaceInspector: public boost::static_visitor<void *> {
		InterfaceInspector(bool &editable, bool &multiple):
			editable(editable),
			multiple(multiple) {
		}
		void *operator()(IReadonlyColorUI *interface) const {
			editable = false;
			multiple = false;
			return interface;
		}
		void *operator()(IReadonlyColorsUI *interface) const {
			editable = false;
			multiple = true;
			return interface;
		}
		void *operator()(IEditableColorUI *interface) const {
			editable = true;
			multiple = false;
			return interface;
		}
		void *operator()(IEditableColorsUI *interface) const {
			editable = true;
			multiple = true;
			return interface;
		}
	private:
		bool &editable, &multiple;
	};
	Appender appender(menu, acceleratorGroup, boost::apply_visitor(InterfaceInspector(editable, multiple), interface));
	appender.appendItem(_("_Add to palette"), GTK_STOCK_ADD, GDK_KEY_A, GdkModifierType(0), G_CALLBACK(onEditableColorAdd));
	if (multiple)
		appender.appendItem(_("A_dd all to palette"), GTK_STOCK_ADD, GDK_KEY_A, GdkModifierType(GDK_CONTROL_MASK), G_CALLBACK(onEditableColorAddAll));
	appender.appendSeparator();
	StandardMenu::appendMenu(menu, colorObject, gs);
	if (editable) {
		appender.appendSeparator();
		appender.appendItem(_("_Edit..."), GTK_STOCK_EDIT, GDK_KEY_E, GdkModifierType(0), G_CALLBACK(onEditableColorEdit));
		appender.appendItem(_("_Paste"), GTK_STOCK_PASTE, GDK_KEY_V, GdkModifierType(GDK_CONTROL_MASK), G_CALLBACK(onEditableColorPaste), clipboard::colorObjectAvailable());
	}
	gtk_widget_show_all(GTK_WIDGET(menu));
	gint32 button, eventTime;
	if (event) {
		button = event->button;
		eventTime = event->time;
	} else {
		button = 0;
		eventTime = gtk_get_current_event_time();
	}
	g_object_set_data_full(G_OBJECT(menu), "color", colorObject->reference(), (GDestroyNotify)releaseColorObject);
	if (editable)
		g_object_set_data(G_OBJECT(menu), "gs", gs);
	gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, nullptr, nullptr, button, eventTime);
	g_object_ref_sink(menu);
	g_object_unref(menu);
}
