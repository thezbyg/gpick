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

#include "GlobalStateStruct.h"
#include "ColorPicker.h"
#include "DragDrop.h"
#include "Converter.h"
#include "DynvHelpers.h"
#include "CopyPaste.h"
#include "FloatingPicker.h"

#include "gtk/Swatch.h"
#include "gtk/Zoomed.h"
#include "gtk/ColorComponent.h"
#include "gtk/ColorWidget.h"

#include "uiApp.h"
#include "uiUtilities.h"
#include "uiColorInput.h"

#include <gdk/gdkkeysyms.h>

#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;
using namespace math;

struct Arguments{
	ColorSource source;

	GtkWidget* main;

	GtkWidget* expanderRGB;
	GtkWidget* expanderHSV;
	GtkWidget* expanderInfo;
	GtkWidget* expanderMain;
	GtkWidget* expanderSettings;

	GtkWidget *swatch_display;
	GtkWidget *zoomed_display;
	GtkWidget *color_code;

	GtkWidget* hue_line;
	GtkWidget* saturation_line;
	GtkWidget* value_line;

	GtkWidget* red_line;
	GtkWidget* green_line;
	GtkWidget* blue_line;

	GtkWidget* color_name;
	GtkWidget* statusbar;

	gboolean add_to_palette;
	gboolean rotate_swatch;
	gboolean copy_to_clipboard;
	//gboolean active;
	guint timeout_source_id;
	
	FloatingPicker floating_picker;

	struct dynvSystem *params;
	GlobalState* gs;

};

static int source_set_color(struct Arguments *args, ColorObject* color);

static void popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu){
	gtk_widget_destroy(GTK_WIDGET(menu));
}

static gboolean updateMainColor( gpointer data ){
	struct Arguments* args=(struct Arguments*)data;

	GdkScreen *screen;
	GdkModifierType state;
	
	int x, y;
	int width, height;
	
	gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, &state);
	width = gdk_screen_get_width(screen);
	height = gdk_screen_get_height(screen);

	Vec2<int> pointer(x,y);
	Vec2<int> window_size(width, height);

	screen_reader_reset_rect(args->gs->screen_reader);
	Rect2<int> sampler_rect, zoomed_rect, final_rect;

	sampler_get_screen_rect(args->gs->sampler, pointer, window_size, &sampler_rect);
	screen_reader_add_rect(args->gs->screen_reader, screen, sampler_rect);

	gtk_zoomed_get_screen_rect(GTK_ZOOMED(args->zoomed_display), pointer, window_size, &zoomed_rect);
	screen_reader_add_rect(args->gs->screen_reader, screen, zoomed_rect);

	screen_reader_update_pixbuf(args->gs->screen_reader, &final_rect);

	Vec2<int> offset;

	offset = Vec2<int>(sampler_rect.getX()-final_rect.getX(), sampler_rect.getY()-final_rect.getY());
	Color c;
	sampler_get_color_sample(args->gs->sampler, pointer, window_size, offset, &c);

	gchar* text = main_get_color_text(args->gs, &c, COLOR_TEXT_TYPE_DISPLAY);

	gtk_color_set_color(GTK_COLOR(args->color_code), &c, text);
	if (text) g_free(text);

	gtk_swatch_set_main_color(GTK_SWATCH(args->swatch_display), &c);


	offset = Vec2<int>(zoomed_rect.getX()-final_rect.getX(), zoomed_rect.getY()-final_rect.getY());
	gtk_zoomed_update(GTK_ZOOMED(args->zoomed_display), pointer, window_size, offset, screen_reader_get_pixbuf(args->gs->screen_reader));

	return TRUE;
}

static gboolean updateMainColorTimer( struct Arguments* args ){
	updateMainColor(args);
}


