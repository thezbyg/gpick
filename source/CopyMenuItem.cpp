/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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
#include "DynvHelpers.h"
#include "uiUtilities.h"
#include "uiListPalette.h"
#include <string>
#include <sstream>
using namespace std;

static PaletteListCallbackReturn addToColorList(ColorObject* color_object, ColorList *color_list)
{
	color_list_add_color_object(color_list, color_object, 1);
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}
struct CopyMenuItemState
{
	CopyMenuItemState(Converter *converter, ColorObject *color_object, GlobalState *gs):
		m_gs(gs),
		m_converter(converter),
		m_color_object(color_object),
		m_palette_widget(nullptr)
	{
	}
	~CopyMenuItemState()
	{
		m_color_object->release();
	}
	void setPaletteWidget(GtkWidget *palette_widget)
	{
		m_palette_widget = palette_widget;
	}
	static void onRelease(CopyMenuItemState *copy_menu_item)
	{
		delete copy_menu_item;
	}
	static void onActivate(GtkWidget *widget, CopyMenuItemState *copy_menu_item_state)
	{
		string text_line;
		if (copy_menu_item_state->m_palette_widget){
			stringstream text(ios::out);
			ColorList *color_list = color_list_new();
			palette_list_foreach_selected(copy_menu_item_state->m_palette_widget, (PaletteListCallback)addToColorList, color_list);
			ConverterSerializePosition position(color_list->colors.size());
			if (position.count() > 0){
				string text_line;
				for (auto &color: color_list->colors){
					if (position.index() + 1 == position.count())
						position.last(true);
					text_line = copy_menu_item_state->m_converter->serialize(color, position);
					if (position.first()){
						text << text_line;
						position.first(false);
					}else{
						text << endl << text_line;
					}
					position.incrementIndex();
				}
			}
			color_list_destroy(color_list);
			text_line = text.str();
			if (text_line.length() > 0){
				gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text_line.c_str(), -1);
				gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), text_line.c_str(), -1);
			}
		}else{
			ConverterSerializePosition position;
			text_line = copy_menu_item_state->m_converter->serialize(copy_menu_item_state->m_color_object, position);
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text_line.c_str(), -1);
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), text_line.c_str(), -1);
		}
	}
	GlobalState *m_gs;
	Converter *m_converter;
	ColorObject *m_color_object;
	GtkWidget *m_palette_widget;
};
GtkWidget* CopyMenuItem::newItem(ColorObject* color_object, GlobalState *gs, bool include_name)
{
	GtkWidget* item = nullptr;
	ConverterSerializePosition position;
	Converter *converter = gs->converters().firstCopyOrAny();
	if (converter){
		string text_line = converter->serialize(color_object, position);
		if (include_name){
			text_line += " - ";
			text_line += color_object->getName();
		}
		item = newMenuItem(text_line.c_str(), GTK_STOCK_COPY);
		CopyMenuItemState *copy_menu_item_state = new CopyMenuItemState(converter, color_object->reference(), gs);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), copy_menu_item_state);
		g_object_set_data_full(G_OBJECT(item), "item_data", copy_menu_item_state, (GDestroyNotify)&CopyMenuItemState::onRelease);
	}else{
		item = newMenuItem("-", GTK_STOCK_COPY);
	}
	return item;
}
GtkWidget* CopyMenuItem::newItem(ColorObject *color_object, Converter *converter, GlobalState *gs)
{
	GtkWidget* item = nullptr;
	ConverterSerializePosition position;
	string text_line = converter->serialize(color_object, position);
	item = newMenuItem(text_line.c_str(), GTK_STOCK_COPY);
	CopyMenuItemState *copy_menu_item_state = new CopyMenuItemState(converter, color_object->reference(), gs);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), copy_menu_item_state);
	g_object_set_data_full(G_OBJECT(item), "item_data", copy_menu_item_state, (GDestroyNotify)&CopyMenuItemState::onRelease);
	return item;
}
GtkWidget* CopyMenuItem::newItem(ColorObject* color_object, GtkWidget *palette_widget, Converter *converter, GlobalState *gs)
{
	GtkWidget* item = nullptr;
	ConverterSerializePosition position;
	string text_line = converter->serialize(color_object, position);
	item = newMenuItem(text_line.c_str(), GTK_STOCK_COPY);
	CopyMenuItemState *copy_menu_item_state = new CopyMenuItemState(converter, color_object->reference(), gs);
	copy_menu_item_state->setPaletteWidget(palette_widget);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), copy_menu_item_state);
	g_object_set_data_full(G_OBJECT(item), "item_data", copy_menu_item_state, (GDestroyNotify)&CopyMenuItemState::onRelease);
	return item;
}
