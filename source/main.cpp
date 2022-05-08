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

#include "main.h"
#include "Paths.h"
#include "uiAbout.h"
#include "uiApp.h"
#include "I18N.h"
#include "version/Version.h"
#include "dynv/Map.h"
#include <gtk/gtk.h>
#include <string>
#include <iostream>
using namespace std;

static gchar **commandline_filename = nullptr;
static gchar *commandline_geometry = nullptr;
static gboolean pick_color = FALSE;
static gboolean output_picked_color = FALSE;
static gboolean output_without_newline = FALSE;
static gboolean single_color_pick_mode = FALSE;
static gboolean version_information = FALSE;
static gboolean do_not_start = FALSE;
static gchar *converter_name = nullptr;
static GOptionEntry commandline_entries[] =
{
	{"geometry", 'g', 0, G_OPTION_ARG_STRING, &commandline_geometry, "Window geometry", "GEOMETRY"},
	{"pick", 'p', 0, G_OPTION_ARG_NONE, &pick_color, "Pick a color", nullptr},
	{"single", 's', 0, G_OPTION_ARG_NONE, &single_color_pick_mode, "Pick one color and exit", nullptr},
	{"output", 'o', 0, G_OPTION_ARG_NONE, &output_picked_color, "Output picked color", nullptr},
	{"no-newline", 0, 0, G_OPTION_ARG_NONE, &output_without_newline, "Output picked color without newline", nullptr},
	{"no-start", 0, 0, G_OPTION_ARG_NONE, &do_not_start, "Do not start Gpick if it is not already running", nullptr},
	{"converter-name", 'c', 0, G_OPTION_ARG_STRING, &converter_name, "Converter name used for floating picker mode", nullptr},
	{"version", 'v', 0, G_OPTION_ARG_NONE, &version_information, "Print version information", nullptr},
	{G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &commandline_filename, nullptr, "[FILE...]"},
	{nullptr}
};
int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	gtk_init(&argc, &argv);
	initialize_i18n();
	g_set_application_name(program_name);
	GError *error = nullptr;
	GOptionContext *context = g_option_context_new("- advanced color picker");
	g_option_context_add_main_entries(context, commandline_entries, 0);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	gchar **argv_copy;
#ifdef WIN32
	argv_copy = g_win32_get_command_line();
#else
	argv_copy = g_strdupv(argv);
#endif
	if (!g_option_context_parse_strv(context, &argv_copy, &error)){
		g_print("option parsing failed: %s\n", error->message);
		g_clear_error(&error);
		g_option_context_free(context);
		g_strfreev(argv_copy);
		return -1;
	}
	if (version_information){
		std::cout << program_name << ' ' << version::versionFull << "-g" + std::string(version::hash) << "\nDate " << version::date << '\n';
		g_option_context_free(context);
		g_strfreev(argv_copy);
		return 0;
	}
	StartupOptions options;
	options.floating_picker_mode = pick_color;
	options.output_picked_color = output_picked_color;
	options.output_without_newline = output_without_newline;
	options.single_color_pick_mode = single_color_pick_mode;
	options.do_not_start = do_not_start;
	if (converter_name != nullptr)
		options.converter_name = converter_name;
	int return_value = 0;
	app_initialize();
	AppArgs *args = app_create_main(options, return_value);
	if (args){
		if (!single_color_pick_mode){
			if (commandline_filename){
				app_load_file(args, commandline_filename[0]);
			}else{
				if (app_is_autoload_enabled(args)){
					auto autosaveFile = buildConfigPath("autosave.gpa");
					app_load_file(args, autosaveFile, true);
				}
			}
		}
		if (commandline_geometry) app_parse_geometry(args, commandline_geometry);
		return_value = app_run(args);
	}
	g_option_context_free(context);
	g_strfreev(argv_copy);
	return return_value;
}
