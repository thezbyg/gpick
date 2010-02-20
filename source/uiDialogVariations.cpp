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
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"

#include <sstream>
using namespace std;

typedef struct DialogVariationsArgs{
	GtkWidget *toggle_multiplication;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;

	struct ColorList *selected_color_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
}DialogVariationsArgs;

static void calc( DialogVariationsArgs *args, bool preview, int limit){
	gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->range_steps));
	gfloat lightness_from=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_lightness_from));
	gfloat lightness_to=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_lightness_to));
	gfloat saturation_from=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_saturation_from));
	gfloat saturation_to=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_saturation_to));
	gboolean multiplication=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_multiplication));

	if (!preview){
		dynv_set_int32(args->params, "steps", steps);
		dynv_set_float(args->params, "lightness_from", lightness_from);
		dynv_set_float(args->params, "lightness_to", lightness_to);
		dynv_set_float(args->params, "saturation_from", saturation_from);
		dynv_set_float(args->params, "saturation_to", saturation_to);
		dynv_set_bool(args->params, "multiplication", multiplication);
	}

	stringstream s;

	Color r, hsl;
	gint step_i;

	struct ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->colors;

	for (ColorList::iter i=args->selected_color_list->colors.begin(); i!=args->selected_color_list->colors.end(); ++i){
		Color in;
		color_object_get_color(*i, &in);
		const char* name = dynv_get_string_wd((*i)->params, "name", 0);

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
			dynv_set_string(color_object->params, "name", s.str().c_str());
			color_list_add_color_object(color_list, color_object, 1);
			color_object_release(color_object);
		}

		//i=g_list_next(i);
	}

	//palette_list_free_color_list(colors);
}

static void update(GtkWidget *widget, DialogVariationsArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}

void dialog_variations_show(GtkWindow* parent, struct ColorList *selected_color_list, GlobalState* gs) {
	DialogVariationsArgs *args = new DialogVariationsArgs;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick.variations");

	GtkWidget *table, *toggle_multiplication;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Variations", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);

	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1),
		dynv_get_int32_wd(args->params, "window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	range_lightness_from = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_from), dynv_get_float_wd(args->params, "lightness_from", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args->range_lightness_from = range_lightness_from;
	g_signal_connect (G_OBJECT (range_lightness_from), "value-changed", G_CALLBACK (update), args);

	range_lightness_to = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_to), dynv_get_float_wd(args->params, "lightness_to", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->range_lightness_to = range_lightness_to;
	g_signal_connect (G_OBJECT (range_lightness_to), "value-changed", G_CALLBACK (update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	range_saturation_from = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_from), dynv_get_float_wd(args->params, "saturation_from", 0));
	gtk_table_attach(GTK_TABLE(table), range_saturation_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args->range_saturation_from = range_saturation_from;
	g_signal_connect (G_OBJECT (range_saturation_from), "value-changed", G_CALLBACK (update), args);

	range_saturation_to = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_to), dynv_get_float_wd(args->params, "saturation_to", 1));
	gtk_table_attach(GTK_TABLE(table), range_saturation_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->range_saturation_to = range_saturation_to;
	g_signal_connect (G_OBJECT (range_saturation_to), "value-changed", G_CALLBACK (update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Steps:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_steps), dynv_get_int32_wd(args->params, "steps", 3));
	gtk_table_attach(GTK_TABLE(table), range_steps,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->range_steps = range_steps;
	g_signal_connect (G_OBJECT (range_steps), "value-changed", G_CALLBACK (update), args);

	toggle_multiplication = gtk_check_button_new_with_mnemonic ("_Use multiplication");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_multiplication), dynv_get_bool_wd(args->params, "multiplication", true));
	gtk_table_attach(GTK_TABLE(table), toggle_multiplication,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->toggle_multiplication = toggle_multiplication;
	g_signal_connect (G_OBJECT (toggle_multiplication), "toggled", G_CALLBACK (update), args);

	GtkWidget* preview_expander;
	struct ColorList* preview_color_list=NULL;
	gtk_table_attach(GTK_TABLE(table), preview_expander=palette_list_preview_new(gs, dynv_get_bool_wd(args->params, "show_preview", true), gs->colors, &preview_color_list), 0, 3, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;


	args->selected_color_list = selected_color_list;
	args->preview_color_list = preview_color_list;

	update(0, args);


	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(args, false, 0);

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	dynv_set_int32(args->params, "window.width", width);
	dynv_set_int32(args->params, "window.height", height);
	dynv_set_bool(args->params, "show_preview", gtk_expander_get_expanded(GTK_EXPANDER(preview_expander)));

	gtk_widget_destroy(dialog);

	dynv_system_release(args->params);
	delete args;
}
