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

#include "uiExport.h"
#include "uiListPalette.h"

#include <fstream>
#include <string>
using namespace std;


gint32 palette_export_gpl(Color* color, const gchar *name, void *userdata){
	(*((ofstream*)userdata))<<gint32(color->rgb.red*255)<<"\t"
						<<gint32(color->rgb.green*255)<<"\t"
						<<gint32(color->rgb.blue*255)<<"\t"<<name<<endl;
	return 0;
}

int show_palette_export_dialog(GtkWindow *parent, GtkWidget* palette, gboolean selected){
	GtkWidget *dialog;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new ("Export",	parent,
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						  NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "GIMP palette *.gpl");
	gtk_file_filter_add_pattern(filter, "*.gpl");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gchar* name = g_path_get_basename(filename);

		ofstream f(filename, ios::out | ios::trunc);
		if (f.is_open()){
			f<<"GIMP Palette"<<endl;
			f<<"Name: "<<name<<endl;
			f<<"#"<<endl;

			if (selected)
				palette_list_foreach_selected(palette, palette_export_gpl, &f);
			else
				palette_list_foreach(palette, palette_export_gpl, &f);

			f.close();
		}
		g_free(name);
		g_free(filename);
	}

	gtk_widget_destroy (dialog);
	return 0;
}
