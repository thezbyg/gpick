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

void dialog_variations_show(GtkWindow* parent, GtkWidget* palette, GKeyFile* settings) {
	GtkWidget *table, *toggle_multiplication;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Variations", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	gint table_y;
	table = gtk_table_new(4, 3, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	range_lightness_from = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_from), g_key_file_get_double_with_default(settings, "Variations Dialog", "Lightness From", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);

	range_lightness_to = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_to), g_key_file_get_double_with_default(settings, "Variations Dialog", "Lightness To", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	range_saturation_from = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_from), g_key_file_get_double_with_default(settings, "Variations Dialog", "Saturation From", 0));
	gtk_table_attach(GTK_TABLE(table), range_saturation_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);

	range_saturation_to = gtk_spin_button_new_with_range (-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_to), g_key_file_get_double_with_default(settings, "Variations Dialog", "Saturation To", 1));
	gtk_table_attach(GTK_TABLE(table), range_saturation_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Steps:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_steps), g_key_file_get_integer_with_default(settings, "Variations Dialog", "Steps", 3));
	gtk_table_attach(GTK_TABLE(table), range_steps,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;


	toggle_multiplication = gtk_check_button_new_with_mnemonic ("_Use multiplication");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_multiplication), g_key_file_get_boolean_with_default(settings, "Variations Dialog", "Multiplication", TRUE));
	gtk_table_attach(GTK_TABLE(table), toggle_multiplication,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;


	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(range_steps));
		gfloat lightness_from=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_lightness_from));
		gfloat lightness_to=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_lightness_to));
		gfloat saturation_from=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_saturation_from));
		gfloat saturation_to=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_saturation_to));
		gboolean multiplication=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_multiplication));

		g_key_file_set_integer(settings, "Variations Dialog", "Steps", steps);
		g_key_file_set_double(settings, "Variations Dialog", "Lightness From", lightness_from);
		g_key_file_set_double(settings, "Variations Dialog", "Lightness To", lightness_to);
		g_key_file_set_double(settings, "Variations Dialog", "Saturation From", saturation_from);
		g_key_file_set_double(settings, "Variations Dialog", "Saturation To", saturation_to);
		g_key_file_set_boolean(settings, "Variations Dialog", "Multiplication", multiplication);

		stringstream s;

		Color r, hsl;
		gint step_i;

		GList *colors=NULL, *i;
		colors=palette_list_make_color_list(palette);
		i=colors;
		while (i){

			for (step_i = 0; step_i < steps; ++step_i) {
				color_rgb_to_hsl(((struct NamedColor*)i->data)->color, &hsl);

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
				s<<((struct NamedColor*)i->data)->name<<" variation "<<step_i;
				palette_list_add_entry_name(palette, s.str().c_str(), &r);
			}

			i=g_list_next(i);
		}

		palette_list_free_color_list(colors);

	}
	gtk_widget_destroy(dialog);

}
