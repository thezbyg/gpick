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

#include "GenerateScheme.h"
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

#define MAX_COLOR_WIDGETS		6

typedef struct GenerateSchemeArgs{
	ColorSource source;

	GtkWidget* main;
	GtkWidget* statusbar;

	GtkWidget *gen_type;
	GtkWidget *wheel_type;
	GtkWidget *color_wheel;
	GtkWidget *hue;
	GtkWidget *saturation;
	GtkWidget *lightness;

	GtkWidget *color_previews;
	GtkWidget *last_focused_color;

	GtkWidget *colors[MAX_COLOR_WIDGETS];
	double color_hue[MAX_COLOR_WIDGETS];

	struct{
		double orig_hue;
		double orig_saturation;
		double orig_value;

		double hue;
		double saturation;
		double value;
	}mod[MAX_COLOR_WIDGETS];

	bool wheel_locked;

	int colors_visible;

	struct dynvSystem *params;

	GlobalState* gs;
	struct ColorList *preview_color_list;
}GenerateSchemeArgs;

typedef struct SchemeType{
	const char *name;
	int32_t colors;
	int32_t turn_types;
	double turn[4];
}SchemeType;

const SchemeType scheme_types[]={
	{"Complementary", 1, 1, {180}},
	{"Analogous", 5, 1, {30}},
	{"Triadic", 2, 1, {120}},
	{"Split-Complementary", 2, 2, {150, 60}},
	{"Rectangle (tetradic)", 3, 2, {60, 120}},
	{"Square", 3, 1, {90}},
	{"Neutral", 5, 1, {15}},
	{"Clash", 2, 2, {90, 180}},
	{"Five-Tone", 4, 4, {115, 40, 50, 40}},
	{"Six-Tone", 5, 2, {30, 90}},
};

static int set_rgb_color(GenerateSchemeArgs *args, struct ColorObject* color, uint32_t color_index);
static int set_rgb_color_by_widget(GenerateSchemeArgs *args, struct ColorObject* color, GtkWidget* color_widget);

static void calc(GenerateSchemeArgs *args, bool preview, bool save_settings){

	int32_t type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->gen_type));
	int32_t wheel_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->wheel_type));

	gtk_color_wheel_set_color_wheel_type(GTK_COLOR_WHEEL(args->color_wheel), &color_wheel_types_get()[wheel_type]);
	gtk_color_wheel_set_n_colors(GTK_COLOR_WHEEL(args->color_wheel), scheme_types[type].colors + 1);

	//gfloat chaos = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos));
	//gboolean correction = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_brightness_correction));

	double hue = gtk_range_get_value(GTK_RANGE(args->hue));
	double saturation = gtk_range_get_value(GTK_RANGE(args->saturation));
	double lightness = gtk_range_get_value(GTK_RANGE(args->lightness));

	if (save_settings){
		dynv_set_int32(args->params, "type", type);
		dynv_set_int32(args->params, "wheel_type", wheel_type);
		dynv_set_float(args->params, "hue", hue);
		dynv_set_float(args->params, "saturation", saturation);
		dynv_set_float(args->params, "lightness", lightness);

	}

	hue /= 360.0;
	saturation /= 100.0;
	lightness /= 100.0;

	Color color, hsl, r;
	gint step_i;

	struct ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->colors;

	const ColorWheelType *wheel = &color_wheel_types_get()[wheel_type];

	float chaos = 0;
	float hue_offset = 0;
	float hue_step;

	Color hsv;

	for (step_i = 0; step_i <= scheme_types[type].colors; ++step_i) {

		wheel->hue_to_hsl(wrap_float(hue + args->mod[step_i].hue), &hsl);
		hsl.hsl.lightness = clamp_float(hsl.hsl.lightness + lightness, 0, 1);
		hsl.hsl.saturation = clamp_float(hsl.hsl.saturation * saturation, 0, 1);

		color_hsl_to_hsv(&hsl, &hsv);
		args->mod[step_i].orig_hue = hue;
		args->mod[step_i].orig_saturation = hsv.hsv.saturation;
		args->mod[step_i].orig_value = hsv.hsv.value;

		hsv.hsv.saturation = clamp_float(hsv.hsv.saturation + args->mod[step_i].saturation, 0, 1);
		hsv.hsv.value = clamp_float(hsv.hsv.value + args->mod[step_i].value, 0, 1);

		color_hsv_to_rgb(&hsv, &r);

		struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
		dynv_set_float(color_object->params, "hue_offset", hue_offset);
		color_list_add_color_object(color_list, color_object, 1);
		color_object_release(color_object);

        //color_rgb_to_hsv(&r, &hsv);
		gtk_color_wheel_set_hue(GTK_COLOR_WHEEL(args->color_wheel), step_i, wrap_float(hue + args->mod[step_i].hue));
		gtk_color_wheel_set_saturation(GTK_COLOR_WHEEL(args->color_wheel), step_i, hsv.hsv.saturation);
		gtk_color_wheel_set_value(GTK_COLOR_WHEEL(args->color_wheel), step_i, hsv.hsv.value);

		hue_step = (scheme_types[type].turn[step_i%scheme_types[type].turn_types]) / (360.0)
			+ chaos*(((random_get(args->gs->random)&0xFFFFFFFF)/(gdouble)0xFFFFFFFF)-0.5);

		hue = wrap_float(hue + hue_step);

		hue_offset = wrap_float(hue_offset + hue_step);
	}

	if (preview){

		uint32_t total_colors = scheme_types[type].colors+1;
		if (total_colors > MAX_COLOR_WIDGETS) total_colors = MAX_COLOR_WIDGETS;

		for (uint32_t i = args->colors_visible; i > total_colors; --i)
			gtk_widget_hide(args->colors[i-1]);
		for (uint32_t i = args->colors_visible; i < total_colors; ++i)
			gtk_widget_show(args->colors[i]);
		args->colors_visible = total_colors;

		uint32_t j = 0;
		char* text;
		for (ColorList::iter i = color_list->colors.begin(); i!=color_list->colors.end(); ++i){
			color_object_get_color(*i, &color);

			text = main_get_color_text(args->gs, &color, COLOR_TEXT_TYPE_DISPLAY);

			args->color_hue[j] = dynv_get_float_wd((*i)->params, "hue_offset", 0);

			gtk_color_set_color(GTK_COLOR(args->colors[j]), &color, text);
			if (text) g_free(text);
			++j;
			if (j >= total_colors) break;
		}

	}
}

