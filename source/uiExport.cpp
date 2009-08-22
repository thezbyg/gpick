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
#include "Endian.h"

#include <string.h>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;


int32_t palette_export_gpl_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	(*(ofstream*)userdata)<<int32_t(color.rgb.red*255)<<"\t"
						<<int32_t(color.rgb.green*255)<<"\t"
						<<int32_t(color.rgb.blue*255)<<"\t"<<name<<endl;
	return 0;
}

int32_t palette_export_gpl(struct ColorList *color_list, const gchar* filename, gboolean selected){
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

static void strip_leading_trailing_chars(string& x, string& stripchars){
   if (x.empty()) return;
   if (stripchars.empty()) return;

   size_t start = x.find_first_not_of(stripchars);
   size_t end = x.find_last_not_of(stripchars);

   if ((start==string::npos)||(end==string::npos)){
	   x.erase();
	   return;
   }

   x = x.substr(start, (end-start+ 1) );
}

int32_t palette_import_gpl(struct ColorList *color_list, const gchar* filename){
	ifstream f(filename, ios::in);
	if (f.is_open()){
		int r=0;
		string line;
		getline(f, line);
		if (f.good() && line=="GIMP Palette"){
			do{
				getline(f, line);
			}while (f.good() && line!="#");

			if (line=="#"){
				int r,g,b;
				Color c;
				struct ColorObject* color_object;
				string strip_chars=" \t";

				for(;;){
					getline(f, line);
					if (!f.good()) break;
					if (line[0]=='#') continue;

					stringstream ss(line);

					ss>>r>>g>>b;


					getline(ss, line);
					if (!f.good()) line="";
					strip_leading_trailing_chars(line, strip_chars);

					c.rgb.red=r/255.0;
					c.rgb.green=g/255.0;
					c.rgb.blue=b/255.0;
					color_object=color_list_new_color_object(color_list, &c);
					dynv_system_set(color_object->params, "string", "name", (void*)line.c_str());
					color_list_add_color_object(color_list, color_object, TRUE);
					color_object_release(color_object);
				}



			}else r=-1;


		}else r=-1;

		f.close();
		return r;
	}
	return -1;
}


int32_t palette_export_mtl_color(struct ColorObject* color_object, void* userdata){
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



int32_t palette_export_mtl(struct ColorList *color_list, const gchar* filename, gboolean selected){
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




int32_t palette_export_ase_color(struct ColorObject* color_object, void* userdata){

	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	glong name_u16_len=0;
	gunichar2 *name_u16 = g_utf8_to_utf16(name, -1, 0, &name_u16_len, 0);
	for (glong i=0; i<name_u16_len; ++i){
		name_u16[i]=UINT16_TO_BE(name_u16[i]);
	}

	uint16_t color_entry=UINT16_TO_BE(0x0001);
	(*(ofstream*)userdata).write((char*)&color_entry, 2);

	int32_t block_size = 2 + (name_u16_len + 1) * 2 + 4 + (3 * 4) + 2;	//name length + name (zero terminated and 2 bytes per char wide) + color name + 3 float values + color type
	block_size=UINT32_TO_BE(block_size);
	(*(ofstream*)userdata).write((char*)&block_size, 4);

	uint16_t name_length=UINT16_TO_BE(uint16_t(name_u16_len+1));
	(*(ofstream*)userdata).write((char*)&name_length, 2);

	(*(ofstream*)userdata).write((char*)name_u16, (name_u16_len+1)*2);

	(*(ofstream*)userdata)<<"RGB ";

	uint32_t r=UINT32_TO_BE(*((guint*)&color.rgb.red)),
			g=UINT32_TO_BE(*((guint*)&color.rgb.green)),
			b=UINT32_TO_BE(*((guint*)&color.rgb.blue));


	(*(ofstream*)userdata).write((char*)&r, 4);
	(*(ofstream*)userdata).write((char*)&g, 4);
	(*(ofstream*)userdata).write((char*)&b, 4);

	int16_t color_type = UINT16_TO_BE(0);
	(*(ofstream*)userdata).write((char*)&color_type, 2);

	g_free(name_u16);

	return 0;
}

int32_t palette_export_ase(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc | ios::binary);
	if (f.is_open()){
		f<<"ASEF";	//magic header
		uint32_t version=UINT32_TO_BE(0x00010000);
		f.write((char*)&version, 4);

		uint32_t blocks=color_list_get_count(color_list);
		blocks=UINT32_TO_BE(blocks);
		f.write((char*)&blocks, 4);

		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){ 
			 palette_export_ase_color(*i, &f);
		}

		/*if (selected)
			palette_list_foreach_selected(palette, palette_export_mtl_color, &f);
		else
			palette_list_foreach(palette, palette_export_ase_color, &f);*/

		//uint16_t terminator=0;
		//f.write((char*)&terminator, 2);

		f.close();
		return 0;
	}
	return -1;
}


int32_t palette_import_ase(struct ColorList *color_list, const gchar* filename){
	ifstream f(filename, ios::binary);
	if (f.is_open()){
		char magic[4];
		f.read(magic, 4);
		if (memcmp(magic, "ASEF", 4)!=0){
			f.close();
			return -1;
		}

		uint32_t version;
		f.read((char*)&version, 4);
		version=UINT32_FROM_BE(version);

		uint32_t blocks;
		f.read((char*)&blocks, 4);
		blocks=UINT32_FROM_BE(blocks);

		uint16_t block_type;
		uint32_t block_size;
		int color_supported;

	    matrix3x3 adaptation_matrix, working_space_matrix;
	    vector3 d50, d65;
	    vector3_set(&d50, 96.442, 100.000,  82.821);
	    vector3_set(&d65, 95.047, 100.000, 108.883);
	    color_get_chromatic_adaptation_matrix(&d50, &d65, &adaptation_matrix);
	    color_get_working_space_matrix(0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, &d65, &working_space_matrix);
	    matrix3x3_inverse(&working_space_matrix, &working_space_matrix);

		for (uint32_t i=0; i<blocks; ++i){
			f.read((char*)&block_type, 2);
			block_type=UINT16_FROM_BE(block_type);
			f.read((char*)&block_size, 4);
			block_size=UINT32_FROM_BE(block_size);

			switch (block_type){
			case 0x0001: //color block
				{
					uint16_t name_length;
					f.read((char*)&name_length, 2);
					name_length=UINT16_FROM_BE(name_length);

					gunichar2 *name_u16=(gunichar2*)g_malloc(name_length*2);
					f.read((char*)name_u16, name_length*2);
					for (uint32_t j=0; j<name_length; ++j){
						name_u16[j]=UINT16_FROM_BE(name_u16[j]);
					}
					gchar *name=g_utf16_to_utf8(name_u16, name_length, 0, 0, 0);

					Color c;

					char colorspace[4];
					f.read(colorspace, 4);
					color_supported=0;

					if (memcmp(colorspace, "RGB ", 4)==0){
						uint32_t rgb[3];
						f.read((char*)&rgb[0], 4);
						f.read((char*)&rgb[1], 4);
						f.read((char*)&rgb[2], 4);

						rgb[0]=UINT32_FROM_BE(rgb[0]);
						rgb[1]=UINT32_FROM_BE(rgb[1]);
						rgb[2]=UINT32_FROM_BE(rgb[2]);

						c.rgb.red=((float*)&rgb[0])[0];
						c.rgb.green=((float*)&rgb[1])[0];
						c.rgb.blue=((float*)&rgb[2])[0];

						color_supported=1;

					}else if (memcmp(colorspace, "CMYK", 4)==0){
						uint32_t cmyk[4];
						f.read((char*)&cmyk[0], 4);
						f.read((char*)&cmyk[1], 4);
						f.read((char*)&cmyk[2], 4);
						f.read((char*)&cmyk[3], 4);

						cmyk[0]=UINT32_FROM_BE(cmyk[0]);
						cmyk[1]=UINT32_FROM_BE(cmyk[1]);
						cmyk[2]=UINT32_FROM_BE(cmyk[2]);
						cmyk[3]=UINT32_FROM_BE(cmyk[3]);

						c.cmyk.c=((float*)&cmyk[0])[0];
						c.cmyk.m=((float*)&cmyk[1])[0];
						c.cmyk.y=((float*)&cmyk[2])[0];
						c.cmyk.k=((float*)&cmyk[3])[0];

						color_supported=0; //no cmyk support

					}else if (memcmp(colorspace, "Gray", 4)==0){
						uint32_t gray;
						f.read((char*)&gray, 4);
						gray=UINT32_FROM_BE(gray);
						c.rgb.red=c.rgb.green=c.rgb.blue=((float*)&gray)[0];

						color_supported=1;

					}else if (memcmp(colorspace, "LAB ", 4)==0){
						Color c2, c3;
						uint32_t lab[3];
						f.read((char*)&lab[0], 4);
						f.read((char*)&lab[1], 4);
						f.read((char*)&lab[2], 4);

						lab[0]=UINT32_FROM_BE(lab[0]);
						lab[1]=UINT32_FROM_BE(lab[1]);
						lab[2]=UINT32_FROM_BE(lab[2]);

						c2.lab.L=((float*)&lab[0])[0]*100;
						c2.lab.a=((float*)&lab[1])[0];
						c2.lab.b=((float*)&lab[2])[0];

					    color_lab_to_xyz(&c2, &c3, &d50);
						color_xyz_chromatic_adaptation(&c3, &c3, &adaptation_matrix);
					    color_xyz_to_rgb(&c3, &c, &working_space_matrix);

					    c.rgb.red=clamp_float(c.rgb.red, 0, 1);
					    c.rgb.green=clamp_float(c.rgb.green, 0, 1);
					    c.rgb.blue=clamp_float(c.rgb.blue, 0, 1);

						color_supported=1;

					}
					if (color_supported){

						struct ColorObject* color_object;
						color_object=color_list_new_color_object(color_list, &c);
						dynv_system_set(color_object->params, "string", "name", name);
						color_list_add_color_object(color_list, color_object, TRUE);
						color_object_release(color_object);

					}

					uint16_t color_type;
					f.read((char*)&color_type, 2);

					g_free(name);
				}
				break;
			default:
				f.seekg(block_size, ios::cur);
			}
		}

		f.close();
		return 0;
	}
	return -1;
}


int32_t palette_export_txt_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const gchar* name=(const gchar*)dynv_system_get(color_object->params, "string", "name");
	if (name==NULL) name="";
	
	(*(ofstream*)userdata)<<color.rgb.red<<", "
						<<color.rgb.green<<", "
						<<color.rgb.blue<<", "<<name<<endl;
	return 0;
}

