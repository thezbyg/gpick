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

#include "uiDialogGenerate.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "MathUtil.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "color_names/ColorNames.h"
#include "ColorRYB.h"
#include "Noise.h"
#include "GenerateScheme.h"
#include "I18N.h"
#include "Random.h"
#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;

typedef struct DialogGenerateArgs
{
	GtkWidget *gen_type;
	GtkWidget *wheel_type;
	GtkWidget *range_colors;
	GtkWidget *range_chaos;
	GtkWidget *range_additional_rotation;
	GtkWidget *range_chaos_seed;
	GtkWidget *toggle_reverse;
	ColorList *color_list;
	ColorList *selected_color_list;
	ColorList *preview_color_list;
	dynv::Ref options;
	GlobalState* gs;
}DialogGenerateArgs;

typedef struct ColorWheelType
{
	const char *name;
	void (*hue_to_hsl)(double hue, Color* hsl);
	void (*rgbhue_to_hue)(double rgbhue, double *hue);
}ColorWheelType;

struct GenerateColorNameAssigner: public ToolColorNameAssigner
{
	protected:
		stringstream m_stream;
		string m_scheme_name;
		int32_t m_ident;
	public:
		GenerateColorNameAssigner(GlobalState *gs):
			ToolColorNameAssigner(gs)
		{
		}
		void assign(ColorObject *color_object, const Color *color, const int32_t ident, const char *scheme_name)
		{
			m_ident = ident;
			m_scheme_name = scheme_name;
			ToolColorNameAssigner::assign(color_object, color);
		}
		virtual std::string getToolSpecificName(ColorObject *color_object, const Color *color)
		{
			m_stream.str("");
			m_stream << _("scheme") << " " << m_scheme_name << " #" << m_ident << "[" << color_names_get(m_gs->getColorNames(), color, false) << "]";
			return m_stream.str();
		}
};
static void rgb_hue2hue(double hue, Color* hsl)
{
	hsl->hsl.hue = static_cast<float>(hue);
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5f;
}
static void rgb_rgbhue2hue(double rgbhue, double *hue)
{
	*hue = rgbhue;
}
static void ryb1_hue2hue(double hue, Color* hsl)
{
	Color c;
	color_rybhue_to_rgb(hue, &c);
	*hsl = c.rgbToHsl();
}
static void ryb1_rgbhue2hue(double rgbhue, double *hue)
{
	color_rgbhue_to_rybhue(rgbhue, hue);
}
static void ryb2_hue2hue(double hue, Color* hsl)
{
	hsl->hsl.hue = static_cast<float>(color_rybhue_to_rgbhue_f(hue));
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5f;
}
static void ryb2_rgbhue2hue(double rgbhue, double *hue)
{
	color_rgbhue_to_rybhue_f(rgbhue, hue);
}
const ColorWheelType color_wheel_types[] = {
	{"RGB", rgb_hue2hue, rgb_rgbhue2hue},
	{"RYB v1", ryb1_hue2hue, ryb1_rgbhue2hue},
	{"RYB v2", ryb2_hue2hue, ryb2_rgbhue2hue},
};
static void calc(DialogGenerateArgs *args, bool preview, int limit)
{
	int32_t type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->gen_type));
	int32_t wheel_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->wheel_type));
	int32_t color_count = static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_colors)));
	float chaos = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos)));
	int32_t chaos_seed = static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos_seed)));
	bool reverse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_reverse));
	float additional_rotation = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_additional_rotation)));
	if (!preview){
		args->options->set("type", type);
		args->options->set("wheel_type", wheel_type);
		args->options->set("colors", color_count);
		args->options->set("chaos", chaos);
		args->options->set("chaos_seed", chaos_seed);
		args->options->set("reverse", reverse);
		args->options->set("additional_rotation", additional_rotation);
	}
	GenerateColorNameAssigner name_assigner(args->gs);
	Color r, hsl, hsl_results;
	double hue;
	double hue_step;
	ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->getColorList();
	const ColorWheelType *wheel = &color_wheel_types[wheel_type];
	struct Random* random = random_new("SHR3", chaos_seed);
	const SchemeType *scheme_type;
	SchemeType static_scheme_type = {_("Static"), 1, 1, {0}};
	if (static_cast<size_t>(type) >= generate_scheme_get_n_scheme_types()){
		scheme_type = &static_scheme_type;
	}else{
		scheme_type = generate_scheme_get_scheme_type(type);
	}
	for (ColorList::iter i = args->selected_color_list->colors.begin(); i != args->selected_color_list->colors.end(); ++i){
		Color in = (*i)->getColor();
		hsl = in.rgbToHsl();
		wheel->rgbhue_to_hue(hsl.hsl.hue, &hue);
		wheel->hue_to_hsl(hue, &hsl_results);
		double saturation = hsl.hsl.saturation * 1 / hsl_results.hsl.saturation;
		double lightness = hsl.hsl.lightness - hsl_results.hsl.lightness;
		for (int32_t i = 0; i < color_count; i++){
			if (preview){
				if (limit <= 0){
					random_destroy(random);
					return;
				}
				limit--;
			}
			wheel->hue_to_hsl(hue, &hsl);
			hsl.hsl.lightness = math::clamp(static_cast<float>(hsl.hsl.lightness + lightness), 0.0f, 1.0f);
			hsl.hsl.saturation = math::clamp(static_cast<float>(hsl.hsl.saturation * saturation), 0.0f, 1.0f);
			r = hsl.hslToRgb();
			ColorObject *color_object = color_list_new_color_object(color_list, &r);
			name_assigner.assign(color_object, &r, i, scheme_type->name);
			color_list_add_color_object(color_list, color_object, 1);
			color_object->release();
			hue_step = (scheme_type->turn[i % scheme_type->turn_types]) / (360.0f)
				+ chaos * (random_get_double(random) - 0.5f) + additional_rotation / 360.0f;
			if (reverse){
				hue = math::wrap(static_cast<float>(hue - hue_step));
			}else{
				hue = math::wrap(static_cast<float>(hue + hue_step));
			}
		}
	}
	random_destroy(random);
}
static void update(GtkWidget *widget, DialogGenerateArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}
void dialog_generate_show(GtkWindow* parent, ColorList *selected_color_list, GlobalState* gs)
{
	DialogGenerateArgs *args = new DialogGenerateArgs;
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick.generate");
	GtkWidget *table;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Generate colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1),
		args->options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(4, 4, FALSE);
	table_y = 0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Colors:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_colors = gtk_spin_button_new_with_range(1, 72, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_colors), args->options->getInt32("colors", 1));
	gtk_table_attach(GTK_TABLE(table), args->range_colors,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->range_colors), "value-changed", G_CALLBACK(update), args);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Type:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->gen_type = gtk_combo_box_text_new();
	for (uint32_t i = 0; i < generate_scheme_get_n_scheme_types(); i++){
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(args->gen_type), _(generate_scheme_get_scheme_type(i)->name));
	}
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(args->gen_type), _("Static"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->gen_type), args->options->getInt32("type", 0));
	g_signal_connect(G_OBJECT(args->gen_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->gen_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Color wheel:"),0,0.5,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->wheel_type = gtk_combo_box_text_new();
	for (uint32_t i = 0; i < sizeof(color_wheel_types) / sizeof(ColorWheelType); i++){
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(args->wheel_type), color_wheel_types[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->wheel_type), args->options->getInt32("wheel_type", 0));
	g_signal_connect(G_OBJECT(args->wheel_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->wheel_type,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Chaos:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_chaos = gtk_spin_button_new_with_range(0,1,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_chaos), args->options->getFloat("chaos", 0));
	gtk_table_attach(GTK_TABLE(table), args->range_chaos,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->range_chaos), "value-changed", G_CALLBACK(update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Seed:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_chaos_seed = gtk_spin_button_new_with_range(0, 0xFFFF, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_chaos_seed), args->options->getInt32("chaos_seed", 0));
	gtk_table_attach(GTK_TABLE(table), args->range_chaos_seed,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->range_chaos_seed), "value-changed", G_CALLBACK(update), args);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Rotation:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_additional_rotation = gtk_spin_button_new_with_range(-360, 360, 0.1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_additional_rotation), args->options->getFloat("range_additional_rotation", 0));
	gtk_table_attach(GTK_TABLE(table), args->range_additional_rotation,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->range_additional_rotation), "value-changed", G_CALLBACK(update), args);
	table_y++;

	args->toggle_reverse = gtk_check_button_new_with_mnemonic(_("_Reverse"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_reverse), args->options->getBool("reverse", false));
	gtk_table_attach(GTK_TABLE(table), args->toggle_reverse,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->toggle_reverse), "toggled", G_CALLBACK(update), args);
	table_y++;

	GtkWidget* preview_expander;
	ColorList* preview_color_list = nullptr;
	gtk_table_attach(GTK_TABLE(table), preview_expander = palette_list_preview_new(gs, true, args->options->getBool("show_preview", true), gs->getColorList(), &preview_color_list), 0, 4, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
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
