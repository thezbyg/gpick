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

#ifndef GPICK_UI_APP_H_
#define GPICK_UI_APP_H_

#include <string>
#include <gtk/gtk.h>
class GlobalState;
class ColorObject;
struct Converters;
struct Color;

int main_show_window(GtkWidget* window, struct dynvSystem *main_params);

typedef struct AppArgs AppArgs;

struct AppOptions
{
	bool floating_picker_mode;
	std::string converter_name;
	bool output_picked_color;
	bool output_without_newline;
	bool single_color_pick_mode;
	bool do_not_start;
};

AppArgs* app_create_main(const AppOptions &options, int &return_value);
int app_load_file(AppArgs *args, const char *filename, bool autoload = false);
int app_run(AppArgs *args);
int app_parse_geometry(AppArgs *args, const char *geometry);

bool app_is_autoload_enabled(AppArgs *args);

#endif /* GPICK_UI_APP_H_ */
