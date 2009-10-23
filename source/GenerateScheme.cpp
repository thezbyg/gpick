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

#include "GenerateScheme.h"
#include "DragDrop.h"

#include "uiUtilities.h"
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
	ColorSource source;
	
	GtkWidget* main;
	
	GtkWidget *gen_type;
	//GtkWidget *range_chaos;
	//GtkWidget *toggle_brightness_correction;
	
	GtkWidget *hue;
	GtkWidget *saturation;
	GtkWidget *lightness;
	
	GtkWidget *color_previews;
	
	GtkWidget *colors[5];
	float color_hue[5];
	int colors_visible;
	
	GlobalState* gs;
	struct ColorList *preview_color_list;
};

static int set_rgb_color(struct Arguments *args, struct ColorObject* color, uint32_t color_index);

static void calc( struct Arguments *args, bool preview, bool save_settings){

	gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->gen_type));
	//gfloat chaos = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos));
	//gboolean correction = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_brightness_correction));

	double hue = gtk_range_get_value(GTK_RANGE(args->hue));
	double saturation = gtk_range_get_value(GTK_RANGE(args->saturation));
	double lightness = gtk_range_get_value(GTK_RANGE(args->lightness));
	
	if (save_settings){
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
	float hue_offset = 0;
	float hue_step;

	for (step_i = 0; step_i <= scheme_types[type].colors; ++step_i) {

		color_rybhue_to_rgb(hue, &r);
		
		color_rgb_to_hsl(&r, &hsl);
		hsl.hsl.lightness = clamp_float(hsl.hsl.lightness + lightness, 0, 1);
		hsl.hsl.saturation = clamp_float(hsl.hsl.saturation * saturation, 0, 1);	
		color_hsl_to_rgb(&hsl, &r);
		
		struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
		dynv_system_set(color_object->params, "float", "hue_offset", (void*)&hue_offset);
		color_list_add_color_object(color_list, color_object, 1);
		color_object_release(color_object);
		
		hue_step = (scheme_types[type].turn[step_i%scheme_types[type].turn_types]) / (360.0) 
			+ chaos*(((random_get(args->gs->random)&0xFFFFFFFF)/(gdouble)0xFFFFFFFF)-0.5);
		
		hue = wrap_float(hue + hue_step);
		
		hue_offset = wrap_float(hue_offset + hue_step);

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
			
			text = main_get_color_text(args->gs, &color, COLOR_TEXT_TYPE_DISPLAY);
			
			args->color_hue[j] = *(float*)dynv_system_get((*i)->params, "float", "hue_offset");
	
			gtk_color_set_color(GTK_COLOR(args->colors[j]), &color, text);
			if (text) g_free(text);
			++j;
			if (j>=total_colors) break;
		}
	
	}
}

static void update(GtkWidget *widget, struct Arguments *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, false);
}

static void on_color_popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu) {
	gtk_widget_destroy(GTK_WIDGET(menu));
}


static void on_color_paste(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	
	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));
	Color c;
	
	gchar* text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
	
		if (main_get_color_from_text(args->gs, (char*)text, &c)==0){
			struct ColorObject* color_object;
			color_object = color_list_new_color_object(args->gs->colors, &c);
			
			for (int i=0; i<args->colors_visible; ++i){
				if (args->colors[i]==color_widget){
					set_rgb_color(args, color_object, i);
					break;
				}
			}
			
			color_object_release(color_object);
		}
		
		g_free(text);
	}
}

