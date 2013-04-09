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

#include "uiApp.h"
#include "GlobalState.h"
#include "GlobalStateStruct.h"


#include "Paths.h"
#include "Converter.h"
#include "CopyPaste.h"

#include "RegisterSources.h"

#include "GenerateScheme.h"
#include "ColorPicker.h"
#include "LayoutPreview.h"

#include "uiAbout.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "uiExport.h"
#include "uiDialogMix.h"
#include "uiDialogVariations.h"
#include "uiDialogGenerate.h"
#include "uiDialogAutonumber.h"
#include "uiDialogSort.h"
#include "uiTransformations.h"

#include "tools/PaletteFromImage.h"
#include "tools/PaletteFromCssFile.h"

#include "dbus/Control.h"

#include "uiDialogOptions.h"
#include "uiConverter.h"
#include "uiStatusIcon.h"

#include "DynvHelpers.h"
#include "FileFormat.h"
#include "MathUtil.h"

#include "Internationalisation.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>
using namespace std;

typedef struct AppArgs{
	GtkWidget *window;

	map<string, ColorSource*> color_source;
	vector<ColorSource*> color_source_index;

	list<string> recent_files;

	ColorSourceManager *csm;
	ColorSource *current_color_source;

	ColorSource *secondary_color_source;
	GtkWidget *secondary_source_widget;
	GtkWidget *secondary_source_scrolled_viewpoint;
	GtkWidget *secondary_source_container;

	GtkWidget *color_list;
	GtkWidget* notebook;
	GtkWidget* statusbar;
	GtkWidget* hpaned;
	GtkWidget* vpaned;

	uiStatusIcon* statusIcon;
	FloatingPicker floating_picker;
	AppOptions options;
	guint bus_id;

	struct dynvSystem *params;
	GlobalState* gs;

	char* current_filename;
	bool imported;
	GtkWidget *precision_loss_icon;

	gint x, y;
	gint width, height;

}AppArgs;

static void updateRecentFileList(AppArgs *args, const char *filename, bool move_up){
	list<string>::iterator i = std::find(args->recent_files.begin(), args->recent_files.end(), string(filename));
	if (i == args->recent_files.end()){
		args->recent_files.push_front(string(filename));
		while (args->recent_files.size() > 10){
			args->recent_files.pop_back();
		}
	}else{
		if (move_up){
			args->recent_files.erase(i);
			args->recent_files.push_front(string(filename));
		}
	}
}

static gboolean delete_event( GtkWidget *widget, GdkEvent *event, AppArgs *args ){

	if (dynv_get_bool_wd(args->params, "close_to_tray", false)){
		gtk_widget_hide(args->window);
		status_icon_set_visible(args->statusIcon, true);
		return true;
	}

	return false;
}

static gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, AppArgs *args){

	if (dynv_get_bool_wd(args->params, "minimize_to_tray", false)){

		if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED){
			if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED){
				gtk_widget_hide(args->window);
				gtk_window_deiconify(GTK_WINDOW(args->window));
				status_icon_set_visible(args->statusIcon, true);
				return true;
			}
		}
	}

	return false;
}

static gboolean on_window_configure(GtkWidget *widget, GdkEventConfigure *event, AppArgs *args){

	if (GTK_WIDGET_VISIBLE(widget)){

		if (gdk_window_get_state(widget->window) & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_ICONIFIED)) {
			return false;
		}

		gint x, y;
		gtk_window_get_position(GTK_WINDOW(widget), &x, &y);

		args->x = x;
		args->y = y;
		args->width = event->width;
		args->height = event->height;

		dynv_set_int32(args->params, "window.x", args->x);
		dynv_set_int32(args->params, "window.y", args->y);
		dynv_set_int32(args->params, "window.width", args->width);
		dynv_set_int32(args->params, "window.height", args->height);

	}

	return false;
}

static void notebook_switch_cb(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, AppArgs *args){
	if (args->current_color_source) color_source_deactivate(args->current_color_source);
	args->current_color_source = NULL;

	if (page_num >= 0 && page_num <= args->color_source_index.size() && args->color_source_index[page_num]){
		color_source_activate(args->color_source_index[page_num]);
		args->current_color_source = args->color_source_index[page_num];
		dynv_set_pointer(args->gs->params, "CurrentColorSource", args->current_color_source);
	}else{
		dynv_set_pointer(args->gs->params, "CurrentColorSource", NULL);
	}
}

static void destroy_cb(GtkWidget *widget, AppArgs *args){
	g_signal_handlers_disconnect_matched(G_OBJECT(args->notebook), G_SIGNAL_MATCH_FUNC, 0, NULL, NULL, (void*)notebook_switch_cb, 0);       //disconnect notebook switch callback, because destroying child widgets triggers it


	//dynv_set_int32(args->params, "notebook_page", gtk_notebook_get_current_page(GTK_NOTEBOOK(args->notebook)));
	dynv_set_string(args->params, "color_source", args->color_source_index[gtk_notebook_get_current_page(GTK_NOTEBOOK(args->notebook))]->identificator);
	dynv_set_int32(args->params, "paned_position", gtk_paned_get_position(GTK_PANED(args->hpaned)));
	dynv_set_int32(args->params, "vertical_paned_position", gtk_paned_get_position(GTK_PANED(args->vpaned)));

	if (args->current_color_source){
		color_source_deactivate(args->current_color_source);
		args->current_color_source = 0;
	}
	if (args->secondary_color_source){
		dynv_set_string(args->params, "secondary_color_source", args->secondary_color_source->identificator);
		color_source_deactivate(args->secondary_color_source);
		color_source_destroy(args->secondary_color_source);
		args->secondary_color_source = 0;
		args->secondary_source_widget = 0;
	}else{
		dynv_set_string(args->params, "secondary_color_source", "");
	}

	for (map<string, ColorSource*>::iterator i = args->color_source.begin(); i != args->color_source.end(); ++i){
		color_source_destroy((*i).second);
	}
	args->color_source.clear();
	args->color_source_index.clear();

	dynv_set_int32(args->params, "window.x", args->x);
	dynv_set_int32(args->params, "window.y", args->y);
	dynv_set_int32(args->params, "window.width", args->width);
	dynv_set_int32(args->params, "window.height", args->height);

	floating_picker_free(args->floating_picker);

	if (app_is_autoload_enabled(args)){
		gchar* autosave_file = build_config_path("autosave.gpa");
		palette_file_save(autosave_file, args->gs->colors);
		g_free(autosave_file);
	}

	color_list_remove_all(args->gs->colors);
	gtk_main_quit();
}


int main_get_color_object_from_text(GlobalState* gs, char* text, struct ColorObject** output_color_object){
	struct ColorObject* color_object;
	Color dummy_color;

	Converters *converters = (Converters*)dynv_get_pointer_wd(gs->params, "Converters", 0);

	typedef multimap<float, struct ColorObject*, greater<float> > ValidConverters;
	ValidConverters valid_converters;

	Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_DISPLAY);

	if (converter){
		if (converter->deserialize_available){
			color_object = color_list_new_color_object(gs->colors, &dummy_color);
			float quality;
			if (converters_color_deserialize(converters, converter->function_name, text, color_object, &quality)==0){
				if (quality>0){
					valid_converters.insert(make_pair(quality, color_object));
				}else{
					color_object_release(color_object);
				}
			}else{
				color_object_release(color_object);
			}
		}
	}

	uint32_t table_size;
	Converter **converter_table;
	if ((converter_table = converters_get_all_type(converters, CONVERTERS_ARRAY_TYPE_PASTE, &table_size))){
		for (uint32_t i=0; i!=table_size; ++i){
			converter = converter_table[i];
			if (converter->deserialize_available){
				color_object = color_list_new_color_object(gs->colors, &dummy_color);
				float quality;
				if (converters_color_deserialize(converters, converter->function_name, text, color_object, &quality)==0){
					if (quality>0){
						valid_converters.insert(make_pair(quality, color_object));
					}else{
						color_object_release(color_object);
					}
				}else{
					color_object_release(color_object);
				}
			}
		}
	}

	bool first = true;

	for (ValidConverters::iterator i=valid_converters.begin(); i!=valid_converters.end(); ++i){
		if (first){
			first = false;
			*output_color_object = (*i).second;
		}else{
			color_object_release((*i).second);
		}
	}

	if (first){
		return -1;
	}else{
		return 0;
	}
}

