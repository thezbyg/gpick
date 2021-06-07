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

#include "uiDialogVariations.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "I18N.h"
#include <sstream>
using namespace std;

typedef struct DialogVariationsArgs
{
	GtkWidget *toggle_multiplication, *toggle_linearization;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;
	ColorList *selected_color_list;
	ColorList *preview_color_list;
	dynv::Ref options;
	GlobalState* gs;
}DialogVariationsArgs;

struct VariationsColorNameAssigner: public ToolColorNameAssigner {
	VariationsColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view name, uint32_t step_i) {
		m_name = name;
		m_step_i = step_i;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) {
		m_stream.str("");
		m_stream << m_name << " " << _("variation") << " " << m_step_i;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_name;
	uint32_t m_step_i;
};
static void calc(DialogVariationsArgs *args, bool preview, int limit)
{
	gint steps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->range_steps));
	float lightness_from = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_lightness_from)));
	float lightness_to = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_lightness_to)));
	float saturation_from = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_saturation_from)));
	float saturation_to = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_saturation_to)));
	bool multiplication = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_multiplication));
	bool linearization = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_linearization));
	if (!preview){
		args->options->set("steps", steps);
		args->options->set("lightness_from", lightness_from);
		args->options->set("lightness_to", lightness_to);
		args->options->set("saturation_from", saturation_from);
		args->options->set("saturation_to", saturation_to);
		args->options->set("multiplication", multiplication);
		args->options->set("linearization", linearization);
	}
	Color r, hsl;
	gint step_i;
	ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->getColorList();
	VariationsColorNameAssigner name_assigner(*args->gs);
	for (ColorList::iter i = args->selected_color_list->colors.begin(); i != args->selected_color_list->colors.end(); ++i){
		Color in = (*i)->getColor();
		const char* name = (*i)->getName().c_str();
		if (linearization)
			in.linearRgbInplace();
		for (step_i = 0; step_i < steps; ++step_i) {
			if (preview){
				if (limit <= 0) return;
				limit--;
			}
			hsl = in.rgbToHsl();
			if (steps == 1){
				if (multiplication){
					hsl.hsl.saturation *= math::mix(saturation_from, saturation_to, 0);
					hsl.hsl.lightness *= math::mix(lightness_from, lightness_to, 0);
				}else{
					hsl.hsl.saturation += math::mix(saturation_from, saturation_to, 0);
					hsl.hsl.lightness += math::mix(lightness_from, lightness_to, 0);
				}
			}else{
				if (multiplication){
					hsl.hsl.saturation *= math::mix(saturation_from, saturation_to, (step_i / (float) (steps - 1)));
					hsl.hsl.lightness *= math::mix(lightness_from, lightness_to, (step_i / (float) (steps - 1)));
				}else{
					hsl.hsl.saturation += math::mix(saturation_from, saturation_to, (step_i / (float) (steps - 1)));
					hsl.hsl.lightness += math::mix(lightness_from, lightness_to, (step_i / (float) (steps - 1)));
				}
			}
			hsl.hsl.saturation = math::clamp(hsl.hsl.saturation, 0.0f, 1.0f);
			hsl.hsl.lightness = math::clamp(hsl.hsl.lightness, 0.0f, 1.0f);
			r = hsl.hslToRgb();
			if (linearization)
				r.nonLinearRgbInplace();
			ColorObject *color_object = color_list_new_color_object(color_list, &r);
			name_assigner.assign(*color_object, name, step_i);
			color_list_add_color_object(color_list, color_object, 1);
			color_object->release();
		}
	}
}
static void update(GtkWidget *widget, DialogVariationsArgs *args)
{
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}
void dialog_variations_show(GtkWindow* parent, ColorList *selected_color_list, GlobalState* gs)
{
	DialogVariationsArgs *args = new DialogVariationsArgs;
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick.variations");
	GtkWidget *table, *toggle_multiplication;
	GtkWidget *range_lightness_from, *range_lightness_to, *range_steps;
	GtkWidget *range_saturation_from, *range_saturation_to;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Variations"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1),
		args->options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gint table_y;
	table = gtk_table_new(5, 3, FALSE);
	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Lightness:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_lightness_from = gtk_spin_button_new_with_range(-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_from), args->options->getFloat("lightness_from", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args->range_lightness_from = range_lightness_from;
	g_signal_connect(G_OBJECT(range_lightness_from), "value-changed", G_CALLBACK(update), args);

	range_lightness_to = gtk_spin_button_new_with_range(-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_lightness_to), args->options->getFloat("lightness_to", 1));
	gtk_table_attach(GTK_TABLE(table), range_lightness_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->range_lightness_to = range_lightness_to;
	g_signal_connect(G_OBJECT(range_lightness_to), "value-changed", G_CALLBACK(update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Saturation:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_saturation_from = gtk_spin_button_new_with_range(-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_from), args->options->getFloat("saturation_from", 0));
	gtk_table_attach(GTK_TABLE(table), range_saturation_from,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args->range_saturation_from = range_saturation_from;
	g_signal_connect(G_OBJECT(range_saturation_from), "value-changed", G_CALLBACK(update), args);

	range_saturation_to = gtk_spin_button_new_with_range(-100,100,0.001);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_saturation_to), args->options->getFloat("saturation_to", 1));
	gtk_table_attach(GTK_TABLE(table), range_saturation_to,2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->range_saturation_to = range_saturation_to;
	g_signal_connect(G_OBJECT(range_saturation_to), "value-changed", G_CALLBACK(update), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Steps:"),0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	range_steps = gtk_spin_button_new_with_range(1, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(range_steps), args->options->getInt32("steps", 3));
	gtk_table_attach(GTK_TABLE(table), range_steps,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->range_steps = range_steps;
	g_signal_connect(G_OBJECT(range_steps), "value-changed", G_CALLBACK(update), args);

	toggle_multiplication = gtk_check_button_new_with_mnemonic(_("_Use multiplication"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_multiplication), args->options->getBool("multiplication", true));
	gtk_table_attach(GTK_TABLE(table), toggle_multiplication,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	args->toggle_multiplication = toggle_multiplication;
	g_signal_connect(G_OBJECT(toggle_multiplication), "toggled", G_CALLBACK(update), args);

	args->toggle_linearization = gtk_check_button_new_with_mnemonic(_("_Linearization"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_linearization), args->options->getBool("linearization", false));
	gtk_table_attach(GTK_TABLE(table), args->toggle_linearization,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->toggle_linearization), "toggled", G_CALLBACK(update), args);
	table_y++;

	GtkWidget* preview_expander;
	ColorList* preview_color_list = nullptr;
	gtk_table_attach(GTK_TABLE(table), preview_expander = palette_list_preview_new(gs, true, args->options->getBool("show_preview", true), gs->getColorList(), &preview_color_list), 0, 3, table_y, table_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
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
