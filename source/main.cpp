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

#include "GlobalState.h"
#include "Paths.h"
#include "Converter.h"
#include "unique/Unique.h"

#include "GenerateScheme.h"
#include "ColorPicker.h"
#include "LayoutPreview.h"
#include "CopyPaste.h"

#include "uiListPalette.h"
#include "uiUtilities.h"
#include "uiExport.h"
#include "uiDialogMix.h"
#include "uiDialogVariations.h"
#include "uiDialogGenerate.h"

#include "uiDialogOptions.h"
#include "uiConverter.h"
#include "uiStatusIcon.h"

#include "FileFormat.h"
#include "MathUtil.h"

#include "version/Version.h" 

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <iostream>
using namespace std;

const gchar* program_name = "gpick";
const gchar* program_authors[] = {"Albertas Vyšniauskas <thezbyg@gmail.com>", NULL };


struct Arguments{
	GtkWidget *window;
	
	ColorSource *color_source[3];
	ColorSource *current_color_source;
	
	GtkWidget *color_list;
	GtkWidget* notebook;
	GtkWidget* statusbar;
	GtkWidget* hpaned;
	
	uiStatusIcon* statusIcon;

	GlobalState* gs;

	char* current_filename;

	gint x, y;
	gint width, height;

};


static gchar **commandline_filename=NULL; 
static gchar *commandline_geometry=NULL; 
static GOptionEntry commandline_entries[] =
{
  { "geometry",	'g', 0, 		G_OPTION_ARG_STRING,			&commandline_geometry, "Window geometry", "GEOMETRY" },
  { G_OPTION_REMAINING, 0, 0, 	G_OPTION_ARG_FILENAME_ARRAY, 	&commandline_filename, NULL, "[FILE...]" },
  { NULL }
};


static gboolean
delete_event( GtkWidget *widget, GdkEvent *event, gpointer data ){
	struct Arguments* window=(struct Arguments*)data;

	if (g_key_file_get_boolean_with_default(window->gs->settings, "Window", "Close to tray", false)){
		gtk_widget_hide(window->window);
		status_icon_set_visible(window->statusIcon, true);
		return TRUE;
	}

	return FALSE;
}

gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer data){
	struct Arguments* window=(struct Arguments*)data;
	
	if (g_key_file_get_boolean_with_default(window->gs->settings, "Window", "Minimize to tray", false)){
	
		if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED){
			if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED){
				gtk_widget_hide(window->window);
				gtk_window_deiconify(GTK_WINDOW(window->window));
				status_icon_set_visible(window->statusIcon, true);
				return TRUE;
			}
		}

	}
	return FALSE;
}

static gboolean
on_window_configure(GtkWidget *widget, GdkEventConfigure *event, gpointer data){
	struct Arguments* window=(struct Arguments*)data;
	

	if (GTK_WIDGET_VISIBLE(widget)){
		
		if (gdk_window_get_state(widget->window) & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_ICONIFIED)) {
			return FALSE;
		}
		
		gint x, y;
		gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
		
		window->x = x;
		window->y = y;	
		window->width = event->width;
		window->height = event->height;	
		
	}else return FALSE;
	
	return FALSE;
}

static void destroy( GtkWidget *widget, gpointer data ){
	struct Arguments* window=(struct Arguments*)data;

	g_key_file_set_integer(window->gs->settings, "Notebook", "Page", gtk_notebook_get_current_page(GTK_NOTEBOOK(window->notebook)));
	g_key_file_set_integer(window->gs->settings, "Window", "Paned position", gtk_paned_get_position(GTK_PANED(window->hpaned)));
	
	if (window->current_color_source){
		color_source_deactivate(window->current_color_source);
		window->current_color_source = 0;
	}
	for (int i=0; i<3; ++i){
		color_source_destroy(window->color_source[i]);
		window->color_source[i] = 0;
	}
	
	g_key_file_set_integer(window->gs->settings, "Window", "X", window->x);
	g_key_file_set_integer(window->gs->settings, "Window", "Y", window->y);
	g_key_file_set_integer(window->gs->settings, "Window", "Width", window->width);
	g_key_file_set_integer(window->gs->settings, "Window", "Height", window->height);
	
    gtk_main_quit ();
}