int main_get_color_from_text(GlobalState* gs, char* text, Color* color){
	struct ColorObject* color_object = 0;
	if (main_get_color_object_from_text(gs, text, &color_object)==0){
		color_object_get_color(color_object, color);
		color_object_release(color_object);
		return 0;
	}
	return -1;
}

char* main_get_color_text(GlobalState* gs, Color* color, ColorTextType text_type){
	char* text = 0;
	struct ColorObject* color_object;
	color_object = color_list_new_color_object(gs->colors, color);
	Converter *converter;

	Converters *converters = (Converters*)dynv_get_pointer_wd(gs->params, "Converters", 0);

	switch (text_type){
	case COLOR_TEXT_TYPE_DISPLAY:
		converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_DISPLAY);
		if (converter){
			converter_get_text(converter->function_name, color_object, 0, gs->params, &text);
		}
		break;
	case COLOR_TEXT_TYPE_COPY:
		converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
		if (converter){
			converter_get_text(converter->function_name, color_object, 0, gs->params, &text);
		}
		break;
	case COLOR_TEXT_TYPE_COLOR_LIST:
		converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COLOR_LIST);
		if (converter){
			converter_get_text(converter->function_name, color_object, 0, gs->params, &text);
		}
		break;
	}

	color_object_release(color_object);

	return text;
}

static void updateProgramName(AppArgs *args){
	stringstream prg_name;
	if (args->current_filename == 0){
		prg_name << _("New palette");
		if (args->precision_loss_icon)
			gtk_widget_hide(args->precision_loss_icon);
	}else{
		gchar* filename = g_path_get_basename(args->current_filename);
		if (args->imported){
			prg_name << filename << " " << _("(Imported)");
			if (args->precision_loss_icon)
				gtk_widget_show(args->precision_loss_icon);
		}else{
			prg_name << filename;
			if (args->precision_loss_icon)
				gtk_widget_hide(args->precision_loss_icon);
		}
		g_free(filename);
	}
	prg_name << " - " << program_name;

	gtk_window_set_title(GTK_WINDOW(args->window), prg_name.str().c_str());
}

static void show_dialog_converter(GtkWidget *widget, AppArgs *args){
	dialog_converter_show(GTK_WINDOW(args->window), args->gs);
	return;
}

static void show_dialog_transformations(GtkWidget *widget, AppArgs *args){
	dialog_transformations_show(GTK_WINDOW(args->window), args->gs);
	return;
}

static void show_dialog_options(GtkWidget *widget, AppArgs *args){
	dialog_options_show(GTK_WINDOW(args->window), args->gs);
	return;
}


