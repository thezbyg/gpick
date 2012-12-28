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

#include "GlobalStateStruct.h"
#include "ColorPicker.h"
#include "DragDrop.h"
#include "Converter.h"
#include "DynvHelpers.h"
#include "CopyPaste.h"
#include "FloatingPicker.h"
#include "ColorRYB.h"
#include "ColorWheelType.h"
#include "ColorSpaceType.h"

#include "gtk/Swatch.h"
#include "gtk/Zoomed.h"
#include "gtk/ColorComponent.h"
#include "gtk/ColorWidget.h"

#include "uiApp.h"
#include "uiUtilities.h"
#include "uiColorInput.h"
#include "uiConverter.h"
#include "Internationalisation.h"

#include <gdk/gdkkeysyms.h>

#include <math.h>
#ifdef _MSC_VER
#define M_PI 3.14159265359
#endif
#include <string.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace std;
using namespace math;

typedef struct ColorPickerArgs{
	ColorSource source;

	GtkWidget* main;

	GtkWidget* expanderRGB;
	GtkWidget* expanderHSV;
	GtkWidget* expanderHSL;
	GtkWidget* expanderCMYK;
	GtkWidget* expanderXYZ;
	GtkWidget* expanderLAB;
	GtkWidget* expanderLCH;

	GtkWidget* expanderInfo;
	GtkWidget* expanderMain;
	GtkWidget* expanderSettings;

	GtkWidget *swatch_display;
	GtkWidget *zoomed_display;
	GtkWidget *color_code;

	GtkWidget* hsl_control;
	GtkWidget* hsv_control;
	GtkWidget* rgb_control;
	GtkWidget* cmyk_control;
	GtkWidget* lab_control;
	GtkWidget* lch_control;

	GtkWidget* color_name;
	GtkWidget* statusbar;

	GtkWidget* contrastCheck;
	GtkWidget* contrastCheckMsg;

	guint timeout_source_id;

	FloatingPicker floating_picker;

	struct dynvSystem *params;
	struct dynvSystem *global_params;
	GlobalState* gs;

}ColorPickerArgs;

struct ColorCompItem{
	GtkWidget *widget;
	GtkColorComponentComp component;
	int component_id;
};

static int source_set_color(ColorPickerArgs *args, ColorObject* color);
static int source_set_nth_color(ColorPickerArgs *args, uint32_t color_n, ColorObject* color);
static int source_get_nth_color(ColorPickerArgs *args, uint32_t color_n, ColorObject** color);

static void updateMainColorNow(ColorPickerArgs* args){
	if (!dynv_get_bool_wd(args->params, "zoomed_enabled", true)){
		Color c;
		gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);
		gchar* text = main_get_color_text(args->gs, &c, COLOR_TEXT_TYPE_DISPLAY);
		gtk_color_set_color(GTK_COLOR(args->color_code), &c, text);
		if (text) g_free(text);
		gtk_swatch_set_main_color(GTK_SWATCH(args->swatch_display), &c);
	}
}

static gboolean updateMainColor( gpointer data ){
	ColorPickerArgs* args=(ColorPickerArgs*)data;

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

	bool zoomed_enabled = dynv_get_bool_wd(args->params, "zoomed_enabled", true);

	if (zoomed_enabled){
		gtk_zoomed_get_screen_rect(GTK_ZOOMED(args->zoomed_display), pointer, window_size, &zoomed_rect);
		screen_reader_add_rect(args->gs->screen_reader, screen, zoomed_rect);
	}

	screen_reader_update_pixbuf(args->gs->screen_reader, &final_rect);

	Vec2<int> offset;

	offset = Vec2<int>(sampler_rect.getX()-final_rect.getX(), sampler_rect.getY()-final_rect.getY());
	Color c;
	sampler_get_color_sample(args->gs->sampler, pointer, window_size, offset, &c);

	gchar* text = main_get_color_text(args->gs, &c, COLOR_TEXT_TYPE_DISPLAY);
	gtk_color_set_color(GTK_COLOR(args->color_code), &c, text);
	if (text) g_free(text);

	gtk_swatch_set_main_color(GTK_SWATCH(args->swatch_display), &c);


	if (zoomed_enabled){
		offset = Vec2<int>(zoomed_rect.getX()-final_rect.getX(), zoomed_rect.getY()-final_rect.getY());
		gtk_zoomed_update(GTK_ZOOMED(args->zoomed_display), pointer, window_size, offset, screen_reader_get_pixbuf(args->gs->screen_reader));
	}

	return TRUE;
}

static gboolean updateMainColorTimer(ColorPickerArgs* args){
	updateMainColor(args);
	return true;
}

static void updateComponentText(ColorPickerArgs *args, GtkColorComponent *component, const char *type){
	Color transformed_color;
	gtk_color_component_get_transformed_color(component, &transformed_color);
	lua_State* L = static_cast<lua_State*>(dynv_get_pointer_wdc(args->gs->params, "lua_State", 0));
	list<string> str = color_space_color_to_text(type, &transformed_color, L);
	int j = 0;
	const char *text[4];
	memset(text, 0, sizeof(text));
	for (list<string>::iterator i = str.begin(); i != str.end(); i++){
		text[j] = (*i).c_str();
		j++;
		if (j > 3) break;
	}
	gtk_color_component_set_text(component, text);
}