static void on_window_notebook_switch(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer data){
	struct Arguments* window=(struct Arguments*)data;
	
	if (window->current_color_source) color_source_deactivate(window->current_color_source);	
	
	if (page_num<0 || page_num>2) return;
	if (!window->color_source[page_num]) return;

	color_source_activate(window->color_source[page_num]);
	window->current_color_source = window->color_source[page_num];
	dynv_system_set(window->gs->params, "ptr", "CurrentColorSource", window->current_color_source);
}

int main_get_color_object_from_text(GlobalState* gs, char* text, struct ColorObject** output_color_object){
	struct ColorObject* color_object;
	Color dummy_color;
	
	Converters *converters = (Converters*)dynv_system_get(gs->params, "ptr", "Converters");

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
	Converter **converter_array;
	
	Converters *converters = (Converters*)dynv_system_get(gs->params, "ptr", "Converters");
	
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

void updateProgramName(struct Arguments* window){
	string prg_name;
	if (window->current_filename==0){
		prg_name="New palette";
	}else{
		gchar* filename=g_path_get_basename(window->current_filename);
		prg_name=filename;
		g_free(filename);
	}
	prg_name+=" - ";
	prg_name+=program_name;

	gtk_window_set_title(GTK_WINDOW(window->window), prg_name.c_str());
}


int main_pick_color(GlobalState* gs, GdkEventKey *event){
	struct Arguments* window=(struct Arguments*)dynv_system_get(gs->params, "ptr", "MainWindowStruct");
	//TODO: return on_key_up(0, event, window);
}

static void about_box_activate_url (GtkAboutDialog *about, const gchar *url,  gpointer data){

    char *open[3];

    if (g_find_program_in_path ("xdg-open"))
    {
            open[0] = (char*)"xdg-open";
    }
    else return;

    open[1] = (gchar *)url;
    open[2] = NULL;

    gdk_spawn_on_screen (gdk_screen_get_default (),NULL,open, NULL,G_SPAWN_SEARCH_PATH,NULL,NULL, NULL, NULL);
}


static void
show_about_box(GtkWidget *widget, struct Arguments* window)
{

	const gchar* license = {
		#include "License.h"
	};

	gchar* version=g_strjoin(".", gpick_build_version, gpick_build_revision, NULL);

	gtk_about_dialog_set_url_hook(about_box_activate_url, NULL, NULL);

	gtk_show_about_dialog (GTK_WINDOW(window->window),
		"authors", program_authors,
		"copyright", "Copyrights \xc2\xa9 2009, Albertas Vyšniauskas",
		"license", license,
		"website", "http://code.google.com/p/gpick/",
		"website-label", NULL,
		"version", version,
		"comments", "Advanced color picker",
		"logo-icon-name", "gpick",
		NULL
	);

	g_free(version);
	return;
}

static void show_dialog_converter(GtkWidget *widget, struct Arguments* window){
	dialog_converter_show(GTK_WINDOW(window->window), window->gs);
	return;
}

static void show_dialog_options(GtkWidget *widget, struct Arguments* window){
	dialog_options_show(GTK_WINDOW(window->window), window->gs->settings);
	return;
}


static void menu_file_new(GtkWidget *widget, struct Arguments* window){
	if (window->current_filename) g_free(window->current_filename);
	window->current_filename=0;
	palette_list_remove_all_entries(window->color_list);
	updateProgramName(window);
}

static void menu_file_open(GtkWidget *widget, struct Arguments* window){

	GtkWidget *dialog;
	GtkFileFilter *filter;
	GKeyFile *settings=window->gs->settings;

	dialog = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW(gtk_widget_get_toplevel(widget)),
						  GTK_FILE_CHOOSER_ACTION_OPEN,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gchar* default_path=g_key_file_get_string_with_default(settings, "Open Dialog", "Path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	g_free(default_path);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "gpick palette *.gpa");
	gtk_file_filter_add_pattern(filter, "*.gpa");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	gboolean finished=FALSE;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			g_key_file_set_string(settings, "Open Dialog", "Path", path);
			g_free(path);

			palette_list_remove_all_entries(window->color_list);
			if (window->current_filename) g_free(window->current_filename);
			window->current_filename=0;


			if (palette_file_load(filename, window->gs->colors)==0){
				window->current_filename=g_strdup(filename);
				updateProgramName(window);
				finished=TRUE;
			}else{
				updateProgramName(window);
				GtkWidget* message;
				message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "File could not be opened");
				gtk_window_set_title(GTK_WINDOW(dialog), "Open");
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);

}



static void
menu_file_save_as(GtkWidget *widget, struct Arguments* window){

	GtkWidget *dialog;
	GtkFileFilter *filter;
	GKeyFile *settings=window->gs->settings;

	dialog = gtk_file_chooser_dialog_new ("Save As", GTK_WINDOW(gtk_widget_get_toplevel(widget)),
						  GTK_FILE_CHOOSER_ACTION_SAVE,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						  GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						  NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	gchar* default_path=g_key_file_get_string_with_default(settings, "Open Dialog", "Path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	g_free(default_path);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "gpick palette *.gpa");
	gtk_file_filter_add_pattern(filter, "*.gpa");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	gboolean finished=FALSE;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			g_key_file_set_string(settings, "Open Dialog", "Path", path);
			g_free(path);

			if (palette_file_save(filename, window->gs->colors)==0){
				if (window->current_filename) g_free(window->current_filename);
				window->current_filename=g_strdup(filename);
				updateProgramName(window);
				finished=TRUE;
			}else{
				GtkWidget* message;
				message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "File could not be saved");
				gtk_window_set_title(GTK_WINDOW(dialog), "Open");
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
}

static void menu_file_save(GtkWidget *widget, struct Arguments *window) {
	if (window->current_filename == 0){
		menu_file_save_as(widget, window);
	}else{
		if (palette_file_save(window->current_filename, window->gs->colors) == 0){

		}else{

		}
	}
}

static gint32 color_list_selected(struct ColorObject* color_object, void *userdata){
	color_list_add_color_object((struct ColorList *)userdata, color_object, 1);
	return 0;
}

static void menu_file_export_all(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	dialog_export_show(GTK_WINDOW(window->window), window->gs->colors, 0, window->gs->settings, FALSE);
}

static void menu_file_import(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	dialog_import_show(GTK_WINDOW(window->window), window->gs->colors, 0, window->gs->settings);
}

static void menu_file_export(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_export_show(GTK_WINDOW(window->window), window->gs->colors, color_list, window->gs->settings, TRUE);
	color_list_destroy(color_list);
}
static void menu_file_activate(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;

	gint32 selected_count = palette_list_get_selected_count(window->color_list);
	gint32 total_count = palette_list_get_count(window->color_list);

	//GtkMenu* menu=GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget)));

	GtkWidget** widgets=(GtkWidget**)g_object_get_data(G_OBJECT(widget), "widgets");
	gtk_widget_set_sensitive(widgets[0], (total_count >= 1));
	gtk_widget_set_sensitive(widgets[1], (selected_count >= 1));
}


static void createMenu(GtkMenuBar *menu_bar, struct Arguments *window, GtkAccelGroup *accel_group) {
	GtkMenu* menu;
	GtkWidget* item;
	GtkWidget* file_item ;

    menu = GTK_MENU(gtk_menu_new ());

    GtkWidget** widgets=(GtkWidget**)g_malloc(sizeof(GtkWidget*)*2);

    item = gtk_menu_item_new_with_image ("_New", gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_new), window);
    gtk_widget_add_accelerator (item, "activate", accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    item = gtk_menu_item_new_with_image ("_Open File...", gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_open), window);
    gtk_widget_add_accelerator (item, "activate", accel_group, GDK_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("_Save", gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_save), window);
    gtk_widget_add_accelerator (item, "activate", accel_group, GDK_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    item = gtk_menu_item_new_with_image ("_Save As...", gtk_image_new_from_stock(GTK_STOCK_SAVE_AS, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_save_as), window);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_image_menu_item_new_with_mnemonic ("Ex_port...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (menu_file_export_all),window);
    widgets[0]=item;

    //gtk_widget_set_sensitive(item, (total_count >= 1));

    item = gtk_image_menu_item_new_with_mnemonic ("Expo_rt Selected...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (menu_file_export),window);
    widgets[1]=item;

    //gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_image_menu_item_new_with_mnemonic ("_Import...");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (menu_file_import),window);
    //gtk_widget_set_sensitive(item, 0);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("E_xit", gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (destroy), window);


    file_item = gtk_menu_item_new_with_mnemonic ("_File");
    g_signal_connect (G_OBJECT (file_item), "activate", G_CALLBACK (menu_file_activate), window);
    g_object_set_data_full(G_OBJECT(file_item), "widgets", widgets, (GDestroyNotify)g_free);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);

    menu = GTK_MENU(gtk_menu_new ());

    item = gtk_menu_item_new_with_image ("Edit _Converters...", gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_dialog_converter), window);
    
    item = gtk_menu_item_new_with_image ("_Preferences...", gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_dialog_options), window);
    
    
    
    file_item = gtk_menu_item_new_with_mnemonic ("_Edit");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
	
	
    menu = GTK_MENU(gtk_menu_new ());

    item = gtk_menu_item_new_with_image ("_About", gtk_image_new_from_stock(GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_about_box),window);


    file_item = gtk_menu_item_new_with_mnemonic ("_Help");
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
/*
static gint32 color_list_selected(struct ColorObject* color_object, void *userdata){
	color_list_add_color_object((struct ColorList *)userdata, color_object, 1);
	return 0;
}*/



static void converter_callback_copy(GtkWidget *widget,  gpointer item) {
	CopyMenuItem* itemdata=(CopyMenuItem*)g_object_get_data(G_OBJECT(widget), "item_data");
	converter_get_clipboard(itemdata->function_name, itemdata->color_object, itemdata->palette_widget, itemdata->gs->params);
}


static GtkWidget* converter_create_copy_menu_item (GtkWidget *menu, const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, GlobalState *gs){
	GtkWidget* item=0;
	gchar* converted;
	
	if (converters_color_serialize((Converters*)dynv_system_get(gs->params, "ptr", "Converters"), function, color_object, &converted)==0){
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
	Converters *converters = (Converters*)dynv_system_get(gs->params, "ptr", "Converters");
	
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
	
	Converters* converters = (Converters*)dynv_system_get(params, "ptr", "Converters");
	for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
	
		gchar* converted;
	
		if (converters_color_serialize((Converters*)dynv_system_get(params, "ptr", "Converters"), function, *i, &converted)==0){
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
		g_free(text);
	}
}

gint32 color_list_mark_selected(struct ColorObject* color_object, void *userdata){
	color_object->selected=1;
	return 0;
}

static void palette_popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu) {
	gtk_widget_destroy(GTK_WIDGET(menu));
}

static void palette_popup_menu_remove_all(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	color_list_remove_all(window->gs->colors);
}

static void palette_popup_menu_remove_selected(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	palette_list_foreach_selected(window->color_list, color_list_mark_selected, 0);
	color_list_remove_selected(window->gs->colors);
}



gint32 palette_popup_menu_mix_list(Color* color, void *userdata){
	*((GList**)userdata) = g_list_append(*((GList**)userdata), color);
	return 0;
}

static void palette_popup_menu_mix(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;	
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_mix_show(GTK_WINDOW(window->window), color_list, window->gs);
	color_list_destroy(color_list);
}



static void palette_popup_menu_variations(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	
	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(window->gs->params);
	
	struct ColorList *color_list = color_list_new(handler_map);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_variations_show(GTK_WINDOW(window->window), color_list, window->gs);
	color_list_destroy(color_list);
	
	dynv_handler_map_release(handler_map);
}

static void palette_popup_menu_generate(GtkWidget *widget, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;	
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_generate_show(GTK_WINDOW(window->window), color_list, window->gs);
	color_list_destroy(color_list);
}

static gboolean palette_popup_menu_show(GtkWidget *widget, GdkEventButton* event, gpointer ptr) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}
	
	GtkWidget* item ;
	gint32 button, event_time;

	struct Arguments* window=(struct Arguments*)ptr;
	
	/*GList* menus=gtk_menu_get_for_attach_widget(widget);
	while (menus){
		gtk_menu_detach(GTK_MENU(menus->data));
		menus = g_list_next(menus);
	}*/

	menu = gtk_menu_new ();

	gint32 selected_count = palette_list_get_selected_count(window->color_list);
	gint32 total_count = palette_list_get_count(window->color_list);

	item = gtk_menu_item_new_with_mnemonic ("_Copy to Clipboard");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_set_sensitive(item, (selected_count >= 1));

	if (total_count>0){
		struct ColorList *color_list = color_list_new(NULL);
		palette_list_forfirst_selected(window->color_list, color_list_selected, color_list);
		if (color_list_get_count(color_list)!=0){
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (*color_list->colors.begin(), window->color_list, window->gs));
		}
		color_list_destroy(color_list);
	}

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("_Mix Colors...", gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_mix),window);
    gtk_widget_set_sensitive(item, (selected_count >= 2));

    item = gtk_menu_item_new_with_image ("_Variations...", gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_variations),window);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_image ("_Generate...", gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_generate),window);
    gtk_widget_set_sensitive(item, (selected_count >= 1));
    

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("_Remove", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_remove_selected),window);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_image ("Remove _All", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_remove_all),window);
    gtk_widget_set_sensitive(item, (total_count >= 1));

    gtk_widget_show_all (GTK_WIDGET(menu));

	if (event){
		button = event->button;
		event_time = event->time;
	}else{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_attach_to_widget (GTK_MENU (menu), widget, palette_popup_menu_detach);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  button, event_time);

	return TRUE;
}