static void menu_file_new(GtkWidget *widget, AppArgs *args){
	if (args->current_filename) g_free(args->current_filename);
	args->current_filename=0;
	color_list_remove_all(args->gs->colors);
	updateProgramName(args);
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

int app_save_file(AppArgs *args, const char *filename){
	int return_value = -1;

	if (filename == NULL){
		if (args->current_filename){
			filename = args->current_filename;
		}else{
			return -1;
		}
	}

	FileType filetype;
  switch (filetype = get_file_type_from_ext(filename)){
		case GPL:
			return_value = palette_export_gpl(args->gs->colors, filename, false);
			break;
		case ASE:
			return_value = palette_export_ase(args->gs->colors, filename, false);
			break;
		case UNKNOWN:
		case GPA:
			return_value = palette_file_save(filename, args->gs->colors);
			break;
	}

	if (return_value == 0){
		if (filetype == GPA || filetype == UNKNOWN){
			args->imported = false;
		}else{
			args->imported = true;
		}
		if (filename != args->current_filename){  // do not touch current_filename if it is assigned to filename
			if (args->current_filename) g_free(args->current_filename);
			args->current_filename = g_strdup(filename);
		}
		updateProgramName(args);
		updateRecentFileList(args, filename, true);
		return 0;
	}else{
		updateProgramName(args);
		return -1;
	}
	return 0;
}

int app_load_file(AppArgs *args, const char *filename, bool autoload){
	int return_value = -1;
	int index;
	bool imported = false;

	switch (get_file_type_from_ext(filename)){
		case GPL:
			return_value = palette_import_gpl(args->gs->colors, filename);
			imported = true;
			break;
		case ASE:
			return_value = palette_import_ase(args->gs->colors, filename);
			imported = true;
			break;
		case GPA:
		case UNKNOWN:
			return_value = palette_file_load(filename, args->gs->colors);
			break;
	}

	if (args->current_filename) g_free(args->current_filename);
	args->current_filename = NULL;

	if (return_value == 0){
		if (imported){
			args->imported = true;
		}else{
			args->imported = false;
		}
		if (!autoload){
			args->current_filename = g_strdup(filename);
			updateRecentFileList(args, filename, false);
		}
		updateProgramName(args);
	}else{
		updateProgramName(args);
		return -1;
	}
	return 0;
}

int app_parse_geometry(AppArgs *args, const char *geometry){
	gtk_window_parse_geometry(GTK_WINDOW(args->window), geometry);
	return 0;
}

static void menu_file_revert(GtkWidget *widget, AppArgs *args){
  string filename;
	if (args->current_filename){
		filename = args->current_filename;
	}else{
		return;
	}

	color_list_remove_all(args->gs->colors);

	if (app_load_file(args, filename.c_str()) == 0){
	}else{
		GtkWidget* message;
		message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
		gtk_window_set_title(GTK_WINDOW(message), _("Open"));
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
	}
}

static void menu_file_open_last(GtkWidget *widget, AppArgs *args){
  const char *filename = args->recent_files.begin()->c_str();

	color_list_remove_all(args->gs->colors);

	if (app_load_file(args, filename) == 0){
	}else{
		GtkWidget* message;
		message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
		gtk_window_set_title(GTK_WINDOW(message), _("Open"));
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
	}
}

static void menu_file_open_nth(GtkWidget *widget, AppArgs *args){
	list<string>::iterator i = args->recent_files.begin();
	uintptr_t index = (uintptr_t)g_object_get_data(G_OBJECT(widget), "index");
	std::advance(i, index);
  const char *filename = (*i).c_str();

	color_list_remove_all(args->gs->colors);

	if (app_load_file(args, filename) == 0){
	}else{
		GtkWidget* message;
		message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
		gtk_window_set_title(GTK_WINDOW(message), _("Open"));
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
	}
}

static void add_file_filters (GtkWidget *dialog, FileType preselect){
	const struct{
		FileType type;
		const char *label;
		const char *filter;
	}filters[] = {
		{GPA, _("Gpick Palette (*.gpa)"), "*.gpa"},
		{GPL, _("GIMP/Inkscape Palette (*.gpl)"), "*.gpl"},
		{ASE, _("Adobe Swatch Exchange (*.ase)"), "*.ase"},
		{UNKNOWN, 0, 0},
	};
	GtkFileFilter *filter;

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All supported formats"));
	for (gint i = 0; filters[i].type != UNKNOWN; ++i) {
		gtk_file_filter_add_pattern(filter, filters[i].filter);
	}
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	for (gint i = 0; filters[i].type != UNKNOWN; ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, filters[i].label);
		gtk_file_filter_add_pattern(filter, filters[i].filter);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if (preselect == filters[i].type){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
}
static void menu_file_open(GtkWidget *widget, AppArgs *args){

	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Open File"), GTK_WINDOW(gtk_widget_get_toplevel(widget)),
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	const gchar* default_path = dynv_get_string_wd(args->params, "open.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);

	add_file_filters(dialog, UNKNOWN);

	gboolean finished=FALSE;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(args->params, "open.path", path);
			g_free(path);

			color_list_remove_all(args->gs->colors);

			if (app_load_file(args, filename) == 0){
				finished = TRUE;
			}else{
				GtkWidget* message;
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
				gtk_window_set_title(GTK_WINDOW(dialog), _("Open"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);

}

static void menu_file_save_as(GtkWidget *widget, AppArgs *args){

	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (_("Save As"), GTK_WINDOW(gtk_widget_get_toplevel(widget)),
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	const gchar* default_path = dynv_get_string_wd(args->params, "save.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);

	add_file_filters(dialog, UNKNOWN);

	gboolean finished=FALSE;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(args->params, "save.path", path);
			g_free(path);

			if (app_save_file(args, filename) == 0){
				finished=TRUE;
			}else{
				GtkWidget* message;
				message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
				gtk_window_set_title(GTK_WINDOW(message), _("Save As"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
}

static void menu_file_save(GtkWidget *widget, AppArgs *args) {
	// If file has no name, "Save As" dialog is shown instead.
	if (args->current_filename == 0){
		menu_file_save_as(widget, args);
	}else{
		if (app_save_file(args, NULL) == 0){
		}else{
			GtkWidget* message;
			message=gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
			gtk_window_set_title(GTK_WINDOW(message), _("Save"));
			gtk_dialog_run(GTK_DIALOG(message));
			gtk_widget_destroy(message);
		}
	}
}

static PaletteListCallbackReturn color_list_selected(struct ColorObject* color_object, void *userdata){
	color_list_add_color_object((struct ColorList *)userdata, color_object, 1);
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}

static void menu_file_export_all(GtkWidget *widget, gpointer data) {
	AppArgs* args=(AppArgs*)data;
	dialog_export_show(GTK_WINDOW(args->window), 0, false, args->gs);
}

static void menu_file_import(GtkWidget *widget, gpointer data) {
	AppArgs* args=(AppArgs*)data;
	dialog_import_show(GTK_WINDOW(args->window), 0, args->gs);
}

static void menu_file_export(GtkWidget *widget, gpointer data) {
	AppArgs* args=(AppArgs*)data;
	struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(args->gs->params);
	struct ColorList *color_list = color_list_new(handler_map);
	dynv_handler_map_release(handler_map);

	palette_list_foreach_selected(args->color_list, color_list_selected, color_list);
	dialog_export_show(GTK_WINDOW(args->window), color_list, true, args->gs);
	color_list_destroy(color_list);
}


typedef struct FileMenuItems{
	GtkWidget *export_all;
	GtkWidget *export_selected;
	GtkWidget *recent_files;
}FileMenuItems;

static void menu_file_activate(GtkWidget *widget, gpointer data) {
	AppArgs* args=(AppArgs*)data;

	gint32 selected_count = palette_list_get_selected_count(args->color_list);
	gint32 total_count = palette_list_get_count(args->color_list);

	//GtkMenu* menu=GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget)));

  FileMenuItems *items = (FileMenuItems*) g_object_get_data(G_OBJECT(widget), "items");
	gtk_widget_set_sensitive(items->export_all, (total_count >= 1));
	gtk_widget_set_sensitive(items->export_selected, (selected_count >= 1));



	if (args->recent_files.size() > 0){

		GtkMenu *menu2 = GTK_MENU(gtk_menu_new());
		GtkWidget *item;

		item = gtk_menu_item_new_with_image (_("Open Last File"), gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu2), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_open_last), args);

		gtk_menu_shell_append (GTK_MENU_SHELL (menu2), gtk_separator_menu_item_new ());

		uint32_t j = 0;
		for (list<string>::iterator i = args->recent_files.begin(); i != args->recent_files.end(); i++){
			item = gtk_menu_item_new_with_label ((*i).c_str());
			gtk_menu_shell_append (GTK_MENU_SHELL (menu2), item);
			g_object_set_data_full(G_OBJECT(item), "index", (void*)j, (GDestroyNotify)NULL);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_open_nth), args);
			j++;
		}

		gtk_widget_show_all(GTK_WIDGET(menu2));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (items->recent_files), GTK_WIDGET(menu2));

		gtk_widget_set_sensitive(items->recent_files, true);
	}else{
		gtk_widget_set_sensitive(items->recent_files, false);
	}

}

static void floating_picker_show_cb(GtkWidget *widget, AppArgs* args) {
	floating_picker_activate(args->floating_picker, false);
}

static void show_about_box_cb(GtkWidget *widget, AppArgs* args) {
	show_about_box(args->window);
}

static void view_palette_cb(GtkWidget *widget, AppArgs* args) {
  if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))){
		g_object_ref(args->vpaned);
		GtkWidget *vbox = gtk_widget_get_parent(GTK_WIDGET(args->vpaned));
		gtk_container_remove(GTK_CONTAINER(vbox), args->vpaned);
		gtk_paned_pack1(GTK_PANED(args->hpaned), args->vpaned, false, false);
		g_object_unref(args->vpaned);
		gtk_widget_show(GTK_WIDGET(args->hpaned));

		dynv_set_bool(args->params, "view.palette", true);
	}else{
		gtk_widget_hide(GTK_WIDGET(args->hpaned));
		g_object_ref(args->vpaned);
		gtk_container_remove(GTK_CONTAINER(args->hpaned), args->vpaned);
		GtkWidget *vbox = gtk_widget_get_parent(GTK_WIDGET(args->hpaned));
		gtk_box_pack_start(GTK_BOX(vbox), args->vpaned, TRUE, TRUE, 5);
		g_object_unref(args->vpaned);

		dynv_set_bool(args->params, "view.palette", false);
	}
}

static void palette_from_image_cb(GtkWidget *widget, AppArgs* args) {
	tools_palette_from_image_show(GTK_WINDOW(args->window), args->gs);
}

static void palette_from_css_file_cb(GtkWidget *widget, AppArgs* args) {
	tools_palette_from_css_file_show(GTK_WINDOW(args->window), args->gs);
}

static void destroy_file_menu_items(FileMenuItems *items){
	delete items;
}

static void activate_secondary_source(AppArgs *args, ColorSource *source){
	if (args->secondary_color_source){
		color_source_deactivate(args->secondary_color_source);
		color_source_destroy(args->secondary_color_source);
		args->secondary_color_source = 0;
		args->secondary_source_widget = 0;
		if (args->secondary_source_scrolled_viewpoint)
			gtk_container_remove(GTK_CONTAINER(args->secondary_source_container), args->secondary_source_scrolled_viewpoint);
		args->secondary_source_scrolled_viewpoint= 0;
		if (!source)
			gtk_widget_hide(args->secondary_source_container);
	}

	if (source){
		string namespace_str = "gpick.secondary_view.";
		namespace_str += source->identificator;

		struct dynvSystem *dynv_namespace = dynv_get_dynv(args->gs->params, namespace_str.c_str());
		source = color_source_implement(source, args->gs, dynv_namespace);
		GtkWidget *new_widget = color_source_get_widget(source);
		dynv_system_release(dynv_namespace);

		args->secondary_color_source = source;
		args->secondary_source_widget = new_widget;

		if (source->needs_viewport){
			GtkWidget *scrolled_window = gtk_scrolled_window_new(0,0);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
			gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_NONE);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), new_widget);

      args->secondary_source_scrolled_viewpoint = scrolled_window;
			gtk_box_pack_start(GTK_BOX(args->secondary_source_container), args->secondary_source_scrolled_viewpoint, true, true, 0);

			gtk_widget_show(args->secondary_source_scrolled_viewpoint);
		}else{
			gtk_box_pack_start(GTK_BOX(args->secondary_source_container), new_widget, true, true, 0);
			gtk_widget_show(new_widget);
		}

		gtk_widget_show(args->secondary_source_container);
		color_source_activate(source);

	}
}


