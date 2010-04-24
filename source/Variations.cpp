/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#include "Variations.h"
#include "DragDrop.h"

#include "GlobalStateStruct.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "ColorRYB.h"
#include "gtk/ColorWidget.h"
#include "gtk/ColorWheel.h"
#include "ColorWheelType.h"
#include "uiColorInput.h"
#include "CopyPaste.h"
#include "Converter.h"
#include "DynvHelpers.h"

#include "uiApp.h"

#include <gdk/gdkkeysyms.h>

#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>
using namespace std;

#define VAR_COLOR_WIDGETS		8
#define MAX_COLOR_LINES			3

typedef struct VariationType{
	const char *name;
	const char *symbol;
	int component_id;
	double strength_mult;
}VariationType;

const VariationType variation_types[] = {
	{"Hue", "H", 0, 1},
	{"Saturation", "S", 1, 1},
	{"Saturation 2x", "S 2x", 1, 2},
	{"Saturation 4x", "S 4x", 1, 4},
	{"Lightness", "L", 2, 1},
};

typedef struct VariationsArgs{
	ColorSource source;

	GtkWidget* main;
	GtkWidget* statusbar;

	GtkWidget *strength;
	GtkWidget *last_focused_color;
	GtkWidget *color_previews;

    struct{
		GtkWidget *color;
		GtkWidget *var_colors[VAR_COLOR_WIDGETS + 1];
		const VariationType *type;
	}color[MAX_COLOR_LINES];

	struct dynvSystem *params;

	struct ColorList *preview_color_list;
	GlobalState* gs;
}VariationsArgs;


static int set_rgb_color(VariationsArgs *args, struct ColorObject* color, uint32_t color_index);
static int set_rgb_color_by_widget(VariationsArgs *args, struct ColorObject* color, GtkWidget* color_widget);

static void calc(VariationsArgs *args, bool preview, bool save_settings){

	double strength = gtk_range_get_value(GTK_RANGE(args->strength));

	if (save_settings){
		dynv_set_float(args->params, "strength", strength);
	}

	Color color, hsl, r, hsl_mod;

	for (int i = 0; i < MAX_COLOR_LINES; ++i){
		gtk_color_get_color(GTK_COLOR(args->color[i].color), &color);

		color_rgb_to_hsl(&color, &hsl);

		for (int j = 0; j < VAR_COLOR_WIDGETS + 1; ++j){
			if (j == VAR_COLOR_WIDGETS / 2) continue;

			color_copy(&hsl, &hsl_mod);

			switch (args->color[i].type->component_id){
			case 0:
				hsl_mod.hsl.hue = clamp_float(hsl.hsl.hue + (args->color[i].type->strength_mult * strength * (j - VAR_COLOR_WIDGETS / 2)) / 400.0, 0, 1);
				break;
			case 1:
				hsl_mod.hsl.saturation = clamp_float(hsl.hsl.saturation + (args->color[i].type->strength_mult * strength * (j - VAR_COLOR_WIDGETS / 2)) / 400.0, 0, 1);
				break;
			case 2:
				hsl_mod.hsl.lightness = clamp_float(hsl.hsl.lightness + (args->color[i].type->strength_mult * strength * (j - VAR_COLOR_WIDGETS / 2)) / 400.0, 0, 1);
				break;
			}

			color_hsl_to_rgb(&hsl_mod, &r);

			gtk_color_set_color(GTK_COLOR(args->color[i].var_colors[j]), &r, "");
		}
	}
}

static void update(GtkWidget *widget, VariationsArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, false);
}


static void on_color_paste(GtkWidget *widget,  gpointer item) {
	VariationsArgs* args=(VariationsArgs*)item;

	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));

	struct ColorObject* color_object;
	if (copypaste_get_color_object(&color_object, args->gs)==0){
		set_rgb_color_by_widget(args, color_object, color_widget);
		color_object_release(color_object);
	}
}


static void on_color_edit(GtkWidget *widget,  gpointer item) {
	VariationsArgs* args=(VariationsArgs*)item;

	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));

	Color c;
	gtk_color_get_color(GTK_COLOR(color_widget), &c);
	struct ColorObject* color_object = color_list_new_color_object(args->gs->colors, &c);
	struct ColorObject* new_color_object = 0;

	if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), args->gs, color_object, &new_color_object )==0){

		set_rgb_color_by_widget(args, new_color_object, color_widget);

		color_object_release(new_color_object);
	}

	color_object_release(color_object);
}


