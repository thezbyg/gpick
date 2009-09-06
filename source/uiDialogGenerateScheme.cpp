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

#include "uiDialogGenerateScheme.h"
#include "uiUtilities.h"
#include "uiConverter.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "ColorRYB.h"
#include "gtk/ColorWidget.h"

#include "main.h"

#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;

struct Arguments{
	GtkWidget *gen_type;
	//GtkWidget *range_chaos;
	//GtkWidget *toggle_brightness_correction;
	
	GtkWidget *hue;
	GtkWidget *saturation;
	GtkWidget *lightness;
	
	GtkWidget *color_previews;
	
	GtkWidget *colors[5];
	int colors_visible;
	
	GlobalState* gs;
	struct ColorList *preview_color_list;
};



static void calc( struct Arguments *args, bool preview){

	gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->gen_type));
	//gfloat chaos = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos));
	//gboolean correction = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_brightness_correction));

	double hue = gtk_range_get_value(GTK_RANGE(args->hue));
	double saturation = gtk_range_get_value(GTK_RANGE(args->saturation));
	double lightness = gtk_range_get_value(GTK_RANGE(args->lightness));
	
	if  (!preview){
		g_key_file_set_integer(args->gs->settings, "Generate Scheme Dialog", "Type", type);
		g_key_file_set_double(args->gs->settings, "Generate Scheme Dialog", "Hue", hue);
		g_key_file_set_double(args->gs->settings, "Generate Scheme Dialog", "Saturation", saturation);
		g_key_file_set_double(args->gs->settings, "Generate Scheme Dialog", "Lightness", lightness);
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
	
	struct {
		int32_t colors;
		int32_t turn_types;
		double turn[2];
	}scheme_types[]={
		{1, 1, {180}},
		{4, 1, {30}},
		{2, 1, {120}},
		{2, 2, {150, 60}},
		{3, 2, {60, 120}},
		{3, 1, {90}},
		{4, 1, {15}},
	};
	
	float chaos = 0;
	

	for (step_i = 0; step_i <= scheme_types[type].colors; ++step_i) {

		color_rybhue_to_rgb(hue, &r);
		
		color_rgb_to_hsl(&r, &hsl);
		hsl.hsl.lightness = clamp_float(hsl.hsl.lightness + lightness, 0, 1);
		hsl.hsl.saturation = clamp_float(hsl.hsl.saturation * saturation, 0, 1);	
		color_hsl_to_rgb(&hsl, &r);
		
		struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
		color_list_add_color_object(color_list, color_object, 1);
		
		hue = wrap_float(hue + (scheme_types[type].turn[step_i%scheme_types[type].turn_types]) / (360.0) 
			+ chaos*(((random_get(args->gs->random)&0xFFFFFFFF)/(gdouble)0xFFFFFFFF)-0.5) );

	}
	
	if (preview){
	
		uint32_t total_colors = scheme_types[type].colors+1;
		if (total_colors>5) total_colors=5;
		
		for (int i=args->colors_visible; i>total_colors; --i)
			gtk_widget_hide(args->colors[i-1]);
		for (int i=args->colors_visible; i<total_colors; ++i)
			gtk_widget_show(args->colors[i]);
		args->colors_visible = total_colors;
	
		int j=0;
		char* text;
		for (ColorList::iter i = color_list->colors.begin(); i!=color_list->colors.end(); ++i){
			color_object_get_color(*i, &color);
			
			text = main_get_color_text(args->gs, &color);
			
			gtk_color_set_color(GTK_COLOR(args->colors[j]), &color, text);
			if (text) g_free(text);
			++j;
			if (j>=total_colors) break;
		}
	
	}
}

static void update(GtkWidget *widget, struct Arguments *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true);
}

static void on_color_popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu) {
	gtk_widget_destroy(GTK_WIDGET(menu));
}

static gboolean on_color_button_press (GtkWidget *widget, GdkEventButton *event, struct Arguments* args) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}
		if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
	
		gint32 button, event_time;
	
		Color c;
		gtk_color_get_color(GTK_COLOR(widget), &c);

		struct ColorObject* color_object;
		color_object = color_list_new_color_object(args->gs->colors, &c);
		menu = converter_create_copy_menu (color_object, 0, args->gs->settings, args->gs->lua);
		color_object_release(color_object);

		gtk_widget_show_all (GTK_WIDGET(menu));

		button = 0;
		event_time = gtk_get_current_event_time ();

		gtk_menu_attach_to_widget (GTK_MENU (menu), widget, on_color_popup_menu_detach);
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button, event_time);
		
		return TRUE;	  
	}
	return FALSE;
}

