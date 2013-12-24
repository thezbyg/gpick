/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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
#include "FileFormat.h"
#include "uiUtilities.h"
#include "uiListPalette.h"
#include "Endian.h"
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"
#include "Internationalisation.h"
#include "version/Version.h"

#include <string.h>
#include <math.h>

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <boost/math/special_functions/round.hpp>

using namespace std;


static int32_t palette_export_gpl_color(struct ColorObject* color_object, void* userdata){
	using boost::math::iround;
	Color color;
	color_object_get_color(color_object, &color);
	const char* name = dynv_get_string_wd(color_object->params, "name", "");

	(*(ofstream*)userdata)
		<< iround(color.rgb.red*255) << "\t"
		<< iround(color.rgb.green*255) << "\t"
		<< iround(color.rgb.blue*255) << "\t" << name << endl;
	return 0;
}


static vector<struct ColorObject*> get_ordered_list(struct ColorList *color_list){
		vector<struct ColorObject*> ordered(color_list_get_count(color_list));
		color_list_get_positions(color_list);

		uint32_t max_index = ~(uint32_t)0;
		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
			if ((*i)->position != ~(uint32_t)0){
				ordered[(*i)->position] = *i;
				if (max_index == ~(uint32_t)0){
					max_index = (*i)->position;
				}else if ((*i)->position > max_index){
          max_index = (*i)->position;
				}
			}
		}

		if (max_index == ~(uint32_t)0){
			return vector<struct ColorObject*>();
		}else{
			ordered.resize(max_index + 1);
			return ordered;
		}
}

int32_t palette_import_gpa(struct ColorList *color_list, const gchar* filename){
	return palette_file_load (filename, color_list);
}

int32_t palette_export_gpa(struct ColorList *color_list, const gchar* filename, gboolean selected){
	return palette_file_save (filename, color_list);
}

int32_t palette_export_gpl(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){

		gchar* name = g_path_get_basename(filename);

		f << "GIMP Palette" << endl;
		f << "Name: " << name << endl;
		f << "Columns: 1" << endl;
		f << "#" << endl;

		g_free(name);

		vector<struct ColorObject*> ordered = get_ordered_list(color_list);
		for (vector<struct ColorObject*>::iterator i=ordered.begin(); i!=ordered.end(); ++i){
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

   if ((start == string::npos) || (end == string::npos)){
	   x.erase();
	   return;
   }

   x = x.substr(start, end - start + 1);
}

int32_t palette_import_gpl(struct ColorList *color_list, const gchar* filename){
	ifstream f(filename, ios::in);
	if (f.is_open()){
		int r = 0;
		string line;
		getline(f, line);
		if (f.good() && line=="GIMP Palette"){
			do{
				getline(f, line);
			}while (f.good() && ((line.size() < 1) || (((line[0] > '9') || (line[0] < '0')) && line[0] != ' ')));

			int r, g, b;
			Color c;
			struct ColorObject* color_object;
			string strip_chars = " \t";

			for(;;){
				if (!f.good()) break;
				if (line[0]!='#'){
					stringstream ss(line);

					ss >> r >> g >> b;

					getline(ss, line);
					if (!f.good()) line = "";
					strip_leading_trailing_chars(line, strip_chars);

					c.rgb.red = r / 255.0;
					c.rgb.green = g / 255.0;
					c.rgb.blue = b / 255.0;
					color_object=color_list_new_color_object(color_list, &c);
					dynv_set_string(color_object->params, "name", line.c_str());
					color_list_add_color_object(color_list, color_object, TRUE);
					color_object_release(color_object);
				}
				getline(f, line);
			}

		}else r = -1;

		f.close();
		return r;
	}
	return -1;
}


static int32_t palette_export_mtl_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const char* name = dynv_get_string_wd(color_object->params, "name", "");

	(*(ofstream*)userdata) << "newmtl " << name << endl;
	(*(ofstream*)userdata) << "Ns 90.000000" << endl;
	(*(ofstream*)userdata) << "Ka 0.000000 0.000000 0.000000" << endl;
	(*(ofstream*)userdata) << "Kd " << color.rgb.red << " " << color.rgb.green << " " << color.rgb.blue << endl;
	(*(ofstream*)userdata) << "Ks 0.500000 0.500000 0.500000" << endl << endl;
	return 0;
}



