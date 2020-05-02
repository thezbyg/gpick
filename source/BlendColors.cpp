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

#include "BlendColors.h"
#include "ColorObject.h"
#include "ColorSource.h"
#include "ColorSourceManager.h"
#include "ColorUtils.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "DragDrop.h"
#include "ColorList.h"
#include "color_names/ColorNames.h"
#include "MathUtil.h"
#include "ColorRYB.h"
#include "gtk/ColorWidget.h"
#include "uiColorInput.h"
#include "ToolColorNaming.h"
#include "I18N.h"
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>
#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <sstream>
using namespace std;

typedef struct BlendColorsArgs{
	ColorSource source;
	GtkWidget *main;
	GtkWidget *mix_type;
	GtkWidget *steps1;
	GtkWidget *steps2;
	GtkWidget *start_color;
	GtkWidget *middle_color;
	GtkWidget *end_color;
	ColorList *preview_color_list;
	dynv::Ref options;
	GlobalState* gs;
}BlendColorsArgs;

struct BlendColorNameAssigner: public ToolColorNameAssigner
{
	protected:
		stringstream m_stream;
		const char *m_color_start;
		const char *m_color_end;
		int m_start_percent;
		int m_end_percent;
		int m_steps;
		int m_stage;
		bool m_is_color_item;
	public:
		BlendColorNameAssigner(GlobalState *gs):
			ToolColorNameAssigner(gs)
		{
			m_is_color_item = false;
		}
		void setNames(const char *start_color_name, const char *end_color_name)
		{
			m_color_start = start_color_name;
			m_color_end = end_color_name;
		}
		void setStepsAndStage(int steps, int stage)
		{
			m_steps = steps;
			m_stage = stage;
		}
		void assign(ColorObject *color_object, const Color *color, int step)
		{
			m_start_percent = step * 100 / (m_steps - 1);
			m_end_percent = 100 - (step * 100 / (m_steps - 1));
			m_is_color_item = (((step == 0 || step == m_steps - 1) && m_stage == 0) || (m_stage == 1 && step == m_steps - 1));
			ToolColorNameAssigner::assign(color_object, color);
		}
		void assign(ColorObject *color_object, const Color *color, const char *item_name)
		{
			m_color_start = item_name;
			m_is_color_item = true;
			ToolColorNameAssigner::assign(color_object, color);
		}
		virtual std::string getToolSpecificName(ColorObject *color_object, const Color *color)
		{
			m_stream.str("");
			if (m_is_color_item){
				if (m_end_percent == 100){
					m_stream << m_color_end << " " << _("blend node");
				}else{
					m_stream << m_color_start << " " << _("blend node");
				}
			}else{
				m_stream << m_color_start << " " << m_start_percent << " " << _("blend") << " " << m_end_percent << " " << m_color_end;
			}
			return m_stream.str();
		}
};
static int source_get_color(BlendColorsArgs *args, ColorObject** color);

static void store(ColorList *color_list, const Color *color, int step, BlendColorNameAssigner &name_assigner)
{
	ColorObject *color_object = color_list_new_color_object(color_list, color);
	name_assigner.assign(color_object, color, step);
	color_list_add_color_object(color_list, color_object, 1);
	color_object->release();
}