static void secondary_view_cb(GtkWidget *widget, AppArgs *args){
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))){
			ColorSource *source = static_cast<ColorSource*>(g_object_get_data(G_OBJECT(widget), "source"));
			activate_secondary_source(args, source);
	}
}

static void createMenu(GtkMenuBar *menu_bar, AppArgs *args, GtkAccelGroup *accel_group) {
	GtkMenu *menu;
	GtkWidget *item;
	GtkWidget* file_item;
	GtkStockItem stock_item;

	menu = GTK_MENU(gtk_menu_new());

	FileMenuItems *items = new FileMenuItems();

	if (gtk_stock_lookup(GTK_STOCK_NEW, &stock_item)){
		item = gtk_menu_item_new_with_image(stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator(item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(menu_file_new), args);
	}

	if (gtk_stock_lookup(GTK_STOCK_OPEN, &stock_item)){
		item = gtk_menu_item_new_with_image(stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator(item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_file_open), args);
	}

	item = gtk_menu_item_new_with_mnemonic(_("Recent _files"));
	items->recent_files = item;
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	if (gtk_stock_lookup(GTK_STOCK_REVERT_TO_SAVED, &stock_item)){
		item = gtk_menu_item_new_with_image(stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_r, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_F5, GdkModifierType(0), GTK_ACCEL_VISIBLE);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_file_revert), args);
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new ());

	if (gtk_stock_lookup(GTK_STOCK_SAVE, &stock_item)){
		item = gtk_menu_item_new_with_image (stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(menu_file_save), args);
	}

	if (gtk_stock_lookup(GTK_STOCK_SAVE_AS, &stock_item)){
		item = gtk_menu_item_new_with_image (stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(menu_file_save_as), args);
	}

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	item = gtk_image_menu_item_new_with_mnemonic(_("Ex_port..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_e, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (menu_file_export_all), args);
	items->export_all = item;

	//gtk_widget_set_sensitive(item, (total_count >= 1));

	item = gtk_image_menu_item_new_with_mnemonic(_("Expo_rt Selected..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_e, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (menu_file_export), args);
	items->export_selected = item;

	//gtk_widget_set_sensitive(item, (selected_count >= 1));

	item = gtk_image_menu_item_new_with_mnemonic(_("_Import..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_i, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (menu_file_import), args);
	//gtk_widget_set_sensitive(item, 0);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	if (gtk_stock_lookup(GTK_STOCK_QUIT, &stock_item)){
		item = gtk_menu_item_new_with_image (stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(destroy_cb), args);
	}

	file_item = gtk_menu_item_new_with_mnemonic(_("_File"));
	g_signal_connect (G_OBJECT (file_item), "activate", G_CALLBACK (menu_file_activate), args);
	g_object_set_data_full(G_OBJECT(file_item), "items", items, (GDestroyNotify)destroy_file_menu_items);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);

	menu = GTK_MENU(gtk_menu_new());

	item = gtk_menu_item_new_with_image(_("Edit _Converters..."), gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_dialog_converter), args);

	item = gtk_menu_item_new_with_image(_("Display _Filters..."), gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_dialog_transformations), args);


	if (gtk_stock_lookup(GTK_STOCK_PREFERENCES, &stock_item)){
		item = gtk_menu_item_new_with_image (stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(show_dialog_options), args);
	}

	file_item = gtk_menu_item_new_with_mnemonic(_("_Edit"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);

	menu = GTK_MENU(gtk_menu_new());

	GtkMenu *menu2 = GTK_MENU(gtk_menu_new());
	GSList *group = NULL;

	ColorSource *source = color_source_manager_get(args->csm, dynv_get_string_wd(args->params, "secondary_color_source", ""));

	item = gtk_radio_menu_item_new_with_label(group, _("None"));
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	if (source == NULL)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
	g_object_set_data_full(G_OBJECT(item), "source", 0, (GDestroyNotify)NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(secondary_view_cb), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_N, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu2), item);


	vector<ColorSource*> sources = color_source_manager_get_all(args->csm);
	for (uint32_t i = 0; i < sources.size(); i++){
		if (!(sources[i]->single_instance_only)){
			item = gtk_radio_menu_item_new_with_label(group, sources[i]->hr_name);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
			if (source == sources[i])
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
			g_object_set_data_full(G_OBJECT(item), "source", sources[i], (GDestroyNotify)NULL);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(secondary_view_cb), args);
			if (color_source_get_default_accelerator(sources[i]))
				gtk_widget_add_accelerator(item, "activate", accel_group, color_source_get_default_accelerator(sources[i]), GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu2), item);
		}
	}

	item = gtk_menu_item_new_with_mnemonic(_("_Secondary View"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), GTK_WIDGET(menu2));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	item = gtk_check_menu_item_new_with_mnemonic(_("Palette"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), dynv_get_bool_wd(args->params, "view.palette", true));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_p, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(view_palette_cb), args);

	file_item = gtk_menu_item_new_with_mnemonic (_("_View"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);

	menu = GTK_MENU(gtk_menu_new());

	item = gtk_menu_item_new_with_mnemonic(_("Palette From _Image..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_from_image_cb), args);

	//item = gtk_menu_item_new_with_mnemonic(_("Palette From _CSS file..."));
	//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	//g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_from_css_file_cb), args);

	file_item = gtk_menu_item_new_with_mnemonic(_("_Tools"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), GTK_WIDGET(menu));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);



	menu = GTK_MENU(gtk_menu_new());

	if (gtk_stock_lookup(GTK_STOCK_ABOUT, &stock_item)){
		item = gtk_menu_item_new_with_image (stock_item.label, gtk_image_new_from_stock(stock_item.stock_id, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(show_about_box_cb), args);
	}

	file_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
}


typedef struct CopyMenuItem{
	gchar* function_name;
	struct ColorObject* color_object;
	GlobalState* gs;
	GtkWidget* palette_widget;
}CopyMenuItem;

static void converter_destroy_params(CopyMenuItem* args){
	color_object_release(args->color_object);
	g_free(args->function_name);
	delete args;
}

static void converter_callback_copy(GtkWidget *widget,  gpointer item) {
	CopyMenuItem* itemdata=(CopyMenuItem*)g_object_get_data(G_OBJECT(widget), "item_data");
	converter_get_clipboard(itemdata->function_name, itemdata->color_object, itemdata->palette_widget, itemdata->gs->params);
}


static GtkWidget* converter_create_copy_menu_item (GtkWidget *menu, const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, GlobalState *gs){
	GtkWidget* item=0;
	gchar* converted;

	if (converters_color_serialize((Converters*)dynv_get_pointer_wd(gs->params, "Converters", 0), function, color_object, &converted)==0){
		item = gtk_menu_item_new_with_image(converted, gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(converter_callback_copy), 0);

		CopyMenuItem* itemdata=new CopyMenuItem;
		itemdata->function_name=g_strdup(function);
		itemdata->palette_widget=palette_widget;
		itemdata->color_object=color_object_ref(color_object);
		itemdata->gs = gs;

		g_object_set_data_full(G_OBJECT(item), "item_data", itemdata, (GDestroyNotify)converter_destroy_params);

		g_free(converted);
	}

	return item;
}

GtkWidget* converter_create_copy_menu (struct ColorObject* color_object, GtkWidget* palette_widget, GlobalState* gs){
	Converters *converters = (Converters*)dynv_get_pointer_wd(gs->params, "Converters", 0);

	GtkWidget *menu;
	menu = gtk_menu_new();

	uint32_t converter_table_size = 0;
	Converter** converter_table = converters_get_all_type(converters, CONVERTERS_ARRAY_TYPE_COPY, &converter_table_size);

	for (uint32_t i=0; i<converter_table_size; ++i){
		GtkWidget* item=converter_create_copy_menu_item(menu, converter_table[i]->function_name, color_object, palette_widget, gs);
		if (item) gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	return menu;
}


void converter_get_text(const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, struct dynvSystem *params, gchar** out_text){
	stringstream text(ios::out);

	int first=true;

	struct ColorList *color_list = color_list_new(NULL);
	if (palette_widget){
		palette_list_foreach_selected(palette_widget, color_list_selected, color_list);
	}else{
		color_list_add_color_object(color_list, color_object, 1);
	}

	Converters* converters = (Converters*)dynv_get_pointer_wd(params, "Converters", 0);

	for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){

		gchar* converted;

		if (converters_color_serialize(converters, function, *i, &converted)==0){
			if (first){
				text<<converted;
				first=false;
			}else{
				text<<endl<<converted;
			}
			g_free(converted);
		}
	}

	color_list_destroy(color_list);

	if (first!=true){
		*out_text = g_strdup(text.str().c_str());
	}else{
		*out_text = 0;
	}
}

void converter_get_clipboard(const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, struct dynvSystem *params){
	gchar* text;
	converter_get_text(function, color_object, palette_widget, params, &text);
	if (text){
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, -1);
		gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), text, -1);
		g_free(text);
	}
}