static void updateDisplays(ColorPickerArgs *args, GtkWidget *except_widget){
	updateMainColorNow(args);
	Color c, c2;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display),&c);

	if (except_widget != args->hsl_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hsl_control), &c);
	if (except_widget != args->hsv_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hsv_control), &c);
	if (except_widget != args->rgb_control)	gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->rgb_control), &c);
	if (except_widget != args->cmyk_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->cmyk_control), &c);
	if (except_widget != args->lab_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->lab_control), &c);
	if (except_widget != args->lch_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->lch_control), &c);

	updateComponentText(args, GTK_COLOR_COMPONENT(args->hsl_control), "hsl");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->hsv_control), "hsv");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->rgb_control), "rgb");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->cmyk_control), "cmyk");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lab_control), "lab");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lch_control), "lch");

	string color_name = color_names_get(args->gs->color_names, &c, true);
	gtk_entry_set_text(GTK_ENTRY(args->color_name), color_name.c_str());

	gtk_color_get_color(GTK_COLOR(args->contrastCheck), &c2);
	gtk_color_set_text_color(GTK_COLOR(args->contrastCheck), &c);

	stringstream ss;
	ss.setf(ios::fixed, ios::floatfield);

	Color c_lab, c2_lab;
	color_rgb_to_lab_d50(&c, &c_lab);
	color_rgb_to_lab_d50(&c2, &c2_lab);

	const ColorWheelType *wheel = &color_wheel_types_get()[0];
	Color hsl1, hsl2;
	double hue1, hue2;
	color_rgb_to_hsl(&c, &hsl1);
	color_rgb_to_hsl(&c2, &hsl2);

	wheel->rgbhue_to_hue(hsl1.hsl.hue, &hue1);
	wheel->rgbhue_to_hue(hsl2.hsl.hue, &hue2);

	double complementary = std::abs(hue1 - hue2);
	complementary -= std::floor(complementary);
	complementary *= std::sin(hsl1.hsl.lightness * M_PI) * std::sin(hsl2.hsl.lightness * M_PI);
	complementary *= std::sin(hsl1.hsl.saturation * M_PI / 2) * std::sin(hsl2.hsl.saturation * M_PI / 2);

	ss << std::setprecision(1) << std::abs(c_lab.lab.L - c2_lab.lab.L) + complementary * 50 << "%";

	gtk_label_set_text(GTK_LABEL(args->contrastCheckMsg), ss.str().c_str());
}


static void on_swatch_active_color_changed( GtkWidget *widget, gint32 new_active_color, gpointer data ){
	ColorPickerArgs* args = (ColorPickerArgs*)data;
	updateDisplays(args, widget);
}

static void on_swatch_color_changed( GtkWidget *widget, gpointer data ){
	ColorPickerArgs* args=(ColorPickerArgs*)data;
	updateDisplays(args, widget);
}



static void on_swatch_menu_add_to_palette(GtkWidget *widget,  gpointer item) {
	ColorPickerArgs* args=(ColorPickerArgs*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
	dynv_set_string(color_object->params, "name", name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_swatch_menu_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	ColorPickerArgs* args=(ColorPickerArgs*)item;
	Color c;
	for (int i = 1; i < 7; ++i) {
		gtk_swatch_get_color(GTK_SWATCH(args->swatch_display), i, &c);

		struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
		string name=color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
		dynv_set_string(color_object->params, "name", name.c_str());
		color_list_add_color_object(args->gs->colors, color_object, 1);
		color_object_release(color_object);
	}
}

static void on_swatch_color_activated(GtkWidget *widget, gpointer item) {
	ColorPickerArgs* args=(ColorPickerArgs*)item;
	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(widget), &c);

	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
	dynv_set_string(color_object->params, "name", name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_swatch_center_activated(GtkWidget *widget, gpointer item) {
	ColorPickerArgs* args=(ColorPickerArgs*)item;

	floating_picker_activate(args->floating_picker, true);

}

static void on_swatch_color_edit(GtkWidget *widget, gpointer item) {
	ColorPickerArgs* args=(ColorPickerArgs*)item;

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


static void paste_cb(GtkWidget *widget, ColorPickerArgs* args) {
	struct ColorObject* color_object;
	if (copypaste_get_color_object(&color_object, args->gs)==0){
		source_set_color(args, color_object);
		color_object_release(color_object);
	}
}

static void swatch_popup_menu_cb(GtkWidget *widget, ColorPickerArgs* args){
	GtkWidget *menu;

	gint32 button, event_time;

	Color c;
	updateMainColor(args);
	gtk_swatch_get_main_color(GTK_SWATCH(args->swatch_display), &c);

	struct ColorObject* color_object;
	color_object = color_list_new_color_object(args->gs->colors, &c);
	menu = converter_create_copy_menu(color_object, 0, args->gs);
	color_object_release(color_object);

	gtk_widget_show_all(GTK_WIDGET(menu));

	button = 0;
	event_time = gtk_get_current_event_time ();

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

	g_object_ref_sink(menu);
	g_object_unref(menu);
}

static gboolean swatch_button_press_cb(GtkWidget *widget, GdkEventButton *event, ColorPickerArgs* args){

	GtkWidget *menu;
	int color_index = gtk_swatch_get_color_at(GTK_SWATCH(widget), event->x, event->y);

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS && color_index>0){
		GtkWidget* item ;
		gint32 button, event_time;

		menu = gtk_menu_new ();

		Color c;
		gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

	    item = gtk_menu_item_new_with_image (_("_Add to palette"), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_menu_add_to_palette), args);

	    item = gtk_menu_item_new_with_image (_("A_dd all to palette"), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_menu_add_all_to_palette), args);

	    gtk_menu_shell_append (GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	    item = gtk_menu_item_new_with_mnemonic (_("_Copy to clipboard"));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	    struct ColorObject* color_object;
	    color_object = color_list_new_color_object(args->gs->colors, &c);
	    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (color_object, 0, args->gs));
		color_object_release(color_object);

		gtk_menu_shell_append (GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		item = gtk_menu_item_new_with_image (_("_Edit..."), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (on_swatch_color_edit), args);

		item = gtk_menu_item_new_with_image (_("_Paste"), gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
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

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

		g_object_ref_sink(menu);
		g_object_unref(menu);
	}
	return false;
}



static gboolean on_swatch_focus_change(GtkWidget *widget, GdkEventFocus *event, gpointer data) {
	ColorPickerArgs* args=(ColorPickerArgs*)data;


	if (event->in){
		gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "swatch_focused"), _("Press Spacebar to sample color under mouse pointer"));
	}else{
		gtk_statusbar_pop(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "swatch_focused"));
	}
	return FALSE;
}