static void calc(BlendColorsArgs *args, bool preview, int limit)
{
	gint steps1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps1));
	gint steps2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps2));
	gint type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));
	Color r;
	gint step_i;
	Color a,b;
	ColorList *color_list;
	color_list = args->preview_color_list;
	BlendColorNameAssigner name_assigner(args->gs);
	int steps;
	for (int stage = 0; stage < 2; stage++){
		if (stage == 0){
			steps = steps1 + 1;
			gtk_color_get_color(GTK_COLOR(args->start_color), &a);
			gtk_color_get_color(GTK_COLOR(args->middle_color), &b);
		}else{
			steps = steps2 + 1;
			gtk_color_get_color(GTK_COLOR(args->middle_color), &a);
			gtk_color_get_color(GTK_COLOR(args->end_color), &b);
		}
		string start_name = color_names_get(args->gs->getColorNames(), &a, false);
		string end_name = color_names_get(args->gs->getColorNames(), &b, false);
		name_assigner.setNames(start_name.c_str(), end_name.c_str());
		name_assigner.setStepsAndStage(steps, stage);
		if (type == 0){
			color_rgb_get_linear(&a, &a);
			color_rgb_get_linear(&b, &b);
		}
		step_i = stage;
		switch (type){
		case 0:
			for (; step_i < steps; ++step_i){
				color_utils::mix(a, b, step_i / (float)(steps - 1), r);
				color_linear_get_rgb(&r, &r);
				store(color_list, &r, step_i, name_assigner);
			}
			break;
		case 1:
			{
				Color a_hsv, b_hsv, r_hsv;
				color_rgb_to_hsv(&a, &a_hsv);
				color_rgb_to_hsv(&b, &b_hsv);
				if (a_hsv.hsv.hue > b_hsv.hsv.hue){
					if (a_hsv.hsv.hue - b_hsv.hsv.hue > 0.5)
						a_hsv.hsv.hue -= 1;
				}else{
					if (b_hsv.hsv.hue - a_hsv.hsv.hue > 0.5)
						b_hsv.hsv.hue -= 1;
				}
				for (; step_i < steps; ++step_i){
					color_utils::mix(a_hsv, b_hsv, step_i / (float)(steps - 1), r_hsv);
					if (r_hsv.hsv.hue < 0) r_hsv.hsv.hue += 1;
					color_hsv_to_rgb(&r_hsv, &r);
					store(color_list, &r, step_i, name_assigner);
				}
			}
			break;
		case 2:
			{
				Color a_lab, b_lab, r_lab;
				color_rgb_to_lab_d50(&a, &a_lab);
				color_rgb_to_lab_d50(&b, &b_lab);
				for (; step_i < steps; ++step_i){
					color_utils::mix(a_lab, b_lab, step_i / (float)(steps - 1), r_lab);
					color_lab_to_rgb_d50(&r_lab, &r);
					color_rgb_normalize(&r);
					store(color_list, &r, step_i, name_assigner);
				}
			}
			break;
		case 3:
			{
				Color a_lch, b_lch, r_lch;
				color_rgb_to_lch_d50(&a, &a_lch);
				color_rgb_to_lch_d50(&b, &b_lch);
				if (a_lch.lch.h>b_lch.lch.h){
					if (a_lch.lch.h - b_lch.lch.h > 180)
						a_lch.lch.h -= 360;
				}else{
					if (b_lch.lch.h - a_lch.lch.h > 180)
						b_lch.lch.h -= 360;
				}
				for (; step_i < steps; ++step_i){
					color_utils::mix(a_lch, b_lch, step_i / (float)(steps - 1), r_lch);
					if (r_lch.lch.h < 0) r_lch.lch.h += 360;
					color_lch_to_rgb_d50(&r_lch, &r);
					color_rgb_normalize(&r);
					store(color_list, &r, step_i, name_assigner);
				}
			}
			break;
		}
	}
}
static void update(GtkWidget *widget, BlendColorsArgs *args)
{
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 101);
}
static void reset_middle_color_cb(GtkWidget *widget, BlendColorsArgs *args)
{
	Color a, b;
	gtk_color_get_color(GTK_COLOR(args->start_color), &a);
	gtk_color_get_color(GTK_COLOR(args->end_color), &b);
	color_multiply(&a, 0.5);
	color_multiply(&b, 0.5);
	color_add(&a, &b);
	gtk_color_set_color(GTK_COLOR(args->middle_color), &a, "");
	update(0, args);
}
static gboolean color_button_press_cb(GtkWidget *widget, GdkEventButton *event, BlendColorsArgs *args)
{
	GtkWidget *menu;
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		GtkWidget* item ;
		gint32 button, event_time;
		menu = gtk_menu_new();
		item = gtk_menu_item_new_with_mnemonic(_("_Reset"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(reset_middle_color_cb), args);
		gtk_widget_show_all(GTK_WIDGET(menu));
		button = event->button;
		event_time = event->time;
		gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, nullptr, nullptr, button, event_time);
		g_object_ref_sink(menu);
		g_object_unref(menu);
		return TRUE;
	}
	return FALSE;
}
static int set_rgb_color(BlendColorsArgs *args, ColorObject* color_object, uint32_t color_index)
{
	Color color = color_object->getColor();
	if (color_index == 1){
		gtk_color_set_color(GTK_COLOR(args->start_color), &color, "");
	}else if (color_index == 2){
		gtk_color_set_color(GTK_COLOR(args->middle_color), &color, "");
	}else if (color_index == 3){
		gtk_color_set_color(GTK_COLOR(args->end_color), &color, "");
	}
	update(0, args);
	return 0;
}
static int get_rgb_color(BlendColorsArgs *args, uint32_t color_index, ColorObject** color)
{
	Color c;
	if (color_index == 1){
		gtk_color_get_color(GTK_COLOR(args->start_color), &c);
	}else if (color_index == 2){
		gtk_color_get_color(GTK_COLOR(args->middle_color), &c);
	}else if (color_index == 3){
		gtk_color_get_color(GTK_COLOR(args->end_color), &c);
	}
	*color = color_list_new_color_object(args->gs->getColorList(), &c);
	BlendColorNameAssigner name_assigner(args->gs);
	const char *item_name[] = {
		"start",
		"middle",
		"end",
	};
	name_assigner.assign(*color, &c, item_name[color_index - 1]);
	return 0;
}
static ColorObject* get_color_object(struct DragDrop* dd)
{
	BlendColorsArgs* args = (BlendColorsArgs*)dd->userdata;
	ColorObject* color_object;
	if (get_rgb_color(args, (uintptr_t)dd->userdata2, &color_object) == 0){
		return color_object;
	}
	return 0;
}
static int set_color_object_at(struct DragDrop* dd, ColorObject* color_object, int x, int y, bool, bool)
{
	BlendColorsArgs* args = static_cast<BlendColorsArgs*>(dd->userdata);
	set_rgb_color(args, color_object, (uintptr_t)dd->userdata2);
	return 0;
}
static int source_get_color(BlendColorsArgs *args, ColorObject** color)
{
	return -1;
}
static int source_set_color(BlendColorsArgs *args, ColorObject* color)
{
	return -1;
}
static int source_activate(BlendColorsArgs *args)
{
	return 0;
}
static int source_deactivate(BlendColorsArgs *args)
{
	return 0;
}
static int source_destroy(BlendColorsArgs *args)
{
	int steps1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps1));
	int steps2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->steps2));
	int type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->mix_type));
	args->options->set("type", type);
	args->options->set("steps1", steps1);
	args->options->set("steps2", steps2);
	Color c;
	gtk_color_get_color(GTK_COLOR(args->start_color), &c);
	args->options->set("start_color", c);
	gtk_color_get_color(GTK_COLOR(args->middle_color), &c);
	args->options->set("middle_color", c);
	gtk_color_get_color(GTK_COLOR(args->end_color), &c);
	args->options->set("end_color", c);
	color_list_destroy(args->preview_color_list);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}
