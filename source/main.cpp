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
#include <math.h>

#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "gtk/uiSwatch.h"
#include "gtk/uiZoomed.h"

#include "uiColorComponent.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "uiExport.h"
#include "uiDialogMix.h"
#include "uiDialogVariations.h"
#include "uiDialogGenerate.h"
#include "uiConverter.h"

#include "FileFormat.h"
#include "Sampler.h"
#include "ColorObject.h"
#include "MathUtil.h"
#include "ColorNames.h"
#include "Random.h"
//#include "LuaSystem.h"
#include "LuaExt.h"
#include "version/Version.h" 

#include "dynv/DynvSystem.h"
#include "dynv/DynvMemoryIO.h"
#include "dynv/DynvVarString.h"
#include "dynv/DynvVarInt32.h"
#include "dynv/DynvVarColor.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

//#include <algorithm>
//#include <list>
//#include <vector>
//#include <iomanip>

using namespace std;


const gchar* program_name = "gpick";
const gchar* program_authors[] = {"Albertas Vyšniauskas <thezbyg@gmail.com>", NULL };



typedef struct MainWindow{
	GtkWidget *window;

	GtkWidget *swatch_display;
	GtkWidget *zoomed_display;
	GtkWidget *color_list;

	GtkWidget* hue_line;
	GtkWidget* saturation_line;
	GtkWidget* value_line;

	GtkWidget* red_line;
	GtkWidget* green_line;
	GtkWidget* blue_line;

	GtkWidget* color_name;
	GtkWidget* notebook;

	GtkWidget* expanderRGB;
	GtkWidget* expanderHSV;
	GtkWidget* expanderInfo;
	GtkWidget* expanderMain;
	
	GtkWidget* statusbar;

	ColorNames* cnames;
	struct Sampler* sampler;
	struct ColorList* colors;
	struct dynvSystem* params;
	GKeyFile* settings;
	//struct LuaSystem* lua;
	
	lua_State *lua;

	Random* random;

	gboolean add_to_palette;
	gboolean rotate_swatch;

	string current_filename;

}MainWindow;


static gchar **commandline_filename=NULL; 
static GOptionEntry commandline_entries[] =
{
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &commandline_filename, "Open palette file", NULL },
  { NULL }
};


static gboolean
delete_event( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    return FALSE;
}

