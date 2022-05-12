/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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
#include "ColorList.h"
#include "ColorObject.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "I18N.h"
#include "common/Guard.h"
#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <sstream>
using namespace std;

typedef struct DialogMixArgs{
	GtkWidget *mix_type;
	GtkWidget *mix_steps;
	GtkWidget *toggle_endpoints;
	ColorList *selected_color_list;
	ColorList *preview_color_list;
	dynv::Ref options;
	GlobalState* gs;
}DialogMixArgs;

struct MixColorNameAssigner: public ToolColorNameAssigner {
	MixColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
		m_isNode = false;
	}
	void setStartName(std::string_view name) {
		m_startName = name;
	}
	void setEndName(std::string_view name) {
		m_endName = name;
	}
	void setStepsAndStage(int steps, int stage) {
		m_steps = steps;
		m_stage = stage;
	}
	void assign(ColorObject &colorObject, int step) {
		m_startPercent = step * 100 / (m_steps - 1);
		m_endPercent = 100 - (step * 100 / (m_steps - 1));
		m_isNode = (((step == 0 || step == m_steps - 1) && m_stage == 0) || (m_stage == 1 && step == m_steps - 1));
		ToolColorNameAssigner::assign(colorObject);
	}
	void assign(ColorObject &colorObject, std::string_view name) {
		m_startName = name;
		m_isNode = false;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		if (m_isNode){
			if (m_endPercent == 100){
				m_stream << m_endName << " " << _("mix node");
			}else{
				m_stream << m_startName << " " << _("mix node");
			}
		}else{
			m_stream << m_startName << " " << m_startPercent << " " << _("mix") << " " << m_endPercent << " " << m_endName;
		}
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_startName, m_endName;
	int m_startPercent, m_endPercent, m_steps, m_stage;
	bool m_isNode;
};

