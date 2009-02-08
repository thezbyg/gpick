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
#include "Color.h"
#include "MathUtil.h"

#include <math.h>
#include <sstream>
using namespace std;

struct namedColor{
	gchar* name;
	Color* color;
};

float transform_hue(float hue, gboolean backwards){

	float values[]={
		1.000000,
		0.894212,
		0.766204,
		0.659817,
		0.579004,
		0.531125,
		0.320088,
		0.216963,
		0.166667,
		0.130719,
		0.094771,
		0.063725,
		0.000000,
	};
	gint samples=sizeof(values)/sizeof(float);

	if (backwards){
		for (gint i=0;i<samples;++i){
			if (values[i]<hue){
				gfloat value1=max_int(i-1, 0)/(float)samples;
				gfloat value2=i/(float)samples;

				gfloat mix = (values[max_int(i-1, 0)]-hue)/(values[max_int(i-1, 0)]-values[i]);

				return mix_float(value1, value2, mix);
			}
		}
		return values[samples-1];
	}else{
		gfloat value1=values[max_int(gint(hue*samples-0), 0)];
		gfloat value2=values[min_int(gint(hue*samples+1), samples-1)];

		return mix_float(value1, value2, hue*samples-floor(hue*samples));
	}

}

gint32 dialog_generate_show_color_list(Color* color, const gchar *name, void *userdata){
	struct namedColor *c=new namedColor;
	c->color=color;
	c->name=g_strdup(name);

	*((GList**)userdata) = g_list_append(*((GList**)userdata), c);
	return 0;
}

void dialog_generate_show(GtkWindow* parent, GtkWidget* palette, GKeyFile* settings){
	GtkWidget *table, *gen_type;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Generate colors", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);

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
	gtk_combo_box_set_active(GTK_COMBO_BOX(gen_type), g_key_file_get_integer_with_default(settings, "Generate Dialog", "Type", 0));
	gtk_table_attach(GTK_TABLE(table), gen_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);



	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(gen_type));
		g_key_file_set_integer(settings, "Generate Dialog", "Type", type);

		Color r, hsv, hsv_copy;
		gint step_i;

		stringstream s;
		GList *colors=NULL, *i;
		palette_list_foreach_selected(palette, dialog_generate_show_color_list, &colors);
		i=colors;
		while (i){
			color_rgb_to_hsv(((struct namedColor *)i->data)->color, &hsv);
			float hue;
			hue=transform_hue(hsv.hsv.hue, TRUE);
			color_copy(&hsv, &hsv_copy);
			hsv_copy.hsv.hue=hue;

			switch (type) {
			case 0:
				s.str("");
				s << ((struct namedColor *) i->data)->name << " complementary";
				hue = wrap_float(hue + (PI) / (2 * PI));
				hsv.hsv.hue = transform_hue(hue, FALSE);
				color_hsv_to_rgb(&hsv, &r);
				palette_list_add_entry_name(palette, s.str().c_str(), &r);
				break;

			case 1:
				for (step_i = 1; step_i < 3; ++step_i) {
					s.str("");
					s << ((struct namedColor *) i->data)->name << " analogous " << step_i;
					hue = wrap_float(hue + (2 * PI / 10) / (2 * PI)); //+36*
					hsv.hsv.hue = transform_hue(hue, FALSE);
					color_hsv_to_rgb(&hsv, &r);
					palette_list_add_entry_name(palette, s.str().c_str(), &r);
				}
				break;

			case 2:
				for (step_i = 1; step_i < 3; ++step_i) {
					s.str("");
					s << ((struct namedColor *) i->data)->name << " triadic " << step_i;
					hue = wrap_float(hue + (2 * PI / 3) / (2 * PI)); //+120*
					hsv.hsv.hue = transform_hue(hue, FALSE);
					color_hsv_to_rgb(&hsv, &r);
					palette_list_add_entry_name(palette, s.str().c_str(), &r);
				}
				break;

			case 3:
				for (step_i = 0; step_i < 2; ++step_i) {
					hue = hsv_copy.hsv.hue;
					s.str("");
					s << ((struct namedColor *) i->data)->name << " split-complementary " << (step_i + 1);
					hue = wrap_float(hue + (PI + ((step_i * 2) - 1) * (2 * PI / 10)) / (2 * PI));
					hsv.hsv.hue = transform_hue(hue, FALSE);
					color_hsv_to_rgb(&hsv, &r);
					palette_list_add_entry_name(palette, s.str().c_str(), &r);
				}
				break;


			case 4:
				for (step_i = 0; step_i < 3; ++step_i) {
					s.str("");
					s << ((struct namedColor *) i->data)->name << " rectangle " << (step_i + 1);
					hue = wrap_float(hue + (PI/5 * ((step_i & 1) + 1)) / (2 * PI));
					hsv.hsv.hue = transform_hue(hue, FALSE);
					color_hsv_to_rgb(&hsv, &r);
					palette_list_add_entry_name(palette, s.str().c_str(), &r);
				}
				break;

			case 5:
				for (step_i=1;step_i<4;++step_i){
					s.str("");
					s<<((struct namedColor *)i->data)->name<<" square "<<step_i;
					hue = wrap_float(hue + (PI/2)/(2*PI));
					hsv.hsv.hue = transform_hue(hue, FALSE);
					color_hsv_to_rgb(&hsv, &r);
					palette_list_add_entry_name(palette, s.str().c_str(), &r);
				}
				break;

			}

			delete (struct namedColor *)i->data;

			i=g_list_next(i);
		}

		g_list_free(colors);

	}
	gtk_widget_destroy(dialog);

}