static gboolean on_key_up (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	ColorPickerArgs* args=(ColorPickerArgs*)data;

	guint modifiers = gtk_accelerator_get_default_mod_mask();

	switch(event->keyval)
	{
		case GDK_m:
			{
				int x, y;
				gdk_display_get_pointer(gdk_display_get_default(), NULL, &x, &y, NULL);
				math::Vec2<int> position(x, y);
				if ((event->state&modifiers)==GDK_CONTROL_MASK){
					gtk_zoomed_set_mark(GTK_ZOOMED(args->zoomed_display), 1, position);
				}else{
					gtk_zoomed_set_mark(GTK_ZOOMED(args->zoomed_display), 0, position);
				}
			}
			break;

		case GDK_c:
			if ((event->state&modifiers)==GDK_CONTROL_MASK){

				Color c;
				updateMainColor(args);
				gtk_swatch_get_main_color(GTK_SWATCH(args->swatch_display), &c);

				struct ColorObject* color_object;
				color_object = color_list_new_color_object(args->gs->colors, &c);

				Converters *converters = (Converters*)dynv_get_pointer_wd(args->gs->params, "Converters", 0);
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, color_object, 0, args->gs->params);
				}

				color_object_release(color_object);
				return TRUE;
			}
			return FALSE;
			break;

		case GDK_1:
			gtk_swatch_set_active_index(GTK_SWATCH(args->swatch_display), 1);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_2:
			gtk_swatch_set_active_index(GTK_SWATCH(args->swatch_display), 2);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_3:
			gtk_swatch_set_active_index(GTK_SWATCH(args->swatch_display), 3);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_4:
			gtk_swatch_set_active_index(GTK_SWATCH(args->swatch_display), 4);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_5:
			gtk_swatch_set_active_index(GTK_SWATCH(args->swatch_display), 5);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_6:
			gtk_swatch_set_active_index(GTK_SWATCH(args->swatch_display), 6);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_Right:
			gtk_swatch_move_active(GTK_SWATCH(args->swatch_display),1);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_Left:
			gtk_swatch_move_active(GTK_SWATCH(args->swatch_display),-1);
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_space:
			updateMainColor(args);
			gtk_swatch_set_color_to_main(GTK_SWATCH(args->swatch_display));

			if (dynv_get_bool_wd(args->params, "sampler.add_to_palette", true)){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

				struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
				string name=color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
				dynv_set_string(color_object->params, "name", name.c_str());
				color_list_add_color_object(args->gs->colors, color_object, 1);
				color_object_release(color_object);
			}

			if (dynv_get_bool_wd(args->params, "sampler.copy_to_clipboard", true)){
				Color c;
				gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

				struct ColorObject* color_object;
				color_object = color_list_new_color_object(args->gs->colors, &c);

				Converters *converters = (Converters*)dynv_get_pointer_wd(args->gs->params, "Converters", 0);
				Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
				if (converter){
					converter_get_clipboard(converter->function_name, color_object, 0, args->gs->params);
				}

				color_object_release(color_object);
			}

			if (dynv_get_bool_wd(args->params, "sampler.rotate_swatch_after_sample", true)){
				gtk_swatch_move_active(GTK_SWATCH(args->swatch_display),1);
			}
			updateDisplays(args, widget);
			return TRUE;
			break;

		case GDK_a:
			on_swatch_menu_add_all_to_palette(NULL, args);
			return TRUE;
			break;

		case GDK_e:
			on_swatch_color_edit(NULL, args);
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
	ColorPickerArgs* args=(ColorPickerArgs*)data;
	sampler_set_oversample(args->gs->sampler, (int)gtk_range_get_value(GTK_RANGE(slider)));
}

