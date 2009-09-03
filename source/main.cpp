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
#include "unique/Unique.h"

#include "gtk/Swatch.h"
#include "gtk/Zoomed.h"
#include "gtk/ColorComponent.h"
#include "gtk/ColorWidget.h"

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


const gchar* program_name = "gpick";
const gchar* program_authors[] = {"Albertas Vyšniauskas <thezbyg@gmail.com>", NULL };


typedef struct MainWindow{
	GtkWidget *window;

	GtkWidget *swatch_display;
	GtkWidget *zoomed_display;
	GtkWidget *color_list;
	GtkWidget *color_code;
	
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

	uiStatusIcon* statusIcon;

	GlobalState* gs;

	gboolean add_to_palette;
	gboolean rotate_swatch;
	gboolean copy_to_clipboard;

	char* current_filename;

}MainWindow;


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
	MainWindow* window=(MainWindow*)data;

	if (g_key_file_get_boolean_with_default(window->gs->settings, "Window", "Close to tray", false)){
		gtk_widget_hide(window->window);
		status_icon_set_visible(window->statusIcon, true);
		return TRUE;
	}

	return FALSE;
}

gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer data){
	MainWindow* window=(MainWindow*)data;

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
	MainWindow* window=(MainWindow*)data;

	if (GTK_WIDGET_VISIBLE(widget)){
	
		if (gdk_window_get_state(widget->window) & GDK_WINDOW_STATE_MAXIMIZED) {
			return FALSE;
		}
		
		gint x, y;
		gtk_window_get_position(GTK_WINDOW(widget), &x, &y);

		g_key_file_set_integer(window->gs->settings, "Window", "X", x);
		g_key_file_set_integer(window->gs->settings, "Window", "Y", y);

		if (GTK_WIDGET_VISIBLE(window->notebook)){
			g_key_file_set_integer(window->gs->settings, "Window", "Width", event->width);
			g_key_file_set_integer(window->gs->settings, "Window", "Height", event->height);
		}
	
	
	}else return FALSE;
	
	return FALSE;
}