static void updateDiplays(struct Arguments *args){
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display),&c);

	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hue_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->saturation_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->value_line), &c);

	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->red_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->green_line), &c);
	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->blue_line), &c);

	string color_name = color_names_get(args->gs->color_names, &c);
	gtk_entry_set_text(GTK_ENTRY(args->color_name), color_name.c_str());

}


static void on_swatch_active_color_changed( GtkWidget *widget, gint32 new_active_color, gpointer data ){
	struct Arguments* args=(struct Arguments*)data;
	updateDiplays(args);
}

static void on_swatch_color_changed( GtkWidget *widget, gpointer data ){
	struct Arguments* args=(struct Arguments*)data;
	updateDiplays(args);
}


static void on_swatch_menu_detach(GtkWidget *attach_widget, GtkMenu *menu){
	gtk_widget_destroy(GTK_WIDGET(menu));
}



static void on_swatch_menu_add_to_palette(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);
	//palette_list_add_entry(args->color_list, args->gs->color_names, &c);
	//color_list_add_color(args->gs->colors, &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_swatch_menu_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	Color c;
	for (int i = 1; i < 7; ++i) {
		gtk_swatch_get_color(GTK_SWATCH(args->swatch_display), i, &c);
		//palette_list_add_entry(args->color_list, args->gs->color_names, &c);
		//color_list_add_color(args->gs->colors, &c);

		struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
		string name=color_names_get(args->gs->color_names, &c);
		dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
		color_list_add_color_object(args->gs->colors, color_object, 1);
		color_object_release(color_object);
	}
}

static void on_swatch_color_activated(GtkWidget *widget, gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(widget), &c);
	//palette_list_add_entry(args->color_list, args->gs->color_names, &c);
	//color_list_add_color(args->gs->colors, &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_swatch_center_activated(GtkWidget *widget, gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	
	floating_picker_activate(args->floating_picker, true);
	
}

static void on_swatch_color_edit(GtkWidget *widget, gpointer item) {
	struct Arguments* args=(struct Arguments*)item;

	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

	struct ColorObject* color_object = color_list_new_color_object(args->gs->colors, &c);
	struct ColorObject* new_color_object = 0;

	if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->swatch_display)), args->gs, color_object, &new_color_object )==0){
		source_set_color(args, new_color_object);
		color_object_release(new_color_object);
	}

	color_object_release(color_object);
}


static void paste_cb(GtkWidget *widget, struct Arguments* args) {
	struct ColorObject* color_object;
	if (copypaste_get_color_object(&color_object, args->gs)==0){
		source_set_color(args, color_object);
		color_object_release(color_object);
	}
}

static gboolean on_swatch_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}
	
	int color_index = gtk_swatch_get_color_at(GTK_SWATCH(widget), event->x, event->y);

	struct Arguments* args=(struct Arguments*)data;

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS && color_index>0){
		GtkWidget* item ;
		gint32 button, event_time;

		menu = gtk_menu_new ();

		Color c;
		gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

	    item = gtk_menu_item_new_with_image ("_Add to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_menu_add_to_palette), args);

	    item = gtk_menu_item_new_with_image ("_Add all to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_menu_add_all_to_palette), args);

	    gtk_menu_shell_append (GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	    item = gtk_menu_item_new_with_mnemonic ("_Copy to clipboard");
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	    struct ColorObject* color_object;
	    color_object = color_list_new_color_object(args->gs->colors, &c);
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (color_object, 0, args->gs));
		color_object_release(color_object);

		gtk_menu_shell_append (GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		item = gtk_menu_item_new_with_image ("_Edit...", gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_color_edit), args);

		item = gtk_menu_item_new_with_image ("_Paste", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (paste_cb), args);

		if (copypaste_is_color_object_available(args->gs)!=0){
			gtk_widget_set_sensitive(item, false);
		}

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



static gboolean on_swatch_focus_change(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
	struct Arguments* args=(struct Arguments*)data;


	if (event->in){
		gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "swatch_focused"), "Press Spacebar to sample color under mouse pointer");
	}else{
		gtk_statusbar_pop(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "swatch_focused"));
	}
	return FALSE;
}




