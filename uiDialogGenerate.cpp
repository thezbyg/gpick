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

#include "uiDialogGenerate.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorObject.h"
#include "MathUtil.h"

#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;




float transform_lightness(float hue1, float hue2){
	float values[]={
		0.50000000,		0.50000000,		0.50000000,		0.50000000,
		0.50000000,		0.50000000,		0.50000000,		0.50000000,
		0.50000000,		0.50000000,		0.50000000,		0.50000000,
		0.50000000,		0.46470600,		0.42745101,		0.39215699,
		0.37450999,		0.35490200,		0.33725500,		0.35294101,
		0.36862749,		0.38431349,		0.37254900,		0.36078450,
		0.34901950,		0.34313750,		0.33529401,		0.32941201,
		0.32549000,		0.31960800,		0.31568649,		0.33921549,
		0.36470601,		0.38823551,		0.42548999,		0.46274501,
	};
	gint samples=sizeof(values)/sizeof(float);
	double n;
	return 	mix_float(values[wrap_int(int(hue2*samples), 0, samples)], values[wrap_int(int(hue2*samples+1), 0, samples)], modf(hue2*samples,&n))/
			mix_float(values[wrap_int(int(hue1*samples), 0, samples)], values[wrap_int(int(hue1*samples+1), 0, samples)], modf(hue1*samples,&n));
}

float transform_hue(float hue, gboolean forward){
	float values[]={
		0.00000000,		0.02156867,		0.04248367,		0.06405234,
		0.07385617,		0.08431367,		0.09411767,		0.10653600,
		0.11830067,		0.13071899,		0.14248367,		0.15490200,
		0.16666667,		0.18354435,		0.19954120,		0.21666662,
		0.25130904,		0.28545108,		0.31976745,		0.38981494,
		0.46010628,		0.53061217,		0.54649121,		0.56159425,
		0.57771534,		0.60190469,		0.62573093,		0.64980155,
		0.68875504,		0.72801632,		0.76708061,		0.80924863,
		0.85215056,		0.89478123,		0.92933953,		0.96468931,
		1.00000000,
	};

	gint samples=sizeof(values)/sizeof(float);
	gfloat new_hue;

	if (!forward){
		for (gint i=0;i<samples;++i){
			if (values[i]>=hue){
				int index1, index2;
				gfloat value1, value2, mix;

				index1=max_int(i, 0);
				index2=min_int(i+1, samples-1);

				value1=index1/(float)(samples-1);
				value2=index2/(float)(samples-1);

				if (index1==index2) return value1;

				mix = (hue-values[index1])/(values[index2]-values[index1]);

				new_hue= mix_float(value1, value2, mix);

				return new_hue;
			}
		}
		return 1;
	}else{
		gfloat value1=values[max_int(gint(hue*(samples-1)), 0)];
		gfloat value2=values[min_int(gint(hue*(samples-1)+1), samples-1)];


		double n;
		gfloat mix = modf(hue*(samples-1), &n);

		new_hue = mix_float(value1, value2, mix);

		return new_hue;
	}
	return 0;

}