static PaletteListCallbackReturn color_list_mark_selected(struct ColorObject* color_object, void *userdata){
	color_object->selected = 1;
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}

static void palette_popup_menu_remove_all(GtkWidget *widget, AppArgs* args) {
	color_list_remove_all(args->gs->colors);
}

static void palette_popup_menu_remove_selected(GtkWidget *widget, AppArgs* args) {
	palette_list_foreach_selected(args->color_list, color_list_mark_selected, 0);
	color_list_remove_selected(args->gs->colors);
}

static PaletteListCallbackReturn color_list_clear_names(struct ColorObject* color_object, void *userdata){
	dynv_set_string(color_object->params, "name", "");
	return PALETTE_LIST_CALLBACK_UPDATE_NAME;
}

static void palette_popup_menu_clear_names(GtkWidget *widget, AppArgs* args) {
	palette_list_foreach_selected(args->color_list, color_list_clear_names, NULL);
}

typedef struct AutonameState{
	ColorNames *color_names;
	bool imprecision_postfix;
}AutonameState;

static PaletteListCallbackReturn color_list_autoname(struct ColorObject* color_object, void *userdata){
	AutonameState *state=(AutonameState *)(userdata);
	Color c;
	color_object_get_color (color_object, &c);
	dynv_set_string(color_object->params, "name", color_names_get(state->color_names, &c, state->imprecision_postfix).c_str());
	return PALETTE_LIST_CALLBACK_UPDATE_NAME;
}

static void palette_popup_menu_autoname(GtkWidget *widget, AppArgs* args) {
	AutonameState state;
	state.color_names = args->gs->color_names;
	state.imprecision_postfix = dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true);
	palette_list_foreach_selected(args->color_list, color_list_autoname, &state);
}

typedef struct AutonumberState{
	std::string name;
	uint32_t index;
	uint32_t nplaces;
	bool decreasing;
	bool append;
}AutonumberState;

static PaletteListCallbackReturn color_list_autonumber(struct ColorObject* color_object, void *userdata){
	AutonumberState *state = (AutonumberState*)userdata;
	stringstream ss;
	if (state->append == true){
		ss << dynv_get_string_wd(color_object->params, "name","") << " ";
	}
	ss << state->name << "-";
	ss.width(state->nplaces);
	ss.fill('0');
	if (state->decreasing){
		ss << right << state->index;
		state->index--;
	}
	else{
		ss << right << state->index++;
	}

	dynv_set_string(color_object->params, "name", ss.str().c_str());
	return PALETTE_LIST_CALLBACK_UPDATE_NAME;
}

static PaletteListCallbackReturn color_list_set_color (struct ColorObject* color_object, void *userdata){
	Color *source_color = (Color *)(userdata);
	color_object_set_color (color_object, source_color);
	return PALETTE_LIST_CALLBACK_UPDATE_ROW;
}



static void palette_popup_menu_autonumber(GtkWidget *widget, AppArgs* args) {
	AutonumberState state;
	int response;
	uint32_t selected_count = palette_list_get_selected_count(args->color_list);

	response = dialog_autonumber_show(GTK_WINDOW(args->window), selected_count, args->gs);
	if (response == GTK_RESPONSE_OK){
		struct dynvSystem *params;

		params = dynv_get_dynv(args->gs->params, "gpick.autonumber");
		state.name = dynv_get_string_wd(params, "name", "autonum");
		state.nplaces = dynv_get_int32_wd(params, "nplaces", 1);
		state.index = dynv_get_int32_wd(params, "startindex", 1);
		state.decreasing = dynv_get_bool_wd(params, "decreasing", true);
		state.append = dynv_get_bool_wd(params, "append", true);

		palette_list_foreach_selected(args->color_list, color_list_autonumber, &state);
		dynv_system_release(params);
	}
}


typedef struct ReplaceState{
	std::list<struct ColorObject*>::reverse_iterator iter;
} ReplaceState;

static PaletteListCallbackReturn color_list_reverse_replace(struct ColorObject** color_object, void *userdata)
{
	ReplaceState *state = reinterpret_cast<ReplaceState*>(userdata);
	*color_object = color_object_ref(*(state->iter));
	state->iter++;
	return PALETTE_LIST_CALLBACK_UPDATE_ROW;
}

static void palette_popup_menu_reverse(GtkWidget *widget, AppArgs* args) {
	struct ColorList *color_list = color_list_new(NULL);
	ReplaceState state;
	palette_list_foreach_selected(args->color_list, color_list_selected, color_list);
	state.iter = color_list->colors.rbegin();
	palette_list_foreach_selected(args->color_list, color_list_reverse_replace, &state);
}

gint32 palette_popup_menu_mix_list(Color* color, void *userdata){
	*((GList**)userdata) = g_list_append(*((GList**)userdata), color);
	return 0;
}

static void palette_popup_menu_mix(GtkWidget *widget, AppArgs* args) {
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(args->color_list, color_list_selected, color_list);
	dialog_mix_show(GTK_WINDOW(args->window), color_list, args->gs);
	color_list_destroy(color_list);
}



static void palette_popup_menu_variations(GtkWidget *widget, AppArgs* args) {

	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(args->gs->params);

	struct ColorList *color_list = color_list_new(handler_map);
	palette_list_foreach_selected(args->color_list, color_list_selected, color_list);
	dialog_variations_show(GTK_WINDOW(args->window), color_list, args->gs);
	color_list_destroy(color_list);

	dynv_handler_map_release(handler_map);
}