static void on_palette_popup_menu(GtkWidget *widget, gpointer item) {
	palette_popup_menu_show(widget, 0, item);
}

static gboolean on_palette_button_press(GtkWidget *widget, GdkEventButton *event, gpointer ptr) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		return palette_popup_menu_show(widget, event, ptr);
	}
	return FALSE;
}

static gboolean on_palette_list_key_press (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	struct Arguments* window=(struct Arguments*)data;

	guint modifiers = gtk_accelerator_get_default_mod_mask();
	
	switch(event->keyval)
	{
		case GDK_c:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){
				
				Converters *converters = (Converters*)dynv_system_get(window->gs->params, "ptr", "Converters");
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, 0, window->color_list, window->gs->params);
				}

				return true;
			}
			return false;
			break;
			
		case GDK_v:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){
				struct ColorObject* color_object;
				if (copypaste_get_color_object(&color_object, window->gs)==0){
					color_list_add_color_object(window->gs->colors, color_object, 1);
					color_object_release(color_object);
				}
				return true;
			}
			return false;
			break;
			
		default:
			return false;
		break;
	}
	return false;
}




void set_main_window_icon() {
	GList *icons = 0;

	GtkIconTheme *icon_theme;
	//GdkPixbuf *pixbuf;
	icon_theme = gtk_icon_theme_get_default();

	gint sizes[] = { 16, 32, 48, 128 };

	for (guint32 i = 0; i < sizeof(sizes) / sizeof(gint); ++i) {
		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, "gpick", sizes[i], GtkIconLookupFlags(0), 0);
		if (pixbuf) {
			icons = g_list_append(icons, pixbuf);
		}
	}

	if (icons){
		gtk_window_set_default_icon_list(icons);

		g_list_foreach(icons, (GFunc)g_object_unref, NULL);
		g_list_free(icons);
	}
}