static void
destroy( GtkWidget *widget, gpointer data )
{
	MainWindow* window=(MainWindow*)data;

	g_key_file_set_integer(window->settings, "Swatch", "Active Color", gtk_swatch_get_active_index(GTK_SWATCH(window->swatch_display)));
	{
	gdouble color_list[3*6];
	for (gint i=0; i<6; ++i){
		Color c;
		gtk_swatch_get_color(GTK_SWATCH(window->swatch_display), i+1, &c);
		color_list[0+3*i]=c.rgb.red;
		color_list[1+3*i]=c.rgb.green;
		color_list[2+3*i]=c.rgb.blue;
	}
	g_key_file_set_double_list(window->settings, "Swatch", "Colors", color_list, sizeof(color_list)/sizeof(gdouble));
	}

	g_key_file_set_integer(window->settings, "Sampler", "Oversample", sampler_get_oversample(window->sampler));
	g_key_file_set_integer(window->settings, "Sampler", "Falloff", sampler_get_falloff(window->sampler));
	g_key_file_set_boolean(window->settings, "Sampler", "Add to palette", window->add_to_palette);
	g_key_file_set_boolean(window->settings, "Sampler", "Rotate swatch after sample", window->rotate_swatch);

	g_key_file_set_double(window->settings, "Zoom", "Zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(window->zoomed_display)));

	g_key_file_set_integer(window->settings, "Notebook", "Page", gtk_notebook_get_current_page(GTK_NOTEBOOK(window->notebook)));

	g_key_file_set_boolean(window->settings, "Expander", "RGB", gtk_expander_get_expanded(GTK_EXPANDER(window->expanderRGB)));
	g_key_file_set_boolean(window->settings, "Expander", "Info", gtk_expander_get_expanded(GTK_EXPANDER(window->expanderInfo)));
	g_key_file_set_boolean(window->settings, "Expander", "HSV", gtk_expander_get_expanded(GTK_EXPANDER(window->expanderHSV)));
	
    gtk_main_quit ();
}




static gboolean
updateMainColor( gpointer data )
{
	MainWindow* window=(MainWindow*)data;

	if (gtk_window_is_active(GTK_WINDOW(window->window))){

		Color c;
		sampler_get_color_sample(window->sampler, &c);
		//sample_color(&c);

		gtk_swatch_set_main_color(GTK_SWATCH(window->swatch_display), &c);

		gtk_zoomed_update(GTK_ZOOMED(window->zoomed_display));
	}

	return TRUE;
}


void
updateDiplays(MainWindow* window)
{
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(window->swatch_display),&c);



	gtk_color_component_set_color(GTK_COLOR_COMPONENT(window->hue_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(window->saturation_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(window->value_line), &c);

	gtk_color_component_set_color(GTK_COLOR_COMPONENT(window->red_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(window->green_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(window->blue_line), &c);

	string color_name = color_names_get(window->cnames, &c);
	gtk_entry_set_text(GTK_ENTRY(window->color_name), color_name.c_str());

}

void updateProgramName(MainWindow* window){
	string prg_name;
	if (window->current_filename==""){
		prg_name="New palette";
	}else{
		gchar* filename=g_path_get_basename(window->current_filename.c_str());
		prg_name=filename;
		g_free(filename);
	}
	prg_name+=" - ";
	prg_name+=program_name;

	gtk_window_set_title(GTK_WINDOW(window->window), prg_name.c_str());
}

static void
on_swatch_active_color_changed( GtkWidget *widget, gint32 new_active_color, gpointer data )
{
	MainWindow* window=(MainWindow*)data;
	updateDiplays(window);
}

static void
on_swatch_color_changed( GtkWidget *widget, gpointer data )
{
	MainWindow* window=(MainWindow*)data;
	updateDiplays(window);
}


static void
on_swatch_menu_detach(GtkWidget *attach_widget, GtkMenu *menu)
{
	gtk_widget_destroy(GTK_WIDGET(menu));
}


static void on_swatch_menu_add_to_palette(GtkWidget *widget,  gpointer item) {
	MainWindow* window=(MainWindow*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(window->swatch_display), &c);
	//palette_list_add_entry(window->color_list, window->cnames, &c);
	//color_list_add_color(window->colors, &c);
				
	struct ColorObject *color_object=color_list_new_color_object(window->colors, &c);
	string name=color_names_get(window->cnames, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(window->colors, color_object, 1);
}

static void on_swatch_menu_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	MainWindow* window=(MainWindow*)item;
	Color c;
	for (int i = 1; i < 7; ++i) {
		gtk_swatch_get_color(GTK_SWATCH(window->swatch_display), i, &c);
		//palette_list_add_entry(window->color_list, window->cnames, &c);
		//color_list_add_color(window->colors, &c);
		
		struct ColorObject *color_object=color_list_new_color_object(window->colors, &c);
		string name=color_names_get(window->cnames, &c);
		dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
		color_list_add_color_object(window->colors, color_object, 1);
	}
}
static void
on_swatch_color_activated (GtkWidget *widget, gpointer item) {
	MainWindow* window=(MainWindow*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(widget), &c);
	//palette_list_add_entry(window->color_list, window->cnames, &c);
	//color_list_add_color(window->colors, &c);
	
	struct ColorObject *color_object=color_list_new_color_object(window->colors, &c);
	string name=color_names_get(window->cnames, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(window->colors, color_object, 1);
}

static gboolean
on_swatch_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	MainWindow* window=(MainWindow*)data;

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		GtkWidget *menu;
		GtkWidget* item ;
		gint32 button, event_time;

		menu = gtk_menu_new ();

		Color c;
		gtk_swatch_get_active_color(GTK_SWATCH(window->swatch_display), &c);

	    item = gtk_menu_item_new_with_image ("_Add to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_menu_add_to_palette),window);

	    item = gtk_menu_item_new_with_image ("_Add all to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_menu_add_all_to_palette),window);

	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	    item = gtk_menu_item_new_with_mnemonic ("_Copy to clipboard");
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    
	    struct ColorObject* color_object;
	    color_object = color_list_new_color_object(window->colors, &c);
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (color_object, 0, window->settings, window->lua));
		color_object_release(color_object);

	    gtk_widget_show_all (GTK_WIDGET(menu));

		if (event){
			button = event->button;
			event_time = event->time;
		}else{
			button = 0;
			event_time = gtk_get_current_event_time ();
		}

		gtk_menu_attach_to_widget (GTK_MENU (menu), widget, on_swatch_menu_detach);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
					  button, event_time);
	}
	return FALSE;
}



gboolean on_swatch_focus_change(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	if (event->in){
		gtk_statusbar_push(GTK_STATUSBAR(window->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "swatch_focused"), "Press SPACE to sample color under pointer");
	}else{
		gtk_statusbar_pop(GTK_STATUSBAR(window->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(window->statusbar), "swatch_focused"));
	}
	return FALSE;
}


gboolean on_key_up (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	MainWindow* window=(MainWindow*)data;

	switch(event->keyval)
	{
		case GDK_Right:
			gtk_swatch_move_active(GTK_SWATCH(window->swatch_display),1);
			updateDiplays(window);
			return TRUE;
			break;

		case GDK_Left:
			gtk_swatch_move_active(GTK_SWATCH(window->swatch_display),-1);
			updateDiplays(window);
			return TRUE;
			break;

		case GDK_space:
			updateMainColor(window);
			gtk_swatch_set_color_to_main(GTK_SWATCH(window->swatch_display));

			if (window->add_to_palette){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(window->swatch_display), &c);
				
				struct ColorObject *color_object=color_list_new_color_object(window->colors, &c);
				string name=color_names_get(window->cnames, &c);
				dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
				color_list_add_color_object(window->colors, color_object, 1);
			}
			if (window->rotate_swatch){
				gtk_swatch_move_active(GTK_SWATCH(window->swatch_display),1);
			}
			updateDiplays(window);
			return TRUE;
			break;

		default:
			return FALSE;
		break;
	}
	return FALSE;
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
show_about_box(GtkWidget *widget, MainWindow* window)
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
		NULL
	);

	g_free(version);
	return;
}

static void
show_dialog_converter(GtkWidget *widget, MainWindow* window){
	dialog_converter_show(GTK_WINDOW(window->window), window->settings, window->lua, window->colors);
	return;
}

static void
menu_file_new(GtkWidget *widget, MainWindow* window){
	window->current_filename="";
	palette_list_remove_all_entries(window->color_list);
	updateProgramName(window);
}

static void
menu_file_open(GtkWidget *widget, MainWindow* window){

	GtkWidget *dialog;
	GtkFileFilter *filter;
	GKeyFile *settings=window->settings;

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
			window->current_filename="";


			if (palette_file_load(filename, window->colors)==0){
				window->current_filename=filename;
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
menu_file_save_as(GtkWidget *widget, MainWindow* window){

	GtkWidget *dialog;
	GtkFileFilter *filter;
	GKeyFile *settings=window->settings;

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

			if (palette_file_save(filename, window->colors)==0){
				window->current_filename=filename;
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

static void menu_file_save(GtkWidget *widget, MainWindow *window) {
	if (window->current_filename == ""){
		menu_file_save_as(widget, window);
	}else{
		if (palette_file_save(window->current_filename.c_str(), window->colors) == 0){

		}else{

		}
	}
}

static gint32 color_list_selected(struct ColorObject* color_object, void *userdata){
	color_list_add_color_object((struct ColorList *)userdata, color_object, 1);
	return 0;
}

static void menu_file_export_all(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	dialog_export_show(GTK_WINDOW(window->window), window->colors, 0, window->settings, FALSE);
}

static void menu_file_import(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	dialog_import_show(GTK_WINDOW(window->window), window->colors, 0, window->settings);
}

static void menu_file_export(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_export_show(GTK_WINDOW(window->window), window->colors, color_list, window->settings, TRUE);
	color_list_destroy(color_list);
}
static void menu_file_activate(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;

	gint32 selected_count = palette_list_get_selected_count(window->color_list);
	gint32 total_count = palette_list_get_count(window->color_list);

	//GtkMenu* menu=GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget)));

	GtkWidget** widgets=(GtkWidget**)g_object_get_data(G_OBJECT(widget), "widgets");
	gtk_widget_set_sensitive(widgets[0], (total_count >= 1));
	gtk_widget_set_sensitive(widgets[1], (selected_count >= 1));
}

static void menu_view_minimal(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	
	if (GTK_WIDGET_VISIBLE(window->notebook)){
		gtk_widget_hide(window->notebook);
		g_key_file_set_boolean(window->settings, "View", "Minimal", TRUE);
	}else{
		gtk_widget_show(window->notebook);
		g_key_file_set_boolean(window->settings, "View", "Minimal", FALSE);
	}
}

static void createMenu(GtkMenuBar *menu_bar, MainWindow *window, GtkAccelGroup *accel_group) {
	GtkMenu* menu;
	GtkWidget* item;
	GtkWidget* file_item ;

    menu = GTK_MENU(gtk_menu_new ());

    GtkWidget** widgets=(GtkWidget**)g_malloc(sizeof(GtkWidget*)*2);

    item = gtk_menu_item_new_with_image ("_New", gtk_image_new_from_stock(GTK_STOCK_NEW, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_file_new), window);
    gtk_widget_add_accelerator (item, "activate", accel_group, GDK_n, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    item = gtk_menu_item_new_with_image ("_Open file...", gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU));
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

    item = gtk_image_menu_item_new_with_mnemonic ("Expo_rt selected...");
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

    item = gtk_menu_item_new_with_image ("Edit _converters...", gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_dialog_converter), window);
    
    file_item = gtk_menu_item_new_with_mnemonic ("_Edit");
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
	
	
    menu = GTK_MENU(gtk_menu_new ());

    item = gtk_check_menu_item_new_with_mnemonic ("Minimal");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_view_minimal), window);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), g_key_file_get_boolean_with_default(window->settings, "View", "Minimal", FALSE));
	gtk_widget_add_accelerator (item, "activate", accel_group, GDK_m, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	
    
    file_item = gtk_menu_item_new_with_mnemonic ("_View");
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


void
on_oversample_value_changed(GtkRange *slider, gpointer data)
{
	MainWindow* window=(MainWindow*)data;
	sampler_set_oversample(window->sampler, (int)gtk_range_get_value(GTK_RANGE(slider)));
}

void
on_zoom_value_changed(GtkRange *slider, gpointer data){
	MainWindow* window=(MainWindow*)data;
	gtk_zoomed_set_zoom(GTK_ZOOMED(window->zoomed_display), gtk_range_get_value(GTK_RANGE(slider)));
}



void
color_component_change_value(GtkSpinButton *spinbutton, Color* c, gpointer data)
{
	MainWindow* window=(MainWindow*)data;
	gtk_swatch_set_active_color(GTK_SWATCH(window->swatch_display),c);
	updateDiplays(window);
}

//g_get_user_config_dir()



gint32 color_list_mark_selected(struct ColorObject* color_object, void *userdata){
	color_object->selected=1;
	return 0;
}

static void palette_popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu) {
	gtk_widget_destroy(GTK_WIDGET(menu));
}

static void palette_popup_menu_remove_all(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	color_list_remove_all(window->colors);
}

static void palette_popup_menu_remove_selected(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	palette_list_foreach_selected(window->color_list, color_list_mark_selected, 0);
	color_list_remove_selected(window->colors);
}



gint32 palette_popup_menu_mix_list(Color* color, void *userdata){
	*((GList**)userdata) = g_list_append(*((GList**)userdata), color);
	return 0;
}

static void palette_popup_menu_mix(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;	
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_mix_show(GTK_WINDOW(window->window), window->colors, color_list, window->settings);
	color_list_destroy(color_list);
}



static void palette_popup_menu_variations(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	
	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(window->params);
	
	struct ColorList *color_list = color_list_new(handler_map);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_variations_show(GTK_WINDOW(window->window), window->colors, color_list, window->settings);
	color_list_destroy(color_list);
	
	dynv_handler_map_release(handler_map);
}

static void palette_popup_menu_generate(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;	
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_generate_show(GTK_WINDOW(window->window), window->colors, color_list, window->settings, window->random);
	color_list_destroy(color_list);
}


static gboolean palette_popup_menu_show(GtkWidget *widget, GdkEventButton* event, gpointer ptr) {
	GtkWidget *menu;
	GtkWidget* item ;
	gint32 button, event_time;

	MainWindow* window=(MainWindow*)ptr;

	menu = gtk_menu_new ();

	gint32 selected_count = palette_list_get_selected_count(window->color_list);
	gint32 total_count = palette_list_get_count(window->color_list);

	item = gtk_menu_item_new_with_mnemonic ("_Copy to clipboard");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_set_sensitive(item, (selected_count >= 1));

	if (total_count>0){
		struct ColorList *color_list = color_list_new(NULL);
		palette_list_forfirst_selected(window->color_list, color_list_selected, color_list);
		if (color_list_get_count(color_list)!=0){
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (*color_list->colors.begin(), window->color_list, window->settings, window->lua));
		}
		color_list_destroy(color_list);
	}

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("_Mix colors...", gtk_image_new_from_stock(GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU));
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


/*	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("_Export all...", gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_export),window);
    gtk_widget_set_sensitive(item, (total_count >= 1));

    item = gtk_menu_item_new_with_image ("Export _selected...", gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_export_selected),window);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_image ("_Import...", gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_export),window);
    gtk_widget_set_sensitive(item, 0);*/

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    item = gtk_menu_item_new_with_image ("_Remove", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK (palette_popup_menu_remove_selected),window);
    gtk_widget_set_sensitive(item, (selected_count >= 1));

    item = gtk_menu_item_new_with_image ("Remove _all", gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
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


static void on_popup_menu(GtkWidget *widget, gpointer user_data) {
	GtkWidget *menu;
	//GtkWidget* item ;
	gint32 button, event_time;
	MainWindow* window=(MainWindow*)user_data;

	Color c;
	updateMainColor(window);
	gtk_swatch_get_main_color(GTK_SWATCH(window->swatch_display), &c);

    struct ColorObject* color_object;
    color_object = color_list_new_color_object(window->colors, &c);
    menu = converter_create_copy_menu (color_object, 0, window->settings, window->lua);
	color_object_release(color_object);

    gtk_widget_show_all (GTK_WIDGET(menu));

	button = 0;
	event_time = gtk_get_current_event_time ();

	gtk_menu_attach_to_widget (GTK_MENU (menu), widget, palette_popup_menu_detach);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  button, event_time);
}

void on_oversample_falloff_changed(GtkWidget *widget, gpointer data) {
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

		gint32 falloff_id;
		gtk_tree_model_get(model, &iter, 2, &falloff_id, -1);

		MainWindow* window=(MainWindow*)data;
		sampler_set_falloff(window->sampler, (enum SamplerFalloff) falloff_id);

	}

	/*gint32 active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	if (active!=-1){



	}	*/
}


