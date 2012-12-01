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

#include "uiDialogGenerate.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"
#include "ToolColorNaming.h"
#include "ColorRYB.h"
#include "Noise.h"
#include "GenerateScheme.h"
#include "Internationalisation.h"

#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;

typedef struct DialogGenerateArgs{
	GtkWidget *gen_type;
	GtkWidget *wheel_type;

	GtkWidget *range_colors;
	GtkWidget *range_chaos;
	GtkWidget *range_chaos_seed;
	GtkWidget *toggle_reverse;

	struct ColorList *color_list;
	struct ColorList *selected_color_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
}DialogGenerateArgs;

typedef struct ColorWheelType{
	const char *name;
	void (*hue_to_hsl)(double hue, Color* hsl);
	void (*rgbhue_to_hue)(double rgbhue, double *hue);
}ColorWheelType;

class GenerateColorNameAssigner: public ToolColorNameAssigner {
	protected:
		stringstream m_stream;
		int32_t m_ident;
		int32_t m_schemetype;
	public:
		GenerateColorNameAssigner(GlobalState *gs):ToolColorNameAssigner(gs){
		}

		void assign(struct ColorObject *color_object, Color *color, const int32_t ident, const int32_t schemetype){
			m_ident = ident;
			m_schemetype = schemetype;
			ToolColorNameAssigner::assign(color_object, color);
		}

		virtual std::string getToolSpecificName(struct ColorObject *color_object, Color *color){
			m_stream.str("");
			m_stream << "scheme " << generate_scheme_get_scheme_type(m_schemetype)->name << " #" << m_ident << "[" << color_names_get(m_gs->color_names, color, false) << "]";
			return m_stream.str();
		}
};

static void rgb_hue2hue(double hue, Color* hsl){
	hsl->hsl.hue = hue;
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5;
}

static void rgb_rgbhue2hue(double rgbhue, double *hue){
	*hue = rgbhue;
}

static void ryb1_hue2hue(double hue, Color* hsl){
	Color c;
	color_rybhue_to_rgb(hue, &c);
	color_rgb_to_hsl(&c, hsl);
}

static void ryb1_rgbhue2hue(double rgbhue, double *hue){
	color_rgbhue_to_rybhue(rgbhue, hue);
}

static void ryb2_hue2hue(double hue, Color* hsl){
	hsl->hsl.hue = color_rybhue_to_rgbhue_f(hue);
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5;
}

static void ryb2_rgbhue2hue(double rgbhue, double *hue){
	color_rgbhue_to_rybhue_f(rgbhue, hue);
}

const ColorWheelType color_wheel_types[]={
	{"RGB", rgb_hue2hue, rgb_rgbhue2hue},
	{"RYB v1", ryb1_hue2hue, ryb1_rgbhue2hue},
	{"RYB v2", ryb2_hue2hue, ryb2_rgbhue2hue},
};



static void calc( DialogGenerateArgs *args, bool preview, int limit){
	int32_t type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->gen_type));
	int32_t wheel_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->wheel_type));
	int32_t color_count = static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_colors)));
	double chaos = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos));
	int32_t chaos_seed = static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_chaos_seed)));
	bool reverse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_reverse));
        GenerateColorNameAssigner name_assigner(args->gs);

	if (!preview){
		dynv_set_int32(args->params, "type", type);
		dynv_set_int32(args->params, "wheel_type", wheel_type);
		dynv_set_int32(args->params, "colors", color_count);
		dynv_set_float(args->params, "chaos", chaos);
		dynv_set_int32(args->params, "chaos_seed", chaos_seed);
		dynv_set_bool(args->params, "reverse", reverse);
	}

	Color r, hsl, hsl_results;;
	double hue;
	double hue_step;

	stringstream s;

	struct ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->colors;

	const ColorWheelType *wheel = &color_wheel_types[wheel_type];

	struct Random* random = random_new("SHR3");
	unsigned long seed = chaos_seed;
	random_seed(random, &seed);
	random_get(random);

	for (ColorList::iter i=args->selected_color_list->colors.begin(); i!=args->selected_color_list->colors.end(); ++i){
		Color in;
		color_object_get_color(*i, &in);

		color_rgb_to_hsl(&in, &hsl);

		wheel->rgbhue_to_hue(hsl.hsl.hue, &hue);

		wheel->hue_to_hsl(hue, &hsl_results);

		double saturation = hsl.hsl.saturation * 1 / hsl_results.hsl.saturation;
		double lightness = hsl.hsl.lightness - hsl_results.hsl.lightness;

		for (int32_t i = 0; i < color_count; i++){
			if (preview){
				if (limit<=0){
					random_destroy(random);
					return;
				}
				limit--;
			}

			wheel->hue_to_hsl(hue, &hsl);

			hsl.hsl.lightness = clamp_float(hsl.hsl.lightness + lightness, 0, 1);
			hsl.hsl.saturation = clamp_float(hsl.hsl.saturation * saturation, 0, 1);

			color_hsl_to_rgb(&hsl, &r);
			struct ColorObject *color_object=color_list_new_color_object(color_list, &r);
                        name_assigner.assign (color_object, &r, i, type);
			color_list_add_color_object(color_list, color_object, 1);
			color_object_release(color_object);

			hue_step = (generate_scheme_get_scheme_type(type)->turn[i % generate_scheme_get_scheme_type(type)->turn_types]) / (360.0)
				+ chaos*(((random_get(random)&0xFFFFFFFF)/(gdouble)0xFFFFFFFF)-0.5);

			if (reverse){
				hue = wrap_float(hue - hue_step);
			}else{
				hue = wrap_float(hue + hue_step);
			}
		}

	}

	random_destroy(random);
}

