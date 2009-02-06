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

#define G_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
#define GLIB_PIXBUF_DISABLE_DEPRECATED

#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>

#include "uiSwatch.h"
#include "uiColorComponent.h"
#include "uiZoomed.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "uiExport.h"

#include "Sampler.h"
#include "Color.h"
#include "MathUtil.h"
#include "ColorNames.h"

#include <iostream>
//#include <string>
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

	ColorNames* cnames;
	struct Sampler* sampler;
	GKeyFile* settings;

	gboolean add_to_palette;

}MainWindow;

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

	g_key_file_set_integer(window->settings, "Sampler", "Oversample", sampler_get_oversample(window->sampler));
	g_key_file_set_integer(window->settings, "Sampler", "Falloff", sampler_get_falloff(window->sampler));
	g_key_file_set_boolean(window->settings, "Sampler", "Add to palette", window->add_to_palette);

	g_key_file_set_double(window->settings, "Zoom", "Zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(window->zoomed_display)));

    gtk_main_quit ();
}

gint g_key_file_get_integer_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, gint default_value) {
	GError *error=NULL;
	gint r=g_key_file_get_integer(key_file, group_name, key, &error);
	if (error){
		g_error_free(error);
		r=default_value;
	}
	return r;
}

gdouble g_key_file_get_double_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, gdouble default_value) {
	GError *error=NULL;
	gdouble r=g_key_file_get_double(key_file, group_name, key, &error);
	if (error){
		g_error_free(error);
		r=default_value;
	}
	return r;
}

gboolean g_key_file_get_boolean_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean default_value) {
	GError *error=NULL;
	gboolean r=g_key_file_get_boolean(key_file, group_name, key, &error);
	if (error){
		g_error_free(error);
		r=default_value;
	}
	return r;
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
	palette_list_add_entry(window->color_list, window->cnames, &c);
}

static void on_swatch_menu_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	MainWindow* window=(MainWindow*)item;
	Color c;
	for (int i = 1; i < 7; ++i) {
		gtk_swatch_get_color(GTK_SWATCH(window->swatch_display), i, &c);
		palette_list_add_entry(window->color_list, window->cnames, &c);
	}
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
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), palette_list_create_copy_menu(&c));

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
			updateDiplays(window);
			if (window->add_to_palette){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(window->swatch_display), &c);
				palette_list_add_entry(window->color_list, window->cnames, &c);
			}
			return TRUE;
			break;

		default:
			return FALSE;
		break;
	}
	return FALSE;
}

GtkWidget*
gtk_label_aligned_new(gchar* text, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale)
{
	GtkWidget* align=gtk_alignment_new(xalign, yalign, xscale, yscale);
	GtkWidget* label = gtk_label_new(text);
	gtk_container_add(GTK_CONTAINER(align), label);
	return align;
}

static void
show_about_box(GtkWidget *widget, MainWindow* window)
{

	gchar* license = {
		#include "License.h"
	};

	gtk_show_about_dialog (GTK_WINDOW(window->window),
		"authors", program_authors,
		"copyright", "Copyrights © 2009, Albertas Vyšniauskas",
		"license", license,
		"website", "",
		"website-label", "",
		NULL
	);
	return;
}

void
createMenu(GtkMenuBar* menu_bar, MainWindow* window)
{
	GtkMenu* menu;
	GtkWidget* item;
	GtkWidget* file_item ;

    menu = GTK_MENU(gtk_menu_new ());

    item = gtk_menu_item_new_with_image ("E_xit", gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (destroy),0);


    file_item = gtk_menu_item_new_with_mnemonic ("_File");
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



static void palette_popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu) {
	gtk_widget_destroy(GTK_WIDGET(menu));
}

static void palette_popup_menu_remove_all(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	palette_list_remove_all_entries(window->color_list);
}

static void palette_popup_menu_remove_selected(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	palette_list_remove_selected_entries(window->color_list);
}

static void palette_popup_menu_export(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	show_palette_export_dialog(0, window->color_list, FALSE);
}

static void palette_popup_menu_export_selected(GtkWidget *widget, gpointer data) {
	MainWindow* window=(MainWindow*)data;
	show_palette_export_dialog(0, window->color_list, TRUE);
}

static gboolean palette_popup_menu_show(GtkWidget *widget, GdkEventButton* event, gpointer ptr) {
	GtkWidget *menu;
	GtkWidget* item ;
	gint32 button, event_time;

	MainWindow* window=(MainWindow*)ptr;

	menu = gtk_menu_new ();

	gint32 selected_count = palette_list_get_selected_count(window->color_list);
	gint32 total_count = palette_list_get_count(window->color_list);

	Color c;
	color_zero(&c);
	if (selected_count == 1) palette_list_get_selected_color(window->color_list, &c);

	item = gtk_menu_item_new_with_mnemonic ("_Copy to clipboard");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_set_sensitive(item, (selected_count == 1));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), palette_list_create_copy_menu(&c));

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

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
    gtk_widget_set_sensitive(item, 0);

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
	GtkWidget* item ;
	gint32 button, event_time;
	MainWindow* window=(MainWindow*)user_data;

	Color c;
	updateMainColor(window);
	gtk_swatch_get_main_color(GTK_SWATCH(window->swatch_display), &c);

	menu = palette_list_create_copy_menu(&c);

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
		s<<"/.gpick/";
		s<<filename;
	}else{
		s<<"/.gpick";
	}
	return s.str();
}