void on_add_to_palette_changed(GtkWidget *widget, gpointer data) {
	((MainWindow*)data)->add_to_palette = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}
void on_rotate_swatch_changed(GtkWidget *widget, gpointer data) {
	((MainWindow*)data)->rotate_swatch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

GtkWidget* create_falloff_type_list (void){
	GtkListStore  		*store;
	GtkCellRenderer     *renderer;
	GtkWidget           *widget;

	store = gtk_list_store_new (3, GDK_TYPE_PIXBUF,G_TYPE_STRING,G_TYPE_INT);

	widget = gtk_combo_box_new_with_model (GTK_TREE_MODEL(store));
	gtk_combo_box_set_add_tearoffs (GTK_COMBO_BOX (widget), 0);

	renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget),renderer,0);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer,"pixbuf",0,NULL);

	renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget),renderer,0);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer,"text",1,NULL);

	g_object_unref (GTK_TREE_MODEL(store));

	GtkTreeIter iter1;

	const char* falloff_types[][2] = {
			{ "falloff-none", "None" },
			{ "falloff-linear", "Linear" },
			{ "falloff-quadratic", "Quadratic" },
			{ "falloff-cubic", "Cubic" },
			{ "falloff-exponential", "Exponential" },
			};

	gint32 falloff_type_ids[]={
		NONE,
		LINEAR,
		QUADRATIC,
		CUBIC,
		EXPONENTIAL,
	};


	GtkIconTheme *icon_theme;
	//GdkPixbuf *pixbuf;
	icon_theme = gtk_icon_theme_get_default ();

	gint icon_size;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, 0, &icon_size);

	//gtk_icon_theme_prepend_search_path(icon_theme, "../res/");

	for (guint32 i=0;i<sizeof(falloff_type_ids)/sizeof(gint32);++i){
		GError *error = NULL;

		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, falloff_types[i][0], icon_size, GtkIconLookupFlags(0), &error);

		if (error) g_error_free (error);

		//GdkPixbuf* img=gdk_pixbuf_new_from_file_at_size(filter_types[i][0],24,24,&gerror);

		gtk_list_store_append(store, &iter1);
		gtk_list_store_set(store, &iter1,
			0, pixbuf,
			1, falloff_types[i][1],
			2, falloff_type_ids[i],
		-1);

		if (pixbuf) g_object_unref (pixbuf);
	}



	return widget;
}