static int32_t palette_export_mtl(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){

    vector<struct ColorObject*> ordered = get_ordered_list(color_list);
		for (vector<struct ColorObject*>::iterator i=ordered.begin(); i!=ordered.end(); ++i){
			 palette_export_mtl_color(*i, &f);
		}
		f.close();
		return 0;
	}
	return -1;
}

typedef union FloatInt{
	float f;
	uint32_t i;
}FloatInt;

static int32_t palette_export_ase_color(struct ColorObject* color_object, void* userdata){

	Color color;
	color_object_get_color(color_object, &color);
	const char* name = dynv_get_string_wd(color_object->params, "name", "");

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

	FloatInt r, g, b;
	r.f = color.rgb.red;
	g.f = color.rgb.green;
	b.f = color.rgb.blue;

	r.i = UINT32_TO_BE(r.i);
	g.i = UINT32_TO_BE(g.i);
	b.i = UINT32_TO_BE(b.i);

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
		f << "ASEF";	//magic header
		uint32_t version = UINT32_TO_BE(0x00010000);
		f.write((char*)&version, 4);

		uint32_t blocks=color_list_get_count(color_list);
		blocks=UINT32_TO_BE(blocks);
		f.write((char*)&blocks, 4);

    vector<struct ColorObject*> ordered = get_ordered_list(color_list);
		for (vector<struct ColorObject*>::iterator i=ordered.begin(); i!=ordered.end(); ++i){
			 palette_export_ase_color(*i, &f);
		}

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
					gchar *name = g_utf16_to_utf8(name_u16, name_length, 0, 0, 0);

					Color c;

					char color_space[4];
					f.read(color_space, 4);
					color_supported = 0;

					if (memcmp(color_space, "RGB ", 4)==0){
						FloatInt rgb[3];
						f.read((char*)&rgb[0], 4);
						f.read((char*)&rgb[1], 4);
						f.read((char*)&rgb[2], 4);

						rgb[0].i = UINT32_FROM_BE(rgb[0].i);
						rgb[1].i = UINT32_FROM_BE(rgb[1].i);
						rgb[2].i = UINT32_FROM_BE(rgb[2].i);

						c.rgb.red = rgb[0].f;
						c.rgb.green = rgb[1].f;
						c.rgb.blue = rgb[2].f;

						color_supported = 1;

					}else if (memcmp(color_space, "CMYK", 4)==0){
						Color c2;
						FloatInt cmyk[4];

						f.read((char*)&cmyk[0], 4);
						f.read((char*)&cmyk[1], 4);
						f.read((char*)&cmyk[2], 4);
						f.read((char*)&cmyk[3], 4);

						cmyk[0].i = UINT32_FROM_BE(cmyk[0].i);
						cmyk[1].i = UINT32_FROM_BE(cmyk[1].i);
						cmyk[2].i = UINT32_FROM_BE(cmyk[2].i);
						cmyk[3].i = UINT32_FROM_BE(cmyk[3].i);

						c2.cmyk.c = cmyk[0].f;
						c2.cmyk.m = cmyk[1].f;
						c2.cmyk.y = cmyk[2].f;
						c2.cmyk.k = cmyk[3].f;

						color_cmyk_to_rgb(&c2, &c);

						color_supported = 1;

					}else if (memcmp(color_space, "Gray", 4)==0){
						FloatInt gray;
						f.read((char*)&gray, 4);
						gray.i = UINT32_FROM_BE(gray.i);
						c.rgb.red = c.rgb.green = c.rgb.blue = gray.f;

						color_supported = 1;

					}else if (memcmp(color_space, "LAB ", 4)==0){
						Color c2;
						FloatInt lab[3];
						f.read((char*)&lab[0], 4);
						f.read((char*)&lab[1], 4);
						f.read((char*)&lab[2], 4);

						lab[0].i = UINT32_FROM_BE(lab[0].i);
						lab[1].i = UINT32_FROM_BE(lab[1].i);
						lab[2].i = UINT32_FROM_BE(lab[2].i);

						c2.lab.L = lab[0].f*100;
						c2.lab.a = lab[1].f;
						c2.lab.b = lab[2].f;

						color_lab_to_rgb_d50(&c2, &c);

						c.rgb.red = clamp_float(c.rgb.red, 0, 1);
						c.rgb.green = clamp_float(c.rgb.green, 0, 1);
						c.rgb.blue = clamp_float(c.rgb.blue, 0, 1);

						color_supported = 1;

					}
					if (color_supported){

						struct ColorObject* color_object;
						color_object=color_list_new_color_object(color_list, &c);
						dynv_set_string(color_object->params, "name", name);
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


static int32_t palette_export_txt_color(struct ColorObject* color_object, void* userdata){
	Color color;
	color_object_get_color(color_object, &color);
	const char* name = dynv_get_string_wd(color_object->params, "name", 0);

	(*(ofstream*)userdata)<<color.rgb.red<<", "
						<<color.rgb.green<<", "
						<<color.rgb.blue<<", "<<name<<endl;
	return 0;
}

static int32_t palette_export_txt(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){

		gchar* name = g_path_get_basename(filename);

		g_free(name);

    vector<struct ColorObject*> ordered = get_ordered_list(color_list);
		for (vector<struct ColorObject*>::iterator i=ordered.begin(); i!=ordered.end(); ++i){
			 palette_export_txt_color(*i, &f);
		}

		f.close();
		return 0;
	}
	return -1;
}

static int32_t palette_export_css_color(struct ColorObject* color_object, void* userdata){
	using boost::math::iround;
	Color color, hsl;
	color_object_get_color(color_object, &color);
	const char* name = dynv_get_string_wd(color_object->params, "name", 0);
	int r, g, b, h, s, l;
	r = iround(color.rgb.red * 255);
	g = iround(color.rgb.green * 255);
	b = iround(color.rgb.blue * 255);
	color_rgb_to_hsl(&color, &hsl);
	h = iround(hsl.hsl.hue * 360);
	s = iround(hsl.hsl.saturation * 100);
	l = iround(hsl.hsl.lightness * 100);
	(*(ofstream*)userdata) << " * " << name 
		<< ": #" << hex << r << g << b
		<< ", rgb(" << dec << r << ", " << g << ", " << b
		<< "), hsl(" << dec << h << ", " << s << "%, " << l << "%)" << endl;
	return 0;
}

int32_t palette_export_css(struct ColorList *color_list, const gchar* filename, gboolean selected){
	ofstream f(filename, ios::out | ios::trunc);
	if (f.is_open()){
		gchar* name = g_path_get_basename(filename);
		g_free(name);
		f << "/**" << endl << " * Generated by Gpick " << gpick_build_version << endl;
    vector<struct ColorObject*> ordered = get_ordered_list(color_list);
		for (vector<struct ColorObject*>::iterator i = ordered.begin(); i != ordered.end(); ++i){
			 palette_export_css_color(*i, &f);
		}
		f << " */" << endl;
		f.close();
		return 0;
	}
	return -1;
}

int dialog_export_show(GtkWindow* parent, struct ColorList *selected_color_list, gboolean selected, GlobalState *gs){

	GtkWidget *dialog;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new (_("Export"),	parent,
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
		{ _("Gpick Palette (*.gpa)"), "*.gpa", palette_export_gpa },
		{ _("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", palette_export_gpl },
		{ _("Alias/WaveFront Material (*.mtl)"), "*.mtl", palette_export_mtl },
		{ _("Adobe Swatch Exchange (*.ase)"), "*.ase", palette_export_ase },
		{ _("Cascading Style Sheet (*.css)"), "*.css", palette_export_css },
		{ _("Text file (*.txt)"), "*.txt", palette_export_txt },
	};


	const char* default_path = dynv_get_string_wd(gs->params, "gpick.export.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);

	const char* selected_filter = dynv_get_string_wd(gs->params, "gpick.export.filter", "*.gpl");

	for (gint i = 0; i != sizeof(formats) / sizeof(struct export_formats); ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		if (g_strcmp0(formats[i].pattern, selected_filter)==0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}

	gboolean saved=FALSE;

	while (!saved){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(gs->params, "gpick.import.path", path);
			g_free(path);

			const gchar *format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
			for (gint i = 0; i != sizeof(formats) / sizeof(struct export_formats); ++i) {
				if (g_strcmp0(formats[i].name, format_name)==0){
					struct ColorList *color_list_arg;
					if (selected){
						color_list_arg = selected_color_list;
					}else{
						color_list_arg = gs->colors;
					}
					if (formats[i].export_function(color_list_arg, filename, selected)==0){
						saved=TRUE;
					}else{
						GtkWidget* message;
						message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be exported"));
						gtk_window_set_title(GTK_WINDOW(dialog), _("Export"));
						gtk_dialog_run(GTK_DIALOG(message));
						gtk_widget_destroy(message);
					}
					dynv_set_string(gs->params, "gpick.export.filter", formats[i].pattern);
				}
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
	return 0;
}

enum FileType{
	GPA,
	GPL,
	ASE,
	UNKNOWN,
};

static FileType get_file_type_from_ext(const char *filename){
	const struct{
		FileType type;
		const char *extension;
	}extensions[] = {
		{GPA, ".gpa"},
		{GPL, ".gpl"},
		{ASE, ".ase"},
		{UNKNOWN, 0},
	};
	const char *ext = g_strrstr(filename, ".");
	if (ext){
		for (int i = 0; extensions[i].type != UNKNOWN; ++i){
			if (g_ascii_strcasecmp(ext, extensions[i].extension) == 0){
				return extensions[i].type;
			}
		}
	}
	return UNKNOWN;
}

int dialog_import_show(GtkWindow* parent, struct ColorList *selected_color_list, GlobalState *gs){

	GtkWidget *dialog;
	GtkWidget* message;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new (_("Import"),	parent,
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	const struct import_formats{
		FileType type;
		const gchar* name;
		const gchar* pattern;
		int32_t (*import_function)(struct ColorList *color_list, const gchar* filename);
	}formats[] = {
		{GPA, _("Gpick Palette (*.gpa)"), "*.gpa", palette_import_gpa },
		{GPL, _("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", palette_import_gpl },
		{ASE, _("Adobe Swatch Exchange (*.ase)"), "*.ase", palette_import_ase },
		{UNKNOWN, NULL, NULL, NULL},
	};

	const char* default_path = dynv_get_string_wd(gs->params, "gpick.import.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	const char* selected_filter = dynv_get_string_wd(gs->params, "gpick.import.filter", "all_supported");

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer)"all");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (g_strcmp0(selected_filter, "all") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All supported formats"));
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer)"all_supported");
	for (gint i = 0; formats[i].name; ++i) {
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
	}
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (g_strcmp0(selected_filter, "all_supported") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	for (gint i = 0; formats[i].name; ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		g_object_set_data(G_OBJECT(filter), "identification", (gpointer)formats[i].pattern);
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		if (g_strcmp0(formats[i].pattern, selected_filter)==0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}

	gboolean finished = false;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(gs->params, "gpick.import.path", path);
			g_free(path);

      FileType type = get_file_type_from_ext(filename);
			if (type == UNKNOWN){
				const gchar *format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
				for (int i = 0; formats[i].name; ++i) {
					if (g_strcmp0(formats[i].name, format_name) == 0){
						type = formats[i].type;
						break;
					}
				}
			}

			if (type == UNKNOWN){
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File format is not supported"));
				gtk_window_set_title(GTK_WINDOW(message), _("Import"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}else{
				for (int i = 0; formats[i].name; ++i) {
					if (formats[i].type == type){
						struct ColorList *color_list_arg;
						color_list_arg = gs->colors;
						if (formats[i].import_function(color_list_arg, filename) == 0){
							finished = true;
						}else{
							message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be imported"));
							gtk_window_set_title(GTK_WINDOW(message), _("Import"));
							gtk_dialog_run(GTK_DIALOG(message));
							gtk_widget_destroy(message);
						}

						const char *identification = (const char*)g_object_get_data(G_OBJECT(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog))), "identification");
						dynv_set_string(gs->params, "gpick.import.filter", identification);
						break;
					}
				}
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
	return 0;
}