static gboolean on_key_up (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	struct Arguments* args=(struct Arguments*)data;

	guint modifiers = gtk_accelerator_get_default_mod_mask();

	switch(event->keyval)
	{
		case GDK_c:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){

				Color c;
				updateMainColor(args);
				gtk_swatch_get_main_color(GTK_SWATCH(args->swatch_display), &c);

				struct ColorObject* color_object;
				color_object = color_list_new_color_object(args->gs->colors, &c);

				Converters *converters = (Converters*)dynv_system_get(args->gs->params, "ptr", "Converters");
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, color_object, 0, args->gs->params);
				}

				color_object_release(color_object);
				return TRUE;
			}
			return FALSE;
			break;

		case GDK_Right:
			gtk_swatch_move_active(GTK_SWATCH(args->swatch_display),1);
			updateDiplays(args);
			return TRUE;
			break;

		case GDK_Left:
			gtk_swatch_move_active(GTK_SWATCH(args->swatch_display),-1);
			updateDiplays(args);
			return TRUE;
			break;

		case GDK_space:
			updateMainColor(args);
			gtk_swatch_set_color_to_main(GTK_SWATCH(args->swatch_display));

			if (args->add_to_palette){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

				struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
				string name=color_names_get(args->gs->color_names, &c);
				dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
				color_list_add_color_object(args->gs->colors, color_object, 1);
				color_object_release(color_object);
			}

			if (args->copy_to_clipboard){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

				struct ColorObject* color_object;
				color_object = color_list_new_color_object(args->gs->colors, &c);

				Converters *converters = (Converters*)dynv_system_get(args->gs->params, "ptr", "Converters");
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, color_object, 0, args->gs->params);
				}

				color_object_release(color_object);
			}

			if (args->rotate_swatch){
				gtk_swatch_move_active(GTK_SWATCH(args->swatch_display),1);
			}
			updateDiplays(args);
			return TRUE;
			break;

		default:
			return FALSE;
		break;
	}
	return FALSE;
}

int color_picker_key_up(ColorSource* color_source, GdkEventKey *event){
	return on_key_up(0, event, color_source);
}

static void on_oversample_value_changed(GtkRange *slider, gpointer data){
	struct Arguments* args=(struct Arguments*)data;
	sampler_set_oversample(args->gs->sampler, (int)gtk_range_get_value(GTK_RANGE(slider)));
}

static void on_zoom_value_changed(GtkRange *slider, gpointer data){
	struct Arguments* args=(struct Arguments*)data;
	gtk_zoomed_set_zoom(GTK_ZOOMED(args->zoomed_display), gtk_range_get_value(GTK_RANGE(slider)));
}



static void color_component_change_value(GtkSpinButton *spinbutton, Color* c, gpointer data){
	struct Arguments* args=(struct Arguments*)data;
	gtk_swatch_set_active_color(GTK_SWATCH(args->swatch_display),c);
	updateDiplays(args);
}



static void on_add_to_palette_changed(GtkWidget *widget, gpointer data) {
	((struct Arguments*)data)->add_to_palette = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_copy_to_clipboard_changed(GtkWidget *widget, gpointer data) {
	((struct Arguments*)data)->copy_to_clipboard = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_rotate_swatch_changed(GtkWidget *widget, gpointer data) {
	((struct Arguments*)data)->rotate_swatch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_oversample_falloff_changed(GtkWidget *widget, gpointer data) {
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

		gint32 falloff_id;
		gtk_tree_model_get(model, &iter, 2, &falloff_id, -1);

		struct Arguments* args=(struct Arguments*)data;
		sampler_set_falloff(args->gs->sampler, (enum SamplerFalloff) falloff_id);

	}
}



static void on_popup_menu(GtkWidget *widget, gpointer user_data) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}

	//GtkWidget* item ;
	gint32 button, event_time;
	struct Arguments* args=(struct Arguments*)user_data;

	Color c;
	updateMainColor(args);
	gtk_swatch_get_main_color(GTK_SWATCH(args->swatch_display), &c);

    struct ColorObject* color_object;
    color_object = color_list_new_color_object(args->gs->colors, &c);
    menu = converter_create_copy_menu (color_object, 0, args->gs);
	color_object_release(color_object);

    gtk_widget_show_all (GTK_WIDGET(menu));

	button = 0;
	event_time = gtk_get_current_event_time ();

	gtk_menu_attach_to_widget (GTK_MENU (menu), widget, popup_menu_detach);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  button, event_time);
}

