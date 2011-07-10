/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#include "BlendColors.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"
#include "DragDrop.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "ColorRYB.h"
#include "gtk/ColorWidget.h"
#include "uiColorInput.h"
#include "CopyPaste.h"
#include "Converter.h"
#include "DynvHelpers.h"

#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>

#include <stdbool.h>
#include <sstream>

using namespace std;

typedef struct BlendColorsArgs{
	ColorSource source;

	GtkWidget *main;

	GtkWidget *mix_type;
	GtkWidget *mix_steps;

	GtkWidget *start_color;
	GtkWidget *end_color;

	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
}BlendColorsArgs;

static int source_get_color(BlendColorsArgs *args, struct ColorObject** color);

static void calc( BlendColorsArgs *args, bool preview, int limit){

	gint steps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->mix_steps));
	gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));

	Color r;
	gint step_i;

	stringstream s;
	s.precision(0);
	s.setf(ios::fixed,ios::floatfield);

	Color a,b;
	matrix3x3 adaptation_matrix, working_space_matrix, working_space_matrix_inverted;
	vector3 d50, d65;
	vector3_set(&d50, 96.442, 100.000,  82.821);
	vector3_set(&d65, 95.047, 100.000, 108.883);
	color_get_chromatic_adaptation_matrix(&d50, &d65, &adaptation_matrix);
	color_get_working_space_matrix(0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, &d65, &working_space_matrix);
	matrix3x3_inverse(&working_space_matrix, &working_space_matrix_inverted);

	struct ColorList *color_list;
	color_list = args->preview_color_list;

	gtk_color_get_color(GTK_COLOR(args->start_color), &a);
	gtk_color_get_color(GTK_COLOR(args->end_color), &b);

	switch (type) {
	case 0:
		for (step_i = 0; step_i < steps; ++step_i) {
			r.rgb.red = mix_float(a.rgb.red, b.rgb.red, step_i/(float)(steps-1));
			r.rgb.green = mix_float(a.rgb.green, b.rgb.green, step_i/(float)(steps-1));
			r.rgb.blue = mix_float(a.rgb.blue, b.rgb.blue, step_i/(float)(steps-1));

			s.str("");
			s<<(step_i/float(steps-1))*100<< " blend " <<100-(step_i/float(steps-1))*100;

			struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
			dynv_set_string(color_object->params, "name", s.str().c_str());
			color_list_add_color_object(color_list, color_object, 1);
			color_object_release(color_object);
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
				s<<(step_i/float(steps-1))*100<< " blend " <<100-(step_i/float(steps-1))*100;

				struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
				dynv_set_string(color_object->params, "name", s.str().c_str());
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
				s<<(step_i/float(steps-1))*100<< " blend " <<100-(step_i/float(steps-1))*100;

				struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
				dynv_set_string(color_object->params, "name", s.str().c_str());
				color_list_add_color_object(color_list, color_object, 1);
			}
		}
		break;

	case 3:
		{
			Color a_lab, b_lab, r_lab;
			color_rgb_to_lab(&a, &a_lab, &d50, &working_space_matrix);
			color_rgb_to_lab(&b, &b_lab, &d50, &working_space_matrix);

			for (step_i = 0; step_i < steps; ++step_i) {
				r_lab.lab.L = mix_float(a_lab.lab.L, b_lab.lab.L, step_i/(float)(steps-1));
				r_lab.lab.a = mix_float(a_lab.lab.a, b_lab.lab.a, step_i/(float)(steps-1));
				r_lab.lab.b = mix_float(a_lab.lab.b, b_lab.lab.b, step_i/(float)(steps-1));

				color_lab_to_rgb(&r_lab, &r, &d50, &working_space_matrix_inverted);
				color_rgb_normalize(&r);

				s.str("");
				s<<(step_i/float(steps-1))*100<< " blend " <<100-(step_i/float(steps-1))*100;

				struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
				dynv_set_string(color_object->params, "name", s.str().c_str());
				color_list_add_color_object(color_list, color_object, 1);
			}
		}
		break;
	}
}


static void update(GtkWidget *widget, BlendColorsArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 101);
}