void set_main_window_icon() {
	GList *icons = 0;

	GtkIconTheme *icon_theme;
	//GdkPixbuf *pixbuf;
	icon_theme = gtk_icon_theme_get_default();

	gint sizes[] = { 16, 32, 48, 128 };

	for (guint32 i = 0; i < sizeof(sizes) / sizeof(gint); ++i) {
		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, "gpick-icon", sizes[i], GtkIconLookupFlags(0), 0);
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


int
main(int argc, char **argv)
{
	gtk_set_locale ();
	gtk_init (&argc, &argv);
	g_set_application_name(program_name);

	GtkIconTheme *icon_theme;
	icon_theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_prepend_search_path(icon_theme, "../res/");

	//printf("%s %s %s\n", g_get_home_dir(), g_get_prgname(), g_get_user_config_dir() );

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
	color_names_load_from_file(window->cnames, "../res/colors.txt");
	color_names_load_from_file(window->cnames, "../res/colors0.txt");

	gtk_window_set_title(GTK_WINDOW(window->window), program_name);

	//gtk_window_set_keep_above (GTK_WINDOW (window->window), TRUE);
	//gtk_window_set_resizable(GTK_WINDOW (window->window), FALSE);

	//gtk_window_set_policy(GTK_WINDOW(window), FALSE, FALSE, TRUE);

    g_signal_connect (G_OBJECT (window->window), "delete_event", G_CALLBACK (delete_event), NULL);
    g_signal_connect (G_OBJECT (window->window), "destroy",      G_CALLBACK (destroy), window);
    g_signal_connect (G_OBJECT (window->window), "key_press_event", G_CALLBACK (on_key_up), window);
    g_signal_connect (G_OBJECT (window->window), "popup-menu",     G_CALLBACK (on_popup_menu), window);

    GtkWidget *widget,*expander,*table,*vbox,*hbox,*statusbar,*notebook,*frame;
    int table_y;

    GtkWidget* vbox_main = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER(window->window), vbox_main);

		GtkWidget* menu_bar;
		menu_bar = gtk_menu_bar_new ();
		gtk_box_pack_start (GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);
		createMenu(GTK_MENU_BAR(menu_bar), window);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start (GTK_BOX(vbox_main), hbox, TRUE, TRUE, 5);

		vbox = gtk_vbox_new(FALSE, 5);
		gtk_box_pack_start (GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

			frame = gtk_frame_new("Swatch");
			gtk_box_pack_start (GTK_BOX(vbox), frame, FALSE, FALSE, 0);

				widget = gtk_swatch_new();
				gtk_container_add (GTK_CONTAINER(frame), widget);
				//gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
				g_signal_connect (G_OBJECT (widget), "active_color_changed", G_CALLBACK (on_swatch_active_color_changed), window);
				g_signal_connect (G_OBJECT (widget), "color_changed", G_CALLBACK (on_swatch_color_changed), window);
				g_signal_connect_after (G_OBJECT (widget), "button-press-event",G_CALLBACK (on_swatch_button_press), window);
				gtk_swatch_set_active_index(GTK_SWATCH(widget), g_key_file_get_integer_with_default(window->settings, "Swatch", "Active Color", 1));
				window->swatch_display = widget;

			frame = gtk_frame_new("Zoomed area");
			gtk_box_pack_start (GTK_BOX(vbox), frame, FALSE, FALSE, 0);

				widget = gtk_zoomed_new();
				gtk_container_add (GTK_CONTAINER(frame), widget);
				//gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
				//g_signal_connect (G_OBJECT (widget), "active_color_changed", G_CALLBACK (on_active_color_changed), window);
				window->zoomed_display = widget;

	notebook = gtk_notebook_new();
	//gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

	gtk_box_pack_start (GTK_BOX(hbox), notebook, TRUE, TRUE, 5);

    vbox = gtk_vbox_new(FALSE, 5);
    //gtk_box_pack_start (GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,gtk_label_new("Information"));

		expander=gtk_expander_new("HSV");
		gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(3, 2, FALSE);
			table_y=0;
			gtk_container_add(GTK_CONTAINER(expander), table);

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(hue);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->hue_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(saturation);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->saturation_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Value:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(value);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->value_line = widget;
				table_y++;

		expander=gtk_expander_new("RGB");
		gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(3, 2, FALSE);
			table_y=0;
			gtk_container_add(GTK_CONTAINER(expander), table);


				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Red:",0,0,0,0) ,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(red);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->red_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Green:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(green);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->green_line = widget;
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Blue",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_color_component_new(blue);
				g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), window);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				window->blue_line = widget;
				table_y++;


		expander=gtk_expander_new("Info");
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

			//gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Add to palette:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			widget = gtk_check_button_new_with_mnemonic ("_Add to palette immediately");
			g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_add_to_palette_changed), window);
			//gtk_range_set_value(GTK_RANGE(widget), g_key_file_get_double_with_default(window->settings, "Zoom", "Zoom", 2));
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(window->settings, "Sampler", "Add to palette", FALSE));
			gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			table_y++;

			/*widget = gtk_entry_new();
			gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
			window->text_rgb = widget;

			widget = gtk_entry_new();
			gtk_box_pack_start (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
			window->text_hsv = widget;*/


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




	statusbar=gtk_statusbar_new();
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "press_space"), "Press SPACE to sample current color");
	gtk_box_pack_end (GTK_BOX(vbox_main), statusbar, 0, 0, 0);




    /*widget = gtk_color_selection_new();
    gtk_color_selection_set_has_opacity_control (GTK_COLOR_SELECTION(widget), FALSE);
    gtk_box_pack_end (GTK_BOX(vbox), widget, FALSE, FALSE, 0);
    window->color_widget = widget;*/



    updateDiplays(window);

	gtk_widget_show_all (window->window);

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


	delete window;

	return 0;
}
