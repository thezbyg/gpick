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

void dialog_mix_show(GtkWindow* parent, struct ColorList *color_list, struct ColorList *selected_color_list, GKeyFile* settings) {
	GtkWidget *table;
	GtkWidget *mix_type, *mix_steps;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Mix colors", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

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

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mix_steps));
		gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(mix_type));

		g_key_file_set_integer(settings, "Mix Dialog", "Type", type);
		g_key_file_set_integer(settings, "Mix Dialog", "Steps", steps);

		Color r;
		gint step_i;

		stringstream s;
		s.precision(0);
		s.setf(ios::fixed,ios::floatfield);
		/*GList *colors=NULL, *i, *j;
		colors=palette_list_make_color_list(palette);
		i=colors;*/
		
		Color a,b;

		ColorList::iter j;
		for (ColorList::iter i=selected_color_list->colors.begin(); i!=selected_color_list->colors.end(); ++i){ 
			color_object_get_color(*i, &a);
			const char* name_a = (const char*)dynv_system_get((*i)->params, "string", "name");
			j=i;
			++j;
			for (; j!=selected_color_list->colors.end(); ++j){ 
			
				color_object_get_color(*j, &b);
				const char* name_b = (const char*)dynv_system_get((*j)->params, "string", "name");

				switch (type) {
				case 0:
					for (step_i = 0; step_i < steps; ++step_i) {
						r.rgb.red = mix_float(a.rgb.red, b.rgb.red, step_i/(float)(steps-1));
						r.rgb.green = mix_float(a.rgb.green, b.rgb.green, step_i/(float)(steps-1));
						r.rgb.blue = mix_float(a.rgb.blue, b.rgb.blue, step_i/(float)(steps-1));

						s.str("");
						s<<name_a<<" "<<(step_i/float(steps-1))*100<< " mix " <<100-(step_i/float(steps-1))*100<<" "<< name_b;

						struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
						dynv_system_set(color_object->params, "string", "name", (void*)s.str().c_str());
						color_list_add_color_object(color_list, color_object, 1);
					}
					break;

				case 1:
					{
						Color a_hsv, b_hsv, r_hsv;
						color_rgb_to_hsv(&a, &a_hsv);
						color_rgb_to_hsv(&b, &b_hsv);

						for (step_i = 0; step_i < steps; ++step_i) {
							r_hsv.hsv.hue = mix_float(a_hsv.hsv.hue, b_hsv.hsv.hue, step_i/(float)(steps-1));
							r_hsv.hsv.saturation = mix_float(a_hsv.hsv.saturation, b_hsv.hsv.saturation, step_i/(float)(steps-1));
							r_hsv.hsv.value = mix_float(a_hsv.hsv.value, b_hsv.hsv.value, step_i/(float)(steps-1));

							color_hsv_to_rgb(&r_hsv, &r);
							
							s.str("");
							s<<name_a<<" "<<(step_i/float(steps-1))*100<< " mix " <<100-(step_i/float(steps-1))*100<<" "<< name_b;

							struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
							dynv_system_set(color_object->params, "string", "name", (void*)s.str().c_str());
							color_list_add_color_object(color_list, color_object, 1);
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
						for (step_i = 0; step_i < steps; ++step_i) {
							r_hsv.hsv.hue = mix_float(a_hsv.hsv.hue, b_hsv.hsv.hue, step_i/(float)(steps-1));
							r_hsv.hsv.saturation = mix_float(a_hsv.hsv.saturation, b_hsv.hsv.saturation, step_i/(float)(steps-1));
							r_hsv.hsv.value = mix_float(a_hsv.hsv.value, b_hsv.hsv.value, step_i/(float)(steps-1));

							if (r_hsv.hsv.hue<0) r_hsv.hsv.hue+=1;
							color_hsv_to_rgb(&r_hsv, &r);							
							
							s.str("");
							s<<name_a<<" "<<(step_i/float(steps-1))*100<< " mix " <<100-(step_i/float(steps-1))*100<<" "<< name_b;

							struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
							dynv_system_set(color_object->params, "string", "name", (void*)s.str().c_str());
							color_list_add_color_object(color_list, color_object, 1);
						}
					}
					break;
				}
			}
		}
	}
	gtk_widget_destroy(dialog);

}