void dialog_generate_show(GtkWindow* parent, struct ColorList *color_list, struct ColorList *selected_color_list, GKeyFile* settings, Random* random){
	GtkWidget *table, *gen_type, *range_colors, *range_chaos, *toggle_brightness_correction;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Generate colors", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(2, 2, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	gen_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Complementary");
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Analogous");
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Triadic");
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Split-Complementary");
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Rectangle (tetradic)");
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Square");
	gtk_combo_box_append_text(GTK_COMBO_BOX(gen_type), "Neutral");
	gtk_combo_box_set_active(GTK_COMBO_BOX(gen_type), g_key_file_get_integer_with_default(settings, "Generate Dialog", "Type", 0));
	gtk_table_attach(GTK_TABLE(table), gen_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Colors:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_colors = gtk_spin_button_new_with_range (1,25,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_colors), g_key_file_get_integer_with_default(settings, "Generate Dialog", "Colors", 1));
	gtk_table_attach(GTK_TABLE(table), range_colors,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Chaos:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_chaos = gtk_spin_button_new_with_range (0,1,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_chaos), g_key_file_get_double_with_default(settings, "Generate Dialog", "Chaos", 0));
	gtk_table_attach(GTK_TABLE(table), range_chaos,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	toggle_brightness_correction = gtk_check_button_new_with_mnemonic ("Brightness correction");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_brightness_correction), g_key_file_get_boolean_with_default(settings, "Generate Dialog", "Brightness Correction", TRUE));
	gtk_table_attach(GTK_TABLE(table), toggle_brightness_correction,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);



	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(gen_type));
		gint color_count=(gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_colors));
		gfloat chaos=gtk_spin_button_get_value(GTK_SPIN_BUTTON(range_chaos));
		gboolean correction=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_brightness_correction));

		g_key_file_set_integer(settings, "Generate Dialog", "Type", type);
		g_key_file_set_integer(settings, "Generate Dialog", "Colors", color_count);
		g_key_file_set_double(settings, "Generate Dialog", "Chaos", chaos);
		g_key_file_set_boolean(settings, "Generate Dialog", "Brightness Correction", correction);

		Color r, hsl;
		float hue;
		gint step_i;

		stringstream s;

		for (ColorList::iter i=selected_color_list->colors.begin(); i!=selected_color_list->colors.end(); ++i){ 
			Color in;
			color_object_get_color(*i, &in);
			color_rgb_to_hsl(&in, &hsl);
			float initial_hue=hsl.hsl.hue;
			float initial_lighness=hsl.hsl.lightness;

			float transformed_hue=transform_hue(initial_hue, FALSE);
			hue=transformed_hue;
			
			const char* name = (const char*)dynv_system_get((*i)->params, "string", "name");

			for (step_i = 0; step_i < color_count; ++step_i) {
				s.str("");

				switch (type) {
				case 0: //Complementary = 180 degree turns
					s << name << " complementary " << step_i;
					hue = wrap_float(hue + (PI) / (2 * PI));
					break;

				case 1: //Analogous = 30 degree turns
					s << name << " analogous " << step_i;
					hue = wrap_float(hue + (2 * PI / 12) / (2 * PI)); //+30*
					break;

				case 2: //Triadic = three 120 degree turns
					s << name << " triadic " << step_i;
					hue = wrap_float(hue + (2 * PI / 3) / (2 * PI)); //+120*
					break;

				case 3: //Split-Complementary = 150, 60, 150 degree turns
					s << name << " split-complementary " << step_i;
					if (step_i % 3 == 1) {
						hue = wrap_float(hue + (PI/3) / (2 * PI)); //+60*
					} else {
						hue = wrap_float(hue + (PI - (PI / 6)) / (2 * PI)); //+150*
					}
					break;

				case 4: //Rectangle (tetradic) = 60, 120 degree turns
					s << name << " rectangle " << step_i;
					if (step_i & 1) {
						hue = wrap_float(hue + (2 * PI / 3) / (2 * PI)); //+120*
					} else {
						hue = wrap_float(hue + (PI / 3) / (2 * PI)); //+60*
					}
					break;

				case 5: //Square = 90 degree turns
					s << name << " square " << step_i;
					hue = wrap_float(hue + (PI/2) / (2 * PI));
					break;

				case 6: //Neutral = 15 degree turns
					s << name << " neutral " << step_i;
					hue = wrap_float(hue + (2 * PI / 24) / (2 * PI)); //+15*
					break;
				}

				hue = wrap_float(hue + chaos*(((random_get(random)&0xFFFFFFFF)/(gdouble)0xFFFFFFFF)-0.5));
				hsl.hsl.hue = transform_hue(hue, TRUE);
				if (correction){
					hsl.hsl.lightness = clamp_float(initial_lighness*transform_lightness( transformed_hue, hue),0,1);
				}else{
					hsl.hsl.lightness = initial_lighness;
				}

				color_hsl_to_rgb(&hsl, &r);
				
				struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
				dynv_system_set(color_object->params, "string", "name", (void*)s.str().c_str());
				color_list_add_color_object(color_list, color_object, 1);
			}
		}
	}
	gtk_widget_destroy(dialog);

}
