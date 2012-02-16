/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#include "BlendColors.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"
#include "DragDrop.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "ColorRYB.h"
#include "gtk/ColorWidget.h"
#include "uiColorInput.h"
#include "CopyPaste.h"
#include "Converter.h"
#include "DynvHelpers.h"
#include "uiApp.h"
#include "Internationalisation.h"

#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>

#include <stdbool.h>
#include <sstream>

using namespace std;

#define STORE_COLOR() struct ColorObject *color_object=color_list_new_color_object(color_list, &r); \
	dynv_set_string(color_object->params, "name", color_names_get(args->gs->color_names, &r, imprecision_postfix).c_str()); \
	color_list_add_color_object(color_list, color_object, 1); \
	color_object_release(color_object)

#define STORE_LINEARCOLOR() color_linear_get_rgb(&r, &r); \
	STORE_COLOR()


typedef struct BlendColorsArgs{
	ColorSource source;

	GtkWidget *main;

	GtkWidget *mix_type;
	GtkWidget *steps1;
	GtkWidget *steps2;

	GtkWidget *start_color;
	GtkWidget *middle_color;
	GtkWidget *end_color;

	GtkWidget *preview_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
}BlendColorsArgs;

static int source_get_color(BlendColorsArgs *args, struct ColorObject** color);

static void calc( BlendColorsArgs *args, bool preview, int limit){

	gint steps1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps1));
	gint steps2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps2));
	gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));

	Color r;
	gint step_i;

	stringstream s;
	s.precision(0);
	s.setf(ios::fixed,ios::floatfield);

	Color a,b;
	matrix3x3 adaptation_matrix, working_space_matrix, working_space_matrix_inverted;
	vector3 d50, d65;
	if (type == 3) {
	    SETUP_LAB (d50,d65,adaptation_matrix,working_space_matrix,working_space_matrix_inverted);
	}

	struct ColorList *color_list;
	color_list = args->preview_color_list;

	bool imprecision_postfix = dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true);

	int steps;
	for (int stage = 0; stage < 2; stage++){
		if (stage == 0){
			steps = steps1 + 1;
			gtk_color_get_color(GTK_COLOR(args->start_color), &a);
			gtk_color_get_color(GTK_COLOR(args->middle_color), &b);
		}else{
			steps = steps2 + 1;
			gtk_color_get_color(GTK_COLOR(args->middle_color), &a);
			gtk_color_get_color(GTK_COLOR(args->end_color), &b);
		}

		if (type == 0){
			color_rgb_get_linear(&a, &a);
			color_rgb_get_linear(&b, &b);
		}
		step_i = stage;


		switch (type) {
		case 0:
			for (; step_i < steps; ++step_i) {
				MIX_COMPONENTS(r.rgb, a.rgb, b.rgb, red, green, blue);
				STORE_LINEARCOLOR();
			}
			break;

		case 1:
			{
				Color a_hsv, b_hsv, r_hsv;
				color_rgb_to_hsv(&a, &a_hsv);
				color_rgb_to_hsv(&b, &b_hsv);

				for (; step_i < steps; ++step_i) {
					MIX_COMPONENTS(r_hsv.hsv, a_hsv.hsv, b_hsv.hsv, hue, saturation, value);
					color_hsv_to_rgb(&r_hsv, &r);
					STORE_COLOR();

				}
			}
			break;

		case 2:
			{
				Color a_hsv, b_hsv, r_hsv;
				color_rgb_to_hsv(&a, &a_hsv);
				color_rgb_to_hsv(&b, &b_hsv);

				if (a_hsv.hsv.hue>b_hsv.hsv.hue){
					if (a_hsv.hsv.hue-b_hsv.hsv.hue>0.5)
						a_hsv.hsv.hue-=1;
				}else{
					if (b_hsv.hsv.hue-a_hsv.hsv.hue>0.5)
						b_hsv.hsv.hue-=1;
				}
				for (; step_i < steps; ++step_i) {
					MIX_COMPONENTS(r_hsv.hsv, a_hsv.hsv, b_hsv.hsv, hue, saturation, value);
					if (r_hsv.hsv.hue<0) r_hsv.hsv.hue+=1;
					color_hsv_to_rgb(&r_hsv, &r);
					STORE_COLOR();
				}
			}
			break;

		case 3:
			{
				Color a_lab, b_lab, r_lab;
				color_rgb_to_lab(&a, &a_lab, &d50, &working_space_matrix);
				color_rgb_to_lab(&b, &b_lab, &d50, &working_space_matrix);

				for (; step_i < steps; ++step_i) {
					MIX_COMPONENTS(r_lab.lab, a_lab.lab, b_lab.lab, L, a, b);
					color_lab_to_rgb(&r_lab, &r, &d50, &working_space_matrix_inverted);
					color_rgb_normalize(&r);
					STORE_COLOR();
				}
			}
			break;
		}
	}
}

