/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"
#include "ToolColorNaming.h"
#include "Internationalisation.h"

#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <sstream>
using namespace std;

typedef struct DialogMixArgs{
	GtkWidget *mix_type;
	GtkWidget *mix_steps;
	GtkWidget *toggle_endpoints;

	struct ColorList *selected_color_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
}DialogMixArgs;

class MixColorNameAssigner: public ToolColorNameAssigner {
	protected:
		stringstream m_stream;
		const char *m_color_start;
		const char *m_color_end;
		int m_start_percent;
		int m_end_percent;
                int m_is_node;
	public:
		MixColorNameAssigner(GlobalState *gs):ToolColorNameAssigner(gs){
		}

		void assign(struct ColorObject *color_object, Color *color, const char *start_color_name, const char *end_color_name, int start_percent, int end_percent, bool is_node){
			m_color_start = start_color_name;
			m_color_end = end_color_name;
			m_start_percent = start_percent;
			m_end_percent = end_percent;
			m_is_node = is_node;
			ToolColorNameAssigner::assign(color_object, color);
		}

		void assign(struct ColorObject *color_object, Color *color, const char *item_name){
			m_color_start = item_name;
			ToolColorNameAssigner::assign(color_object, color);
		}

		virtual std::string getToolSpecificName(struct ColorObject *color_object, Color *color){
			m_stream.str("");
			if (m_is_node){
				if (m_end_percent == 100){
					m_stream << m_color_end << " " << _("mix node");
				}else{
					m_stream << m_color_start << " " << _("mix node");
				}
			}else{
				m_stream << m_color_start << " " << m_start_percent << " " << _("mix") << " " << m_end_percent << " " << m_color_end;
			}
			return m_stream.str();
		}
};


#define STORE_COLOR() struct ColorObject *color_object=color_list_new_color_object(color_list, &r); \
    float mixfactor = step_i/(float)(steps-1); \
    name_assigner.assign(color_object, &r, name_a, name_b, (int)((1.0 - mixfactor)*100), (int)(mixfactor*100), with_endpoints && (step_i == 0 || step_i == (max_step - 1))); \
	color_list_add_color_object(color_list, color_object, 1); \
	color_object_release(color_object)

#define STORE_LINEARCOLOR() color_linear_get_rgb(&r, &r); \
	STORE_COLOR()