static void update(GtkWidget *widget, GenerateSchemeArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, false);
}


static void hue_changed_cb(GtkWidget *widget, gint color_id, GenerateSchemeArgs *args){
	if (args->wheel_locked){
		double hue = gtk_range_get_value(GTK_RANGE(args->hue)) / 360.0;
		hue = wrap_float(hue - args->mod[color_id].hue + gtk_color_wheel_get_hue(GTK_COLOR_WHEEL(widget), color_id) - args->mod[color_id].orig_hue);

		gtk_range_set_value(GTK_RANGE(args->hue), hue * 360.0);
	}else{
		args->mod[color_id].hue = gtk_color_wheel_get_hue(GTK_COLOR_WHEEL(widget), color_id) - args->mod[color_id].orig_hue;
		update(widget, args);
	}
}

static void saturation_value_changed_cb(GtkWidget *widget, gint color_id, GenerateSchemeArgs *args){
	if (args->wheel_locked){
		double saturation = gtk_range_get_value(GTK_RANGE(args->saturation)) / 100.0;
		double lightness = gtk_range_get_value(GTK_RANGE(args->lightness)) / 100.0;

		//lightness = gtk_color_wheel_get_value(GTK_COLOR_WHEEL(widget), color_id);

		gtk_range_set_value(GTK_RANGE(args->saturation), saturation * 100.0);
		gtk_range_set_value(GTK_RANGE(args->lightness), lightness * 100.0);
	}else{
		args->mod[color_id].saturation = gtk_color_wheel_get_saturation(GTK_COLOR_WHEEL(widget), color_id) - args->mod[color_id].orig_saturation;
		args->mod[color_id].value = gtk_color_wheel_get_value(GTK_COLOR_WHEEL(widget), color_id) - args->mod[color_id].orig_value;

		update(widget, args);
	}
}

static void color_wheel_locked_toggled_cb(GtkWidget* widget, GenerateSchemeArgs* args){
	args->wheel_locked = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
	gtk_color_wheel_set_block_editable(GTK_COLOR_WHEEL(args->color_wheel), !args->wheel_locked);
}