int color_list_on_insert(struct ColorList* color_list, struct ColorObject* color_object){
	palette_list_add_entry(((struct Arguments*)color_list->userdata)->color_list, color_object);
	return 0;
}

int color_list_on_delete_selected(struct ColorList* color_list){
	palette_list_remove_selected_entries(((struct Arguments*)color_list->userdata)->color_list);
	return 0;
}

int color_list_on_delete(struct ColorList* color_list, struct ColorObject* color_object){
	palette_list_remove_entry(((struct Arguments*)color_list->userdata)->color_list, color_object);
	return 0;
}

int color_list_on_clear(struct ColorList* color_list){
	palette_list_remove_all_entries(((struct Arguments*)color_list->userdata)->color_list);
	return 0;
}

gint32 callback_color_list_on_get_positions(struct ColorObject* color_object, void *userdata){
	color_object->position=*((unsigned long*)userdata);
	(*((unsigned long*)userdata))++;
	return 0;
}

int color_list_on_get_positions(struct ColorList* color_list){
	unsigned long item=0;
	palette_list_foreach(((struct Arguments*)color_list->userdata)->color_list, callback_color_list_on_get_positions, &item );
	return 0;
}

int main_show_window(GtkWidget* window, GKeyFile* settings){
	
	if (GTK_WIDGET_VISIBLE(window)){
		gtk_window_deiconify(GTK_WINDOW(window));
		return -1;	//already visible
	}
	
	
	gint x, y;
	gint width, height;
	
	x = g_key_file_get_integer(settings, "Window", "X", 0);
	y = g_key_file_get_integer(settings, "Window", "Y", 0),
	
	width = g_key_file_get_integer(settings, "Window", "Width", 0);
	height = g_key_file_get_integer(settings, "Window", "Height", 0);
	
	if (x<0 || y<0 || x>gdk_screen_width() || y>gdk_screen_height()){
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	}else{
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
		gtk_window_move(GTK_WINDOW(window), x, y);
	}
	
	if (g_key_file_get_boolean_with_default(settings, "View", "Minimal", false)){
		//gtk_window_resize(GTK_WINDOW(window), -1, -1);
	}else{
	
		gtk_window_resize(GTK_WINDOW(window), width, height);
	}
	
	gtk_widget_show(window);
	return 0;
}