static void update(GtkWidget *widget, DialogGenerateArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}

void dialog_generate_show(GtkWindow* parent, struct ColorList *selected_color_list, GlobalState* gs){
	DialogGenerateArgs *args = new DialogGenerateArgs;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick.generate");

	GtkWidget *table;

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Generate colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);

	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1),
		dynv_get_int32_wd(args->params, "window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(4, 4, FALSE);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Colors:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_colors = gtk_spin_button_new_with_range (1, 72, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_colors), dynv_get_int32_wd(args->params, "colors", 1));
	gtk_table_attach(GTK_TABLE(table), args->range_colors,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT (args->range_colors), "value-changed", G_CALLBACK (update), args);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Type:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->gen_type = gtk_combo_box_new_text();
	for (uint32_t i = 0; i < generate_scheme_get_n_scheme_types(); i++){
		gtk_combo_box_append_text(GTK_COMBO_BOX(args->gen_type), generate_scheme_get_scheme_type(i)->name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->gen_type), dynv_get_int32_wd(args->params, "type", 0));
	g_signal_connect (G_OBJECT (args->gen_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->gen_type,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Color wheel:"),0,0.5,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->wheel_type = gtk_combo_box_new_text();
	for (uint32_t i=0; i<sizeof(color_wheel_types)/sizeof(ColorWheelType); i++){
		gtk_combo_box_append_text(GTK_COMBO_BOX(args->wheel_type), color_wheel_types[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->wheel_type), dynv_get_int32_wd(args->params, "wheel_type", 0));
	g_signal_connect (G_OBJECT (args->wheel_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->wheel_type,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,0);
	table_y++;


	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Chaos:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_chaos = gtk_spin_button_new_with_range (0,1,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_chaos), dynv_get_float_wd(args->params, "chaos", 0));
	gtk_table_attach(GTK_TABLE(table), args->range_chaos,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT (args->range_chaos), "value-changed", G_CALLBACK (update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Seed:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_chaos_seed = gtk_spin_button_new_with_range (0, 0xFFFF, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_chaos_seed), dynv_get_int32_wd(args->params, "chaos_seed", 0));
	gtk_table_attach(GTK_TABLE(table), args->range_chaos_seed,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT (args->range_chaos_seed), "value-changed", G_CALLBACK (update), args);

	table_y++;

	args->toggle_reverse = gtk_check_button_new_with_mnemonic (_("_Reverse"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_reverse), dynv_get_bool_wd(args->params, "reverse", false));
	gtk_table_attach(GTK_TABLE(table), args->toggle_reverse,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT(args->toggle_reverse), "toggled", G_CALLBACK (update), args);
	table_y++;



	GtkWidget* preview_expander;
	struct ColorList* preview_color_list=NULL;
	gtk_table_attach(GTK_TABLE(table), preview_expander=palette_list_preview_new(gs, true, dynv_get_bool_wd(args->params, "show_preview", true), gs->colors, &preview_color_list), 0, 4, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
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