static void color_reset_scheme_cb(GtkWidget *widget, GenerateSchemeArgs *args){
	for (int i = 0; i < MAX_COLOR_WIDGETS; ++i){
		args->mod[i].hue = 0;
		args->mod[i].saturation = 0;
		args->mod[i].value = 0;
	}
	update(widget, args);
}

static void color_wheel_show_menu(GtkWidget* widget, GenerateSchemeArgs* args, GdkEventButton *event ){
	GtkWidget *menu;
	GtkWidget* item;

	menu = gtk_menu_new();

	item = gtk_check_menu_item_new_with_mnemonic("_Locked");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), args->wheel_locked);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(color_wheel_locked_toggled_cb), args);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	item = gtk_menu_item_new_with_image("_Reset scheme", gtk_image_new_from_stock(GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(color_reset_scheme_cb), args);

	gtk_widget_show_all(GTK_WIDGET(menu));

	gint32 button, event_time;
	if (event){
		button = event->button;
		event_time = event->time;
	}else{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

	g_object_ref_sink(menu);
	g_object_unref(menu);
}

static gboolean color_wheel_button_press_cb(GtkWidget *widget, GdkEventButton *event, GenerateSchemeArgs* args) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		color_wheel_show_menu(widget, args, event);
	}
	return false;
}

static void color_wheel_popup_menu_cb(GtkWidget *widget, GenerateSchemeArgs* args){
	color_wheel_show_menu(widget, args, 0);
}

static void on_color_paste(GtkWidget *widget,  gpointer item) {
	GenerateSchemeArgs* args=(GenerateSchemeArgs*)item;

	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));

	struct ColorObject* color_object;
	if (copypaste_get_color_object(&color_object, args->gs)==0){
		set_rgb_color_by_widget(args, color_object, color_widget);
		color_object_release(color_object);
	}
}

static void color_reset_cb(GtkWidget *widget, GenerateSchemeArgs *args){
	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));

	for (int i = 0; i < args->colors_visible; ++i){
		if (args->colors[i] == color_widget){

			args->mod[i].hue = 0;
			args->mod[i].saturation = 0;
			args->mod[i].value = 0;

			update(widget, args);
			return;
		}
	}
}