static PaletteListCallbackReturn add_to_palette_cb_helper(struct ColorObject* color_object, void *userdata){
	BlendColorsArgs *args = (BlendColorsArgs*)userdata;
	color_list_add_color_object(args->gs->colors, color_object, 1);
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}

static gboolean add_to_palette_cb(GtkWidget *widget, BlendColorsArgs *args) {
	palette_list_foreach_selected(args->preview_list, add_to_palette_cb_helper, args);
	return true;
}

static gboolean add_all_to_palette_cb(GtkWidget *widget, BlendColorsArgs *args) {
	palette_list_foreach(args->preview_list, add_to_palette_cb_helper, args);
	return true;
}

static PaletteListCallbackReturn color_list_selected(struct ColorObject* color_object, void *userdata){
	color_list_add_color_object((struct ColorList *)userdata, color_object, 1);
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
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

static gboolean preview_list_button_press_cb(GtkWidget *widget, GdkEventButton *event, BlendColorsArgs *args) {
	GtkWidget *menu;

	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS){
		//add_to_palette_cb(widget, args);
		//return true;
	}else if (event->button == 3 && event->type == GDK_BUTTON_PRESS){

		GtkWidget* item ;
		gint32 button, event_time;

		menu = gtk_menu_new ();

		bool selection_avail = palette_list_get_selected_count(widget) != 0;

	    item = gtk_menu_item_new_with_image (_("_Add to palette"), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_to_palette_cb), args);
		if (!selection_avail) gtk_widget_set_sensitive(item, false);


	    item = gtk_menu_item_new_with_image (_("A_dd all to palette"), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_all_to_palette_cb), args);

	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	    item = gtk_menu_item_new_with_mnemonic (_("_Copy to clipboard"));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		if (selection_avail){
			struct ColorList *color_list = color_list_new(NULL);
			palette_list_forfirst_selected(args->preview_list, color_list_selected, color_list);
			if (color_list_get_count(color_list) != 0){
				gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (*color_list->colors.begin(), args->preview_list, args->gs));
			}
			color_list_destroy(color_list);
		}else{
			gtk_widget_set_sensitive(item, false);
		}

		gtk_widget_show_all (GTK_WIDGET(menu));

		button = event->button;
		event_time = event->time;

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

		g_object_ref_sink(menu);
		g_object_unref(menu);

		return TRUE;
	}
	return FALSE;
}


static void update(GtkWidget *widget, BlendColorsArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 101);
}

static void reset_middle_color_cb(GtkWidget *widget, BlendColorsArgs *args){
    Color a, b;
		gtk_color_get_color(GTK_COLOR(args->start_color), &a);
		gtk_color_get_color(GTK_COLOR(args->end_color), &b);
		color_multiply(&a, 0.5);
		color_multiply(&b, 0.5);
		color_add(&a, &b);
		gtk_color_set_color(GTK_COLOR(args->middle_color), &a, "");
		update(0, args);
}

static gboolean color_button_press_cb(GtkWidget *widget, GdkEventButton *event, BlendColorsArgs *args) {
	GtkWidget *menu;

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		GtkWidget* item ;
		gint32 button, event_time;
		menu = gtk_menu_new ();

	    item = gtk_menu_item_new_with_mnemonic(_("_Reset"));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (reset_middle_color_cb), args);

		gtk_widget_show_all (GTK_WIDGET(menu));

		button = event->button;
		event_time = event->time;

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

		g_object_ref_sink(menu);
		g_object_unref(menu);

		return TRUE;
	}
	return FALSE;
}

static int set_rgb_color(BlendColorsArgs *args, struct ColorObject* color, uint32_t color_index){
	Color c;
	color_object_get_color(color, &c);
	if (color_index == 1){
		gtk_color_set_color(GTK_COLOR(args->start_color), &c, "");
	}else if (color_index == 2){
		gtk_color_set_color(GTK_COLOR(args->middle_color), &c, "");
	}else if (color_index == 3){
		gtk_color_set_color(GTK_COLOR(args->end_color), &c, "");
	}
	update(0, args);
	return 0;
}

static int get_rgb_color(BlendColorsArgs *args, uint32_t color_index, struct ColorObject** color){
	Color c;
	if (color_index == 1){
		gtk_color_get_color(GTK_COLOR(args->start_color), &c);
	}else if (color_index == 2){
		gtk_color_get_color(GTK_COLOR(args->middle_color), &c);
	}else if (color_index == 3){
		gtk_color_get_color(GTK_COLOR(args->end_color), &c);
	}

	*color = color_list_new_color_object(args->gs->colors, &c);

	string name = color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
	dynv_set_string((*color)->params, "name", name.c_str());

	return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	BlendColorsArgs* args = (BlendColorsArgs*)dd->userdata;
	struct ColorObject* colorobject;
	if (get_rgb_color(args, (uintptr_t)dd->userdata2, &colorobject) == 0){
		return colorobject;
	}
	return 0;
}



