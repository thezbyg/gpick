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


#include "main.h"
#include "Paths.h"
#include "uiAbout.h"
#include "uiApp.h"

#include <gtk/gtk.h>

static gchar **commandline_filename=NULL;
static gchar *commandline_geometry=NULL;
static GOptionEntry commandline_entries[] =
{
  { "geometry",	'g', 0, 		G_OPTION_ARG_STRING,			&commandline_geometry, "Window geometry", "GEOMETRY" },
  { G_OPTION_REMAINING, 0, 0, 	G_OPTION_ARG_FILENAME_ARRAY, 	&commandline_filename, NULL, "[FILE...]" },
  { NULL }
};

int main(int argc, char **argv){

	gtk_set_locale ();
	gtk_init (&argc, &argv);
	g_set_application_name(program_name);

	gchar* tmp;

	GtkIconTheme *icon_theme;
	icon_theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_append_search_path(icon_theme, tmp=build_filename(0));
	g_free(tmp);

	GError *error = NULL;
	GOptionContext *context;
	context = g_option_context_new ("- advanced color picker");
	g_option_context_add_main_entries (context, commandline_entries, 0);

	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)){
		g_print ("option parsing failed: %s\n", error->message);
		return -1;
	}
	
	struct Arguments *args = app_create_main();
	if (args){
	
		if (commandline_filename) app_load_file(args, commandline_filename[0]);
		if (commandline_geometry) app_parse_geometry(args, commandline_geometry);

		int r = app_run(args);
	}
	
	g_option_context_free(context);
	
	return 0;
	
}