static void on_color_edit(GtkWidget *widget,  gpointer item) {
	GenerateSchemeArgs* args=(GenerateSchemeArgs*)item;

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
	GenerateSchemeArgs* args=(GenerateSchemeArgs*)item;
	Color c;

	gtk_color_get_color(GTK_COLOR(g_object_get_data(G_OBJECT(widget), "color_widget")), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_set_string(color_object->params, "name", name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_color_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	GenerateSchemeArgs* args=(GenerateSchemeArgs*)item;
	Color c;

	for (int i = 0; i < args->colors_visible; ++i){
		gtk_color_get_color(GTK_COLOR(args->colors[i]), &c);

		struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
		string name=color_names_get(args->gs->color_names, &c);
		dynv_set_string(color_object->params, "name", name.c_str());
		color_list_add_color_object(args->gs->colors, color_object, 1);
		color_object_release(color_object);
	}

}

static gboolean color_focus_in_cb(GtkWidget *widget, GdkEventFocus *event, GenerateSchemeArgs *args){
	args->last_focused_color = widget;

	for (int i = 0; i < MAX_COLOR_WIDGETS; ++i){
		if (widget == args->colors[i]){
			gtk_color_wheel_set_selected(GTK_COLOR_WHEEL(args->color_wheel), i);
			return false;
		}
	}

	return false;
}

static void on_color_activate(GtkWidget *widget,  gpointer item) {
	GenerateSchemeArgs* args=(GenerateSchemeArgs*)item;
	Color c;

	gtk_color_get_color(GTK_COLOR(widget), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_set_string(color_object->params, "name", name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void color_show_menu(GtkWidget* widget, GenerateSchemeArgs* args, GdkEventButton *event ){
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

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	item = gtk_menu_item_new_with_image("_Reset", gtk_image_new_from_stock(GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(color_reset_cb), args);
	g_object_set_data(G_OBJECT(item), "color_widget", widget);

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

static gboolean on_color_button_press (GtkWidget *widget, GdkEventButton *event, GenerateSchemeArgs* args) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		color_show_menu(widget, args, event);
	}
	return false;
}

static void on_color_popup_menu(GtkWidget *widget, GenerateSchemeArgs* args){
	color_show_menu(widget, args, 0);
}

static gboolean on_color_key_press (GtkWidget *widget, GdkEventKey *event, GenerateSchemeArgs* args){
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

static gchar* format_saturation_value_cb (GtkScale *scale, gdouble value){
	return g_strdup_printf ("%d%%", int(value));
}

static gchar* format_lightness_value_cb (GtkScale *scale, gdouble value){
	if (value>=0)
		return g_strdup_printf ("+%d%%", int(value));
	else
		return g_strdup_printf ("-%d%%", -int(value));
}

static int source_destroy(GenerateSchemeArgs *args){
	color_list_destroy(args->preview_color_list);
	dynv_system_release(args->params);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static int source_get_color(GenerateSchemeArgs *args, ColorObject** color){
	Color c;
	if (args->last_focused_color){
		gtk_color_get_color(GTK_COLOR(args->last_focused_color), &c);
	}else{
		gtk_color_get_color(GTK_COLOR(args->colors[0]), &c);
	}
	*color = color_list_new_color_object(args->gs->colors, &c);

	string name = color_names_get(args->gs->color_names, &c);
	dynv_set_string((*color)->params, "name", name.c_str());
	return 0;
}

static int set_rgb_color_by_widget(GenerateSchemeArgs *args, struct ColorObject* color_object, GtkWidget* color_widget){
	for (int i=0; i<args->colors_visible; ++i){
		if (args->colors[i]==color_widget){
			set_rgb_color(args, color_object, i);
			return 0;
		}
	}
	return -1;
}

static int set_rgb_color(GenerateSchemeArgs *args, struct ColorObject* color, uint32_t color_index){
	Color c;
	color_object_get_color(color, &c);

	double hue;
	double saturation;
	double lightness;
	double shifted_hue;

	Color hsl, hsv, hsl_results;
	color_rgb_to_hsv(&c, &hsv);

	//hsv.hsv.saturation = clamp_float(hsv.hsv.saturation - args->mod[color_index].saturation, 0, 1);
	//hsv.hsv.value = clamp_float(hsv.hsv.value - args->mod[color_index].value, 0, 1);

	color_hsv_to_hsl(&hsv, &hsl);

	int32_t wheel_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->wheel_type));
	const ColorWheelType *wheel = &color_wheel_types_get()[wheel_type];

	wheel->rgbhue_to_hue(hsl.hsl.hue, &hue);

	//color_rgbhue_to_rybhue(hsl.hsl.hue, &hue);

	shifted_hue = wrap_float(hue - args->color_hue[color_index] - args->mod[color_index].hue);

	wheel->hue_to_hsl(hue, &hsl_results);

	saturation = hsl.hsl.saturation * 1/hsl_results.hsl.saturation;
	lightness = hsl.hsl.lightness - hsl_results.hsl.lightness;

	shifted_hue *= 360.0;
	saturation *= 100.0;
	lightness *= 100.0;

	gtk_range_set_value(GTK_RANGE(args->hue), shifted_hue);
	gtk_range_set_value(GTK_RANGE(args->saturation), saturation);
	gtk_range_set_value(GTK_RANGE(args->lightness), lightness);

	return 0;
}


static int source_set_color(GenerateSchemeArgs *args, struct ColorObject* color){
	if (args->last_focused_color){
		return set_rgb_color_by_widget(args, color, args->last_focused_color);
	}else{
		return set_rgb_color(args, color, 0);
	}
}

static int source_activate(GenerateSchemeArgs *args){
	gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "empty"), "");
	return 0;
}

static int source_deactivate(GenerateSchemeArgs *args){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, true);


	dynv_set_bool(args->params, "wheel_locked", args->wheel_locked);

	float hsv_shift_array[MAX_COLOR_WIDGETS * 3];
	for (uint32_t i = 0; i < MAX_COLOR_WIDGETS; ++i){
		hsv_shift_array[i * 3 + 0] = args->mod[i].hue;
		hsv_shift_array[i * 3 + 1] = args->mod[i].saturation;
		hsv_shift_array[i * 3 + 2] = args->mod[i].value;
	}
	dynv_set_float_array(args->params, "hsv_shift", hsv_shift_array, MAX_COLOR_WIDGETS * 3);

    return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	GenerateSchemeArgs* args = (GenerateSchemeArgs*)dd->userdata;
	struct ColorObject* colorobject;
	if (source_get_color(args, &colorobject) == 0){
		return colorobject;
	}
	return 0;
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	GenerateSchemeArgs* args = static_cast<GenerateSchemeArgs*>(dd->userdata);
	set_rgb_color(args, colorobject, (uintptr_t)dd->userdata2);
	return 0;
}

static int set_color_object_at_color_wheel(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	int item = gtk_color_wheel_get_at(GTK_COLOR_WHEEL(dd->widget), x, y);
	GenerateSchemeArgs* args = static_cast<GenerateSchemeArgs*>(dd->userdata);

	if (item == -1){

	}else if (item >= 0){
		Color c, hsl;
		double hue;
		color_object_get_color(colorobject, &c);
		color_rgb_to_hsl(&c, &hsl);

		int32_t wheel_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->wheel_type));
		const ColorWheelType *wheel = &color_wheel_types_get()[wheel_type];
		wheel->rgbhue_to_hue(hsl.hsl.hue, &hue);

		if (args->wheel_locked){
			double hue_shift = (hue - args->mod[item].orig_hue) - args->mod[item].hue;
            hue = wrap_float(gtk_range_get_value(GTK_RANGE(args->hue)) / 360.0 + hue_shift);

			gtk_range_set_value(GTK_RANGE(args->hue), hue * 360.0);
		}else{
			args->mod[item].hue = hue - args->mod[item].orig_hue;
			update(dd->widget, args);
		}
	}
	return 0;
}