static void store(ColorList *color_list, const Color *color, int step, MixColorNameAssigner &name_assigner)
{
	ColorObject *color_object = color_list_new_color_object(color_list, color);
	name_assigner.assign(*color_object, step);
	color_list_add_color_object(color_list, color_object, 1);
	color_object->release();
}
static void calc( DialogMixArgs *args, bool preview, int limit)
{
	gint steps=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->mix_steps));
	gint type=gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));
	bool with_endpoints=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_endpoints));
	gint start_step = 0;
	gint max_step = steps;
	MixColorNameAssigner name_assigner(*args->gs);

	if (!preview){
		args->options->set("type", type);
		args->options->set("steps", steps);
		args->options->set("includeendpoints", with_endpoints);
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
	Color a ,b;
	ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->getColorList();
	common::Guard colorListGuard(color_list_start_changes(color_list), color_list_end_changes, color_list);
	decltype(args->selected_color_list->colors)::iterator j;
	for (auto i = args->selected_color_list->colors.begin(); i != args->selected_color_list->colors.end(); ++i){
		a = (*i)->getColor();
		if (type == 0)
			a.linearRgbInplace();
		name_assigner.setStartName((*i)->getName().c_str());
		j = i;
		++j;
		for (; j != args->selected_color_list->colors.end(); ++j){
			if (preview){
				if (limit <= 0) return;
				limit--;
			}
			b = (*j)->getColor();
			if (type == 0)
				b.linearRgbInplace();
			name_assigner.setEndName((*j)->getName().c_str());
			name_assigner.setStepsAndStage(steps, 0);

			switch (type){
			case 0:
				for (step_i = start_step; step_i < max_step; ++step_i) {
					r = math::mix(a, b, step_i / (float)(steps - 1));
					r.nonLinearRgbInplace();
					store(color_list, &r, step_i, name_assigner);
				}
				break;

			case 1:
				{
					Color a_hsv, b_hsv, r_hsv;
					a_hsv = a.rgbToHsv();
					b_hsv = b.rgbToHsv();
					if (a_hsv.hsv.hue>b_hsv.hsv.hue){
						if (a_hsv.hsv.hue-b_hsv.hsv.hue>0.5f)
							a_hsv.hsv.hue-=1;
					}else{
						if (b_hsv.hsv.hue-a_hsv.hsv.hue>0.5f)
							b_hsv.hsv.hue-=1;
					}
					for (step_i = start_step; step_i < max_step; ++step_i) {
						r_hsv = math::mix(a_hsv, b_hsv, step_i / (float)(steps - 1));
						if (r_hsv.hsv.hue < 0) r_hsv.hsv.hue += 1;
						r = r_hsv.hsvToRgb();
						store(color_list, &r, step_i, name_assigner);
					}
				}
				break;

			case 2:
				{
					Color a_lab, b_lab, r_lab;
					a_lab = a.rgbToLabD50();
					b_lab = b.rgbToLabD50();
					for (step_i = start_step; step_i < max_step; ++step_i) {
						r_lab = math::mix(a_lab, b_lab, step_i / (float)(steps - 1));
						r = r_lab.labToRgbD50().normalizeRgbInplace();
						store(color_list, &r, step_i, name_assigner);
					}
				}
				break;

			case 3:
				{
					Color a_lch, b_lch, r_lch;
					a_lch = a.rgbToLchD50();
					b_lch = b.rgbToLchD50();
					if (a_lch.lch.h>b_lch.lch.h){
						if (a_lch.lch.h-b_lch.lch.h>180)
							a_lch.lch.h-=360;
					}else{
						if (b_lch.lch.h-a_lch.lch.h>180)
							b_lch.lch.h-=360;
					}
					for (step_i = start_step; step_i < max_step; ++step_i) {
						r_lch = math::mix(a_lch, b_lch, step_i / (float)(steps - 1));
						if (r_lch.lch.h < 0) r_lch.lch.h += 360;
						r = r_lch.lchToRgbD50().normalizeRgbInplace();
						store(color_list, &r, step_i, name_assigner);
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

void dialog_mix_show(GtkWindow* parent, ColorList *selected_color_list, GlobalState* gs) {
	DialogMixArgs *args = new DialogMixArgs;
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick.mix");

	GtkWidget *table;
	GtkWidget *mix_type, *mix_steps;

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Mix colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			nullptr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1),
		args->options->getInt32("window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(3, 2, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Type:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_type = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("RGB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("HSV"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("LAB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("LCH"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(mix_type), args->options->getInt32("type", 0));
	gtk_table_attach(GTK_TABLE(table), mix_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->mix_type = mix_type;
	g_signal_connect(G_OBJECT(mix_type), "changed", G_CALLBACK (update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Steps:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range (3,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), args->options->getInt32("steps", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->mix_steps = mix_steps;
	g_signal_connect (G_OBJECT (mix_steps), "value-changed", G_CALLBACK (update), args);

	args->toggle_endpoints = gtk_check_button_new_with_mnemonic (_("_Include Endpoints"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_endpoints), args->options->getBool("includeendpoints", true));
	gtk_table_attach(GTK_TABLE(table), args->toggle_endpoints,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT(args->toggle_endpoints), "toggled", G_CALLBACK (update), args);
	table_y++;

	GtkWidget* preview_expander;
	ColorList* preview_color_list=nullptr;
	gtk_table_attach(GTK_TABLE(table), preview_expander = palette_list_preview_new(gs, true, args->options->getBool("show_preview", true), gs->getColorList(), &preview_color_list), 0, 2, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;

	args->selected_color_list = selected_color_list;
	args->preview_color_list = preview_color_list;

	update(0, args);

	gtk_widget_show_all(table);
	setDialogContent(dialog, table);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(args, false, 0);

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("window.width", width);
	args->options->set("window.height", height);
	args->options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(preview_expander)));
	gtk_widget_destroy(dialog);
	color_list_destroy(args->preview_color_list);
	delete args;
}
