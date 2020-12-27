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
#include "ColorObject.h"
#include "Clipboard.h"
#include "Converters.h"
#include "Converter.h"
#include "GlobalState.h"
#include "ColorList.h"
#include "uiUtilities.h"
#include "uiColorInput.h"
#include "I18N.h"
#include "common/CastToVariant.h"
#include "IMenuExtension.h"
#include <gdk/gdkkeysyms.h>
#include <map>

struct CopyMenuItemState: public boost::static_visitor<> {
	CopyMenuItemState(Converter *converter, const ColorObject &colorObject, GlobalState *gs):
		m_converter(converter),
		m_data(colorObject),
		m_gs(gs),
		m_recursion(false) {
	}
	CopyMenuItemState(Converter *converter, IReadonlyColorUI *interface, GlobalState *gs):
		m_converter(converter),
		m_data(interface),
		m_gs(gs),
		m_recursion(false) {
	}
	static void onReleaseState(CopyMenuItemState *state) {
		delete state;
	}
	static void onActivate(GtkWidget *widget, CopyMenuItemState *state) {
		boost::apply_visitor(*state, state->m_data);
	}
	void operator()(const ColorObject &colorObject) {
		clipboard::set(colorObject, m_gs, m_converter);
	}
	// this method will be called recursively if up-casting fails, so m_recursion is used to detect that
	void operator()(IReadonlyColorUI *interface) {
		if (m_recursion) {
			auto color = interface->getColor();
			clipboard::set(color, m_gs, m_converter);
			return;
		}
		m_recursion = true;
		auto variant = common::castToVariant<IReadonlyColorUI *, IEditableColorsUI *, IReadonlyColorsUI *>(interface);
		boost::apply_visitor(*this, variant);
		m_recursion = false;
	}
	void operator()(IEditableColorsUI *interface) {
		auto colors = interface->getColors(true);
		clipboard::set(colors, m_gs, m_converter);
	}
	void operator()(IReadonlyColorsUI *interface) {
		auto colors = interface->getColors(true);
		clipboard::set(colors, m_gs, m_converter);
	}
private:
	Converter *m_converter;
	boost::variant<ColorObject, IReadonlyColorUI *> m_data;
	GlobalState *m_gs;
	bool m_recursion;
};
GtkWidget *StandardMenu::newItem(const ColorObject &colorObject, GlobalState *gs, bool includeName) {
	ConverterSerializePosition position;
	auto converter = gs->converters().firstCopyOrAny();
	if (!converter)
		return nullptr;
	auto textLine = converter->serialize(colorObject, position);
	if (includeName) {
		textLine += " - ";
		textLine += colorObject.getName();
	}
	auto item = newMenuItem(textLine.c_str(), GTK_STOCK_COPY);
	auto state = new CopyMenuItemState(converter, colorObject, gs);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), state);
	g_object_set_data_full(G_OBJECT(item), "item_data", state, (GDestroyNotify)&CopyMenuItemState::onReleaseState);
	return item;
}
GtkWidget *StandardMenu::newItem(const ColorObject &colorObject, Converter *converter, GlobalState *gs) {
	ConverterSerializePosition position;
	auto textLine = converter->serialize(colorObject, position);
	auto item = newMenuItem(textLine.c_str(), GTK_STOCK_COPY);
	auto state = new CopyMenuItemState(converter, colorObject, gs);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), state);
	g_object_set_data_full(G_OBJECT(item), "item_data", state, (GDestroyNotify)&CopyMenuItemState::onReleaseState);
	return item;
}
GtkWidget *StandardMenu::newItem(const ColorObject &colorObject, IReadonlyColorUI *interface, Converter *converter, GlobalState *gs) {
	ConverterSerializePosition position;
	auto textLine = converter->serialize(colorObject, position);
	auto item = newMenuItem(textLine.c_str(), GTK_STOCK_COPY);
	auto state = new CopyMenuItemState(converter, interface, gs);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), state);
	g_object_set_data_full(G_OBJECT(item), "item_data", state, (GDestroyNotify)&CopyMenuItemState::onReleaseState);
	return item;
}
GtkWidget *StandardMenu::newMenu(const ColorObject &colorObject, GlobalState *gs) {
	GtkWidget *menu = gtk_menu_new();
	for (auto &converter: gs->converters().allCopy()) {
		GtkWidget *item = newItem(colorObject, converter, gs);
		if (item) gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}
	return menu;
}
GtkWidget *StandardMenu::newMenu(const ColorObject &colorObject, IReadonlyColorUI *interface, GlobalState *gs) {
	GtkWidget *menu = gtk_menu_new();
	for (auto &converter: gs->converters().allCopy()) {
		GtkWidget *item = newItem(colorObject, interface, converter, gs);
		if (item) gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}
	return menu;
}
GtkWidget *StandardMenu::newNearestColorsMenu(const ColorObject &colorObject, GlobalState *gs) {
	GtkWidget *menu = gtk_menu_new();
	std::multimap<float, ColorObject *> colorDistances;
	Color sourceColor = colorObject.getColor().rgbToLabD50();
	for (auto &colorObject: gs->getColorList()->colors) {
		Color targetColor = colorObject->getColor().rgbToLabD50();
		colorDistances.insert(std::pair<float, ColorObject *>(Color::distanceLch(sourceColor, targetColor), colorObject));
	}
	int count = 0;
	for (auto item: colorDistances) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), newItem(*item.second, gs, true));
		if (++count >= 3) break;
	}
	return menu;
}
static void buildMenu(GtkWidget *menu, GtkWidget **copyToClipboard, GtkWidget **nearestFromPalette) {
	GtkAccelGroup *accel_group = gtk_menu_get_accel_group(GTK_MENU(menu));
	GtkWidget *item = *copyToClipboard = gtk_menu_item_new_with_mnemonic(_("_Copy to clipboard"));
	if (accel_group)
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_c, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	item = *nearestFromPalette = gtk_menu_item_new_with_mnemonic(_("_Nearest from palette"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}
void StandardMenu::appendMenu(GtkWidget *menu, const ColorObject &colorObject, GlobalState *gs) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), newMenu(colorObject, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), newNearestColorsMenu(colorObject, gs));
}
void StandardMenu::appendMenu(GtkWidget *menu, IReadonlyColorUI *interface, GlobalState *gs) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	if (interface->hasSelectedColor()) {
		auto colorObject = interface->getColor();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), newMenu(colorObject, interface, gs));
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), newNearestColorsMenu(colorObject, gs));
	} else {
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), gtk_menu_new());
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), gtk_menu_new());
		gtk_widget_set_sensitive(copyToClipboard, false);
		gtk_widget_set_sensitive(nearestFromPalette, false);
	}
}
void StandardMenu::appendMenu(GtkWidget *menu, const Color &color, GlobalState *gs) {
	GtkWidget *copyToClipboard, *nearestFromPalette;
	buildMenu(menu, &copyToClipboard, &nearestFromPalette);
	ColorObject colorObject("", color);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copyToClipboard), newMenu(colorObject, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearestFromPalette), newNearestColorsMenu(colorObject, gs));
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
	if (colorObject)
		readonlyColorUI->addToPalette(*colorObject);
	else
		readonlyColorUI->addToPalette(ColorObject());
}
static void onEditableColorAddAll(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	dynamic_cast<IReadonlyColorsUI *>(readonlyColorUI)->addAllToPalette();
}
static void onEditableColorEdit(GtkWidget *widget, IReadonlyColorUI *readonlyColorUI) {
	auto *colorObject = reinterpret_cast<ColorObject *>(g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "color"));
	auto *gs = reinterpret_cast<GlobalState *>(g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "gs"));
	ColorObject *newColorObject;
	if (colorObject) {
		if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), gs, colorObject, &newColorObject) == 0) {
			dynamic_cast<IEditableColorUI *>(readonlyColorUI)->setColor(*newColorObject);
			newColorObject->release();
		}
	} else {
		auto colorObject = readonlyColorUI->getColor();
		if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), gs, &colorObject, &newColorObject) == 0) {
			dynamic_cast<IEditableColorUI *>(readonlyColorUI)->setColor(*newColorObject);
			newColorObject->release();
		}
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
struct InterfaceInspector: public boost::static_visitor<IReadonlyColorUI *> {
	InterfaceInspector(bool &editable, bool &currentlyEditable, bool &multiple, bool &hasSelection, bool &hasColor):
		editable(editable),
		currentlyEditable(currentlyEditable),
		multiple(multiple),
		hasSelection(hasSelection),
		hasColor(hasColor) {
	}
	template<typename T>
	void common(T interface) const {
		hasSelection = interface->hasSelectedColor();
		hasColor = interface->hasColor();
	}
	IReadonlyColorUI *operator()(IReadonlyColorUI *interface) const {
		editable = false;
		currentlyEditable = false;
		multiple = false;
		hasSelection = interface->hasSelectedColor();
		hasColor = true;
		return interface;
	}
	IReadonlyColorUI *operator()(IReadonlyColorsUI *interface) const {
		editable = false;
		currentlyEditable = false;
		multiple = true;
		common(interface);
		return interface;
	}
	IReadonlyColorUI *operator()(IEditableColorUI *interface) const {
		editable = true;
		currentlyEditable = interface->isEditable();
		multiple = false;
		hasSelection = interface->hasSelectedColor();
		hasColor = true;
		return interface;
	}
	IReadonlyColorUI *operator()(IEditableColorsUI *interface) const {
		editable = true;
		currentlyEditable = interface->isEditable();
		multiple = true;
		common(interface);
		return interface;
	}
private:
	bool &editable, &currentlyEditable, &multiple, &hasSelection, &hasColor;
};
void StandardMenu::contextForColorObject(ColorObject *colorObject, GlobalState *gs, GdkEventButton *event, Interface interface) {
	auto menu = gtk_menu_new();
	auto acceleratorGroup = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), acceleratorGroup);
	bool editable = false, currentlyEditable = false, multiple = false, hasSelection = false, hasColor = false;
	IReadonlyColorUI *baseInterface;
	Appender appender(menu, acceleratorGroup, (baseInterface = boost::apply_visitor(InterfaceInspector(editable, currentlyEditable, multiple, hasSelection, hasColor), interface)));
	appender.appendItem(_("_Add to palette"), GTK_STOCK_ADD, GDK_KEY_A, GdkModifierType(0), G_CALLBACK(onEditableColorAdd), hasSelection);
	if (multiple) {
		appender.appendItem(_("A_dd all to palette"), GTK_STOCK_ADD, GDK_KEY_A, GdkModifierType(GDK_CONTROL_MASK), G_CALLBACK(onEditableColorAddAll), hasColor);
	}
	auto *menuExtension = dynamic_cast<IMenuExtension *>(baseInterface);
	if (menuExtension)
		menuExtension->extendMenu(menu, IMenuExtension::Position::middle);
	appender.appendSeparator();
	StandardMenu::appendMenu(menu, baseInterface, gs);
	if (editable) {
		appender.appendSeparator();
		appender.appendItem(_("_Edit..."), GTK_STOCK_EDIT, GDK_KEY_E, GdkModifierType(0), G_CALLBACK(onEditableColorEdit), hasSelection && currentlyEditable);
		appender.appendItem(_("_Paste"), GTK_STOCK_PASTE, GDK_KEY_V, GdkModifierType(GDK_CONTROL_MASK), G_CALLBACK(onEditableColorPaste), hasSelection && currentlyEditable && clipboard::colorObjectAvailable());
	}
	if (menuExtension)
		menuExtension->extendMenu(menu, IMenuExtension::Position::end);
	g_object_set_data_full(G_OBJECT(menu), "color", colorObject->reference(), (GDestroyNotify)releaseColorObject);
	if (editable)
		g_object_set_data(G_OBJECT(menu), "gs", gs);
	showContextMenu(menu, event);
}
void StandardMenu::forInterface(GlobalState *gs, GdkEventButton *event, Interface interface) {
	auto menu = gtk_menu_new();
	auto acceleratorGroup = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), acceleratorGroup);
	bool editable = false, currentlyEditable = false, multiple = false, hasSelection = false, hasColor = false;
	IReadonlyColorUI *baseInterface;
	Appender appender(menu, acceleratorGroup, (baseInterface = boost::apply_visitor(InterfaceInspector(editable, currentlyEditable, multiple, hasSelection, hasColor), interface)));
	appender.appendItem(_("_Add to palette"), GTK_STOCK_ADD, GDK_KEY_A, GdkModifierType(0), G_CALLBACK(onEditableColorAdd), hasSelection);
	if (multiple) {
		appender.appendItem(_("A_dd all to palette"), GTK_STOCK_ADD, GDK_KEY_A, GdkModifierType(GDK_CONTROL_MASK), G_CALLBACK(onEditableColorAddAll), hasColor);
	}
	auto *menuExtension = dynamic_cast<IMenuExtension *>(baseInterface);
	if (menuExtension)
		menuExtension->extendMenu(menu, IMenuExtension::Position::middle);
	appender.appendSeparator();
	StandardMenu::appendMenu(menu, baseInterface, gs);
	if (editable) {
		appender.appendSeparator();
		appender.appendItem(_("_Edit..."), GTK_STOCK_EDIT, GDK_KEY_E, GdkModifierType(0), G_CALLBACK(onEditableColorEdit), hasSelection && currentlyEditable);
		appender.appendItem(_("_Paste"), GTK_STOCK_PASTE, GDK_KEY_V, GdkModifierType(GDK_CONTROL_MASK), G_CALLBACK(onEditableColorPaste), hasSelection && currentlyEditable && clipboard::colorObjectAvailable());
	}
	if (menuExtension)
		menuExtension->extendMenu(menu, IMenuExtension::Position::end);
	if (editable)
		g_object_set_data(G_OBJECT(menu), "gs", gs);
	showContextMenu(menu, event);
}