static void palette_popup_menu_generate(GtkWidget *widget, AppArgs* args) {

	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(args->color_list, color_list_selected, color_list);
	dialog_generate_show(GTK_WINDOW(args->window), color_list, args->gs);
	color_list_destroy(color_list);
}

typedef struct GroupAndSortState{
	std::list<struct ColorObject*>::iterator iter;
} GroupAndSortState;

static PaletteListCallbackReturn color_list_group_and_sort_replace(struct ColorObject** color_object, void *userdata)
{
	GroupAndSortState *state = reinterpret_cast<GroupAndSortState*>(userdata);
	*color_object = color_object_ref(*(state->iter));
	state->iter++;
	return PALETTE_LIST_CALLBACK_UPDATE_ROW;
}

static void palette_popup_menu_group_and_sort(GtkWidget *widget, AppArgs* args)
{
	struct ColorList *color_list = color_list_new(NULL);
	struct ColorList *sorted_color_list = color_list_new(NULL);
	palette_list_foreach_selected(args->color_list, color_list_selected, color_list);
	if (dialog_sort_show(GTK_WINDOW(args->window), color_list, sorted_color_list, args->gs)){
		GroupAndSortState state;
		state.iter = sorted_color_list->colors.begin();
		palette_list_foreach_selected(args->color_list, color_list_group_and_sort_replace, &state);
	}
	color_list_destroy(color_list);
	color_list_destroy(sorted_color_list);
}

static gboolean palette_popup_menu_show(GtkWidget *widget, GdkEventButton* event, AppArgs *args) {
	GtkWidget *menu;
	GtkWidget* item ;
	gint32 button, event_time;

	menu = gtk_menu_new ();

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);

	gint32 selected_count = palette_list_get_selected_count(args->color_list);
	gint32 total_count = palette_list_get_count(args->color_list);

	item = gtk_menu_item_new_with_mnemonic(_("_Copy to Clipboard"));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_c, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_set_sensitive(item, (selected_count >= 1));

	if (total_count>0){
		struct ColorList *color_list = color_list_new(NULL);
		palette_list_forfirst_selected(args->color_list, color_list_selected, color_list);
		if (color_list_get_count(color_list)!=0){
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (*color_list->colors.begin(), args->color_list, args->gs));
		}
		color_list_destroy(color_list);
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    item = gtk_menu_item_new_with_image (_("_Mix Colors..."), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_mix), args);
    gtk_widget_set_sensitive(item, (selected_count >= 2));

    item = gtk_menu_item_new_with_image (_("_Variations..."), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_variations), args);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_image (_("_Generate..."), gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_generate), args);
    gtk_widget_set_sensitive(item, (selected_count >= 1));


	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());


    item = gtk_menu_item_new_with_mnemonic (_("C_lear names"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_clear_names), args);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_E, GdkModifierType(0), GTK_ACCEL_VISIBLE);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

	item = gtk_menu_item_new_with_mnemonic (_("Autona_me"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_autoname), args);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_N, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_mnemonic (_("Auto_number..."));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_autonumber), args);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_a, GdkModifierType(0), GTK_ACCEL_VISIBLE);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_mnemonic(_("R_everse"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_reverse), args);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_v, GdkModifierType(0), GTK_ACCEL_VISIBLE);
    gtk_widget_set_sensitive(item, (selected_count >= 2));

    item = gtk_menu_item_new_with_mnemonic(_("Group and _sort..."));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_popup_menu_group_and_sort), args);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_g, GdkModifierType(0), GTK_ACCEL_VISIBLE);
    gtk_widget_set_sensitive(item, (selected_count >= 2));

		gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image (_("_Remove"), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_remove_selected), args);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_Delete, GdkModifierType(0), GTK_ACCEL_VISIBLE);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_image (_("Remove _All"), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_remove_all), args);
    gtk_widget_set_sensitive(item, (total_count >= 1));

    gtk_widget_show_all (GTK_WIDGET(menu));

	if (event){
		button = event->button;
		event_time = event->time;
	}else{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

	g_object_ref_sink(G_OBJECT(accel_group));
	g_object_unref(G_OBJECT(accel_group));

	g_object_ref_sink(menu);
	g_object_unref(menu);

	return TRUE;
}

static void on_palette_popup_menu(GtkWidget *widget, AppArgs *args) {
	palette_popup_menu_show(widget, 0, args);
}

static gboolean on_palette_button_press(GtkWidget *widget, GdkEventButton *event, AppArgs *args) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		return palette_popup_menu_show(widget, event, args);
	}
	return FALSE;
}

static gboolean on_palette_list_key_press(GtkWidget *widget, GdkEventKey *event, AppArgs *args){

	guint modifiers = gtk_accelerator_get_default_mod_mask();

	switch(event->keyval)
	{
		case GDK_1:
		case GDK_KP_1:
		case GDK_2:
		case GDK_KP_2:
		case GDK_3:
		case GDK_KP_3:
		case GDK_4:
		case GDK_KP_4:
		case GDK_5:
		case GDK_KP_5:
		case GDK_6:
		case GDK_KP_6:
			{
				struct ColorList *color_list = color_list_new(NULL);
				palette_list_forfirst_selected(args->color_list, color_list_selected, color_list);
				if (color_list_get_count(color_list) > 0){
					ColorSource *color_source = (ColorSource*)dynv_get_pointer_wd(args->gs->params, "CurrentColorSource", 0);
					uint32_t color_index = 0;
					switch(event->keyval)
					{
						case GDK_KP_1:
						case GDK_1: color_index = 0; break;
						case GDK_KP_2:
						case GDK_2: color_index = 1; break;
						case GDK_KP_3:
						case GDK_3: color_index = 2; break;
						case GDK_KP_4:
						case GDK_4: color_index = 3; break;
						case GDK_KP_5:
						case GDK_5: color_index = 4; break;
						case GDK_KP_6:
						case GDK_6: color_index = 5; break;
					}

					if ((event->state&modifiers)==GDK_CONTROL_MASK){
						struct ColorObject *source_color_object;
						Color source_color;
						color_source_get_nth_color(color_source, color_index, &source_color_object);
						color_object_get_color (source_color_object, &source_color);
						palette_list_forfirst_selected (args->color_list, color_list_set_color, &source_color);
					}
					else{
						color_source_set_nth_color(color_source, color_index, *color_list->colors.begin());
					}
				}
				color_list_destroy(color_list);
				return true;
			}
			return false;
			break;

		case GDK_c:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){

				Converters *converters = (Converters*)dynv_get_pointer_wd(args->gs->params, "Converters", 0);
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, 0, args->color_list, args->gs->params);
				}

				return true;
			}
			return false;
			break;

		case GDK_v:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){
				struct ColorObject* color_object;
				if (copypaste_get_color_object(&color_object, args->gs)==0){
					color_list_add_color_object(args->gs->colors, color_object, 1);
					color_object_release(color_object);
				}
				return true;
			}else{
				palette_popup_menu_reverse(widget, args);
				return true;
			}
			return false;
			break;

		case GDK_g:
			palette_popup_menu_group_and_sort(widget, args);
			return true;
			break;

		case GDK_Delete:
			palette_popup_menu_remove_selected(widget, args);
			break;

		case GDK_a:
			if ((event->state & GDK_CONTROL_MASK) == 0){
				palette_popup_menu_autonumber(widget, args);
				return true;
			}
			break;
		case GDK_e:
			if ((event->state & GDK_CONTROL_MASK) == 0){
				palette_popup_menu_clear_names(widget, args);
				return true;
			}
			break;
		case GDK_n:
			if ((event->state & GDK_CONTROL_MASK) == 0){
				palette_popup_menu_autoname(widget, args);
				return true;
			}
			break;
		default:
			return false;
		break;
	}
	return false;
}


