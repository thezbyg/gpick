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

#include "Clipboard.h"
#include "Converter.h"
#include "GlobalState.h"
#include "ColorObject.h"
#include "Color.h"
#include "uiListPalette.h"
#include "ColorList.h"
#include <gtk/gtk.h>
#include <sstream>
using namespace std;

static PaletteListCallbackReturn addToColorList(ColorObject* color_object, ColorList *color_list)
{
	color_list_add_color_object(color_list, color_object, 1);
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}
void Clipboard::set(const std::string &value)
{
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), value.c_str(), -1);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), value.c_str(), -1);
}
void Clipboard::set(const ColorObject *color_object, GlobalState *gs)
{
	string text_line;
	ConverterSerializePosition position;
	auto converters = gs->getConverters();
	auto converter = converters_get_first(converters, ConverterArrayType::copy);
	if (converters_color_serialize(converter, color_object, position, text_line) == 0){
		set(text_line);
	}
}
void Clipboard::set(const Color &color, GlobalState *gs)
{
	ColorObject color_object("", color);
	set(&color_object, gs);
}
void Clipboard::set(GtkWidget *palette_widget, GlobalState *gs)
{
	string text_line;
	auto converters = gs->getConverters();
	auto converter = converters_get_first(converters, ConverterArrayType::copy);
	stringstream text(ios::out);
	ColorList *color_list = color_list_new(nullptr);
	palette_list_foreach_selected(palette_widget, (PaletteListCallback)addToColorList, color_list);
	ConverterSerializePosition position(color_list->colors.size());
	if (position.count > 0){
		string text_line;
		for (ColorList::iter i = color_list->colors.begin(); i != color_list->colors.end(); ++i){
			if (position.index + 1 == position.count)
				position.last = true;
			if (converters_color_serialize(converter, *i, position, text_line) == 0){
				if (position.first){
					text << text_line;
					position.first = false;
				}else{
					text << endl << text_line;
				}
				position.index++;
			}
		}
	}
	color_list_destroy(color_list);
	text_line = text.str();
	if (text_line.length() > 0){
		set(text_line);
	}
}