static void on_color_add_to_palette(GtkWidget *widget,  gpointer item) {
	VariationsArgs* args=(VariationsArgs*)item;
	Color c;

	gtk_color_get_color(GTK_COLOR(g_object_get_data(G_OBJECT(widget), "color_widget")), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_set_string(color_object->params, "name", name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_color_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	VariationsArgs* args=(VariationsArgs*)item;
	Color c;

	for (int i = 0; i < MAX_COLOR_LINES; ++i){
		for (int j = 0; j < VAR_COLOR_WIDGETS + 1; ++j){

			gtk_color_get_color(GTK_COLOR(args->color[i].var_colors[j]), &c);

			struct ColorObject *color_object = color_list_new_color_object(args->gs->colors, &c);
			string name=color_names_get(args->gs->color_names, &c);
			dynv_set_string(color_object->params, "name", name.c_str());
			color_list_add_color_object(args->gs->colors, color_object, 1);
			color_object_release(color_object);
		}
	}

}

static gboolean color_focus_in_cb(GtkWidget *widget, GdkEventFocus *event, VariationsArgs *args){
	args->last_focused_color = widget;
	return false;
}


static void on_color_activate(GtkWidget *widget,  gpointer item) {
	VariationsArgs* args=(VariationsArgs*)item;
	Color c;

	gtk_color_get_color(GTK_COLOR(widget), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_set_string(color_object->params, "name", name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void color_show_menu(GtkWidget* widget, VariationsArgs* args, GdkEventButton *event ){
	GtkWidget *menu;
	GtkWidget* item;

	menu = gtk_menu_new ();

	item = gtk_menu_item_new_with_image ("_Add to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_color_add_to_palette), args);
	g_object_set_data(G_OBJECT(item), "color_widget", widget);

	item = gtk_menu_item_new_with_image ("_Add all to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_color_add_all_to_palette), args);
	g_object_set_data(G_OBJECT(item), "color_widget", widget);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	item = gtk_menu_item_new_with_mnemonic ("_Copy to clipboard");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	Color c;
	gtk_color_get_color(GTK_COLOR(widget), &c);

	struct ColorObject* color_object;
	color_object = color_list_new_color_object(args->gs->colors, &c);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (color_object, 0, args->gs));
	color_object_release(color_object);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	for (int i = 0; i < MAX_COLOR_LINES; ++i){
		if (args->color[i].color == widget){

			gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

			item = gtk_menu_item_new_with_image ("_Edit...", gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_color_edit), args);
			g_object_set_data(G_OBJECT(item), "color_widget", widget);

			item = gtk_menu_item_new_with_image ("_Paste", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_color_paste), args);
			g_object_set_data(G_OBJECT(item), "color_widget", widget);

			if (copypaste_is_color_object_available(args->gs)!=0){
				gtk_widget_set_sensitive(item, false);
			}

			break;
		}
	}

	gtk_widget_show_all (GTK_WIDGET(menu));

	gint32 button, event_time;
	if (event){
		button = event->button;
		event_time = event->time;
	}else{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button, event_time);

	g_object_ref_sink(menu);
	g_object_unref(menu);
}


static gboolean on_color_button_press (GtkWidget *widget, GdkEventButton *event, VariationsArgs* args) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		color_show_menu(widget, args, event);
	}
	return false;
}

static void on_color_popup_menu(GtkWidget *widget, VariationsArgs* args){
	color_show_menu(widget, args, 0);
}


static gboolean on_color_key_press (GtkWidget *widget, GdkEventKey *event, VariationsArgs* args){
	guint modifiers = gtk_accelerator_get_default_mod_mask();

	Color c;
	struct ColorObject* color_object;
	GtkWidget* color_widget = widget;

	switch(event->keyval){
		case GDK_c:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){

				gtk_color_get_color(GTK_COLOR(color_widget), &c);
				color_object = color_list_new_color_object(args->gs->colors, &c);

				Converters *converters = (Converters*)dynv_get_pointer_wd(args->gs->params, "Converters", 0);
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, color_object, 0, args->gs->params);
				}

				color_object_release(color_object);

				return true;
			}
			return false;
			break;

		case GDK_v:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){
				if (copypaste_get_color_object(&color_object, args->gs)==0){
					set_rgb_color_by_widget(args, color_object, color_widget);
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

static int source_destroy(VariationsArgs *args){

	Color c;
	char tmp[32];
	for (gint i = 0; i < MAX_COLOR_LINES; ++i){
		sprintf(tmp, "color%d", i);
		gtk_color_get_color(GTK_COLOR(args->color[i].color), &c);
		dynv_set_color(args->params, tmp, &c);
	}

	color_list_destroy(args->preview_color_list);
	dynv_system_release(args->params);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static int source_get_color(VariationsArgs *args, ColorObject** color){
	Color c;
	gtk_color_get_color(GTK_COLOR(args->color[0].color), &c);
	*color = color_list_new_color_object(args->gs->colors, &c);
	return 0;
}

static int set_rgb_color_by_widget(VariationsArgs *args, struct ColorObject* color_object, GtkWidget* color_widget){

	for (int i = 0; i < MAX_COLOR_LINES; ++i){
		if (args->color[i].color == color_widget){
			set_rgb_color(args, color_object, i);
			return 0;
		}
	}
	return -1;
}

static int set_rgb_color(VariationsArgs *args, struct ColorObject* color, uint32_t color_index){
	Color c;
	color_object_get_color(color, &c);
	gtk_color_set_color(GTK_COLOR(args->color[color_index].color), &c, args->color[color_index].type->symbol);
	update(0, args);
	return 0;
}


static int source_set_color(VariationsArgs *args, struct ColorObject* color){
	if (args->last_focused_color){
		return set_rgb_color_by_widget(args, color, args->last_focused_color);
	}else{
		return set_rgb_color(args, color, 0);
	}
}

static int source_activate(VariationsArgs *args){
	gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "empty"), "");
	return 0;
}

static int source_deactivate(VariationsArgs *args){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, true);
    return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	Color c;
	gtk_color_get_color(GTK_COLOR(dd->widget), &c);
	struct ColorObject* colorobject = color_object_new(dd->handler_map);
	color_object_set_color(colorobject, &c);
	return colorobject;
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	VariationsArgs* args = static_cast<VariationsArgs*>(dd->userdata);
	set_rgb_color(args, colorobject, (uintptr_t)dd->userdata2);
	return 0;
}