static void on_color_edit(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	
	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));
	Color c;
	gtk_color_get_color(GTK_COLOR(color_widget), &c);
	gchar* text = main_get_color_text(args->gs, &c, COLOR_TEXT_TYPE_DISPLAY);
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Edit color", GTK_WINDOW(gtk_widget_get_toplevel(args->main)), GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
			
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	
	GtkWidget* vbox = gtk_vbox_new(false, 5);
	
	GtkWidget* hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_aligned_new("Color:",0,0.5,0,0), false, false, 0);
	
	GtkWidget* entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
	
	gtk_entry_set_text(GTK_ENTRY(entry), text);
	g_free(text);
	
	
	
	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);
	
	//gtk_window_set_default_size(GTK_WINDOW(dialog), 320, 240);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		if (main_get_color_from_text(args->gs, (char*)gtk_entry_get_text(GTK_ENTRY(entry)), &c)==0){
			struct ColorObject* color_object;
			color_object = color_list_new_color_object(args->gs->colors, &c);
			
			for (int i=0; i<args->colors_visible; ++i){
				if (args->colors[i]==color_widget){
					set_rgb_color(args, color_object, i);
					break;
				}
			}
			
			color_object_release(color_object);
		}		
	}
	gtk_widget_destroy(dialog);	
}
	
static void on_color_add_to_palette(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	Color c;
	
	gtk_color_get_color(GTK_COLOR(g_object_get_data(G_OBJECT(widget), "color_widget")), &c);
				
	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static void on_color_add_all_to_palette(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	Color c;
	
	for (int i=0; i<args->colors_visible; ++i){
		gtk_color_get_color(GTK_COLOR(args->colors[i]), &c);
		
		struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
		string name=color_names_get(args->gs->color_names, &c);
		dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
		color_list_add_color_object(args->gs->colors, color_object, 1);
		color_object_release(color_object);
	}		

}


static void on_color_activate(GtkWidget *widget,  gpointer item) {
	struct Arguments* args=(struct Arguments*)item;
	Color c;

	gtk_color_get_color(GTK_COLOR(widget), &c);
			
	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	string name=color_names_get(args->gs->color_names, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)name.c_str());
	color_list_add_color_object(args->gs->colors, color_object, 1);
	color_object_release(color_object);
}

static gboolean on_color_button_press (GtkWidget *widget, GdkEventButton *event, struct Arguments* args) {
	static GtkWidget *menu=NULL;
	if (menu) {
		gtk_menu_detach(GTK_MENU(menu));
		menu=NULL;
	}
	
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
	
		GtkWidget* item ;
		gint32 button, event_time;

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
		
		if (!gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD))){
			gtk_widget_set_sensitive(item, false);
		}
		
		gtk_widget_show_all (GTK_WIDGET(menu));

		button = event->button;
		event_time = event->time;

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