static void calc( DialogMixArgs *args, bool preview, int limit){

	gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->mix_steps));
	gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));
	bool with_endpoints=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_endpoints));
	gint start_step = 0;
	gint max_step = steps;
	MixColorNameAssigner name_assigner(args->gs);

	if (!preview){
		dynv_set_int32(args->params, "type", type);
		dynv_set_int32(args->params, "steps", steps);
		dynv_set_bool(args->params, "includeendpoints", with_endpoints);
	}

	if (with_endpoints == false){
		start_step = 1;
		max_step = steps - 1;
	}

	Color r;
	gint step_i;

	stringstream s;
	s.precision(0);
	s.setf(ios::fixed,ios::floatfield);

	Color a,b;

	struct ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->colors;

	ColorList::iter j;
	for (ColorList::iter i=args->selected_color_list->colors.begin(); i!=args->selected_color_list->colors.end(); ++i){

		color_object_get_color(*i, &a);
		if (type == 0)
			color_rgb_get_linear(&a, &a);

		const char* name_a = dynv_get_string_wd((*i)->params, "name", 0);
		j=i;
		++j;
		for (; j!=args->selected_color_list->colors.end(); ++j){

			if (preview){
				if (limit<=0) return;
				limit--;
			}

			color_object_get_color(*j, &b);
			if (type == 0)
				color_rgb_get_linear(&b, &b);
			const char* name_b = dynv_get_string_wd((*j)->params, "name", 0);

			switch (type) {
			case 0:
				for (step_i = start_step; step_i < max_step; ++step_i) {
					MIX_COMPONENTS(r.rgb, a.rgb, b.rgb, red, green, blue);
					STORE_LINEARCOLOR();
				}
				break;

			case 1:
				{
					Color a_hsv, b_hsv, r_hsv;
					color_rgb_to_hsv(&a, &a_hsv);
					color_rgb_to_hsv(&b, &b_hsv);

					for (step_i = start_step; step_i < max_step; ++step_i) {
						MIX_COMPONENTS(r_hsv.hsv, a_hsv.hsv, b_hsv.hsv, hue, saturation, value);
						color_hsv_to_rgb(&r_hsv, &r);
						STORE_COLOR();
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
					for (step_i = start_step; step_i < max_step; ++step_i) {
						MIX_COMPONENTS(r_hsv.hsv, a_hsv.hsv, b_hsv.hsv, hue, saturation, value);

						if (r_hsv.hsv.hue<0) r_hsv.hsv.hue+=1;

						color_hsv_to_rgb(&r_hsv, &r);
						STORE_COLOR();
					}
				}
				break;

			case 3:
				{
					Color a_lab, b_lab, r_lab;
					color_rgb_to_lab_d50(&a, &a_lab);
					color_rgb_to_lab_d50(&b, &b_lab);

					for (step_i = start_step; step_i < max_step; ++step_i) {
						MIX_COMPONENTS(r_lab.lab, a_lab.lab, b_lab.lab, L, a, b);

						color_lab_to_rgb_d50(&r_lab, &r);
						color_rgb_normalize(&r);
						STORE_COLOR();
					}
				}
				break;

			case 4:
				{
					Color a_lch, b_lch, r_lch;
					color_rgb_to_lch_d50(&a, &a_lch);
					color_rgb_to_lch_d50(&b, &b_lch);

					if (a_lch.lch.h>b_lch.lch.h){
						if (a_lch.lch.h-b_lch.lch.h>180)
							a_lch.lch.h-=360;
					}else{
						if (b_lch.lch.h-a_lch.lch.h>180)
							b_lch.lch.h-=360;
					}
					for (step_i = start_step; step_i < max_step; ++step_i) {
						MIX_COMPONENTS(r_lch.lch, a_lch.lch, b_lch.lch, L, C, h);
						if (r_lch.lch.h<0) r_lch.lch.h+=360;
						color_lch_to_rgb_d50(&r_lch, &r);
						color_rgb_normalize(&r);
						STORE_COLOR();
					}
				}
				break;
			}
		}
	}
}

static void update(GtkWidget *widget, DialogMixArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}

void dialog_mix_show(GtkWindow* parent, struct ColorList *selected_color_list, GlobalState* gs) {
	DialogMixArgs *args = new DialogMixArgs;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick.mix");

	GtkWidget *table;
	GtkWidget *mix_type, *mix_steps;

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Mix colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);

	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1),
		dynv_get_int32_wd(args->params, "window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(3, 2, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Type:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_type = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("RGB"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("HSV"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("HSV shortest hue distance"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("LAB"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(mix_type), _("LCH shortest h distance"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(mix_type), dynv_get_int32_wd(args->params, "type", 0));
	gtk_table_attach(GTK_TABLE(table), mix_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->mix_type = mix_type;
	g_signal_connect (G_OBJECT (mix_type), "changed", G_CALLBACK (update), args);


	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Steps:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), dynv_get_int32_wd(args->params, "steps", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->mix_steps = mix_steps;
	g_signal_connect (G_OBJECT (mix_steps), "value-changed", G_CALLBACK (update), args);

	args->toggle_endpoints = gtk_check_button_new_with_mnemonic (_("_Include Endpoints"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_endpoints), dynv_get_bool_wd(args->params, "includeendpoints", true));
	gtk_table_attach(GTK_TABLE(table), args->toggle_endpoints,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT(args->toggle_endpoints), "toggled", G_CALLBACK (update), args);
	table_y++;

	GtkWidget* preview_expander;
	struct ColorList* preview_color_list=NULL;
	gtk_table_attach(GTK_TABLE(table), preview_expander=palette_list_preview_new(gs, true, dynv_get_bool_wd(args->params, "show_preview", true), gs->colors, &preview_color_list), 0, 2, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
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

	color_list_destroy(args->preview_color_list);
	dynv_system_release(args->params);
	delete args;
}