static int color_list_on_insert(struct ColorList* color_list, struct ColorObject* color_object){
	palette_list_add_entry(((AppArgs*)color_list->userdata)->color_list, color_object);
	return 0;
}

static int color_list_on_delete_selected(struct ColorList* color_list){
	palette_list_remove_selected_entries(((AppArgs*)color_list->userdata)->color_list);
	return 0;
}

static int color_list_on_delete(struct ColorList* color_list, struct ColorObject* color_object){
	palette_list_remove_entry(((AppArgs*)color_list->userdata)->color_list, color_object);
	return 0;
}

static int color_list_on_clear(struct ColorList* color_list){
	palette_list_remove_all_entries(((AppArgs*)color_list->userdata)->color_list);
	return 0;
}

static PaletteListCallbackReturn callback_color_list_on_get_positions(struct ColorObject* color_object, void *userdata){
	color_object->position = *((unsigned long*)userdata);
	(*((unsigned long*)userdata))++;
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}

static int color_list_on_get_positions(struct ColorList* color_list){
	unsigned long item = 0;
	palette_list_foreach(((AppArgs*)color_list->userdata)->color_list, callback_color_list_on_get_positions, &item );
	return 0;
}

int main_show_window(GtkWidget* window, struct dynvSystem *main_params){

	if (GTK_WIDGET_VISIBLE(window)){
		gtk_window_deiconify(GTK_WINDOW(window));
		return -1;	//already visible
	}


	gint x, y;
	gint width, height;

	x = dynv_get_int32_wd(main_params, "window.x", -1);
	y = dynv_get_int32_wd(main_params, "window.y", -1);

	width = dynv_get_int32_wd(main_params, "window.width", -1);
	height = dynv_get_int32_wd(main_params, "window.height", -1);

	if (x<0 || y<0 || x>gdk_screen_width() || y>gdk_screen_height()){
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	}else{
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
		gtk_window_move(GTK_WINDOW(window), x, y);
	}

	if (width>0 && height>0)
		gtk_window_resize(GTK_WINDOW(window), width, height);

	gtk_widget_show(window);
	return 0;
}

static gboolean on_window_focus_change(GtkWidget *widget, GdkEventFocus *event, AppArgs* args) {

	if (event->in){

		if (args->secondary_color_source)	color_source_activate(args->secondary_color_source);
		if (args->current_color_source) color_source_activate(args->current_color_source);
	}else{
		if (args->secondary_color_source)	color_source_deactivate(args->secondary_color_source);
		if (args->current_color_source) color_source_deactivate(args->current_color_source);
	}

	return FALSE;
}

static void set_main_window_icon() {
	GList *icons = 0;

	GtkIconTheme *icon_theme;
	GError *error = NULL;
	icon_theme = gtk_icon_theme_get_default();

	gint sizes[] = { 16, 32, 48, 128 };

	for (guint32 i = 0; i < sizeof(sizes) / sizeof(gint); ++i) {
		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, "gpick", sizes[i], GtkIconLookupFlags(0), &error);
		if (pixbuf && (error == NULL)){
			icons = g_list_append(icons, pixbuf);
		}
		if (error){
			cerr << error->message << endl;
			g_error_free(error);
			error = NULL;
		}
	}

	if (icons){
		gtk_window_set_default_icon_list(icons);

		g_list_foreach(icons, (GFunc)g_object_unref, NULL);
		g_list_free(icons);
	}
}

bool app_is_autoload_enabled(AppArgs *args){
	return dynv_get_bool_wd(args->params, "main.save_restore_palette", true);
}

static bool app_on_control_activate_floating_picker(void *userdata)
{
	AppArgs *args = reinterpret_cast<AppArgs*>(userdata);
	floating_picker_activate(args->floating_picker, false);
	return true;
}

static bool app_on_single_instance_activate(void *userdata)
{
	AppArgs *args = reinterpret_cast<AppArgs*>(userdata);
	status_icon_set_visible(args->statusIcon, false);
	main_show_window(args->window, args->params);
	return true;
}

