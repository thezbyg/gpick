/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#ifndef UIAPP_H_
#define UIAPP_H_

#include <gtk/gtk.h>
#include "GlobalState.h"
#include "Color.h"

int main_show_window(GtkWidget* window, struct dynvSystem *main_params);

enum ColorTextType{
	COLOR_TEXT_TYPE_DISPLAY,
	COLOR_TEXT_TYPE_COPY,
	COLOR_TEXT_TYPE_COLOR_LIST,
};

char* main_get_color_text(GlobalState* gs, Color* color, ColorTextType text_type);
int main_get_color_from_text(GlobalState* gs, char* text, Color* color);
int main_get_color_object_from_text(GlobalState* gs, char* text, struct ColorObject** output_color_object);

GtkWidget* converter_create_copy_menu (struct ColorObject* color_object, GtkWidget* palette_widget, GlobalState* gs);
void converter_get_clipboard(const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, struct dynvSystem* params);
void converter_get_text(const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, struct dynvSystem* params, gchar** text);

typedef struct AppArgs AppArgs;

AppArgs* app_create_main();
int app_load_file(AppArgs *args, const char *filename, bool autoload = false);
int app_run(AppArgs *args);
int app_parse_geometry(AppArgs *args, const char *geometry);

bool app_is_autoload_enabled(AppArgs *args);


#endif /* UIAPP_H_ */