string build_config_path(const gchar *filename){
	stringstream s;
	s<<g_get_user_config_dir();
	if (filename){
		s<<"/gpick/";
		s<<filename;
	}else{
		s<<"/gpick";
	}
	return s.str();
}


gchar* get_data_dir(){
	static gchar* data_dir=NULL;
	if (data_dir) return data_dir;

	GList *paths=NULL, *i=NULL;
	gchar *tmp;

	i=g_list_append(i, (gchar*)g_strdup("share"));
	paths=i;
	
	i=g_list_append(i, (gchar*)g_get_user_data_dir());
	
	const gchar* const *datadirs = g_get_system_data_dirs();
	for (gint datadirs_i=0; datadirs[datadirs_i];++datadirs_i){
		i=g_list_append(i, (gchar*)datadirs[datadirs_i]);
	}

	i=paths;
	struct stat sb;
	while (i){
		tmp = g_build_filename((gchar*)i->data, "gpick", NULL);
		//cout<<tmp<<endl;
		if (g_stat( tmp, &sb )==0){
			data_dir=g_strdup(tmp);
			g_free(tmp);
			break;
		}
		g_free(tmp);
		i=g_list_next(i);
	}

	g_list_free(paths);

	if (data_dir==NULL){
		data_dir=g_strdup("");
		return data_dir;
	}

	return data_dir;
}