static void on_zoom_value_changed(GtkRange *slider, gpointer data){
	ColorPickerArgs* args=(ColorPickerArgs*)data;
	gtk_zoomed_set_zoom(GTK_ZOOMED(args->zoomed_display), gtk_range_get_value(GTK_RANGE(slider)));
}

static void color_component_change_value(GtkWidget *widget, Color* c, ColorPickerArgs* args){
	gtk_swatch_set_active_color(GTK_SWATCH(args->swatch_display), c);
	updateDisplays(args, widget);
}

static void color_component_input_clicked(GtkWidget *widget, int component_id, ColorPickerArgs* args){
	dialog_color_component_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), GTK_COLOR_COMPONENT(widget), component_id, dynv_get_dynv(args->params, "component_edit"));
}

static void ser_decimal_get(GtkColorComponentComp component, int component_id, Color* color, const char *text){
	double v;
	stringstream ss(text);
	ss >> v;
	switch (component){
		case hsv:
		case hsl:
			if (component_id == 0){
				color->ma[component_id] = v / 360;
			}else{
				color->ma[component_id] = v / 100;
			}
			break;
		default:
			color->ma[component_id] = v / 100;
	}
}

static string ser_decimal_set(GtkColorComponentComp component, int component_id, Color* color){
    stringstream ss;
    switch (component){
		case hsv:
		case hsl:
			if (component_id == 0){
				ss << setprecision(0) << fixed << color->ma[component_id] * 360;
			}else{
				ss << setprecision(0) << fixed << color->ma[component_id] * 100;
			}
			break;
		default:
			ss << setprecision(0) << fixed << color->ma[component_id] * 100;
	}
    return ss.str();
}

static struct{
	const char *name;
	const char *human_name;
	void (*get)(GtkColorComponentComp component, int component_id, Color* color, const char *text);
	string (*set)(GtkColorComponentComp component, int component_id, Color* color);
}serial[] = {
	{"decimal", "Decimal", ser_decimal_get, ser_decimal_set},
};



static void color_component_copy(GtkWidget *widget, ColorPickerArgs* args){
	struct ColorCompItem *comp_item = (struct ColorCompItem*)g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "comp_item");
	const char *text = gtk_color_component_get_text(GTK_COLOR_COMPONENT(comp_item->widget), comp_item->component_id);
	if (text){
		gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, strlen(text));
		gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	}
}

static void color_component_paste(GtkWidget *widget, ColorPickerArgs* args){
	Color color;
	struct ColorCompItem *comp_item = (struct ColorCompItem*)g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "comp_item");
	gtk_color_component_get_transformed_color(GTK_COLOR_COMPONENT(comp_item->widget), &color);

	gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
		serial[0].get(comp_item->component, comp_item->component_id, &color, text);
		gtk_color_component_set_transformed_color(GTK_COLOR_COMPONENT(comp_item->widget), &color);
		g_free(text);

		gtk_color_component_get_color(GTK_COLOR_COMPONENT(comp_item->widget), &color);
		gtk_swatch_set_active_color(GTK_SWATCH(args->swatch_display), &color);
		updateDisplays(args, comp_item->widget);
	}
}

static void color_component_edit(GtkWidget *widget, ColorPickerArgs* args){
	struct ColorCompItem *comp_item = (struct ColorCompItem*)g_object_get_data(G_OBJECT(gtk_widget_get_parent(widget)), "comp_item");
	dialog_color_component_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), GTK_COLOR_COMPONENT(comp_item->widget), comp_item->component_id, dynv_get_dynv(args->params, "component_edit"));
}

static void destroy_comp_item(struct ColorCompItem *comp_item){
	delete comp_item;
}

static gboolean color_component_key_up_cb(GtkWidget *widget, GdkEventButton *event, ColorPickerArgs* args){

	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)){

		GtkWidget *menu;
		GtkWidget* item;
		gint32 button, event_time;

		menu = gtk_menu_new ();

		struct ColorCompItem *comp_item = new struct ColorCompItem;
		comp_item->widget = widget;
		comp_item->component_id = gtk_color_component_get_component_id_at(GTK_COLOR_COMPONENT(widget), event->x, event->y);
		comp_item->component = gtk_color_component_get_component(GTK_COLOR_COMPONENT(widget));

		g_object_set_data_full(G_OBJECT(menu), "comp_item", comp_item, (GDestroyNotify)destroy_comp_item);

		item = gtk_menu_item_new_with_image(_("Copy"), gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(color_component_copy), args);

		item = gtk_menu_item_new_with_image(_("Paste"), gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(color_component_paste), args);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		item = gtk_menu_item_new_with_image(_("Edit"), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(color_component_edit), args);

		gtk_widget_show_all(GTK_WIDGET(menu));

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
		return true;
	}

	return false;
}


