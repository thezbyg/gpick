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

#include "uiDialogMix.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"

#include <sstream>
using namespace std;

void dialog_mix_show(GtkWindow* parent, GtkWidget* palette, GKeyFile* settings) {
	GtkWidget *table;
	GtkWidget *mix_type, *mix_steps;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Mix colors", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

	gint table_y;
	table = gtk_table_new(2, 2, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "RGB");
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "HSV");
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "HSV shortest hue distance");
	gtk_combo_box_set_active(GTK_COMBO_BOX(mix_type), g_key_file_get_integer_with_default(settings, "Mix Dialog", "Type", 0));
	gtk_table_attach(GTK_TABLE(table), mix_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Steps:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), g_key_file_get_integer_with_default(settings, "Mix Dialog", "Steps", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;


	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mix_steps));
		gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(mix_type));

		g_key_file_set_integer(settings, "Mix Dialog", "Type", type);
		g_key_file_set_integer(settings, "Mix Dialog", "Steps", steps);

		Color r;
		gint step_i;

		stringstream s;
		s.precision(0);
		s.setf(ios::fixed,ios::floatfield);
		GList *colors=NULL, *i, *j;
		colors=palette_list_make_color_list(palette);
		i=colors;

		while (i){
			j=g_list_next(i);
			while (j){

				struct NamedColor *a=((struct NamedColor *) i->data);
				struct NamedColor *b=((struct NamedColor *) j->data);

				switch (type) {
				case 0:
					for (step_i = 0; step_i < steps; ++step_i) {
						r.rgb.red = mix_float(a->color->rgb.red, b->color->rgb.red, step_i/(float)(steps-1));
						r.rgb.green = mix_float(a->color->rgb.green, b->color->rgb.green, step_i/(float)(steps-1));
						r.rgb.blue = mix_float(a->color->rgb.blue, b->color->rgb.blue, step_i/(float)(steps-1));

						s.str("");
						s<<a->name <<" "<<(step_i/float(steps-1))*100<< " mix " <<100-(step_i/float(steps-1))*100<<" "<< b->name;
						palette_list_add_entry_name(palette, s.str().c_str() ,&r);
					}
					break;

				case 1:
					{
						Color a_hsv, b_hsv, r_hsv;
						color_rgb_to_hsv(a->color, &a_hsv);
						color_rgb_to_hsv(b->color, &b_hsv);

						for (step_i = 0; step_i < steps; ++step_i) {
							r_hsv.hsv.hue = mix_float(a_hsv.hsv.hue, b_hsv.hsv.hue, step_i/(float)(steps-1));
							r_hsv.hsv.saturation = mix_float(a_hsv.hsv.saturation, b_hsv.hsv.saturation, step_i/(float)(steps-1));
							r_hsv.hsv.value = mix_float(a_hsv.hsv.value, b_hsv.hsv.value, step_i/(float)(steps-1));

							color_hsv_to_rgb(&r_hsv, &r);
							palette_list_add_entry_name(palette, s.str().c_str() ,&r);
						}
					}
					break;

				case 2:
					{
						Color a_hsv, b_hsv, r_hsv;
						color_rgb_to_hsv(a->color, &a_hsv);
						color_rgb_to_hsv(b->color, &b_hsv);

						if (a_hsv.hsv.hue>b_hsv.hsv.hue){
							if (a_hsv.hsv.hue-b_hsv.hsv.hue>0.5)
								a_hsv.hsv.hue-=1;
						}else{
							if (b_hsv.hsv.hue-a_hsv.hsv.hue>0.5)
								b_hsv.hsv.hue-=1;
						}
						for (step_i = 0; step_i < steps; ++step_i) {
							r_hsv.hsv.hue = mix_float(a_hsv.hsv.hue, b_hsv.hsv.hue, step_i/(float)(steps-1));
							r_hsv.hsv.saturation = mix_float(a_hsv.hsv.saturation, b_hsv.hsv.saturation, step_i/(float)(steps-1));
							r_hsv.hsv.value = mix_float(a_hsv.hsv.value, b_hsv.hsv.value, step_i/(float)(steps-1));

							if (r_hsv.hsv.hue<0) r_hsv.hsv.hue+=1;
							color_hsv_to_rgb(&r_hsv, &r);
							palette_list_add_entry_name(palette, s.str().c_str() ,&r);
						}
					}
					break;
				}
				j=g_list_next(j);
			}
			i=g_list_next(i);
		}
		palette_list_free_color_list(colors);
	}
	gtk_widget_destroy(dialog);

}