gchar* build_filename(const gchar* filename){
	return g_build_filename(get_data_dir(), filename, NULL);
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
	palette_list_add_entry(((MainWindow*)color_list->userdata)->color_list, color_object);
	return 0;
}

int color_list_on_delete_selected(struct ColorList* color_list){
	palette_list_remove_selected_entries(((MainWindow*)color_list->userdata)->color_list);
	return 0;
}

int color_list_on_clear(struct ColorList* color_list){
	palette_list_remove_all_entries(((MainWindow*)color_list->userdata)->color_list);
	return 0;
}

gint32 callback_color_list_on_get_positions(struct ColorObject* color_object, void *userdata){
	color_object->position=*((unsigned long*)userdata);
	(*((unsigned long*)userdata))++;
	return 0;
}

int color_list_on_get_positions(struct ColorList* color_list){
	unsigned long item=0;
	palette_list_foreach(((MainWindow*)color_list->userdata)->color_list, callback_color_list_on_get_positions, &item );
	return 0;
}

int
main(int argc, char **argv)
{
	gtk_set_locale ();
	gtk_init (&argc, &argv);
	g_set_application_name(program_name);

	GtkIconTheme *icon_theme;
	icon_theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_prepend_search_path(icon_theme, get_data_dir());

	GError *error = NULL;
	GOptionContext *context;
	context = g_option_context_new ("- advanced color picker");
	g_option_context_add_main_entries (context, commandline_entries, 0);
	
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (context, &argc, &argv, &error)){
		g_print ("option parsing failed: %s\n", error->message);
		return -1;
	}

	MainWindow* window=new MainWindow;

	window->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	window->settings = g_key_file_new();
	string config_file = build_config_path("settings");
	if (!(g_key_file_load_from_file(window->settings, config_file.c_str(), G_KEY_FILE_KEEP_COMMENTS, 0))){
		string config_path = build_config_path(NULL);
		g_mkdir(config_path.c_str(), S_IRWXU);
	}
	
	set_main_window_icon();


	window->sampler = sampler_new();
	window->cnames = color_names_new();
	window->random = random_new("SHR3");
	gulong seed_value=time(0)|1;
	random_seed(window->random, &seed_value);
	
	struct dynvHandlerMap* handler_map=dynv_handler_map_create();
	
	dynv_handler_map_add_handler(handler_map, dynv_var_string_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_int32_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_color_new());
	
	window->params = dynv_system_create(handler_map);
	window->colors = color_list_new(handler_map);
	window->colors->on_insert = color_list_on_insert;
	window->colors->on_clear = color_list_on_clear;
	window->colors->on_delete_selected = color_list_on_delete_selected;
	window->colors->on_get_positions = color_list_on_get_positions;
	window->colors->userdata = window;
	dynv_handler_map_release(handler_map);
	
	gchar* tmp;

	color_names_load_from_file(window->cnames, tmp=build_filename("colors.txt"));
	g_free(tmp);
	color_names_load_from_file(window->cnames, tmp=build_filename("colors0.txt"));
	g_free(tmp);
	
	{
		lua_State *L= luaL_newstate();
		luaL_openlibs(L);

		gchar* tmp;
		int status;
		
		lua_ext_colors_openlib(L);
		
		gchar* lua_path=build_filename("?.lua");
		lua_pushstring(L, "package");
		lua_gettable(L, LUA_GLOBALSINDEX);
		lua_pushstring(L, "path");
		lua_pushstring(L, lua_path);
		lua_settable(L, -3);
		g_free(lua_path);
		
		tmp=build_filename("init.lua");
		status = luaL_loadfile(L, tmp) || lua_pcall(L, 0, 0, 0);
		if (status) {
			cerr<<"init script load failed: "<<lua_tostring (L, -1)<<endl;
		}
		g_free(tmp);
		
		window->lua = L;
	}
		
	{
		gchar** source_array;
		gsize source_array_size;
		if ((source_array = g_key_file_get_string_list(window->settings, "Converter", "Names", &source_array_size, 0))){
			g_strfreev(source_array);	
		}else{
			const char* name_array[]={
				"color_web_hex",
				"color_css_rgb",
				"color_css_hsl",
			};
			g_key_file_set_string_list(window->settings, "Converter", "Names", name_array, sizeof(name_array)/sizeof(name_array[0]));
		}
	}

	updateProgramName(window);

    g_signal_connect (G_OBJECT (window->window), "delete_event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (window->window), "destroy",      G_CALLBACK (destroy), window);

    GtkAccelGroup *accel_group=gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window->window), accel_group);

    //gtk_accel_group_connect(accel_group, GDK_s, GdkModifierType(GDK_CONTROL_MASK), GtkAccelFlags(GTK_ACCEL_VISIBLE), g_cclosure_new (G_CALLBACK (menu_file_save),window,NULL));

    GtkWidget *widget,*expander,*table,*vbox,*hbox,*statusbar,*notebook,*frame;
    int table_y;

    GtkWidget* vbox_main = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER(window->window), vbox_main);

		GtkWidget* menu_bar;
		menu_bar = gtk_menu_bar_new ();
		gtk_box_pack_start (GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);
		

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start (GTK_BOX(vbox_main), hbox, TRUE, TRUE, 5);

		vbox = gtk_vbox_new(FALSE, 5);
		gtk_box_pack_start (GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

			frame = gtk_frame_new("Swatch");
			gtk_box_pack_start (GTK_BOX(vbox), frame, FALSE, FALSE, 0);

				widget = gtk_swatch_new();
				gtk_container_add (GTK_CONTAINER(frame), widget);


				g_signal_connect (G_OBJECT (widget), "focus-in-event", G_CALLBACK (on_swatch_focus_change), window);
				g_signal_connect (G_OBJECT (widget), "focus-out-event", G_CALLBACK (on_swatch_focus_change), window);

				g_signal_connect (G_OBJECT (widget), "key_press_event", G_CALLBACK (on_key_up), window);
				g_signal_connect (G_OBJECT (widget), "popup-menu",     G_CALLBACK (on_popup_menu), window);
				g_signal_connect (G_OBJECT (widget), "active_color_changed", G_CALLBACK (on_swatch_active_color_changed), window);
				g_signal_connect (G_OBJECT (widget), "color_changed", G_CALLBACK (on_swatch_color_changed), window);
				g_signal_connect (G_OBJECT (widget), "color_activated", G_CALLBACK (on_swatch_color_activated), window);
				g_signal_connect_after (G_OBJECT (widget), "button-press-event",G_CALLBACK (on_swatch_button_press), window);
				gtk_swatch_set_active_index(GTK_SWATCH(widget), g_key_file_get_integer_with_default(window->settings, "Swatch", "Active Color", 1));
				window->swatch_display = widget;

				{
					gsize size;
					gdouble* color_list=g_key_file_get_double_list(window->settings, "Swatch", "Colors", &size, 0);
					if (color_list){
						Color c;
						for (gsize i=0; i<size; i+=3){
							c.rgb.red=color_list[i+0];
							c.rgb.green=color_list[i+1];
							c.rgb.blue=color_list[i+2];

							gtk_swatch_set_color(GTK_SWATCH(window->swatch_display), i/3+1, &c);
						}
						g_free(color_list);
					}
				}

			frame = gtk_frame_new("Zoomed area");
			gtk_box_pack_start (GTK_BOX(vbox), frame, FALSE, FALSE, 0);

				widget = gtk_zoomed_new();
				gtk_container_add (GTK_CONTAINER(frame), widget);
				//gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
				//g_signal_connect (G_OBJECT (widget), "active_color_changed", G_CALLBACK (on_active_color_changed), window);
				window->zoomed_display = widget;

	//window->expanderMain=gtk_expander_new("");
	//gtk_expander_set_expanded(GTK_EXPANDER(window->expanderMain), g_key_file_get_boolean_with_default(window->settings, "Expander", "Main", FALSE));
	//gtk_box_pack_start (GTK_BOX(hbox), window->expanderMain, TRUE, TRUE, 5);
				
	notebook = gtk_notebook_new();
		
	//gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
				
	//gtk_container_add(GTK_CONTAINER(window->expanderMain), notebook);
	gtk_box_pack_start (GTK_BOX(hbox), notebook, TRUE, TRUE, 5);

    vbox = gtk_vbox_new(FALSE, 5);
    //gtk_box_pack_start (GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,gtk_label_new("Information"));

		expander=gtk_expander_new("HSV");
		gtk_expander_set_expanded(GTK_EXPANDER(expander), g_key_file_get_boolean_with_default(window->settings, "Expander", "HSV", FALSE));
		window->expanderHSV=expander;
		gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(3, 2, FALSE);
			table_y=0;
			gtk_container_add(GTK_CONTAINER(expander), table);

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(hue);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->hue_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(saturation);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->saturation_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Value:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(value);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->value_line = widget;
				table_y++;

		expander=gtk_expander_new("RGB");
		gtk_expander_set_expanded(GTK_EXPANDER(expander), g_key_file_get_boolean_with_default(window->settings, "Expander", "RGB", FALSE));
		window->expanderRGB=expander;
		gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(3, 2, FALSE);
			table_y=0;
			gtk_container_add(GTK_CONTAINER(expander), table);


				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Red:",0,0,0,0) ,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(red);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->red_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Green:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(green);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->green_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Blue",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(blue);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->blue_line = widget;
				table_y++;


		expander=gtk_expander_new("Info");
		gtk_expander_set_expanded(GTK_EXPANDER(expander), g_key_file_get_boolean_with_default(window->settings, "Expander", "Info", FALSE));
		window->expanderInfo=expander;
		gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(3, 2, FALSE);
			table_y=0;
			gtk_container_add(GTK_CONTAINER(expander), table);

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Color name:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_entry_new();
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				gtk_editable_set_editable(GTK_EDITABLE(widget), FALSE);
				//gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
				window->color_name = widget;
				table_y++;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,gtk_label_new("Settings"));

		table = gtk_table_new(3, 2, FALSE);
		table_y=0;
		gtk_box_pack_start (GTK_BOX(vbox), table, FALSE, FALSE, 0);

			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Oversample:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = gtk_hscale_new_with_range (0,16,1);
			g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_oversample_value_changed), window);
			gtk_range_set_value(GTK_RANGE(widget), g_key_file_get_double_with_default(window->settings, "Sampler", "Oversample", 0));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Falloff:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = create_falloff_type_list();
			gtk_combo_box_set_active(GTK_COMBO_BOX(widget), g_key_file_get_integer_with_default(window->settings, "Sampler", "Falloff", NONE));
			g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (on_oversample_falloff_changed), window);
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Zoom:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = gtk_hscale_new_with_range (2,10,0.5);
			g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_zoom_value_changed), window);
			gtk_range_set_value(GTK_RANGE(widget), g_key_file_get_double_with_default(window->settings, "Zoom", "Zoom", 2));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			widget = gtk_check_button_new_with_mnemonic ("_Add to palette immediately");
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_add_to_palette_changed), window);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(window->settings, "Sampler", "Add to palette", FALSE));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			widget = gtk_check_button_new_with_mnemonic ("_Rotate swatch after sample");
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_rotate_swatch_changed), window);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(window->settings, "Sampler", "Rotate swatch after sample", FALSE));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;


	vbox = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,gtk_label_new("Palette"));

		widget = palette_list_new(window->swatch_display);
		window->color_list = widget;

		g_signal_connect (G_OBJECT (widget), "popup-menu",     G_CALLBACK (on_palette_popup_menu), window);
		g_signal_connect (G_OBJECT (widget), "button-press-event",G_CALLBACK (on_palette_button_press), window);

		GtkWidget *scrolled_window;
		scrolled_window=gtk_scrolled_window_new (0,0);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
		gtk_container_add(GTK_CONTAINER(scrolled_window),window->color_list );
		gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	window->notebook=notebook;
	gtk_widget_show_all (window->notebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->notebook), g_key_file_get_integer_with_default(window->settings, "Notebook", "Page", 0));

	statusbar=gtk_statusbar_new();
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "focus_swatch"), "Click on swatch area to begin adding colors to palette");
	gtk_box_pack_end (GTK_BOX(vbox_main), statusbar, 0, 0, 0);
	window->statusbar=statusbar;
	
	createMenu(GTK_MENU_BAR(menu_bar), window, accel_group);
	
	gtk_widget_show_all(vbox_main);	

    updateDiplays(window);
	
	if (g_key_file_get_boolean_with_default(window->settings, "View", "Minimal", FALSE)){
		gtk_widget_hide(notebook);
	}
	
	

	/*if (argc > 1){
		gtk_window_parse_geometry (GTK_WINDOW (window->window), argv[1]);
	}*/

	if (commandline_filename){
		if (palette_file_load(commandline_filename[0], window->colors)==0){
			window->current_filename=commandline_filename[0];
			updateProgramName(window);
		}
	}
	
	GdkGeometry size_hints = { -1,-1, 0, 0, 0, 0, 0, 0, 0.0, 0.0, GDK_GRAVITY_NORTH_WEST };
	gtk_window_set_geometry_hints (GTK_WINDOW(window->window), window->window, &size_hints, GdkWindowHints(GDK_HINT_MIN_SIZE));

	gtk_widget_show (window->window);

	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 66, updateMainColor, window, 0);

	gtk_main ();

	{
		ofstream f(config_file.c_str(), ios::out | ios::trunc | ios::binary);
		if (f.is_open()){
			gsize size;
			gchar* data=g_key_file_to_data(window->settings, &size, 0);

			f.write(data, size);
			g_free(data);
			f.close();
		}
	}
	
	color_list_destroy(window->colors);
	random_destroy(window->random);
	g_key_file_free(window->settings);
	color_names_destroy(window->cnames);
	sampler_destroy(window->sampler);
	dynv_system_release(window->params);
	delete window;

	return 0;
}