AppArgs* app_create_main(const AppOptions *options){
	AppArgs* args=new AppArgs;
	memcpy(&args->options, options, sizeof(AppOptions));

	color_init();

	GlobalState *gs = global_state_create();
	args->gs = gs;
	global_state_init(args->gs, GLOBALSTATE_CONFIGURATION);

	args->bus_id = gpick_own_name(app_on_control_activate_floating_picker, app_on_single_instance_activate, args);

    bool cancel_startup = false;

	if (!cancel_startup && args->options.floating_picker_mode){
		if (gpick_control_activate_floating_picker()){
			cancel_startup = true;
		}
	}
	if (!cancel_startup && dynv_get_bool_wd(gs->params, "gpick.main.single_instance", false)){
		if (gpick_single_instance_activate()){
			cancel_startup = true;
		}
	}

	if (cancel_startup){
		delete args;
		return 0;
	}

	args->current_filename = 0;
	args->imported = false;
	args->precision_loss_icon = 0;
	args->current_color_source = 0;
	args->secondary_color_source = 0;
	args->secondary_source_widget = 0;
	args->secondary_source_scrolled_viewpoint= 0;


	global_state_init(args->gs, GLOBALSTATE_ALL);

	args->gs->colors->on_insert = color_list_on_insert;
	args->gs->colors->on_clear = color_list_on_clear;
	args->gs->colors->on_delete_selected = color_list_on_delete_selected;
	args->gs->colors->on_get_positions = color_list_on_get_positions;
	args->gs->colors->on_delete = color_list_on_delete;
	args->gs->colors->userdata = args;

	dynv_set_pointer(args->gs->params, "MainWindowStruct", args);

	args->params = dynv_get_dynv(args->gs->params, "gpick.main");

	args->csm = color_source_manager_create();
	register_sources(args->csm);

	args->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	set_main_window_icon();

	updateProgramName(args);

	g_signal_connect(G_OBJECT(args->window), "delete_event", 		G_CALLBACK(delete_event), args);
	g_signal_connect(G_OBJECT(args->window), "destroy",      		G_CALLBACK(destroy_cb), args);
	g_signal_connect(G_OBJECT(args->window), "configure-event",		G_CALLBACK(on_window_configure), args);
	g_signal_connect(G_OBJECT(args->window), "window-state-event",	G_CALLBACK(on_window_state_event), args);
	g_signal_connect(G_OBJECT(args->window), "focus-in-event", 		G_CALLBACK(on_window_focus_change), args);
	g_signal_connect(G_OBJECT(args->window), "focus-out-event", 	G_CALLBACK(on_window_focus_change), args);

	GtkAccelGroup *accel_group=gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(args->window), accel_group);
	g_object_unref(G_OBJECT (accel_group));

    //gtk_accel_group_connect(accel_group, GDK_s, GdkModifierType(GDK_CONTROL_MASK), GtkAccelFlags(GTK_ACCEL_VISIBLE), g_cclosure_new (G_CALLBACK (menu_file_save),window,NULL));

    GtkWidget *widget, *statusbar, *notebook, *vpaned, *hpaned;

    GtkWidget* vbox_main = gtk_vbox_new(false, 0);
    gtk_container_add (GTK_CONTAINER(args->window), vbox_main);

		GtkWidget* menu_bar;
		menu_bar = gtk_menu_bar_new ();
		gtk_box_pack_start (GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);

 	hpaned = gtk_hpaned_new();
  bool color_list_visible = dynv_get_bool_wd(args->params, "view.palette", true);


	vpaned = gtk_vpaned_new();
	args->vpaned = vpaned;
	if (color_list_visible)
		gtk_paned_pack1(GTK_PANED(hpaned), vpaned, false, false);
	else
		gtk_box_pack_start(GTK_BOX(vbox_main), vpaned, TRUE, TRUE, 5);

	notebook = gtk_notebook_new();
	g_signal_connect(G_OBJECT (notebook), "switch-page", G_CALLBACK(notebook_switch_cb), args);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), true);

	statusbar=gtk_statusbar_new();
	dynv_set_pointer(args->gs->params, "StatusBar", statusbar);


	gtk_paned_pack1(GTK_PANED(vpaned), notebook, false, false);

	if (color_list_visible){
		gtk_box_pack_start(GTK_BOX(vbox_main), hpaned, TRUE, TRUE, 5);
		gtk_widget_show_all(vbox_main);
	}else{
		gtk_widget_show_all(vbox_main);
		gtk_box_pack_start(GTK_BOX(vbox_main), hpaned, TRUE, TRUE, 5);
	}


		ColorSource *source;
		struct dynvSystem *dynv_namespace;

		dynv_namespace = dynv_get_dynv(gs->params, "gpick.picker");
		source = color_source_implement(color_source_manager_get(args->csm, "color_picker"), args->gs, dynv_namespace);
		widget = color_source_get_widget(source);
		dynv_system_release(dynv_namespace);

		args->color_source[source->identificator] = source;
		args->color_source_index.push_back(source);
		args->floating_picker = floating_picker_new(args->window, args->gs, source);
		color_picker_set_floating_picker(source, args->floating_picker);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new_with_mnemonic(_("Color pic_ker")));
		gtk_widget_show(widget);

		dynv_namespace = dynv_get_dynv(gs->params, "gpick.generate_scheme");
		source = color_source_implement(color_source_manager_get(args->csm, "generate_scheme"), args->gs, dynv_namespace);
		widget = color_source_get_widget(source);
		dynv_system_release(dynv_namespace);
		args->color_source[source->identificator] = source;
		args->color_source_index.push_back(source);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new_with_mnemonic(_("Scheme _generation")));
		gtk_widget_show(widget);

		{
			widget = gtk_vbox_new(false, 0);
			gtk_paned_pack2(GTK_PANED(vpaned), widget, false, false);
			args->secondary_source_container = widget;
			source = color_source_manager_get(args->csm, dynv_get_string_wd(args->params, "secondary_color_source", ""));
			if (source) activate_secondary_source(args, source);
		}



		dynv_namespace = dynv_get_dynv(gs->params, "gpick.layout_preview");
		source = color_source_implement(color_source_manager_get(args->csm, "layout_preview"), args->gs, dynv_namespace);
		widget = color_source_get_widget(source);
		dynv_system_release(dynv_namespace);
		args->color_source[source->identificator] = source;
		args->color_source_index.push_back(source);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new_with_mnemonic(_("Lay_out preview")));
		gtk_widget_show(widget);


		GtkWidget *count_label = gtk_label_new("");
		widget = palette_list_new(args->gs, count_label);
		args->color_list = widget;
		gtk_widget_show(widget);

		g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK (on_palette_popup_menu), args);
		g_signal_connect(G_OBJECT(widget), "button-press-event",G_CALLBACK (on_palette_button_press), args);
		g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK (on_palette_list_key_press), args);

		GtkWidget *scrolled_window;
		scrolled_window=gtk_scrolled_window_new (0,0);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
		gtk_container_add(GTK_CONTAINER(scrolled_window), args->color_list );
		//gtk_box_pack_start (GTK_BOX(hbox), scrolled_window, TRUE, TRUE, 0);
		gtk_paned_pack2(GTK_PANED(hpaned), scrolled_window, TRUE, FALSE);
		gtk_widget_show(scrolled_window);

	args->hpaned = hpaned;
	args->notebook = notebook;

	{
		const char *tab = dynv_get_string_wd(args->params, "color_source", "");

		map<string, ColorSource*>::iterator i = args->color_source.find(tab);
		if (i != args->color_source.end()){
			uint32_t tab_index = 0;
			for (vector<ColorSource*>::iterator j = args->color_source_index.begin(); j != args->color_source_index.end(); ++j){
				if ((*j) == (*i).second){
					gtk_notebook_set_current_page(GTK_NOTEBOOK(args->notebook), tab_index);
					break;
				}
				tab_index++;
			}

		}
	}


	gtk_box_pack_end (GTK_BOX(vbox_main), statusbar, 0, 0, 0);
	args->statusbar = statusbar;

	GtkWidget *button = gtk_button_new();
	gtk_button_set_focus_on_click(GTK_BUTTON(button), false);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(floating_picker_show_cb), args);
	gtk_widget_add_accelerator(button, "clicked", accel_group, GDK_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_set_tooltip_text(button, _("Pick colors (Ctrl+P)"));
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("gpick", GTK_ICON_SIZE_MENU));
	gtk_box_pack_end(GTK_BOX(statusbar), button, false, false, 0);
	gtk_widget_show_all(button);

	gtk_box_pack_end(GTK_BOX(statusbar), count_label, false, false, 0);
	gtk_widget_show_all(count_label);

	widget = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
	gtk_widget_set_tooltip_text(widget, _("File is currently in a non-native format, possible loss of precision and/or metadata."));
	args->precision_loss_icon = widget;
	if (args->imported)
		gtk_widget_show(widget);
	gtk_box_pack_end(GTK_BOX(statusbar), widget, false, false, 0);

	gtk_widget_show(statusbar);


	{
		//Load recent file list
		char** recent_array;
		uint32_t recent_array_size;
		if ((recent_array = (char**)dynv_get_string_array_wd(args->gs->params, "gpick.recent.files", 0, 0, &recent_array_size))){
			for (int i = 0; i < recent_array_size; i++){
				args->recent_files.push_back(string(recent_array[i]));
			}
			delete [] recent_array;
		}
	}

	createMenu(GTK_MENU_BAR(menu_bar), args, accel_group);
	gtk_widget_show_all(menu_bar);



	args->statusIcon = status_icon_new(args->window, args->gs, args->floating_picker);

	return args;
}


int app_run(AppArgs *args){

	gtk_widget_realize(args->window);

	if (args->options.floating_picker_mode || dynv_get_bool_wd(args->params, "start_in_tray", false)){
		status_icon_set_visible (args->statusIcon, true);
	}else{
		main_show_window(args->window, args->params);
	}

	gtk_paned_set_position(GTK_PANED(args->hpaned), dynv_get_int32_wd(args->params, "paned_position", -1));
	gtk_paned_set_position(GTK_PANED(args->vpaned), dynv_get_int32_wd(args->params, "vertical_paned_position", -1));

	if (args->options.floating_picker_mode)
		floating_picker_activate(args->floating_picker, false);

	gtk_main();

	{
		//Save recent file list
		const char** recent_array;
		uint32_t recent_array_size;

		recent_array_size = args->recent_files.size();
		recent_array = new const char* [recent_array_size];

		uint32_t j =0;
		for (list<string>::iterator i = args->recent_files.begin(); i != args->recent_files.end(); i++){
			recent_array[j] = (*i).c_str();
			j++;
		}

		dynv_set_string_array(args->gs->params, "gpick.recent.files", recent_array, recent_array_size);
		delete [] recent_array;
	}

	gpick_unown_name(args->bus_id);

	status_icon_destroy(args->statusIcon);
	global_state_term(args->gs);
	dynv_system_release(args->params);
	global_state_destroy(args->gs);
	if (args->current_filename) g_free(args->current_filename);
	color_source_manager_destroy(args->csm);
	delete args;

	return 0;
}