static gchar* format_value_callback (GtkScale *scale, gdouble value){
	return g_strdup_printf ("%d%%", int(value));
}

static gchar* format_value_callback2 (GtkScale *scale, gdouble value){
	if (value>=0)
		return g_strdup_printf ("+%d%%", int(value));
	else
		return g_strdup_printf ("-%d%%", -int(value));
}

void dialog_generate_scheme_show(GtkWindow* parent, struct ColorList *selected_color_list, GlobalState* gs){
	struct Arguments args;
	
	GtkWidget *table, *vbox, *hbox, *widget;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Generate color scheme", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	
	gtk_window_set_default_size(GTK_WINDOW(dialog), g_key_file_get_integer_with_default(gs->settings, "Generate Scheme Dialog", "Width", -1), 
		g_key_file_get_integer_with_default(gs->settings, "Generate Scheme Dialog", "Height", -1));
	
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	vbox = gtk_vbox_new(FALSE, 5);
	
	args.color_previews = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), args.color_previews, TRUE, TRUE, 5);
	
		for (int i=0; i<5; ++i){
			widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_box_pack_start(GTK_BOX(args.color_previews), widget, TRUE, TRUE, 0);
			
			args.colors[i] = widget;
			
			g_signal_connect (G_OBJECT(widget), "button-press-event", G_CALLBACK (on_color_button_press), &args);

		}

	gint table_y;
	table = gtk_table_new(4, 4, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);
	table_y=0;
		
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args.hue = gtk_hscale_new_with_range(0, 360, 1);
	gtk_range_set_value(GTK_RANGE(args.hue), g_key_file_get_double_with_default(gs->settings, "Generate Scheme Dialog", "Hue", 180));
	g_signal_connect (G_OBJECT (args.hue), "value-changed", G_CALLBACK (update), &args);
	gtk_table_attach(GTK_TABLE(table), args.hue,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args.saturation = gtk_hscale_new_with_range(0, 120, 1);
	gtk_range_set_value(GTK_RANGE(args.saturation), g_key_file_get_double_with_default(gs->settings, "Generate Scheme Dialog", "Saturation", 100));
	g_signal_connect (G_OBJECT (args.saturation), "value-changed", G_CALLBACK (update), &args);
	g_signal_connect (G_OBJECT (args.saturation), "format-value", G_CALLBACK (format_value_callback), &args);
	gtk_table_attach(GTK_TABLE(table), args.saturation,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args.lightness = gtk_hscale_new_with_range(-50, 80, 1);
	gtk_range_set_value(GTK_RANGE(args.lightness), g_key_file_get_double_with_default(gs->settings, "Generate Scheme Dialog", "Lightness", 0));
	g_signal_connect (G_OBJECT (args.lightness), "value-changed", G_CALLBACK (update), &args);
	g_signal_connect (G_OBJECT (args.lightness), "format-value", G_CALLBACK (format_value_callback2), &args);
	gtk_table_attach(GTK_TABLE(table), args.lightness,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	table_y=0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:",0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args.gen_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Complementary");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Analogous");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Triadic");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Split-Complementary");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Rectangle (tetradic)");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Square");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args.gen_type), "Neutral");
	gtk_combo_box_set_active(GTK_COMBO_BOX(args.gen_type), g_key_file_get_integer_with_default(gs->settings, "Generate Scheme Dialog", "Type", 0));
	g_signal_connect (G_OBJECT (args.gen_type), "changed", G_CALLBACK (update), &args);
	gtk_table_attach(GTK_TABLE(table), args.gen_type,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,0);
	table_y++;


	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(gs->colors->params);
	struct ColorList* preview_color_list=color_list_new(handler_map);
	dynv_handler_map_release(handler_map);
	
	
	args.preview_color_list = preview_color_list;
	args.colors_visible = 5;
	args.gs = gs;
	
	gtk_widget_show_all(vbox);
	
	if (selected_color_list && color_list_get_count(selected_color_list)>0){
	/*	GdkColor gdkcolor;
		Color color;
		color_object_get_color(*selected_color_list->colors.begin(), &color);*/
		//TODO: set hue, saturation and lighness based on color	
	}
	
	update(0, &args);

	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(&args, false);
	
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	g_key_file_set_integer(gs->settings, "Generate Scheme Dialog", "Width", width);
	g_key_file_set_integer(gs->settings, "Generate Scheme Dialog", "Height", height);
	
	color_list_destroy(preview_color_list);

	gtk_widget_destroy(dialog);

}