static ColorSource* source_implement(ColorSource *source, GlobalState *gs, const dynv::Ref &options)
{
	BlendColorsArgs *args = new BlendColorsArgs;
	args->options = options;
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
	table = gtk_table_new(6, 2, FALSE);
	table_y = 0;
	struct DragDrop dd;
	dragdrop_init(&dd, gs);
	dd.converterType = Converters::Type::display;
	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;
	Color c;
	color_set(&c, 0.5);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Start:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->start_color = widget = gtk_color_new();
	gtk_color_set_color(GTK_COLOR(args->start_color), args->options->getColor("start_color", c));
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.userdata2 = (void*)1;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Middle:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->middle_color = widget = gtk_color_new();
	g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(color_button_press_cb), args);
	gtk_color_set_color(GTK_COLOR(args->middle_color), args->options->getColor("middle_color", c));
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.userdata2 = (void*)2;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("End:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->end_color = widget = gtk_color_new();
	gtk_color_set_color(GTK_COLOR(args->end_color), args->options->getColor("end_color", c));
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.userdata2 = (void*)3;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);
	table_y = 0;
	GtkWidget* vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_aligned_new(_("Type:"),0,0,0,0), false, false, 0);
	args->mix_type = mix_type = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("RGB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("HSV"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("LAB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mix_type), _("LCH"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(mix_type), args->options->getInt32("type", 0));
	gtk_box_pack_start(GTK_BOX(vbox), mix_type, false, false, 0);
	g_signal_connect(G_OBJECT(mix_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), vbox, 4, 5, table_y, table_y+3, GtkAttachOptions(GTK_FILL),GtkAttachOptions(GTK_FILL),5,0);
	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Start steps:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range(1,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), args->options->getInt32("steps1", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->steps1 = mix_steps;
	g_signal_connect(G_OBJECT(mix_steps), "value-changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("End steps:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	mix_steps = gtk_spin_button_new_with_range(1,255,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(mix_steps), args->options->getInt32("steps2", 3));
	gtk_table_attach(GTK_TABLE(table), mix_steps,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->steps2 = mix_steps;
	g_signal_connect(G_OBJECT(mix_steps), "value-changed", G_CALLBACK(update), args);
	table_y = 3;
	ColorList* preview_color_list = nullptr;
	gtk_table_attach(GTK_TABLE(table), palette_list_preview_new(gs, false, false, gs->getColorList(), &preview_color_list), 0, 5, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;
	args->preview_color_list = preview_color_list;
	update(0, args);
	gtk_widget_show_all(table);
	args->main = table;
	args->source.widget = table;
	return (ColorSource*)args;
}
int blend_colors_source_register(ColorSourceManager *csm)
{
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "blend_colors", _("Blend colors"));
	color_source->implement = source_implement;
	color_source->default_accelerator = GDK_KEY_b;
	color_source_manager_add_source(csm, color_source);
	return 0;
}
