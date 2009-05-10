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
#include "uiUtilities.h"
#include "uiListPalette.h"

#include <fstream>
#include <string>
using namespace std;


gint32 palette_export_gpl_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	(*(ofstream*)userdata)<<gint32(color.rgb.red*255)<<"\t"
						<<gint32(color.rgb.green*255)<<"\t"
						<<gint32(color.rgb.blue*255)<<"\t"<<name<<endl;
	return 0;
}

gint32 palette_export_gpl(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){

		gchar* name = g_path_get_basename(filename);

		f<<"GIMP Palette"<<endl;
		f<<"Name: "<<name<<endl;
		f<<"Columns: 1"<<endl;
		f<<"#"<<endl;

		g_free(name);

		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){ 
			 palette_export_gpl_color(*i, &f);
		}

		f.close();
		return 0;
	}
	return -1;
}



gint32 palette_export_mtl_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	(*(ofstream*)userdata)<<"newmtl "<<name<<endl;
	(*(ofstream*)userdata)<<"Ns 90.000000"<<endl;
	(*(ofstream*)userdata)<<"Ka 0.000000 0.000000 0.000000"<<endl;
	(*(ofstream*)userdata)<<"Kd "<<color.rgb.red<<" "<<color.rgb.green<<" "<<color.rgb.blue<<endl;
	(*(ofstream*)userdata)<<"Ks 0.500000 0.500000 0.500000"<<endl<<endl;
	return 0;
}



gint32 palette_export_mtl(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){
		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){ 
			 palette_export_mtl_color(*i, &f);
		}
		f.close();
		return 0;
	}
	return -1;
}




gint32 palette_export_ase_color(struct ColorObject* color_object, void* userdata){

	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	glong name_u16_len=0;
	gunichar2 *name_u16 = g_utf8_to_utf16(name, -1, 0, &name_u16_len, 0);
	for (glong i=0; i<name_u16_len; ++i){
		name_u16[i]=GINT16_FROM_BE(name_u16[i]);
	}

	guint16 color_entry=GINT16_FROM_BE(0x0001);
	(*(ofstream*)userdata).write((char*)&color_entry, 2);

	gint32 block_size = 2 + (name_u16_len + 1) * 2 + 4 + (3 * 4) + 2;
	block_size=GINT32_FROM_BE(block_size);
	(*(ofstream*)userdata).write((char*)&block_size, 4);

	guint16 name_length=GINT16_FROM_BE(guint16(name_u16_len+1));
	(*(ofstream*)userdata).write((char*)&name_length, 2);

	(*(ofstream*)userdata).write((char*)name_u16, (name_u16_len+1)*2);

	(*(ofstream*)userdata)<<"RGB ";

	guint32 r=GINT32_FROM_BE(*((guint*)&color.rgb.red)),
			g=GINT32_FROM_BE(*((guint*)&color.rgb.green)),
			b=GINT32_FROM_BE(*((guint*)&color.rgb.blue));


	(*(ofstream*)userdata).write((char*)&r, 4);
	(*(ofstream*)userdata).write((char*)&g, 4);
	(*(ofstream*)userdata).write((char*)&b, 4);

	gint16 color_type = GINT16_FROM_BE(0);
	(*(ofstream*)userdata).write((char*)&color_type, 2);

	g_free(name_u16);

	return 0;
}

gint32 palette_export_ase(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc | ios::binary);
	if (f.is_open()){
		f<<"ASEF";	//magic header
		guint32 version=GINT32_FROM_BE(0x00010000);
		f.write((char*)&version, 4);

		guint32 blocks=color_list_get_count(color_list);
		blocks=GINT32_FROM_BE(blocks);
		f.write((char*)&blocks, 4);

		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){ 
			 palette_export_ase_color(*i, &f);
		}

		/*if (selected)
			palette_list_foreach_selected(palette, palette_export_mtl_color, &f);
		else
			palette_list_foreach(palette, palette_export_ase_color, &f);*/

		//guint16 terminator=0;
		//f.write((char*)&terminator, 2);

		f.close();
		return 0;
	}
	return -1;
}



gint32 palette_export_txt_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	(*(ofstream*)userdata)<<color.rgb.red<<", "
						<<color.rgb.green<<", "
						<<color.rgb.blue<<", "<<name<<endl;
	return 0;
}

gint32 palette_export_txt(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){

		gchar* name = g_path_get_basename(filename);

		g_free(name);
		
		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){ 
			 palette_export_txt_color(*i, &f);
		}

		/*if (selected)
			palette_list_foreach_selected(palette, palette_export_txt_color, &f);
		else
			palette_list_foreach(palette, palette_export_txt_color, &f);*/
		f.close();
		return 0;
	}
	return -1;
}

int dialog_export_show(GtkWindow* parent, struct ColorList *color_list, struct ColorList *selected_color_list, GKeyFile* settings, gboolean selected) {
/*
int show_palette_export_dialog(GtkWindow *parent, struct ColorList *color_list, struct ColorList *selected_color_list, gboolean selected, GKeyFile* settings){*/
	GtkWidget *dialog;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new ("Export",	parent,
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						  NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	struct export_formats{
		const gchar* name;
		const gchar* pattern;
		gint32 (*export_function)(struct ColorList *color_list, const gchar* filename, gboolean selected);
	};
	struct export_formats formats[] = {
		{ "GIMP/Inkscape Palette *.gpl", "*.gpl", palette_export_gpl },
		{ "Alias/WaveFront Material *.mtl", "*.mtl", palette_export_mtl },
		{ "Adobe Swatch Exchange *.ase", "*.ase", palette_export_ase },
		{ "Text file *.txt", "*.txt", palette_export_txt },
	};


	gchar* default_path=g_key_file_get_string_with_default(settings, "Export Dialog", "Path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	//gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(dialog), default_filename);
	g_free(default_path);

	gchar* selected_filter=g_key_file_get_string_with_default(settings, "Export Dialog", "Filter", "*.gpl");

	for (gint i = 0; i != sizeof(formats) / sizeof(struct export_formats); ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		if (g_strcmp0(formats[i].pattern, selected_filter)==0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}

	g_free(selected_filter);

	gboolean saved=FALSE;

	while (!saved){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			g_key_file_set_string(settings, "Export Dialog", "Path", path);
			g_free(path);

			const gchar *format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
			for (gint i = 0; i != sizeof(formats) / sizeof(struct export_formats); ++i) {
				if (g_strcmp0(formats[i].name, format_name)==0){
					struct ColorList *color_list_arg;
					if (selected){
						color_list_arg=selected_color_list;
					}else{
						color_list_arg=color_list;
					}			
					if (formats[i].export_function(color_list_arg, filename, selected)==0){
						saved=TRUE;
					}else{
						GtkWidget* message;
						message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "File could not be exported");
						gtk_window_set_title(GTK_WINDOW(dialog), "Export");
						gtk_dialog_run(GTK_DIALOG(message));
						gtk_widget_destroy(message);
					}
					g_key_file_set_string(settings, "Export Dialog", "Filter", formats[i].pattern);
				}
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
	return 0;
}