static void on_oversample_falloff_changed(GtkWidget *widget, gpointer data) {
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));

		gint32 falloff_id;
		gtk_tree_model_get(model, &iter, 2, &falloff_id, -1);

		ColorPickerArgs* args = (ColorPickerArgs*)data;
		sampler_set_falloff(args->gs->sampler, (enum SamplerFalloff) falloff_id);

	}
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
			{ "gpick-falloff-none", _("None")},
			{ "gpick-falloff-linear", _("Linear")},
			{ "gpick-falloff-quadratic", _("Quadratic")},
			{ "gpick-falloff-cubic", _("Cubic")},
			{ "gpick-falloff-exponential", _("Exponential")},
			};

	gint32 falloff_type_ids[]={
		NONE,
		LINEAR,
		QUADRATIC,
		CUBIC,
		EXPONENTIAL,
	};

	GtkIconTheme *icon_theme;
	icon_theme = gtk_icon_theme_get_default ();

	gint icon_size;
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, 0, &icon_size);

	for (guint32 i=0;i<sizeof(falloff_type_ids)/sizeof(gint32);++i){
		GError *error = NULL;

		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, falloff_types[i][0], icon_size, GtkIconLookupFlags(0), &error);
		if (error) g_error_free (error);

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



static int source_destroy(ColorPickerArgs *args){

	dynv_set_int32(args->params, "swatch.active_color", gtk_swatch_get_active_index(GTK_SWATCH(args->swatch_display)));

	Color c;

	char tmp[32];
	for (gint i=1; i<7; ++i){
		sprintf(tmp, "swatch.color%d", i);
		gtk_swatch_get_color(GTK_SWATCH(args->swatch_display), i, &c);
		dynv_set_color(args->params, tmp, &c);
	}

	dynv_set_int32(args->params, "sampler.oversample", sampler_get_oversample(args->gs->sampler));
	dynv_set_int32(args->params, "sampler.falloff", sampler_get_falloff(args->gs->sampler));

	dynv_set_float(args->params, "zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(args->zoomed_display)));
	dynv_set_int32(args->params, "zoom_size", gtk_zoomed_get_size(GTK_ZOOMED(args->zoomed_display)));

	dynv_set_bool(args->params, "expander.settings", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderSettings)));
	dynv_set_bool(args->params, "expander.rgb", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderRGB)));
	dynv_set_bool(args->params, "expander.hsv", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderHSV)));
	dynv_set_bool(args->params, "expander.hsl", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderHSL)));
	dynv_set_bool(args->params, "expander.lab", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderLAB)));
	dynv_set_bool(args->params, "expander.lch", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderLCH)));
	dynv_set_bool(args->params, "expander.cmyk", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderCMYK)));
	dynv_set_bool(args->params, "expander.info", gtk_expander_get_expanded(GTK_EXPANDER(args->expanderInfo)));

	gtk_color_get_color(GTK_COLOR(args->contrastCheck), &c);
	dynv_set_color(args->params, "contrast.color", &c);

	gtk_widget_destroy(args->main);

	dynv_system_release(args->params);
	dynv_system_release(args->global_params);
	delete args;
	return 0;
}

static int source_get_color(ColorPickerArgs *args, ColorObject** color){

	Color c;
	gtk_swatch_get_active_color(GTK_SWATCH(args->swatch_display), &c);

	struct ColorObject *color_object = color_list_new_color_object(args->gs->colors, &c);
	string name = color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
	dynv_set_string(color_object->params, "name", name.c_str());

	*color = color_object;

	return 0;
}

static int source_set_nth_color(ColorPickerArgs *args, uint32_t color_n, ColorObject* color){
	if (color_n < 0 || color_n > 6) return -1;
	Color c;
	color_object_get_color(color, &c);
	gtk_swatch_set_color(GTK_SWATCH(args->swatch_display), color_n + 1, &c);

	updateDisplays(args, 0);
	return 0;
}

static int source_get_nth_color(ColorPickerArgs *args, uint32_t color_n, ColorObject** color){
	if (color_n < 0 || color_n > 6) return -1;
	Color c;

	gtk_swatch_get_color(GTK_SWATCH(args->swatch_display), color_n + 1, &c);

	struct ColorObject *color_object = color_list_new_color_object(args->gs->colors, &c);
	string name = color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
	dynv_set_string(color_object->params, "name", name.c_str());

	*color = color_object;

	return 0;
}


static int source_set_color(ColorPickerArgs *args, ColorObject* color){
	Color c;
	color_object_get_color(color, &c);
	gtk_swatch_set_active_color(GTK_SWATCH(args->swatch_display), &c);

	updateDisplays(args, 0);
	return 0;
}