static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	BlendColorsArgs* args = static_cast<BlendColorsArgs*>(dd->userdata);
	set_rgb_color(args, colorobject, (uintptr_t)dd->userdata2);
	return 0;
}

static int source_get_color(BlendColorsArgs *args, struct ColorObject** color){
	return -1;
}

static int source_set_color(BlendColorsArgs *args, struct ColorObject* color){
	return -1;
}

static int source_activate(BlendColorsArgs *args){
	return 0;
}


static int source_deactivate(BlendColorsArgs *args){
	return 0;
}

static int source_destroy(BlendColorsArgs *args){
	gint steps1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps1));
	gint steps2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps2));
	gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));
	dynv_set_int32(args->params, "type", type);
	dynv_set_int32(args->params, "steps1", steps1);
	dynv_set_int32(args->params, "steps2", steps2);

	Color c;
	gtk_color_get_color(GTK_COLOR(args->start_color), &c);
	dynv_set_color(args->params, "start_color", &c);
	gtk_color_get_color(GTK_COLOR(args->middle_color), &c);
	dynv_set_color(args->params, "middle_color", &c);
	gtk_color_get_color(GTK_COLOR(args->end_color), &c);
	dynv_set_color(args->params, "end_color", &c);

	color_list_destroy(args->preview_color_list);

	dynv_system_release(args->params);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static ColorSource* source_implement(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace){
	BlendColorsArgs *args = new BlendColorsArgs;

	args->params = dynv_system_ref(dynv_namespace);
	args->gs = gs;

	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;


	GtkWidget *table, *widget;
	GtkWidget *mix_type, *mix_steps;

	gint table_y;
	table = gtk_table_new(6, 2, FALSE);
	table_y = 0;

	struct DragDrop dd;
	dragdrop_init(&dd, gs);

	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;

	Color c;
	color_set(&c, 0.5);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Start:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->start_color = widget = gtk_color_new();
	gtk_color_set_color(GTK_COLOR(args->start_color), dynv_get_color_wdc(args->params, "start_color", &c), "");
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata2 = (void*)1;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Middle:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->middle_color = widget = gtk_color_new();

	g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(color_button_press_cb), args);
	gtk_color_set_color(GTK_COLOR(args->middle_color), dynv_get_color_wdc(args->params, "middle_color", &c), "");
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata2 = (void*)2;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("End:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->end_color = widget = gtk_color_new();
	gtk_color_set_color(GTK_COLOR(args->end_color), dynv_get_color_wdc(args->params, "end_color", &c), "");
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);

	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata2 = (void*)3;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	table_y = 0;

	GtkWidget* vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_aligned_new(_("Type:"),0,0,0,0), false, false, 0);
	args->mix_type = mix_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("RGB"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("HSV"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("HSV shortest hue distance"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("LAB"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(mix_type), dynv_get_int32_wd(args->params, "type", 0));
	gtk_box_pack_start(GTK_BOX(vbox), mix_type, false, false, 0);
	g_signal_connect(G_OBJECT(mix_type), "changed", G_CALLBACK (update), args);
	gtk_table_attach(GTK_TABLE(table), vbox, 4, 5, table_y, table_y+3, GtkAttachOptions(GTK_FILL),GtkAttachOptions(GTK_FILL),5,0);

	table_y = 0;


	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Start steps:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range (1,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), dynv_get_int32_wd(args->params, "steps1", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->steps1 = mix_steps;
	g_signal_connect(G_OBJECT(mix_steps), "value-changed", G_CALLBACK (update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("End steps:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range (1,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), dynv_get_int32_wd(args->params, "steps2", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->steps2 = mix_steps;
	g_signal_connect(G_OBJECT(mix_steps), "value-changed", G_CALLBACK (update), args);

  table_y = 3;

	GtkWidget* preview;
	struct ColorList* preview_color_list = NULL;
	gtk_table_attach(GTK_TABLE(table), preview = palette_list_preview_new(gs, false, false, gs->colors, &preview_color_list), 0, 5, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);

	args->preview_list = palette_list_get_widget(preview_color_list);

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->preview_list));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect(G_OBJECT(args->preview_list), "button-press-event", G_CALLBACK(preview_list_button_press_cb), args);
	table_y++;

	args->preview_color_list = preview_color_list;

	update(0, args);

	gtk_widget_show_all(table);

	args->main = table;
	args->source.widget = table;

	return (ColorSource*)args;
}


int blend_colors_source_register(ColorSourceManager *csm){
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "blend_colors", _("Blend colors"));
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace))source_implement;
	color_source_manager_add_source(csm, color_source);
	return 0;
}