static GtkWidget* create_falloff_type_list (void){
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




static int source_destroy(struct Arguments *args){

	dynv_set_int32(args->params, "swatch.active_color", gtk_swatch_get_active_index(GTK_SWATCH(args->swatch_display)));

	char tmp[32];
	for (gint i=1; i<7; ++i){
		sprintf(tmp, "swatch.color%d", i);
		Color c;
		gtk_swatch_get_color(GTK_SWATCH(args->swatch_display), i, &c);
		dynv_set_color(args->params, tmp, &c);
	}

	dynv_set_int32(args->params, "sampler.oversample", sampler_get_oversample(args->gs->sampler));
	dynv_set_int32(args->params, "sampler.falloff", sampler_get_falloff(args->gs->sampler));
	dynv_set_bool(args->params, "sampler.add_to_palette", args->add_to_palette);
	dynv_set_bool(args->params, "sampler.copy_to_clipboard", args->copy_to_clipboard);
	dynv_set_bool(args->params, "sampler.rotate_swatch_after_sample", args->rotate_swatch);

	dynv_set_float(args->params, "zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(args->zoomed_display)));

	dynv_set_bool(args->params, "expander.settings", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderSettings)));
	dynv_set_bool(args->params, "expander.rgb", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderRGB)));
	dynv_set_bool(args->params, "expander.hsv", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderHSV)));
	dynv_set_bool(args->params, "expander.info", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderInfo)));

	gtk_widget_destroy(args->main);

	dynv_system_release(args->params);
	delete args;
	return 0;
}

static int source_get_color(struct Arguments *args, ColorObject** color){

	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());

	*color = color_object;

	return 0;
}


static int source_set_color(struct Arguments *args, ColorObject* color){

	Color c;
	color_object_get_color(color, &c);
	gtk_swatch_set_active_color(GTK_SWATCH(args->swatch_display), &c);

	updateDiplays(args);
	
	return 0;
}

static int source_activate(struct Arguments *args){

	if (args->timeout_source_id > 0) {
		g_source_remove(args->timeout_source_id);
		args->timeout_source_id = 0;
	}

	float refresh_rate = dynv_get_float_wd(args->params, "refresh_rate", 30);
	args->timeout_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000/refresh_rate, (GSourceFunc)updateMainColorTimer, args, (GDestroyNotify)NULL);

	gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "focus_swatch"), "Click on swatch area to begin adding colors to palette");
	
	return 0;
}

static int source_deactivate(struct Arguments *args){
	
	gtk_statusbar_pop(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "focus_swatch"));
	
	if (args->timeout_source_id > 0){
		g_source_remove(args->timeout_source_id);
		args->timeout_source_id = 0;
	}
	return 0;
}


static struct ColorObject* get_color_object(struct DragDrop* dd){
	struct Arguments* args=(struct Arguments*)dd->userdata;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);
	struct ColorObject* colorobject = color_object_new(dd->handler_map);
	color_object_set_color(colorobject, &c);
	return colorobject;
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	gint color_index = gtk_swatch_get_color_at(GTK_SWATCH(dd->widget), x, y);
	Color c;
	color_object_get_color(colorobject, &c);
	gtk_swatch_set_color(GTK_SWATCH(dd->widget), color_index, &c);
	
	updateDiplays((struct Arguments*)dd->userdata);
	
	return 0;
}