static int source_activate(ColorPickerArgs *args){

	if (args->timeout_source_id > 0) {
		g_source_remove(args->timeout_source_id);
		args->timeout_source_id = 0;
	}

  struct{
		GtkWidget *widget;
		const char *setting;
	}color_spaces[] = {
		{args->expanderCMYK, "color_space.cmyk"},
		{args->expanderHSL, "color_space.hsl"},
		{args->expanderHSV, "color_space.hsv"},
		{args->expanderLAB, "color_space.lab"},
		{args->expanderLCH, "color_space.lch"},
		{args->expanderRGB, "color_space.rgb"},
		{0, 0},
	};
	for (int i = 0; color_spaces[i].setting; i++){
		if (dynv_get_bool_wd(args->params, color_spaces[i].setting, true))
			gtk_widget_show(color_spaces[i].widget);
		else
			gtk_widget_hide(color_spaces[i].widget);
	}

	bool out_of_gamut_mask = dynv_get_bool_wd(args->params, "out_of_gamut_mask", true);

	gtk_color_component_set_out_of_gamut_mask(GTK_COLOR_COMPONENT(args->lab_control), out_of_gamut_mask);
	gtk_color_component_set_lab_illuminant(GTK_COLOR_COMPONENT(args->lab_control), color_get_illuminant(dynv_get_string_wd(args->params, "lab.illuminant", "D50")));
	gtk_color_component_set_lab_observer(GTK_COLOR_COMPONENT(args->lab_control), color_get_observer(dynv_get_string_wd(args->params, "lab.observer", "2")));
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lab_control), "lab");

	gtk_color_component_set_out_of_gamut_mask(GTK_COLOR_COMPONENT(args->lch_control), out_of_gamut_mask);
	gtk_color_component_set_lab_illuminant(GTK_COLOR_COMPONENT(args->lch_control), color_get_illuminant(dynv_get_string_wd(args->params, "lab.illuminant", "D50")));
	gtk_color_component_set_lab_observer(GTK_COLOR_COMPONENT(args->lch_control), color_get_observer(dynv_get_string_wd(args->params, "lab.observer", "2")));
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lch_control), "lch");

	transformation::Chain *chain = static_cast<transformation::Chain*>(dynv_get_pointer_wdc(args->gs->params, "TransformationChain", 0));
	gtk_swatch_set_transformation_chain(GTK_SWATCH(args->swatch_display), chain);
	gtk_color_set_transformation_chain(GTK_COLOR(args->color_code), chain);
	gtk_color_set_transformation_chain(GTK_COLOR(args->contrastCheck), chain);

	if (dynv_get_bool_wd(args->params, "zoomed_enabled", true)){
		float refresh_rate = dynv_get_float_wd(args->global_params, "refresh_rate", 30);
		args->timeout_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000/refresh_rate, (GSourceFunc)updateMainColorTimer, args, (GDestroyNotify)NULL);
	}

	gtk_zoomed_set_size(GTK_ZOOMED(args->zoomed_display), dynv_get_int32_wd(args->params, "zoom_size", 150));

	gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "focus_swatch"), "Click on swatch area to begin adding colors to palette");

	return 0;
}

static int source_deactivate(ColorPickerArgs *args){

	gtk_statusbar_pop(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "focus_swatch"));

	if (args->timeout_source_id > 0){
		g_source_remove(args->timeout_source_id);
		args->timeout_source_id = 0;
	}
	return 0;
}


static struct ColorObject* get_color_object(struct DragDrop* dd){
	ColorPickerArgs* args=(ColorPickerArgs*)dd->userdata;
	ColorObject *colorobject;
	source_get_color(args, &colorobject);
	return colorobject;
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	gint color_index = gtk_swatch_get_color_at(GTK_SWATCH(dd->widget), x, y);
	Color c;
	color_object_get_color(colorobject, &c);
	gtk_swatch_set_color(GTK_SWATCH(dd->widget), color_index, &c);

	updateDisplays((ColorPickerArgs*)dd->userdata, 0);

	return 0;
}

static bool test_at(struct DragDrop* dd, int x, int y){
	gint color_index = gtk_swatch_get_color_at(GTK_SWATCH(dd->widget), x, y);
	if (color_index>0) return true;
	return false;
}

void color_picker_set_floating_picker(ColorSource *color_source, FloatingPicker floating_picker){
	ColorPickerArgs* args = (ColorPickerArgs*)color_source;
	args->floating_picker = floating_picker;
}

static struct ColorObject* get_color_object_contrast(struct DragDrop* dd){
	ColorPickerArgs* args = static_cast<ColorPickerArgs*>(dd->userdata);
	Color c;
	gtk_color_get_color(GTK_COLOR(dd->widget), &c);
	struct ColorObject* colorobject = color_object_new(dd->handler_map);
	color_object_set_color(colorobject, &c);

	string name = color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
	dynv_set_string(colorobject->params, "name", name.c_str());

	return colorobject;
}

static int set_color_object_at_contrast(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	ColorPickerArgs* args = static_cast<ColorPickerArgs*>(dd->userdata);
	Color c;
	color_object_get_color(colorobject, &c);
	gtk_color_set_color(GTK_COLOR(args->contrastCheck), &c, "Sample");
	updateDisplays((ColorPickerArgs*)dd->userdata, 0);
	return 0;
}

static void show_dialog_converter(GtkWidget *widget, ColorPickerArgs *args){
	dialog_converter_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), args->gs);
	return;
}

static void on_zoomed_activate(GtkWidget *widget, ColorPickerArgs *args){
	if (dynv_get_bool_wd(args->params, "zoomed_enabled", true)){
		gtk_zoomed_set_fade(GTK_ZOOMED(args->zoomed_display), true);
		dynv_set_bool(args->params, "zoomed_enabled", false);

		if (args->timeout_source_id > 0){
			g_source_remove(args->timeout_source_id);
			args->timeout_source_id = 0;
		}
	}else{
		gtk_zoomed_set_fade(GTK_ZOOMED(args->zoomed_display), false);
		dynv_set_bool(args->params, "zoomed_enabled", true);

		if (args->timeout_source_id > 0){
			g_source_remove(args->timeout_source_id);
			args->timeout_source_id = 0;
		}
		float refresh_rate = dynv_get_float_wd(args->global_params, "refresh_rate", 30);
		args->timeout_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000/refresh_rate, (GSourceFunc)updateMainColorTimer, args, (GDestroyNotify)NULL);
	}
	return;
}