static int set_rgb_color(BlendColorsArgs *args, struct ColorObject* color, uint32_t color_index){
	Color c;
	color_object_get_color(color, &c);
	if (color_index == 1){
		gtk_color_set_color(GTK_COLOR(args->start_color), &c, "");
	}else if (color_index == 2){
		gtk_color_set_color(GTK_COLOR(args->end_color), &c, "");
	}
	update(0, args);
	return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	BlendColorsArgs* args = (BlendColorsArgs*)dd->userdata;
	struct ColorObject* colorobject;
	if (source_get_color(args, &colorobject) == 0){
		return colorobject;
	}
	return 0;
}



static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	BlendColorsArgs* args = static_cast<BlendColorsArgs*>(dd->userdata);
	set_rgb_color(args, colorobject, (uintptr_t)dd->userdata2);
	return 0;
}

static int source_get_color(BlendColorsArgs *args, struct ColorObject** color){
	return -1;
}

static int source_set_color(BlendColorsArgs *args, struct ColorObject* color){
	return -1;
}

static int source_activate(BlendColorsArgs *args){
	return 0;
}


static int source_deactivate(BlendColorsArgs *args){
	return 0;
}

static int source_destroy(BlendColorsArgs *args){
	gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->mix_steps));
	gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));
	dynv_set_int32(args->params, "type", type);
	dynv_set_int32(args->params, "steps", steps);

	Color c;
	gtk_color_get_color(GTK_COLOR(args->start_color), &c);
	dynv_set_color(args->params, "start_color", &c);
	gtk_color_get_color(GTK_COLOR(args->end_color), &c);
	dynv_set_color(args->params, "end_color", &c);

	color_list_destroy(args->preview_color_list);

	dynv_system_release(args->params);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static ColorSource* source_implement(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace){
	BlendColorsArgs *args = new BlendColorsArgs;

	args->params = dynv_system_ref(dynv_namespace);
	args->gs = gs;

	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;


	GtkWidget *table, *widget;
	GtkWidget *mix_type, *mix_steps;

	gint table_y;
	table = gtk_table_new(5, 2, FALSE);
	table_y = 0;

	struct DragDrop dd;
	dragdrop_init(&dd, gs);

	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;

	Color c;
	color_set(&c, 0.5);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Start color:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->start_color = widget = gtk_color_new();
	gtk_color_set_color(GTK_COLOR(args->start_color), dynv_get_color_wdc(args->params, "start_color", &c), "");
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 5);

	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata2 = (void*)1;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("End color:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->end_color = widget = gtk_color_new();
	gtk_color_set_color(GTK_COLOR(args->end_color), dynv_get_color_wdc(args->params, "end_color", &c), "");
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 5);

	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata2 = (void*)2;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	table_y = 0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:",0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "RGB");
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "HSV");
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "HSV shortest hue distance");
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), "LAB");
	gtk_combo_box_set_active(GTK_COMBO_BOX(mix_type), dynv_get_int32_wd(args->params, "type", 0));
	gtk_table_attach(GTK_TABLE(table), mix_type,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->mix_type = mix_type;
	g_signal_connect(G_OBJECT(mix_type), "changed", G_CALLBACK (update), args);


	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Steps:",0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), dynv_get_int32_wd(args->params, "steps", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->mix_steps = mix_steps;
	g_signal_connect(G_OBJECT(mix_steps), "value-changed", G_CALLBACK (update), args);

	GtkWidget* preview_expander;
	struct ColorList* preview_color_list=NULL;
	gtk_table_attach(GTK_TABLE(table), preview_expander=palette_list_preview_new(gs, dynv_get_bool_wd(args->params, "show_preview", true), gs->colors, &preview_color_list), 0, 4, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;

	args->preview_color_list = preview_color_list;

	update(0, args);

	gtk_widget_show_all(table);

	args->main = table;
	args->source.widget = table;

	return (ColorSource*)args;
}


int blend_colors_source_register(ColorSourceManager *csm){
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "blend_colors", "Blend colors");
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace))source_implement;
	color_source_manager_add_source(csm, color_source);
	return 0;
}