static void
destroy( GtkWidget *widget, gpointer data )
{
	MainWindow* window=(MainWindow*)data;

	g_key_file_set_integer(window->gs->settings, "Swatch", "Active Color", gtk_swatch_get_active_index(GTK_SWATCH(window->swatch_display)));
	{
	gdouble color_list[3*6];
	for (gint i=0; i<6; ++i){
		Color c;
		gtk_swatch_get_color(GTK_SWATCH(window->swatch_display), i+1, &c);
		color_list[0+3*i]=c.rgb.red;
		color_list[1+3*i]=c.rgb.green;
		color_list[2+3*i]=c.rgb.blue;
	}
	g_key_file_set_double_list(window->gs->settings, "Swatch", "Colors", color_list, sizeof(color_list)/sizeof(gdouble));
	}

	g_key_file_set_integer(window->gs->settings, "Sampler", "Oversample", sampler_get_oversample(window->gs->sampler));
	g_key_file_set_integer(window->gs->settings, "Sampler", "Falloff", sampler_get_falloff(window->gs->sampler));
	g_key_file_set_boolean(window->gs->settings, "Sampler", "Add to palette", window->add_to_palette);
	g_key_file_set_boolean(window->gs->settings, "Sampler", "Copy to clipboard", window->copy_to_clipboard);
	g_key_file_set_boolean(window->gs->settings, "Sampler", "Rotate swatch after sample", window->rotate_swatch);

	g_key_file_set_double(window->gs->settings, "Zoom", "Zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(window->zoomed_display)));

	g_key_file_set_integer(window->gs->settings, "Notebook", "Page", gtk_notebook_get_current_page(GTK_NOTEBOOK(window->notebook)));

	g_key_file_set_boolean(window->gs->settings, "Expander", "RGB", gtk_expander_get_expanded(GTK_EXPANDER(window->expanderRGB)));
	g_key_file_set_boolean(window->gs->settings, "Expander", "Info", gtk_expander_get_expanded(GTK_EXPANDER(window->expanderInfo)));
	g_key_file_set_boolean(window->gs->settings, "Expander", "HSV", gtk_expander_get_expanded(GTK_EXPANDER(window->expanderHSV)));
	
    gtk_main_quit ();
}






static gboolean updateMainColor( gpointer data ){
	MainWindow* window=(MainWindow*)data;

	Color c;
	sampler_get_color_sample(window->gs->sampler, &c);
	
	
	gchar* text = 0;
	struct ColorObject* color_object;
	color_object = color_list_new_color_object(window->gs->colors, &c);
	
	gchar** source_array;
	gsize source_array_size;
	if ((source_array = g_key_file_get_string_list(window->gs->settings, "Converter", "Names", &source_array_size, 0))){
		if (source_array_size>0){	
			converter_get_text(source_array[0], color_object, 0, window->gs->lua, &text);
		}					
		g_strfreev(source_array);
	}
	color_object_release(color_object);
	
	gtk_color_set_color(GTK_COLOR(window->color_code), &c, text);
	if (text) g_free(text);
	
	gtk_swatch_set_main_color(GTK_SWATCH(window->swatch_display), &c);	gtk_zoomed_update(GTK_ZOOMED(window->zoomed_display));
	
	return TRUE;
}

static gboolean updateMainColorTimer( gpointer data ){
	MainWindow* window=(MainWindow*)data;
	if (gtk_window_is_active(GTK_WINDOW(window->window))){
		updateMainColor(window);
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

	string color_name = color_names_get(window->gs->color_names, &c);
	gtk_entry_set_text(GTK_ENTRY(window->color_name), color_name.c_str());

}

void updateProgramName(MainWindow* window){
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
	//palette_list_add_entry(window->color_list, window->gs->color_names, &c);
	//color_list_add_color(window->gs->colors, &c);
				
	struct ColorObject *color_object=color_list_new_color_object(window->gs->colors, &c);
	string name=color_names_get(window->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(window->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_swatch_menu_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	MainWindow* window=(MainWindow*)item;
	Color c;
	for (int i = 1; i < 7; ++i) {
		gtk_swatch_get_color(GTK_SWATCH(window->swatch_display), i, &c);
		//palette_list_add_entry(window->color_list, window->gs->color_names, &c);
		//color_list_add_color(window->gs->colors, &c);
		
		struct ColorObject *color_object=color_list_new_color_object(window->gs->colors, &c);
		string name=color_names_get(window->gs->color_names, &c);
		dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
		color_list_add_color_object(window->gs->colors, color_object, 1);
		color_object_release(color_object);
	}
}
static void
on_swatch_color_activated (GtkWidget *widget, gpointer item) {
	MainWindow* window=(MainWindow*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(widget), &c);
	//palette_list_add_entry(window->color_list, window->gs->color_names, &c);
	//color_list_add_color(window->gs->colors, &c);
	
	struct ColorObject *color_object=color_list_new_color_object(window->gs->colors, &c);
	string name=color_names_get(window->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(window->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static gboolean
on_swatch_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}
	
	MainWindow* window=(MainWindow*)data;

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
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
	    color_object = color_list_new_color_object(window->gs->colors, &c);
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (color_object, 0, window->gs->settings, window->gs->lua));
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
		case GDK_c:
			if ((event->state&(GDK_CONTROL_MASK|GDK_SHIFT_MASK|GDK_MOD1_MASK))==GDK_CONTROL_MASK){
								
				Color c;
				updateMainColor(window);
				gtk_swatch_get_main_color(GTK_SWATCH(window->swatch_display), &c);

				struct ColorObject* color_object;
				color_object = color_list_new_color_object(window->gs->colors, &c);
				
				gchar** source_array;
				gsize source_array_size;
				if ((source_array = g_key_file_get_string_list(window->gs->settings, "Converter", "Names", &source_array_size, 0))){
					if (source_array_size>0){	
						converter_get_clipboard(source_array[0], color_object, 0, window->gs->lua);
					}					
					g_strfreev(source_array);
				}
				color_object_release(color_object);
				return TRUE;
			}
			return FALSE;
			break;
		
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
				
				struct ColorObject *color_object=color_list_new_color_object(window->gs->colors, &c);
				string name=color_names_get(window->gs->color_names, &c);
				dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
				color_list_add_color_object(window->gs->colors, color_object, 1);
				color_object_release(color_object);
			}
			
			if (window->copy_to_clipboard){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(window->swatch_display), &c);
				
				struct ColorObject* color_object;
				color_object = color_list_new_color_object(window->gs->colors, &c);
				
				gchar** source_array;
				gsize source_array_size;
				if ((source_array = g_key_file_get_string_list(window->gs->settings, "Converter", "Names", &source_array_size, 0))){
					if (source_array_size>0){	
						converter_get_clipboard(source_array[0], color_object, 0, window->gs->lua);
					}					
					g_strfreev(source_array);
				}
				color_object_release(color_object);
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

int main_pick_color(GlobalState* gs, GdkEventKey *event){
	MainWindow* window=(MainWindow*)dynv_system_get(gs->params, "ptr", "MainWindowStruct");
	return on_key_up(0, event, window);
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
		"logo-icon-name", "gpick",
		NULL
	);

	g_free(version);
	return;
}

static void
show_dialog_converter(GtkWidget *widget, MainWindow* window){
	dialog_converter_show(GTK_WINDOW(window->window), window->gs->settings, window->gs->lua, window->gs->colors);
	return;
}

static void
show_dialog_options(GtkWidget *widget, MainWindow* window){
	dialog_options_show(GTK_WINDOW(window->window), window->gs->settings);
	return;
}


static void
menu_file_new(GtkWidget *widget, MainWindow* window){
	if (window->current_filename) g_free(window->current_filename);
	window->current_filename=0;
	palette_list_remove_all_entries(window->color_list);
	updateProgramName(window);
}

static void
menu_file_open(GtkWidget *widget, MainWindow* window){

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
menu_file_save_as(GtkWidget *widget, MainWindow* window){

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

static void menu_file_save(GtkWidget *widget, MainWindow *window) {
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
	MainWindow* window=(MainWindow*)data;
	dialog_export_show(GTK_WINDOW(window->window), window->gs->colors, 0, window->gs->settings, FALSE);
}

static void menu_file_import(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	dialog_import_show(GTK_WINDOW(window->window), window->gs->colors, 0, window->gs->settings);
}

static void menu_file_export(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_export_show(GTK_WINDOW(window->window), window->gs->colors, color_list, window->gs->settings, TRUE);
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
		g_key_file_set_boolean(window->gs->settings, "View", "Minimal", TRUE);
		
		gtk_window_resize(GTK_WINDOW(window->window), 1, 1);		//shrink to min size
		
	}else{
		gtk_window_resize(GTK_WINDOW(window->window), g_key_file_get_integer_with_default(window->gs->settings, "Window", "Width", 1), g_key_file_get_integer_with_default(window->gs->settings, "Window", "Height", 1));
		
		gtk_widget_show(window->notebook);
		g_key_file_set_boolean(window->gs->settings, "View", "Minimal", FALSE);
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

    item = gtk_check_menu_item_new_with_mnemonic ("Minimal");
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (menu_view_minimal), window);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), g_key_file_get_boolean_with_default(window->gs->settings, "View", "Minimal", FALSE));
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
	sampler_set_oversample(window->gs->sampler, (int)gtk_range_get_value(GTK_RANGE(slider)));
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
	color_list_remove_all(window->gs->colors);
}

static void palette_popup_menu_remove_selected(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	palette_list_foreach_selected(window->color_list, color_list_mark_selected, 0);
	color_list_remove_selected(window->gs->colors);
}



gint32 palette_popup_menu_mix_list(Color* color, void *userdata){
	*((GList**)userdata) = g_list_append(*((GList**)userdata), color);
	return 0;
}

static void palette_popup_menu_mix(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;	
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_mix_show(GTK_WINDOW(window->window), window->gs->colors, color_list, window->gs->settings);
	color_list_destroy(color_list);
}



static void palette_popup_menu_variations(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	
	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(window->gs->params);
	
	struct ColorList *color_list = color_list_new(handler_map);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_variations_show(GTK_WINDOW(window->window), window->gs->colors, color_list, window->gs->settings);
	color_list_destroy(color_list);
	
	dynv_handler_map_release(handler_map);
}

static void palette_popup_menu_generate(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;	
	struct ColorList *color_list = color_list_new(NULL);
	palette_list_foreach_selected(window->color_list, color_list_selected, color_list);
	dialog_generate_show(GTK_WINDOW(window->window), window->gs->colors, color_list, window->gs->settings, window->gs->random);
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

	MainWindow* window=(MainWindow*)ptr;
	
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
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (*color_list->colors.begin(), window->color_list, window->gs->settings, window->gs->lua));
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
	MainWindow* window=(MainWindow*)data;

	switch(event->keyval)
	{
		case GDK_c:
			if ((event->state&(GDK_CONTROL_MASK|GDK_SHIFT_MASK|GDK_MOD1_MASK))==GDK_CONTROL_MASK){
				
				gchar** source_array;
				gsize source_array_size;
				if ((source_array = g_key_file_get_string_list(window->gs->settings, "Converter", "Names", &source_array_size, 0))){
					if (source_array_size>0){					
						converter_get_clipboard(source_array[0], 0, window->color_list, window->gs->lua);
					}					
					g_strfreev(source_array);
				}
				return TRUE;
			}
			return FALSE;
			break;
			
		default:
			return FALSE;
		break;
	}
	return FALSE;
}

static void on_popup_menu(GtkWidget *widget, gpointer user_data) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}
	
	//GtkWidget* item ;
	gint32 button, event_time;
	MainWindow* window=(MainWindow*)user_data;

	Color c;
	updateMainColor(window);
	gtk_swatch_get_main_color(GTK_SWATCH(window->swatch_display), &c);

    struct ColorObject* color_object;
    color_object = color_list_new_color_object(window->gs->colors, &c);
    menu = converter_create_copy_menu (color_object, 0, window->gs->settings, window->gs->lua);
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
		sampler_set_falloff(window->gs->sampler, (enum SamplerFalloff) falloff_id);

	}
}


