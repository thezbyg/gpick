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

#include "uiDialogVariations.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"

#include <sstream>
using namespace std;

struct Arguments{
	GtkWidget *toggle_multiplication;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;
	
	struct ColorList *color_list;
	struct ColorList *selected_color_list;
	struct ColorList *preview_color_list;
	
	GKeyFile* settings;
};

static void calc( struct Arguments *args, bool preview, int limit){
	gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->range_steps));
	gfloat lightness_from=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_lightness_from));
	gfloat lightness_to=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_lightness_to));
	gfloat saturation_from=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_saturation_from));
	gfloat saturation_to=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_saturation_to));
	gboolean multiplication=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_multiplication));

	if (!preview){
		g_key_file_set_integer(args->settings, "Variations Dialog", "Steps", steps);
		g_key_file_set_double(args->settings, "Variations Dialog", "Lightness From", lightness_from);
		g_key_file_set_double(args->settings, "Variations Dialog", "Lightness To", lightness_to);
		g_key_file_set_double(args->settings, "Variations Dialog", "Saturation From", saturation_from);
		g_key_file_set_double(args->settings, "Variations Dialog", "Saturation To", saturation_to);
		g_key_file_set_boolean(args->settings, "Variations Dialog", "Multiplication", multiplication);
	}
	
	stringstream s;

	Color r, hsl;
	gint step_i;

	struct ColorList *color_list;
	if (preview) 
		color_list = args->preview_color_list;
	else
		color_list = args->color_list;
	
	for (ColorList::iter i=args->selected_color_list->colors.begin(); i!=args->selected_color_list->colors.end(); ++i){ 
		Color in;
		color_object_get_color(*i, &in);
		const char* name = (const char*)dynv_system_get((*i)->params, "string", "name");
		
		for (step_i = 0; step_i < steps; ++step_i) {	

			if (preview){
				if (limit<=0) return;
				limit--;
			}
			
			color_rgb_to_hsl(&in, &hsl);

			if (multiplication){
				hsl.hsl.saturation *= mix_float(saturation_from, saturation_to, (step_i / (float) (steps - 1)));
				hsl.hsl.lightness *= mix_float(lightness_from, lightness_to, (step_i / (float) (steps - 1)));
			}else{
				hsl.hsl.saturation += mix_float(saturation_from, saturation_to, (step_i / (float) (steps - 1)));
				hsl.hsl.lightness += mix_float(lightness_from, lightness_to, (step_i / (float) (steps - 1)));
			}

			hsl.hsl.saturation = clamp_float(hsl.hsl.saturation, 0, 1);
			hsl.hsl.lightness = clamp_float(hsl.hsl.lightness, 0, 1);

			color_hsl_to_rgb(&hsl, &r);

			s.str("");
			s<<name<<" variation "<<step_i;
			//palette_list_add_entry_name(palette, s.str().c_str(), &r);
			//color_list_add_color(color_list, &r);
			
			struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
			dynv_system_set(color_object->params, "string", "name", (void*)s.str().c_str());
			color_list_add_color_object(color_list, color_object, 1);
		}

		//i=g_list_next(i);
	}

	//palette_list_free_color_list(colors);
}

static void update(GtkWidget *widget, struct Arguments *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}

void dialog_variations_show(GtkWindow* parent, struct ColorList *color_list, struct ColorList *selected_color_list, GKeyFile* settings) {
	struct Arguments args;
	
	GtkWidget *table, *toggle_multiplication;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Variations", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	
	gtk_window_set_default_size(GTK_WINDOW(dialog), g_key_file_get_integer_with_default(settings, "Variations Dialog", "Width", -1), 
		g_key_file_get_integer_with_default(settings, "Variations Dialog", "Height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	range_lightness_from = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_from), g_key_file_get_double_with_default(settings, "Variations Dialog", "Lightness From", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args.range_lightness_from = range_lightness_from;
	g_signal_connect (G_OBJECT (range_lightness_from), "value-changed", G_CALLBACK (update), &args);

	range_lightness_to = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_to), g_key_file_get_double_with_default(settings, "Variations Dialog", "Lightness To", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args.range_lightness_to = range_lightness_to;
	g_signal_connect (G_OBJECT (range_lightness_to), "value-changed", G_CALLBACK (update), &args);
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	range_saturation_from = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_from), g_key_file_get_double_with_default(settings, "Variations Dialog", "Saturation From", 0));
	gtk_table_attach(GTK_TABLE(table), range_saturation_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args.range_saturation_from = range_saturation_from;
	g_signal_connect (G_OBJECT (range_saturation_from), "value-changed", G_CALLBACK (update), &args);

	range_saturation_to = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_to), g_key_file_get_double_with_default(settings, "Variations Dialog", "Saturation To", 1));
	gtk_table_attach(GTK_TABLE(table), range_saturation_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args.range_saturation_to = range_saturation_to;
	g_signal_connect (G_OBJECT (range_saturation_to), "value-changed", G_CALLBACK (update), &args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Steps:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_steps), g_key_file_get_integer_with_default(settings, "Variations Dialog", "Steps", 3));
	gtk_table_attach(GTK_TABLE(table), range_steps,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args.range_steps = range_steps;
	g_signal_connect (G_OBJECT (range_steps), "value-changed", G_CALLBACK (update), &args);

	toggle_multiplication = gtk_check_button_new_with_mnemonic ("_Use multiplication");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_multiplication), g_key_file_get_boolean_with_default(settings, "Variations Dialog", "Multiplication", TRUE));
	gtk_table_attach(GTK_TABLE(table), toggle_multiplication,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args.toggle_multiplication = toggle_multiplication;
	g_signal_connect (G_OBJECT (toggle_multiplication), "toggled", G_CALLBACK (update), &args);
	
	GtkWidget* preview_expander;
	struct ColorList* preview_color_list=NULL;
	gtk_table_attach(GTK_TABLE(table), preview_expander=palette_list_preview_new(g_key_file_get_boolean_with_default(settings, "Preview", "Show", true), color_list, &preview_color_list), 0, 3, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;
	
	args.color_list = color_list;
	args.selected_color_list = selected_color_list;
	args.preview_color_list = preview_color_list;
	args.settings = settings;
	
	update(0, &args);


	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(&args, false, 0);

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	g_key_file_set_integer(settings, "Variations Dialog", "Width", width);
	g_key_file_set_integer(settings, "Variations Dialog", "Height", height);
	
	g_key_file_set_boolean(settings, "Preview", "Show", gtk_expander_get_expanded(GTK_EXPANDER(preview_expander)));

	gtk_widget_destroy(dialog);

}