static int source_destroy(struct Arguments *args){
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static int source_get_color(struct Arguments *args, ColorObject** color){
	Color c;
	gtk_color_get_color(GTK_COLOR(args->colors[0]), &c);
	*color = color_list_new_color_object(args->gs->colors, &c);
	return 0;
}

static int set_rgb_color(struct Arguments *args, struct ColorObject* color, uint32_t color_index){
	Color c;
	color_object_get_color(color, &c);
	
	double hue;
	double saturation;
	double lightness;
	double shifted_hue;

	Color hsl, hsl_results;
	color_rgb_to_hsl(&c, &hsl);
	color_rgbhue_to_rybhue(hsl.hsl.hue, &hue);
	
	shifted_hue = wrap_float(hue - args->color_hue[color_index]);
	
	color_rybhue_to_rgb(hue, &c);
	color_rgb_to_hsl(&c, &hsl_results);
	
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
	
	
static int source_set_color(struct Arguments *args, struct ColorObject* color){
	return set_rgb_color(args, color, 0);
}

static int source_deactivate(struct Arguments *args){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, true);
	return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	struct Arguments* args=(struct Arguments*)dd->userdata;
	Color c;
	gtk_color_get_color(GTK_COLOR(dd->widget), &c);
	struct ColorObject* colorobject = color_object_new(dd->handler_map);
	color_object_set_color(colorobject, &c);
	return colorobject;	
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	struct Arguments* args=(struct Arguments*)dd->userdata;
	set_rgb_color(args, colorobject, (uintptr_t)dd->userdata2);
	return 0;
}

ColorSource* generate_scheme_new(GlobalState* gs, GtkWidget **out_widget){
	struct Arguments* args=new struct Arguments;

	color_source_init(&args->source);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	
	GtkWidget *table, *vbox, *hbox, *widget;
	
	hbox = gtk_hbox_new(FALSE, 5);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
	
	args->color_previews = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), args->color_previews, TRUE, TRUE, 5);
	
	struct DragDrop dd;
	dragdrop_init(&dd, gs);
	
	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;

	for (int i=0; i<5; ++i){
		widget = gtk_color_new();
		gtk_color_set_rounded(GTK_COLOR(widget), true);
		gtk_color_set_hcenter(GTK_COLOR(widget), true);
		gtk_box_pack_start(GTK_BOX(args->color_previews), widget, TRUE, TRUE, 0);
		
		args->colors[i] = widget;
		
		g_signal_connect (G_OBJECT(widget), "button-press-event", G_CALLBACK (on_color_button_press), args);
		g_signal_connect (G_OBJECT(widget), "activated", G_CALLBACK (on_color_activate), args);
			
		gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
		gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
		
		dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
		
		dd.userdata2 = (void*)i;
		//if (i==0){
			dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);
		/*}else{
			dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE), &dd);
		}*/
	}

	gint table_y;
	table = gtk_table_new(4, 4, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 5);
	table_y=0;
		
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->hue = gtk_hscale_new_with_range(0, 360, 1);
	gtk_range_set_value(GTK_RANGE(args->hue), g_key_file_get_double_with_default(gs->settings, "Generate Scheme Dialog", "Hue", 180));
	g_signal_connect (G_OBJECT (args->hue), "value-changed", G_CALLBACK (update), args);
	gtk_table_attach(GTK_TABLE(table), args->hue,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->saturation = gtk_hscale_new_with_range(0, 120, 1);
	gtk_range_set_value(GTK_RANGE(args->saturation), g_key_file_get_double_with_default(gs->settings, "Generate Scheme Dialog", "Saturation", 100));
	g_signal_connect (G_OBJECT (args->saturation), "value-changed", G_CALLBACK (update), args);
	g_signal_connect (G_OBJECT (args->saturation), "format-value", G_CALLBACK (format_value_callback), args);
	gtk_table_attach(GTK_TABLE(table), args->saturation,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->lightness = gtk_hscale_new_with_range(-50, 80, 1);
	gtk_range_set_value(GTK_RANGE(args->lightness), g_key_file_get_double_with_default(gs->settings, "Generate Scheme Dialog", "Lightness", 0));
	g_signal_connect (G_OBJECT (args->lightness), "value-changed", G_CALLBACK (update), args);
	g_signal_connect (G_OBJECT (args->lightness), "format-value", G_CALLBACK (format_value_callback2), args);
	gtk_table_attach(GTK_TABLE(table), args->lightness,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	//table_y=0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->gen_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Complementary");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Analogous");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Triadic");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Split-Complementary");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Rectangle (tetradic)");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Square");
	gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), "Neutral");
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->gen_type), g_key_file_get_integer_with_default(gs->settings, "Generate Scheme Dialog", "Type", 0));
	g_signal_connect (G_OBJECT (args->gen_type), "changed", G_CALLBACK (update), args);
	gtk_table_attach(GTK_TABLE(table), args->gen_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,0);
	table_y++;


	struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(gs->colors->params);
	struct ColorList* preview_color_list=color_list_new(handler_map);
	dynv_handler_map_release(handler_map);
	
	
	args->preview_color_list = preview_color_list;
	args->colors_visible = 5;
	args->gs = gs;
	
	gtk_widget_show_all(vbox);
	
	update(0, args);
	
	args->main = hbox;
	
	*out_widget = hbox;
	
	return (ColorSource*)args;
}

