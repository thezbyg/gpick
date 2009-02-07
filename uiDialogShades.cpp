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

#include "uiDialogShades.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"


gint32 dialog_shades_show_color_list(Color* color, const gchar *name, void *userdata){
	*((GList**)userdata) = g_list_append(*((GList**)userdata), color);
	return 0;
}

void dialog_shades_show(GtkWindow* parent, GtkWidget* palette, GKeyFile* settings) {
	GtkWidget *widget, *table;
	GtkWidget *range_lightness, *range_steps, *range_saturation;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Generate shades", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	gint table_y;
	table = gtk_table_new(2, 2, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Lightness add:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_lightness = gtk_spin_button_new_with_range (-100,100,0.1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness), g_key_file_get_double_with_default(settings, "Shades Dialog", "Lightness", 0));
	gtk_table_attach(GTK_TABLE(table), range_lightness,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Saturation add:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_saturation = gtk_spin_button_new_with_range (-100,100,0.1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation), g_key_file_get_double_with_default(settings, "Shades Dialog", "Saturation", 0));
	gtk_table_attach(GTK_TABLE(table), range_saturation,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Steps:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_steps), g_key_file_get_integer_with_default(settings, "Shades Dialog", "Steps", 3));
	gtk_table_attach(GTK_TABLE(table), range_steps,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;


	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(range_steps));
		gfloat lightness=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_lightness));
		gfloat saturation=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_saturation));

		g_key_file_set_integer(settings, "Shades Dialog", "Steps", steps);
		g_key_file_set_double(settings, "Shades Dialog", "Lightness", lightness);
		g_key_file_set_double(settings, "Shades Dialog", "Saturation", saturation);

		lightness/=100;
		saturation/=100;

		Color r, hsl;
		gint step_i;

		GList *colors=NULL, *i;
		palette_list_foreach_selected(palette, dialog_shades_show_color_list, &colors);
		i=colors;
		while (i){

			for (step_i = 0; step_i < steps; ++step_i) {
				color_rgb_to_hsl((Color*)i->data, &hsl);

				hsl.hsl.saturation += saturation * (step_i / (float) (steps - 1));
				hsl.hsl.lightness += lightness * (step_i / (float) (steps - 1));

				hsl.hsl.saturation = clamp_float(hsl.hsl.saturation, 0, 1);
				hsl.hsl.lightness = clamp_float(hsl.hsl.lightness, 0, 1);

				color_hsl_to_rgb(&hsl, &r);
				palette_list_add_entry_name(palette, "shade", &r);
			}

			i=g_list_next(i);
		}

		g_list_free(colors);

	}
	gtk_widget_destroy(dialog);

}