static bool test_at_color_wheel(struct DragDrop* dd, int x, int y){
	int item = gtk_color_wheel_get_at(GTK_COLOR_WHEEL(dd->widget), x, y);
	if (item == -1){

	}else if (item >= 0){

		return true;
	}
	return false;
}

static ColorSource* source_implement(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace){
	GenerateSchemeArgs* args = new GenerateSchemeArgs;

	args->params = dynv_system_ref(dynv_namespace);
	args->statusbar = (GtkWidget*)dynv_get_pointer_wd(gs->params, "StatusBar", 0);

	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;

	uint32_t hsv_shift_array_size = 0;
	float *hsv_shift_array = dynv_get_float_array_wd(args->params, "hsv_shift", 0, 0, &hsv_shift_array_size);
    hsv_shift_array_size /= 3;

    for (uint32_t i = 0; i < MAX_COLOR_WIDGETS; ++i){
		if (i < hsv_shift_array_size){
			args->mod[i].hue = hsv_shift_array[i * 3 + 0];
			args->mod[i].saturation = hsv_shift_array[i * 3 + 1];
			args->mod[i].value = hsv_shift_array[i * 3 + 2];
		}else{
			args->mod[i].hue = 0;
			args->mod[i].saturation = 0;
			args->mod[i].value = 0;
		}
	}

	if (hsv_shift_array) delete [] hsv_shift_array;


	GtkWidget *table, *vbox, *hbox, *widget, *hbox2;

	hbox = gtk_hbox_new(FALSE, 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	args->color_previews = gtk_table_new(3, 2, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->color_previews, true, true, 5);

	struct DragDrop dd;
	dragdrop_init(&dd, gs);

	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;

	for (int i = 0; i < MAX_COLOR_WIDGETS; ++i){
		widget = gtk_color_new();
		gtk_color_set_rounded(GTK_COLOR(widget), true);
		gtk_color_set_hcenter(GTK_COLOR(widget), true);

		gtk_table_attach(GTK_TABLE(args->color_previews), widget, i % 2, (i % 2) + 1, i / 2, i / 2 + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);

		//gtk_box_pack_start(GTK_BOX(args->color_previews), widget, TRUE, TRUE, 0);

		args->colors[i] = widget;

		g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(on_color_button_press), args);
		g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(on_color_activate), args);
		g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(on_color_key_press), args);
		g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(on_color_popup_menu), args);
		g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(color_focus_in_cb), args);

		//setup drag&drop
		gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
		gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
		dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
		dd.userdata2 = (void*)i;
		dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);
	}


	hbox2 = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 5);

    args->color_wheel = gtk_color_wheel_new();
	gtk_box_pack_start(GTK_BOX(hbox2), args->color_wheel, false, false, 0);
	g_signal_connect(G_OBJECT(args->color_wheel), "hue_changed", G_CALLBACK(hue_changed_cb), args);
	g_signal_connect(G_OBJECT(args->color_wheel), "saturation_value_changed", G_CALLBACK(saturation_value_changed_cb), args);
	g_signal_connect(G_OBJECT(args->color_wheel), "popup-menu", G_CALLBACK(color_wheel_popup_menu_cb), args);
	g_signal_connect(G_OBJECT(args->color_wheel), "button-press-event", G_CALLBACK(color_wheel_button_press_cb), args);
	args->wheel_locked = dynv_get_bool_wd(args->params, "wheel_locked", true);
	gtk_color_wheel_set_block_editable(GTK_COLOR_WHEEL(args->color_wheel), !args->wheel_locked);

	gtk_drag_dest_set(args->color_wheel, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata = args;
	dd.userdata2 = 0;
	dd.set_color_object_at = set_color_object_at_color_wheel;
	dd.test_at = test_at_color_wheel;
	dragdrop_widget_attach(args->color_wheel, DragDropFlags(DRAGDROP_DESTINATION), &dd);


	gint table_y;
	table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(hbox2), table, true, true, 5);
	table_y = 0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->hue = gtk_hscale_new_with_range(0, 360, 1);
	gtk_range_set_value(GTK_RANGE(args->hue), dynv_get_float_wd(args->params, "hue", 180));
	g_signal_connect (G_OBJECT (args->hue), "value-changed", G_CALLBACK (update), args);
	gtk_table_attach(GTK_TABLE(table), args->hue,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->saturation = gtk_hscale_new_with_range(0, 120, 1);
	gtk_range_set_value(GTK_RANGE(args->saturation), dynv_get_float_wd(args->params, "saturation", 100));
	g_signal_connect (G_OBJECT (args->saturation), "value-changed", G_CALLBACK (update), args);
	g_signal_connect (G_OBJECT (args->saturation), "format-value", G_CALLBACK (format_saturation_value_cb), args);
	gtk_table_attach(GTK_TABLE(table), args->saturation,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->lightness = gtk_hscale_new_with_range(-50, 80, 1);
	gtk_range_set_value(GTK_RANGE(args->lightness), dynv_get_float_wd(args->params, "lightness", 0));
	g_signal_connect (G_OBJECT (args->lightness), "value-changed", G_CALLBACK (update), args);
	g_signal_connect (G_OBJECT (args->lightness), "format-value", G_CALLBACK (format_lightness_value_cb), args);
	gtk_table_attach(GTK_TABLE(table), args->lightness,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	//table_y=0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:",0,0.5,0,0),0,1,table_y,table_y+1, GTK_FILL, GTK_SHRINK, 5, 5);
	args->gen_type = gtk_combo_box_new_text();
	for (uint32_t i=0; i<sizeof(scheme_types)/sizeof(SchemeType); i++){
		gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), scheme_types[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->gen_type), dynv_get_int32_wd(args->params, "type", 0));
	g_signal_connect (G_OBJECT (args->gen_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->gen_type,1,2,table_y,table_y+1, GTK_FILL, GTK_SHRINK,5,0);
    table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Color wheel:",0,0.5,0,0),0,1,table_y,table_y+1, GTK_FILL, GTK_SHRINK, 5, 5);
	args->wheel_type = gtk_combo_box_new_text();

	for (uint32_t i = 0; i < color_wheel_types_get_n(); i++){
		gtk_combo_box_append_text(GTK_COMBO_BOX(args->wheel_type), color_wheel_types_get()[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->wheel_type), dynv_get_int32_wd(args->params, "wheel_type", 0));
	g_signal_connect (G_OBJECT (args->wheel_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->wheel_type,1,2,table_y,table_y+1, GTK_FILL, GTK_SHRINK,5,0);

	table_y++;

	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(gs->colors->params);
	struct ColorList* preview_color_list = color_list_new(handler_map);
	dynv_handler_map_release(handler_map);


	args->preview_color_list = preview_color_list;
	args->colors_visible = MAX_COLOR_WIDGETS;
	args->gs = gs;

	gtk_widget_show_all(hbox);

	update(0, args);

	args->main = hbox;

	args->source.widget = hbox;

	return (ColorSource*)args;
}

int generate_scheme_source_register(ColorSourceManager *csm){
    ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "generate_scheme", "Scheme generation");
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace))source_implement;
    color_source_manager_add_source(csm, color_source);
	return 0;
}