static bool test_at(struct DragDrop* dd, int x, int y){
	gint color_index = gtk_swatch_get_color_at(GTK_SWATCH(dd->widget), x, y);
	if (color_index>0) return true;
	return false;
}

void color_picker_set_floating_picker(ColorSource *color_source, FloatingPicker floating_picker){
	struct Arguments* args = (struct Arguments*)color_source;
	args->floating_picker = floating_picker;
}

ColorSource* color_picker_new(GlobalState* gs, GtkWidget **out_widget){
	struct Arguments* args=new struct Arguments;

	args->params = dynv_get_dynv(gs->params, "gpick.picker");
	args->statusbar = (GtkWidget*)dynv_system_get(gs->params, "ptr", "StatusBar");
	args->floating_picker = 0;

	color_source_init(&args->source);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.activate = (int (*)(ColorSource *source))source_activate;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;

	args->gs = gs;
	args->timeout_source_id = 0;

	GtkWidget *vbox, *hbox, *hbox2, *widget, *expander, *table, *main_hbox, *scrolled;
	int table_y;

	main_hbox = gtk_hbox_new(false, 5);

		vbox = gtk_vbox_new(false, 5);
		gtk_box_pack_start (GTK_BOX(main_hbox), vbox, false, false, 0);

			widget = gtk_swatch_new();
			gtk_box_pack_start (GTK_BOX(vbox), widget, false, false, 0);

			g_signal_connect (G_OBJECT (widget), "focus-in-event", G_CALLBACK (on_swatch_focus_change), args);
			g_signal_connect (G_OBJECT (widget), "focus-out-event", G_CALLBACK (on_swatch_focus_change), args);

			g_signal_connect (G_OBJECT (widget), "key_press_event", G_CALLBACK (on_key_up), args);
			g_signal_connect (G_OBJECT (widget), "popup-menu",     G_CALLBACK (on_popup_menu), args);
			g_signal_connect (G_OBJECT (widget), "active_color_changed", G_CALLBACK (on_swatch_active_color_changed), args);
			g_signal_connect (G_OBJECT (widget), "color_changed", G_CALLBACK (on_swatch_color_changed), args);
			g_signal_connect (G_OBJECT (widget), "color_activated", G_CALLBACK (on_swatch_color_activated), args);
			g_signal_connect (G_OBJECT (widget), "center_activated", G_CALLBACK (on_swatch_center_activated), args);
			g_signal_connect_after (G_OBJECT (widget), "button-press-event",G_CALLBACK (on_swatch_button_press), args);


			gtk_swatch_set_active_index(GTK_SWATCH(widget), dynv_get_int32_wd(args->params, "swatch.active_color", 1));
			args->swatch_display = widget;

			gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
			gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);

			struct DragDrop dd;
			dragdrop_init(&dd, gs);

			dd.userdata = args;
			dd.get_color_object = get_color_object;
			dd.set_color_object_at = set_color_object_at;
			dd.test_at = test_at;
			dd.handler_map = dynv_system_get_handler_map(gs->colors->params);

			dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

			{
				char tmp[32];

				const Color *c;
				for (gint i=1; i<7; ++i){
					sprintf(tmp, "swatch.color%d", i);
					c = dynv_get_color_wd(args->params, tmp, 0);
					if (c){
						gtk_swatch_set_color(GTK_SWATCH(args->swatch_display), i, (Color*)c);
					}
				}
			}

			args->color_code = gtk_color_new();
			gtk_box_pack_start (GTK_BOX(vbox), args->color_code, false, true, 0);

			args->zoomed_display = gtk_zoomed_new();
			gtk_box_pack_start (GTK_BOX(vbox), args->zoomed_display, false, false, 0);


		scrolled = gtk_scrolled_window_new(0, 0);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start (GTK_BOX(main_hbox), scrolled, true, true, 0);

		vbox = gtk_vbox_new(false, 5);
		//gtk_box_pack_start (GTK_BOX(main_hbox), vbox, FALSE, FALSE, 0);
		//gtk_container_add(GTK_CONTAINER(scrolled), vbox);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), vbox);

			expander=gtk_expander_new("Settings");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.settings", false));
			args->expanderSettings=expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(6, 2, FALSE);
			table_y=0;
			//gtk_box_pack_start (GTK_BOX(vbox), table, FALSE, FALSE, 0);
			gtk_container_add(GTK_CONTAINER(expander), table);

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Oversample:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_hscale_new_with_range (0,16,1);
				g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_oversample_value_changed), args);
				gtk_range_set_value(GTK_RANGE(widget), dynv_get_int32_wd(args->params, "sampler.oversample", 0));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Falloff:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = create_falloff_type_list();
				gtk_combo_box_set_active(GTK_COMBO_BOX(widget), dynv_get_int32_wd(args->params, "sampler.falloff", NONE));
				g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (on_oversample_falloff_changed), args);
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Zoom:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_hscale_new_with_range (2, 15, 0.5);
				g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_zoom_value_changed), args);
				gtk_range_set_value(GTK_RANGE(widget), dynv_get_float_wd(args->params, "zoom", 2));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				widget = gtk_check_button_new_with_mnemonic ("_Add to palette immediately");
				g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_add_to_palette_changed), args);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "sampler.add_to_palette", false));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				widget = gtk_check_button_new_with_mnemonic ("_Copy to clipboard immediately");
				g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_copy_to_clipboard_changed), args);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "sampler.copy_to_clipboard", false));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				widget = gtk_check_button_new_with_mnemonic ("_Rotate swatch after sample");
				g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (on_rotate_swatch_changed), args);
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "sampler.rotate_swatch_after_sample", false));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

			expander=gtk_expander_new("HSV");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.hsv", false));
			args->expanderHSV=expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				table = gtk_table_new(3, 2, FALSE);
				table_y=0;
				gtk_container_add(GTK_CONTAINER(expander), table);

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_component_new(hue);
					g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), args);
					gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->hue_line = widget;
					table_y++;

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_component_new(saturation);
					g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), args);
					gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->saturation_line = widget;
					table_y++;

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Value:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_component_new(value);
					g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), args);
					gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->value_line = widget;
					table_y++;

			expander=gtk_expander_new("RGB");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.rgb", false));
			args->expanderRGB=expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				table = gtk_table_new(3, 2, FALSE);
				table_y=0;
				gtk_container_add(GTK_CONTAINER(expander), table);


					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Red:",0,0.5,0,0) ,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_component_new(red);
					g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), args);
					gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->red_line = widget;
					table_y++;

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Green:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_component_new(green);
					g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), args);
					gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->green_line = widget;
					table_y++;

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Blue",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_component_new(blue);
					g_signal_connect (G_OBJECT (widget), "color-changed", G_CALLBACK (color_component_change_value), args);
					gtk_table_attach(GTK_TABLE(table), gtk_widget_aligned_new(widget,1,0,0,0),1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->blue_line = widget;
					table_y++;


			expander=gtk_expander_new("Info");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.info", false));
			args->expanderInfo=expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				table = gtk_table_new(3, 2, FALSE);
				table_y=0;
				gtk_container_add(GTK_CONTAINER(expander), table);

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Color name:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_entry_new();
					gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					gtk_editable_set_editable(GTK_EDITABLE(widget), FALSE);
					//gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
					args->color_name = widget;
					table_y++;


	updateDiplays(args);


	args->main = main_hbox;

	gtk_widget_show_all(main_hbox);

	*out_widget = main_hbox;



	return (ColorSource*)args;
}