int32_t palette_export_txt(struct ColorList *color_list, const gchar* filename, gboolean selected){
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

	GtkWidget *dialog;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new ("Export",	parent,
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	struct export_formats{
		const gchar* name;
		const gchar* pattern;
		int32_t (*export_function)(struct ColorList *color_list, const gchar* filename, gboolean selected);
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
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
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



int dialog_import_show(GtkWindow* parent, struct ColorList *color_list, struct ColorList *selected_color_list, GKeyFile* settings){
	GtkWidget *dialog;

	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new ("Import",	parent,
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	struct import_formats{
		const gchar* name;
		const gchar* pattern;
		int32_t (*import_function)(struct ColorList *color_list, const gchar* filename);
	};
	struct import_formats formats[] = {
		{ "GIMP/Inkscape Palette *.gpl", "*.gpl", palette_import_gpl },
		{ "Adobe Swatch Exchange *.ase", "*.ase", palette_import_ase },

		/*{ "Alias/WaveFront Material *.mtl", "*.mtl", palette_export_mtl },

		{ "Text file *.txt", "*.txt", palette_export_txt },*/
	};


	gchar* default_path=g_key_file_get_string_with_default(settings, "Import Dialog", "Path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	//gtk_file_chooser_set_filename (GTK_FILE_CHOOSER(dialog), default_filename);
	g_free(default_path);

	gchar* selected_filter=g_key_file_get_string_with_default(settings, "Import Dialog", "Filter", "*.gpl");

	for (gint i = 0; i != sizeof(formats) / sizeof(struct import_formats); ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		if (g_strcmp0(formats[i].pattern, selected_filter)==0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}

	g_free(selected_filter);

	gboolean finished=FALSE;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			g_key_file_set_string(settings, "Import Dialog", "Path", path);
			g_free(path);

			const gchar *format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
			for (gint i = 0; i != sizeof(formats) / sizeof(struct import_formats); ++i) {
				if (g_strcmp0(formats[i].name, format_name)==0){
					struct ColorList *color_list_arg;
					color_list_arg=color_list;
					if (formats[i].import_function(color_list_arg, filename)==0){
						finished=TRUE;
					}else{
						GtkWidget* message;
						message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "File could not be imported");
						gtk_window_set_title(GTK_WINDOW(dialog), "Import");
						gtk_dialog_run(GTK_DIALOG(message));
						gtk_widget_destroy(message);
					}
					g_key_file_set_string(settings, "Import Dialog", "Filter", formats[i].pattern);
				}
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
	return 0;
}

