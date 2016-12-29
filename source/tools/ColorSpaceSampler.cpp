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

#include "ColorSpaceSampler.h"
#include "../ColorList.h"
#include "../ColorObject.h"
#include "../GlobalState.h"
#include "../Internationalisation.h"
#include "../DynvHelpers.h"
#include "../uiListPalette.h"
#include "../uiUtilities.h"
#include "../ToolColorNaming.h"
#include <sstream>
#include <algorithm>
using namespace std;

#define N_AXIS 3

typedef struct AxisOptions
{
	GtkWidget *range_samples;
	GtkWidget *range_min_value;
	GtkWidget *range_max_value;
	GtkWidget *toggle_include_max_value;
	int samples;
	float min_value;
	float max_value;
	bool include_max_value;
}AxisOptions;

typedef struct ColorSpaceSamplerArgs
{
	GtkWidget *combo_color_space;
	GtkWidget *toggle_linearization;
	int color_space;
	bool linearization;
	AxisOptions axis[N_AXIS];
	GtkWidget *preview_expander;
	ColorList *color_list;
	ColorList *preview_color_list;
	struct dynvSystem *params;
	GlobalState* gs;
}ColorSpaceSamplerArgs;

class ColorSpaceSamplerNameAssigner: public ToolColorNameAssigner
{
	protected:
		stringstream m_stream;
	public:
		ColorSpaceSamplerNameAssigner(GlobalState *gs):
			ToolColorNameAssigner(gs)
		{
		}
		void assign(ColorObject *color_object, const Color *color)
		{
			ToolColorNameAssigner::assign(color_object, color);
		}
		virtual std::string getToolSpecificName(ColorObject *color_object, const Color *color)
		{
			m_stream.str("");
			m_stream << _("color space");
			return m_stream.str();
		}
};
static void calc(ColorSpaceSamplerArgs *args, bool preview, size_t limit)
{
	ColorSpaceSamplerNameAssigner name_assigner(args->gs);
	ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->getColorList();
	vector<Color> values;
	size_t value_count = args->axis[0].samples * args->axis[1].samples * args->axis[2].samples;
	if (preview)
		value_count = std::min(limit, value_count);
	values.resize(value_count);
	size_t value_i = 0;
	for (int x = 0; x < args->axis[0].samples; x++){
		float x_value = (args->axis[0].samples > 1) ? (args->axis[0].min_value + (args->axis[0].max_value - args->axis[0].min_value) * (x / (double)(args->axis[0].samples - 1))) : args->axis[0].min_value;
		for (int y = 0; y < args->axis[1].samples; y++){
			float y_value = (args->axis[1].samples > 1) ? (args->axis[1].min_value + (args->axis[1].max_value - args->axis[1].min_value) * (y / (double)(args->axis[1].samples - 1))) : args->axis[1].min_value;
			for (int z = 0; z < args->axis[2].samples; z++){
				float z_value = (args->axis[2].samples > 1) ? (args->axis[2].min_value + (args->axis[2].max_value - args->axis[2].min_value) * (z / (double)(args->axis[2].samples - 1))) : args->axis[2].min_value;
				values[value_i].ma[0] = x_value;
				values[value_i].ma[1] = y_value;
				values[value_i].ma[2] = z_value;
				value_i++;
				if (preview && value_i >= limit){
					x = args->axis[0].samples;
					y = args->axis[1].samples;
					break;
				}
			}
		}
	}
	Color t;
	for (size_t i = 0; i < value_count; i++){
		if (preview){
			if (limit <= 0) return;
			limit--;
		}
		switch (args->color_space){
			case 0:
				color_copy(&values[i], &t);
				break;
			case 1:
				color_hsv_to_rgb(&values[i], &t);
				break;
			case 2:
				color_hsl_to_rgb(&values[i], &t);
				break;
			case 3:
				color_copy(&values[i], &t);
				t.lab.L *= 100;
				t.lab.a = (t.lab.a - 0.5) * 290;
				t.lab.b = (t.lab.b - 0.5) * 290;
				color_lab_to_rgb_d50(&t, &t);
				break;
			case 4:
				color_copy(&values[i], &t);
				t.lch.L *= 100;
				t.lch.C *= 136;
				t.lch.h *= 360;
				color_lch_to_rgb_d50(&t, &t);
				break;
		}
		if (args->linearization)
			color_linear_get_rgb(&t, &t);
		color_rgb_normalize(&t);
		ColorObject *color_object = color_list_new_color_object(color_list, &t);
		name_assigner.assign(color_object, &t);
		color_list_add_color_object(color_list, color_object, 1);
		color_object->release();
	}
}
static void destroy_cb(GtkWidget* widget, ColorSpaceSamplerArgs *args)
{
	color_list_destroy(args->preview_color_list);
	dynv_system_release(args->params);
	delete args;
}
static void get_settings(ColorSpaceSamplerArgs *args)
{
	args->color_space = gtk_combo_box_get_active(GTK_COMBO_BOX(args->combo_color_space));
	args->linearization = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_linearization));
	for (int i = 0; i < N_AXIS; i++){
		args->axis[i].samples = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->axis[i].range_samples));
		args->axis[i].min_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->axis[i].range_min_value));
		args->axis[i].max_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->axis[i].range_max_value));
	}
}
static dynvSystem* get_axis_config(int axis, ColorSpaceSamplerArgs *args)
{
	stringstream config_name;
	config_name << "axis" << axis;
	string config_name_string = config_name.str();
	return dynv_get_dynv(args->params, config_name_string.c_str());
}
static void save_settings(ColorSpaceSamplerArgs *args)
{
	dynv_set_int32(args->params, "color_space", args->color_space);
	dynv_set_bool(args->params, "linearization", args->linearization);
	for (int i = 0; i < N_AXIS; i++){
		dynvSystem *axis_config = get_axis_config(i, args);
		dynv_set_int32(axis_config, "samples", args->axis[i].samples);
		dynv_set_float(axis_config, "min_value", args->axis[i].min_value);
		dynv_set_float(axis_config, "max_value", args->axis[i].max_value);
		dynv_system_release(axis_config);
	}
}
static void update(GtkWidget *widget, ColorSpaceSamplerArgs *args)
{
	color_list_remove_all(args->preview_color_list);
	get_settings(args);
	calc(args, true, 100);
}
static void response_cb(GtkWidget* widget, gint response_id, ColorSpaceSamplerArgs *args)
{
	get_settings(args);
	save_settings(args);
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(widget), &width, &height);
	dynv_set_int32(args->params, "window.width", width);
	dynv_set_int32(args->params, "window.height", height);
	dynv_set_bool(args->params, "show_preview", gtk_expander_get_expanded(GTK_EXPANDER(args->preview_expander)));
	switch (response_id){
		case GTK_RESPONSE_APPLY:
			calc(args, false, 0);
			break;
		case GTK_RESPONSE_DELETE_EVENT:
			break;
		case GTK_RESPONSE_CLOSE:
			gtk_widget_destroy(widget);
			break;
	}
}
void tools_color_space_sampler_show(GtkWindow* parent, GlobalState* gs)
{
	ColorSpaceSamplerArgs *args = new ColorSpaceSamplerArgs;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->getSettings(), "gpick.tools.color_space_sampler");

	int table_m_y;
	GtkWidget *table, *table_m, *widget;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Color space sampler"), parent, GtkDialogFlags(GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_ADD, GTK_RESPONSE_APPLY, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1), dynv_get_int32_wd(args->params, "window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY, GTK_RESPONSE_CLOSE, -1);

	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	int table_y;

	table = gtk_table_new(7, 3, FALSE);
	gtk_table_attach(GTK_TABLE(table_m), table, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;

	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Color space:")), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->combo_color_space = widget = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("RGB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("HSV"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("HSL"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("LAB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("LCH"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), dynv_get_int32_wd(args->params, "color_space", 0));
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(update), args);
	table_y++;

	args->toggle_linearization = gtk_check_button_new_with_mnemonic(_("_Linearization"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_linearization), dynv_get_bool_wd(args->params, "linearization", false));
	gtk_table_attach(GTK_TABLE(table), args->toggle_linearization,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->toggle_linearization), "toggled", G_CALLBACK(update), args);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Sample count")), 1, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Minimal value")), 3, 5, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Maximal value")), 5, 7, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	table_y++;

	const char *axis_names[] = {
		_("X axis"),
		_("Y axis"),
		_("Z axis"),
	};
	for (int i = 0; i < N_AXIS; i++){
		string axis_name = string(axis_names[i]) + ":";
		dynvSystem *axis_config = get_axis_config(i, args);
		gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(axis_name.c_str()), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
		args->axis[i].range_samples = widget = gtk_spin_button_new_with_range(1, 255, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), dynv_get_int32_wd(axis_config, "samples", 12));
		gtk_table_attach(GTK_TABLE(table), widget, 1, 3, table_y, table_y + 1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL, 3, 3);
		g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(update), args);

		args->axis[i].range_min_value = widget = gtk_spin_button_new_with_range(0, 1, 0.001);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), dynv_get_float_wd(axis_config, "min_value", 0));
		gtk_table_attach(GTK_TABLE(table), widget, 3, 5, table_y, table_y + 1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL, 3, 3);
		g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(update), args);

		args->axis[i].range_max_value = widget = gtk_spin_button_new_with_range(0, 1, 0.001);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), dynv_get_float_wd(axis_config, "max_value", 1));
		gtk_table_attach(GTK_TABLE(table), widget, 5, 7, table_y, table_y + 1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL, 3, 3);
		g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(update), args);

		table_y++;
		dynv_system_release(axis_config);
	}
	ColorList* preview_color_list = nullptr;
	gtk_table_attach(GTK_TABLE(table_m), args->preview_expander = palette_list_preview_new(gs, true, dynv_get_bool_wd(args->params, "show_preview", true), gs->getColorList(), &preview_color_list), 0, 1, table_m_y, table_m_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_m_y++;

	args->preview_color_list = preview_color_list;
	get_settings(args);
	calc(args, true, 100);
	gtk_widget_show_all(table_m);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table_m);
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(destroy_cb), args);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(response_cb), args);
	gtk_widget_show(dialog);
}