void on_add_to_palette_changed(GtkWidget *widget, gpointer data) {
	((MainWindow*)data)->add_to_palette = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

void on_copy_to_clipboard_changed(GtkWidget *widget, gpointer data) {
	((MainWindow*)data)->copy_to_clipboard = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
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
			{ "gpick-falloff-none", "None" },
			{ "gpick-falloff-linear", "Linear" },
			{ "gpick-falloff-quadratic", "Quadratic" },
			{ "gpick-falloff-cubic", "Cubic" },
			{ "gpick-falloff-exponential", "Exponential" },
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

int unique_show_window(void* data){
	MainWindow* window=(MainWindow*)data;
	
	status_icon_set_visible (window->statusIcon, false);
	main_show_window(window->window, window->gs->settings);

	return 0;
}

int main(int argc, char **argv){
	MainWindow* window=new MainWindow;
	
	gtk_set_locale ();
	gtk_init (&argc, &argv);
	g_set_application_name(program_name);
	
	if (unique_init(unique_show_window, window)==0){
			
	}else{
		delete window;
		return 1;
	}

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

	
	window->current_filename = 0;
	
	
	window->gs = new GlobalState;
	global_state_init(window->gs);
	window->gs->colors->on_insert = color_list_on_insert;
	window->gs->colors->on_clear = color_list_on_clear;
	window->gs->colors->on_delete_selected = color_list_on_delete_selected;
	window->gs->colors->on_get_positions = color_list_on_get_positions;
	window->gs->colors->userdata = window;
	
	dynv_system_set(window->gs->params, "ptr", "MainWindowStruct", window);

	window->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	set_main_window_icon();

	{
		gchar** source_array;
		gsize source_array_size;
		if ((source_array = g_key_file_get_string_list(window->gs->settings, "Converter", "Names", &source_array_size, 0))){
			g_strfreev(source_array);	
		}else{
			const char* name_array[]={
				"color_web_hex",
				"color_css_rgb",
				"color_css_hsl",
			};
			g_key_file_set_string_list(window->gs->settings, "Converter", "Names", name_array, sizeof(name_array)/sizeof(name_array[0]));
		}
	}

	updateProgramName(window);

	g_signal_connect (G_OBJECT (window->window), "delete_event", 		G_CALLBACK (delete_event), window);
	g_signal_connect (G_OBJECT (window->window), "destroy",      		G_CALLBACK (destroy), window);
	g_signal_connect (G_OBJECT (window->window), "configure-event",		G_CALLBACK (on_window_configure), window);
	g_signal_connect (G_OBJECT (window->window), "window-state-event",	G_CALLBACK (on_window_state_event), window);

	
    GtkAccelGroup *accel_group=gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window->window), accel_group);
	g_object_unref(G_OBJECT (accel_group));

    //gtk_accel_group_connect(accel_group, GDK_s, GdkModifierType(GDK_CONTROL_MASK), GtkAccelFlags(GTK_ACCEL_VISIBLE), g_cclosure_new (G_CALLBACK (menu_file_save),window,NULL));

    GtkWidget *widget,*expander,*table,*vbox,*hbox,*statusbar,*notebook,*frame,*vbox2;
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

				gtk_swatch_set_active_index(GTK_SWATCH(widget), g_key_file_get_integer_with_default(window->gs->settings, "Swatch", "Active Color", 1));
				window->swatch_display = widget;

				{
					gsize size;
					gdouble* color_list=g_key_file_get_double_list(window->gs->settings, "Swatch", "Colors", &size, 0);
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
			gtk_box_pack_start (GTK_BOX(vbox), frame, FALSE, FALSE, 5);

				vbox2 = gtk_vbox_new(FALSE, 0);
				gtk_container_add (GTK_CONTAINER(frame), vbox2);
				
					window->zoomed_display = gtk_zoomed_new();
					gtk_box_pack_start (GTK_BOX(vbox2), window->zoomed_display, FALSE, FALSE, 0);

					window->color_code = gtk_color_new();
					gtk_box_pack_start (GTK_BOX(vbox2), window->color_code, TRUE, TRUE, 0);	
					


	//window->expanderMain=gtk_expander_new("");
	//gtk_expander_set_expanded(GTK_EXPANDER(window->expanderMain), g_key_file_get_boolean_with_default(window->gs->settings, "Expander", "Main", FALSE));
	//gtk_box_pack_start (GTK_BOX(hbox), window->expanderMain, TRUE, TRUE, 5);
				
	notebook = gtk_notebook_new();
		
	//gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
				
	//gtk_container_add(GTK_CONTAINER(window->expanderMain), notebook);
	gtk_box_pack_start (GTK_BOX(hbox), notebook, TRUE, TRUE, 5);

    vbox = gtk_vbox_new(FALSE, 5);
    //gtk_box_pack_start (GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,gtk_label_new("Information"));

		expander=gtk_expander_new("HSV");
		gtk_expander_set_expanded(GTK_EXPANDER(expander), g_key_file_get_boolean_with_default(window->gs->settings, "Expander", "HSV", FALSE));
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
		gtk_expander_set_expanded(GTK_EXPANDER(expander), g_key_file_get_boolean_with_default(window->gs->settings, "Expander", "RGB", FALSE));
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
		gtk_expander_set_expanded(GTK_EXPANDER(expander), g_key_file_get_boolean_with_default(window->gs->settings, "Expander", "Info", FALSE));
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

		table = gtk_table_new(6, 2, FALSE);
		table_y=0;
		gtk_box_pack_start (GTK_BOX(vbox), table, FALSE, FALSE, 0);

			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Oversample:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = gtk_hscale_new_with_range (0,16,1);
			g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_oversample_value_changed), window);
			gtk_range_set_value(GTK_RANGE(widget), g_key_file_get_double_with_default(window->gs->settings, "Sampler", "Oversample", 0));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Falloff:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = create_falloff_type_list();
			gtk_combo_box_set_active(GTK_COMBO_BOX(widget), g_key_file_get_integer_with_default(window->gs->settings, "Sampler", "Falloff", NONE));
			g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (on_oversample_falloff_changed), window);
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Zoom:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = gtk_hscale_new_with_range (2,10,0.5);
			g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_zoom_value_changed), window);
			gtk_range_set_value(GTK_RANGE(widget), g_key_file_get_double_with_default(window->gs->settings, "Zoom", "Zoom", 2));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			widget = gtk_check_button_new_with_mnemonic ("_Add to palette immediately");
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_add_to_palette_changed), window);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(window->gs->settings, "Sampler", "Add to palette", FALSE));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;
			
			widget = gtk_check_button_new_with_mnemonic ("_Copy to clipboard immediately");
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_copy_to_clipboard_changed), window);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(window->gs->settings, "Sampler", "Copy to clipboard", FALSE));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			widget = gtk_check_button_new_with_mnemonic ("_Rotate swatch after sample");
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_rotate_swatch_changed), window);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(window->gs->settings, "Sampler", "Rotate swatch after sample", FALSE));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,gtk_label_new("Palette"));

		widget = palette_list_new(window->swatch_display);
		window->color_list = widget;

		g_signal_connect (G_OBJECT (widget), "popup-menu",     G_CALLBACK (on_palette_popup_menu), window);
		g_signal_connect (G_OBJECT (widget), "button-press-event",G_CALLBACK (on_palette_button_press), window);
		g_signal_connect (G_OBJECT (widget), "key_press_event", G_CALLBACK (on_palette_list_key_press), window);

		GtkWidget *scrolled_window;
		scrolled_window=gtk_scrolled_window_new (0,0);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
		gtk_container_add(GTK_CONTAINER(scrolled_window),window->color_list );
		gtk_box_pack_start (GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	window->notebook=notebook;
	gtk_widget_show_all (window->notebook);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(window->notebook), g_key_file_get_integer_with_default(window->gs->settings, "Notebook", "Page", 0));

	statusbar=gtk_statusbar_new();
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "focus_swatch"), "Click on swatch area to begin adding colors to palette");
	gtk_box_pack_end (GTK_BOX(vbox_main), statusbar, 0, 0, 0);
	window->statusbar=statusbar;
	
	createMenu(GTK_MENU_BAR(menu_bar), window, accel_group);
	
	gtk_widget_show_all(vbox_main);	

    updateDiplays(window);
	
	if (g_key_file_get_boolean_with_default(window->gs->settings, "View", "Minimal", false)){
		gtk_widget_hide(notebook);
	}
	
	if (commandline_filename){
		if (palette_file_load(commandline_filename[0], window->gs->colors)==0){
			window->current_filename=g_strdup(commandline_filename[0]);
			updateProgramName(window);
		}
	}
	
	/*if (g_key_file_get_boolean_with_default(window->gs->settings, "View", "Minimal", false)==false){
		gtk_window_resize(GTK_WINDOW(window->window), g_key_file_get_integer_with_default(window->gs->settings, "Window", "Width", 1), g_key_file_get_integer_with_default(window->gs->settings, "Window", "Height", 1));
	}*/
	
	//GdkGeometry size_hints = { -1,-1, 0, 0, 0, 0, 0, 0, 0.0, 0.0, GDK_GRAVITY_CENTER };
	//gtk_window_set_geometry_hints (GTK_WINDOW(window->window), window->window, &size_hints, GdkWindowHints(GDK_HINT_WIN_GRAVITY));
	
	if (commandline_geometry){
		gtk_window_parse_geometry (GTK_WINDOW (window->window), (const gchar*)commandline_geometry);
	}

	window->statusIcon = status_icon_new(window->window, window->gs);
	
	gtk_widget_realize(window->window);
	
	if (g_key_file_get_boolean_with_default(window->gs->settings, "Window", "Start in tray", false)){
		status_icon_set_visible (window->statusIcon, true);
	}else{
		main_show_window(window->window, window->gs->settings);
	}
	
	
	//gtk_status_icon_set_visible (statusIcon, TRUE);

	g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 66, updateMainColorTimer, window, 0);

	

	gtk_main ();
	
	unique_term();

	status_icon_destroy(window->statusIcon);
	
	global_state_term(window->gs);
	delete window->gs;
	
	if (window->current_filename) g_free(window->current_filename);
	
	delete window;
	
	g_option_context_free(context);
	
	return 0;
}