static gboolean on_window_focus_change(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
	struct Arguments* window=(struct Arguments*)data;
	
	if (event->in){
		if (window->current_color_source) color_source_activate(window->current_color_source);
	}else{
		if (window->current_color_source) color_source_deactivate(window->current_color_source);
	}
	
	return FALSE;
}

int unique_show_window(struct Arguments* args){
	status_icon_set_visible (args->statusIcon, false);
	main_show_window(args->window, args->gs->settings);
	return 0;
}

int main(int argc, char **argv){
	struct Arguments* args=new struct Arguments;
	
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

	
	args->current_filename = 0;
	args->current_color_source = 0;

	args->gs = new GlobalState;
	global_state_init(args->gs);
	args->gs->colors->on_insert = color_list_on_insert;
	args->gs->colors->on_clear = color_list_on_clear;
	args->gs->colors->on_delete_selected = color_list_on_delete_selected;
	args->gs->colors->on_get_positions = color_list_on_get_positions;
	args->gs->colors->on_delete = color_list_on_delete;
	args->gs->colors->userdata = args;
	
	dynv_system_set(args->gs->params, "ptr", "MainWindowStruct", args);
	
	
	if (g_key_file_get_boolean_with_default(args->gs->settings, "Program", "Single instance", false)){
		if (unique_init((unique_cb_t)unique_show_window, args)==0){
			
		}else{
			delete args;
			return 1;
		}
	}	
	
	args->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	set_main_window_icon();

	updateProgramName(args);

	g_signal_connect(G_OBJECT(args->window), "delete_event", 		G_CALLBACK (delete_event), args);
	g_signal_connect(G_OBJECT(args->window), "destroy",      		G_CALLBACK (destroy), args);
	g_signal_connect(G_OBJECT(args->window), "configure-event",		G_CALLBACK (on_window_configure), args);
	g_signal_connect(G_OBJECT(args->window), "window-state-event",	G_CALLBACK (on_window_state_event), args);
	g_signal_connect(G_OBJECT(args->window), "focus-in-event", 		G_CALLBACK (on_window_focus_change), args);
	g_signal_connect(G_OBJECT(args->window), "focus-out-event", 	G_CALLBACK (on_window_focus_change), args);
	
    GtkAccelGroup *accel_group=gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(args->window), accel_group);
	g_object_unref(G_OBJECT (accel_group));

    //gtk_accel_group_connect(accel_group, GDK_s, GdkModifierType(GDK_CONTROL_MASK), GtkAccelFlags(GTK_ACCEL_VISIBLE), g_cclosure_new (G_CALLBACK (menu_file_save),window,NULL));

    GtkWidget *widget,*expander,*table,*vbox,*hbox,*statusbar,*notebook,*frame,*vbox2,*hbox2,*hpaned;
    int table_y;

    GtkWidget* vbox_main = gtk_vbox_new(false, 0);
    gtk_container_add (GTK_CONTAINER(args->window), vbox_main);

		GtkWidget* menu_bar;
		menu_bar = gtk_menu_bar_new ();
		gtk_box_pack_start (GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);
		
 	hpaned = gtk_hpaned_new();
 	gtk_box_pack_start (GTK_BOX(vbox_main), hpaned, TRUE, TRUE, 5);
 	
    //hbox = gtk_hbox_new(FALSE, 5);
    //gtk_box_pack_start (GTK_BOX(vbox_main), hbox, TRUE, TRUE, 5);
   
    
	notebook = gtk_notebook_new();
	g_signal_connect (G_OBJECT (notebook), "switch-page", G_CALLBACK (on_window_notebook_switch), args);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), true);
	
	//gtk_box_pack_start (GTK_BOX(hbox), notebook, FALSE, TRUE, 5); 
	gtk_paned_pack1(GTK_PANED(hpaned), notebook, FALSE, FALSE);
    
	
	gtk_widget_show_all(vbox_main);
	

		args->color_source[0] = color_picker_new(args->gs, &widget);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook),widget,gtk_label_new("Color picker"));
		gtk_widget_show(widget);
		
		args->color_source[1] = generate_scheme_new(args->gs, &widget);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook),widget,gtk_label_new("Scheme generation"));
		gtk_widget_show(widget);
		
		args->color_source[2] = layout_preview_new(args->gs, &widget);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook),widget,gtk_label_new("Layout preview"));
		gtk_widget_show(widget);
		
		widget = palette_list_new(args->gs);
		args->color_list = widget;
		gtk_widget_show(widget);

		g_signal_connect(G_OBJECT(widget), "popup-menu",     G_CALLBACK (on_palette_popup_menu), args);
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
	
	//gtk_widget_show_all (window->notebook);
	
	{
		
		int page = g_key_file_get_integer_with_default(args->gs->settings, "Notebook", "Page", 0);
		
		if (page>2) page=2;
		else if (page<0) page=0;
		
		gtk_notebook_set_current_page(GTK_NOTEBOOK(args->notebook), page);
	}
	
	
	statusbar=gtk_statusbar_new();
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "focus_swatch"), "Click on swatch area to begin adding colors to palette");
	gtk_box_pack_end (GTK_BOX(vbox_main), statusbar, 0, 0, 0);
	args->statusbar=statusbar;
	dynv_system_set(args->gs->params, "ptr", "StatusBar", statusbar);
	gtk_widget_show(statusbar);
	
	createMenu(GTK_MENU_BAR(menu_bar), args, accel_group);
	gtk_widget_show_all(menu_bar);

	
	//gtk_widget_show_all(vbox_main);	



	if (commandline_filename){
		if (palette_file_load(commandline_filename[0], args->gs->colors)==0){
			args->current_filename=g_strdup(commandline_filename[0]);
			updateProgramName(args);
		}
	}
	
	/*if (g_key_file_get_boolean_with_default(window->gs->settings, "View", "Minimal", false)==false){
		gtk_window_resize(GTK_WINDOW(window->window), g_key_file_get_integer_with_default(window->gs->settings, "Window", "Width", 1), g_key_file_get_integer_with_default(window->gs->settings, "Window", "Height", 1));
	}*/
	
	//GdkGeometry size_hints = { -1,-1, 0, 0, 0, 0, 0, 0, 0.0, 0.0, GDK_GRAVITY_CENTER };
	//gtk_window_set_geometry_hints (GTK_WINDOW(window->window), window->window, &size_hints, GdkWindowHints(GDK_HINT_WIN_GRAVITY));
	
	if (commandline_geometry){
		gtk_window_parse_geometry(GTK_WINDOW(args->window), (const gchar*)commandline_geometry);
	}

	args->statusIcon = status_icon_new(args->window, args->gs, args->color_source[0]);
	
	gtk_widget_realize(args->window);
	
	gtk_paned_set_position(GTK_PANED(hpaned), g_key_file_get_integer_with_default(args->gs->settings, "Window", "Paned position", -1) );
	
	if (g_key_file_get_boolean_with_default(args->gs->settings, "Window", "Start in tray", false)){
		status_icon_set_visible (args->statusIcon, true);
	}else{
		main_show_window(args->window, args->gs->settings);
	}
	
	
	
	gtk_main ();
	
	
	
	unique_term();
	status_icon_destroy(args->statusIcon);
	global_state_term(args->gs);
	delete args->gs;
	if (args->current_filename) g_free(args->current_filename);
	delete args;
	g_option_context_free(context);
	
	return 0;
}
