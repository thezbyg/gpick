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

#include "CopyMenuItem.h"
#include "ColorObject.h"
#include "Converter.h"
#include "GlobalState.h"
#include "DynvHelpers.h"
#include "uiUtilities.h"
#include <string>
using namespace std;

class CopyMenuItemState
{
	public:
		CopyMenuItemState(Converter *converter, ColorObject *color_object, GlobalState *gs):
			m_converter(converter),
			m_color_object(color_object),
			m_gs(gs)
		{
		}
		~CopyMenuItemState()
		{
			color_object_release(m_color_object);
		}
		static void onRelease(CopyMenuItemState *copy_menu_item)
		{
			delete copy_menu_item;
		}
		static void onActivate(GtkWidget *widget, CopyMenuItemState *copy_menu_item_state)
		{
			string text_line;
			ConverterSerializePosition position;
			position.first = true;
			position.last = true;
			position.index = 0;
			position.count = 1;
			if (converters_color_serialize(copy_menu_item_state->m_converter, copy_menu_item_state->m_color_object, position, text_line) == 0){
				gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text_line.c_str(), -1);
				gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), text_line.c_str(), -1);
			}
		}
		Converter *m_converter;
		ColorObject *m_color_object;
		GlobalState *m_gs;
};
GtkWidget* CopyMenuItem::newItem(ColorObject* color_object, GlobalState *gs, bool include_name)
{
	GtkWidget* item = nullptr;
	string text_line;
	ConverterSerializePosition position;
	position.first = true;
	position.last = true;
	position.index = 0;
	position.count = 1;
	auto converters = gs->getConverters();
	Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
	if (converters_color_serialize(converter, color_object, position, text_line) == 0){
		if (include_name){
			text_line += " - ";
			text_line += color_object_get_name(color_object);
		}
		item = gtk_menu_item_new_with_image(text_line.c_str(), gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
		CopyMenuItemState *copy_menu_item_state = new CopyMenuItemState(converter, color_object_ref(color_object), gs);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(&CopyMenuItemState::onActivate), copy_menu_item_state);
		g_object_set_data_full(G_OBJECT(item), "item_data", copy_menu_item_state, (GDestroyNotify)&CopyMenuItemState::onRelease);
	}
	return item;
}