static ColorSource* source_implement(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace){
	VariationsArgs* args = new VariationsArgs;

	args->params = dynv_system_ref(dynv_namespace);
	args->statusbar = (GtkWidget*)dynv_get_pointer_wd(gs->params, "StatusBar", 0);

	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;


	GtkWidget *table, *vbox, *hbox, *widget, *hbox2;

	hbox = gtk_hbox_new(FALSE, 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	args->color_previews = gtk_table_new(MAX_COLOR_LINES, VAR_COLOR_WIDGETS + 1, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->color_previews, true, true, 5);

	struct DragDrop dd;
	dragdrop_init(&dd, gs);

	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;

	for (int i = 0; i < MAX_COLOR_LINES; ++i){
		for (int j = 0; j < VAR_COLOR_WIDGETS + 1; ++j){

			widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_color_set_roundness(GTK_COLOR(widget), 5);

			gtk_table_attach(GTK_TABLE(args->color_previews), widget, j, j + 1, i, i + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(0), 0, 0);

			args->color[i].var_colors[j] = widget;
			args->color[i].type = &variation_types[i];

			g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(on_color_button_press), args);
			g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(on_color_activate), args);
			g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(on_color_key_press), args);
			g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(on_color_popup_menu), args);
			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(color_focus_in_cb), args);

			//setup drag&drop
            if (j == VAR_COLOR_WIDGETS / 2){
				gtk_widget_set_size_request(widget, 50, 40);

				gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
				gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
				dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
				dd.userdata2 = (void*)i;
				dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

				args->color[i].color = args->color[i].var_colors[j];

			}else{
				gtk_widget_set_size_request(widget, 30, 30);

				gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
				dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
				dd.userdata2 = (void*)i;
				dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE), &dd);
			}
		}
	}

	Color c;
	color_set(&c, 0.5);
	char tmp[32];
	for (gint i = 0; i < MAX_COLOR_LINES; ++i){
		sprintf(tmp, "color%d", i);
		gtk_color_set_color(GTK_COLOR(args->color[i].color), dynv_get_color_wdc(args->params, tmp, &c), args->color[i].type->symbol);
	}

	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 5);

	gint table_y;
	table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(hbox2), table, true, true, 5);
	table_y = 0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Strength:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->strength = gtk_hscale_new_with_range(1, 100, 1);
	gtk_range_set_value(GTK_RANGE(args->strength), dynv_get_float_wd(args->params, "strength", 30));
	g_signal_connect(G_OBJECT(args->strength), "value-changed", G_CALLBACK (update), args);
	gtk_table_attach(GTK_TABLE(table), args->strength,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(gs->colors->params);
	struct ColorList* preview_color_list = color_list_new(handler_map);
	dynv_handler_map_release(handler_map);

	args->preview_color_list = preview_color_list;
	//args->colors_visible = MAX_COLOR_WIDGETS;
	args->gs = gs;

	gtk_widget_show_all(hbox);

	update(0, args);

	args->main = hbox;

	args->source.widget = hbox;

	return (ColorSource*)args;
}

int variations_source_register(ColorSourceManager *csm){
    ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "variations", "Variations");
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace))source_implement;
    color_source_manager_add_source(csm, color_source);
	return 0;
}