static ColorSource* source_implement(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace){
	ColorPickerArgs* args = new ColorPickerArgs;

	args->params = dynv_system_ref(dynv_namespace);
	args->global_params = dynv_get_dynv(gs->params, "gpick.picker");
	args->statusbar = (GtkWidget*)dynv_get_pointer_wd(gs->params, "StatusBar", 0);
	args->floating_picker = 0;

	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.get_nth_color = (int (*)(ColorSource *source, uint32_t color_n, ColorObject** color))source_get_nth_color;
	args->source.set_nth_color = (int (*)(ColorSource *source, uint32_t color_n, ColorObject* color))source_set_nth_color;
	args->source.activate = (int (*)(ColorSource *source))source_activate;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;

	args->gs = gs;
	args->timeout_source_id = 0;

	GtkWidget *vbox, *widget, *expander, *table, *main_hbox, *scrolled;
	int table_y;

	main_hbox = gtk_hbox_new(false, 5);

		vbox = gtk_vbox_new(false, 5);
		gtk_box_pack_start(GTK_BOX(main_hbox), vbox, false, false, 0);

			widget = gtk_swatch_new();
			gtk_box_pack_start(GTK_BOX(vbox), widget, false, false, 0);

			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(on_swatch_focus_change), args);
			g_signal_connect(G_OBJECT(widget), "focus-out-event", G_CALLBACK(on_swatch_focus_change), args);

			g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(on_key_up), args);
			g_signal_connect(G_OBJECT(widget), "active_color_changed", G_CALLBACK(on_swatch_active_color_changed), args);
			g_signal_connect(G_OBJECT(widget), "color_changed", G_CALLBACK(on_swatch_color_changed), args);
			g_signal_connect(G_OBJECT(widget), "color_activated", G_CALLBACK(on_swatch_color_activated), args);
			g_signal_connect(G_OBJECT(widget), "center_activated", G_CALLBACK(on_swatch_center_activated), args);
			g_signal_connect_after(G_OBJECT(widget), "button-press-event",G_CALLBACK(swatch_button_press_cb), args);
			g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(swatch_popup_menu_cb), args);

			gtk_swatch_set_active_index(GTK_SWATCH(widget), dynv_get_int32_wd(args->params, "swatch.active_color", 1));

			args->swatch_display = widget;

			gtk_drag_dest_set(widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
			gtk_drag_source_set(widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);

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
			g_signal_connect(G_OBJECT(args->color_code), "activated", G_CALLBACK(show_dialog_converter), args);


			args->zoomed_display = gtk_zoomed_new();
			if (!dynv_get_bool_wd(args->params, "zoomed_enabled", true)){
				gtk_zoomed_set_fade(GTK_ZOOMED(args->zoomed_display), true);
			}
			gtk_zoomed_set_size(GTK_ZOOMED(args->zoomed_display), dynv_get_int32_wd(args->params, "zoom_size", 150));
			gtk_box_pack_start (GTK_BOX(vbox), args->zoomed_display, false, false, 0);
			g_signal_connect(G_OBJECT(args->zoomed_display), "activated", G_CALLBACK(on_zoomed_activate), args);


		scrolled = gtk_scrolled_window_new(0, 0);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start (GTK_BOX(main_hbox), scrolled, true, true, 0);

		vbox = gtk_vbox_new(false, 5);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), vbox);

			expander=gtk_expander_new(_("Settings"));
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.settings", false));
			args->expanderSettings=expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			table = gtk_table_new(6, 2, FALSE);
			table_y=0;

			gtk_container_add(GTK_CONTAINER(expander), table);

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Oversample:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_hscale_new_with_range (0,16,1);
				g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_oversample_value_changed), args);
				gtk_range_set_value(GTK_RANGE(widget), dynv_get_int32_wd(args->params, "sampler.oversample", 0));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Falloff:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = create_falloff_type_list();
				g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (on_oversample_falloff_changed), args);
				gtk_combo_box_set_active(GTK_COMBO_BOX(widget), dynv_get_int32_wd(args->params, "sampler.falloff", NONE));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

				gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Zoom:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
				widget = gtk_hscale_new_with_range (0, 100, 1);
				g_signal_connect (G_OBJECT (widget), "value-changed", G_CALLBACK (on_zoom_value_changed), args);
				gtk_range_set_value(GTK_RANGE(widget), dynv_get_float_wd(args->params, "zoom", 20));
				gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
				table_y++;

			expander=gtk_expander_new("HSV");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.hsv", false));
			args->expanderHSV=expander;
			gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				widget = gtk_color_component_new(hsv);
				const char *hsv_labels[] = {"H", _("Hue"), "S", _("Saturation"), "V", _("Value"), NULL};
				gtk_color_component_set_label(GTK_COLOR_COMPONENT(widget), hsv_labels);
				args->hsv_control = widget;
				g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(color_component_change_value), args);
				g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(color_component_key_up_cb), args);
				g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(color_component_input_clicked), args);
				gtk_container_add(GTK_CONTAINER(expander), widget);

			expander=gtk_expander_new("HSL");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.hsl", false));
			args->expanderHSL = expander;
			gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				widget = gtk_color_component_new(hsl);
				const char *hsl_labels[] = {"H", _("Hue"), "S", _("Saturation"), "L", _("Lightness"), NULL};
				gtk_color_component_set_label(GTK_COLOR_COMPONENT(widget), hsl_labels);
				args->hsl_control = widget;
				g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(color_component_change_value), args);
				g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(color_component_key_up_cb), args);
				g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(color_component_input_clicked), args);
				gtk_container_add(GTK_CONTAINER(expander), widget);

			expander=gtk_expander_new("RGB");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.rgb", false));
			args->expanderRGB = expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				widget = gtk_color_component_new(rgb);
				const char *rgb_labels[] = {"R", _("Red"), "G", _("Green"), "B", _("Blue"), NULL};
				gtk_color_component_set_label(GTK_COLOR_COMPONENT(widget), rgb_labels);
				args->rgb_control = widget;
				g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(color_component_change_value), args);
				g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(color_component_key_up_cb), args);
				g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(color_component_input_clicked), args);
				gtk_container_add(GTK_CONTAINER(expander), widget);

			expander=gtk_expander_new("CMYK");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.cmyk", false));
			args->expanderCMYK = expander;
			gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				widget = gtk_color_component_new(cmyk);
				const char *cmyk_labels[] = {"C", _("Cyan"), "M", _("Magenta"), "Y", _("Yellow"), "K", _("Key"), NULL};
				gtk_color_component_set_label(GTK_COLOR_COMPONENT(widget), cmyk_labels);
				args->cmyk_control = widget;
				g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(color_component_change_value), args);
				g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(color_component_key_up_cb), args);
				g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(color_component_input_clicked), args);
				gtk_container_add(GTK_CONTAINER(expander), widget);

			expander = gtk_expander_new("Lab");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.lab", false));
			args->expanderLAB = expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				widget = gtk_color_component_new(lab);
				const char *lab_labels[] = {"L", _("Lightness"), "a", "a", "b", "b", NULL};
				gtk_color_component_set_label(GTK_COLOR_COMPONENT(widget), lab_labels);
				args->lab_control = widget;
				g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(color_component_change_value), args);
				g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(color_component_key_up_cb), args);
				g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(color_component_input_clicked), args);
				gtk_container_add(GTK_CONTAINER(expander), widget);

			expander = gtk_expander_new("LCH");
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.lch", false));
			args->expanderLCH = expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				widget = gtk_color_component_new(lch);
				const char *lch_labels[] = {"L", _("Lightness"), "C", "Chroma", "H", "Hue", NULL};
				gtk_color_component_set_label(GTK_COLOR_COMPONENT(widget), lch_labels);
				args->lch_control = widget;
				g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(color_component_change_value), args);
				g_signal_connect(G_OBJECT(widget), "button_release_event", G_CALLBACK(color_component_key_up_cb), args);
				g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(color_component_input_clicked), args);
				gtk_container_add(GTK_CONTAINER(expander), widget);

			expander=gtk_expander_new(_("Info"));
			gtk_expander_set_expanded(GTK_EXPANDER(expander), dynv_get_bool_wd(args->params, "expander.info", false));
			args->expanderInfo=expander;
			gtk_box_pack_start (GTK_BOX(vbox), expander, FALSE, FALSE, 0);

				table = gtk_table_new(3, 2, FALSE);
				table_y=0;
				gtk_container_add(GTK_CONTAINER(expander), table);

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Color name:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_entry_new();
					gtk_table_attach(GTK_TABLE(table), widget,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					gtk_editable_set_editable(GTK_EDITABLE(widget), FALSE);
					//gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
					args->color_name = widget;
					table_y++;

					dragdrop_init(&dd, gs);
					dd.userdata = args;
					dd.get_color_object = get_color_object_contrast;
					dd.set_color_object_at = set_color_object_at_contrast;

					gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Contrast:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
					widget = gtk_color_new();

					Color *c;
					if ((c = const_cast<Color*>(dynv_get_color_wd(args->params, "contrast.color", 0)))){
						gtk_color_set_color(GTK_COLOR(widget), c, _("Sample"));
					}else{
						Color c;
						c.rgb.red = 1;
						c.rgb.green = 1;
						c.rgb.blue = 1;
						gtk_color_set_color(GTK_COLOR(widget), &c, _("Sample"));
					}
					gtk_color_set_rounded(GTK_COLOR(widget), true);
					gtk_color_set_hcenter(GTK_COLOR(widget), true);
					gtk_color_set_roundness(GTK_COLOR(widget), 5);
					gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
					args->contrastCheck = widget;

					gtk_drag_dest_set(widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
					gtk_drag_source_set(widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
					dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
					dd.userdata2 = 0;
					dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

					gtk_table_attach(GTK_TABLE(table), args->contrastCheckMsg = gtk_label_new(""),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,5);

					table_y++;


	updateDisplays(args, 0);


	args->main = main_hbox;

	gtk_widget_show_all(main_hbox);

    args->source.widget = main_hbox;

	return (ColorSource*)args;
}

int color_picker_source_register(ColorSourceManager *csm){
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "color_picker", _("Color picker"));
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace))source_implement;
	color_source->single_instance_only = true;
	color_source->default_accelerator = GDK_c;
	color_source_manager_add_source(csm, color_source);
	return 0;
}

