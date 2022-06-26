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
namespace {
struct VariationsColorNameAssigner: public ToolColorNameAssigner {
	VariationsColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view name, uint32_t stepIndex) {
		m_name = name;
		m_stepIndex = stepIndex;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) {
		m_stream.str("");
		m_stream << m_name << " " << _("variation") << " " << m_stepIndex;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_name;
	uint32_t m_stepIndex;
};
struct DialogVariationsArgs {
	ColorList &selectedColorList;
	GlobalState &gs;
	dynv::Ref options;
	common::Ref<ColorList> previewColorList;
	GtkWidget *dialog, *multiplicationToggle, *linearizationToggle;
	GtkWidget *lightnessFromSpin, *lightnessToSpin, *stepsSpin;
	GtkWidget *saturationFromSpin, *saturationToSpin, *previewExpander;
	DialogVariationsArgs(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		selectedColorList(selectedColorList),
		gs(gs) {
		options = gs.settings().getOrCreateMap("gpick.variations");
		dialog = gtk_dialog_new_with_buttons(_("Variations"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
		gtk_window_set_default_size(GTK_WINDOW(dialog), options->getInt32("window.width", -1), options->getInt32("window.height", -1));
		gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
		gint table_y;
		GtkWidget *table = gtk_table_new(5, 3, FALSE);
		table_y = 0;
		gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Lightness:"), 0, 0, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
		lightnessFromSpin = gtk_spin_button_new_with_range(-100, 100, 0.001);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(lightnessFromSpin), options->getFloat("lightness_from", 1));
		gtk_table_attach(GTK_TABLE(table), lightnessFromSpin, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		g_signal_connect(G_OBJECT(lightnessFromSpin), "value-changed", G_CALLBACK(onUpdate), this);

		lightnessToSpin = gtk_spin_button_new_with_range(-100, 100, 0.001);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(lightnessToSpin), options->getFloat("lightness_to", 1));
		gtk_table_attach(GTK_TABLE(table), lightnessToSpin, 2, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		table_y++;
		g_signal_connect(G_OBJECT(lightnessToSpin), "value-changed", G_CALLBACK(onUpdate), this);

		gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Saturation:"), 0, 0, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
		saturationFromSpin = gtk_spin_button_new_with_range(-100, 100, 0.001);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(saturationFromSpin), options->getFloat("saturation_from", 0));
		gtk_table_attach(GTK_TABLE(table), saturationFromSpin, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		g_signal_connect(G_OBJECT(saturationFromSpin), "value-changed", G_CALLBACK(onUpdate), this);

		saturationToSpin = gtk_spin_button_new_with_range(-100, 100, 0.001);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(saturationToSpin), options->getFloat("saturation_to", 1));
		gtk_table_attach(GTK_TABLE(table), saturationToSpin, 2, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		table_y++;
		g_signal_connect(G_OBJECT(saturationToSpin), "value-changed", G_CALLBACK(onUpdate), this);

		gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Steps:"), 0, 0, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
		stepsSpin = gtk_spin_button_new_with_range(1, 255, 1);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(stepsSpin), options->getInt32("steps", 3));
		gtk_table_attach(GTK_TABLE(table), stepsSpin, 1, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		table_y++;
		g_signal_connect(G_OBJECT(stepsSpin), "value-changed", G_CALLBACK(onUpdate), this);

		multiplicationToggle = gtk_check_button_new_with_mnemonic(_("_Use multiplication"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multiplicationToggle), options->getBool("multiplication", true));
		gtk_table_attach(GTK_TABLE(table), multiplicationToggle, 1, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		table_y++;
		g_signal_connect(G_OBJECT(multiplicationToggle), "toggled", G_CALLBACK(onUpdate), this);

		linearizationToggle = gtk_check_button_new_with_mnemonic(_("_Linearization"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linearizationToggle), options->getBool("linearization", false));
		gtk_table_attach(GTK_TABLE(table), linearizationToggle, 1, 4, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		g_signal_connect(G_OBJECT(linearizationToggle), "toggled", G_CALLBACK(onUpdate), this);
		table_y++;

		gtk_table_attach(GTK_TABLE(table), previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), 0, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
		table_y++;
		update(true);
		setDialogContent(dialog, table);
	};
	~DialogVariationsArgs() {
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
		options->set("window.width", width);
		options->set("window.height", height);
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
		gtk_widget_destroy(dialog);
	}
	void run() {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			update(false);
		}
	}
	void update(bool preview) {
		if (preview)
			previewColorList->removeAll();
		int32_t steps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(stepsSpin));
		float lightnessFrom = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(lightnessFromSpin)));
		float lightnessTo = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(lightnessToSpin)));
		float saturationFrom = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(saturationFromSpin)));
		float saturationTo = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(saturationToSpin)));
		bool multiplication = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multiplicationToggle));
		bool linearization = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linearizationToggle));
		if (!preview) {
			options->set("steps", steps);
			options->set("lightness_from", lightnessFrom);
			options->set("lightness_to", lightnessTo);
			options->set("saturation_from", saturationFrom);
			options->set("saturation_to", saturationTo);
			options->set("multiplication", multiplication);
			options->set("linearization", linearization);
		}
		Color r, hsl;
		gint stepIndex;
		ColorList &colorList = preview ? *previewColorList : gs.colorList();
		VariationsColorNameAssigner nameAssigner(gs);
		common::Guard colorListGuard = colorList.changeGuard();
		int count = 0;
		for (auto *colorObject: selectedColorList) {
			Color in = colorObject->getColor();
			std::string_view name = colorObject->getName();
			if (linearization)
				in.linearRgbInplace();
			for (stepIndex = 0; stepIndex < steps; ++stepIndex) {
				if (preview) {
					if (count >= 100)
						return;
					++count;
				}
				hsl = in.rgbToHsl();
				if (steps == 1) {
					if (multiplication) {
						hsl.hsl.saturation *= math::mix(saturationFrom, saturationTo, 0);
						hsl.hsl.lightness *= math::mix(lightnessFrom, lightnessTo, 0);
					} else {
						hsl.hsl.saturation += math::mix(saturationFrom, saturationTo, 0);
						hsl.hsl.lightness += math::mix(lightnessFrom, lightnessTo, 0);
					}
				} else {
					if (multiplication) {
						hsl.hsl.saturation *= math::mix(saturationFrom, saturationTo, (stepIndex / (float)(steps - 1)));
						hsl.hsl.lightness *= math::mix(lightnessFrom, lightnessTo, (stepIndex / (float)(steps - 1)));
					} else {
						hsl.hsl.saturation += math::mix(saturationFrom, saturationTo, (stepIndex / (float)(steps - 1)));
						hsl.hsl.lightness += math::mix(lightnessFrom, lightnessTo, (stepIndex / (float)(steps - 1)));
					}
				}
				hsl.hsl.saturation = math::clamp(hsl.hsl.saturation, 0.0f, 1.0f);
				hsl.hsl.lightness = math::clamp(hsl.hsl.lightness, 0.0f, 1.0f);
				r = hsl.hslToRgb();
				if (linearization)
					r.nonLinearRgbInplace();
				ColorObject colorObject(r);
				nameAssigner.assign(colorObject, name, stepIndex);
				colorList.add(colorObject);
			}
		}
	}
	static void onUpdate(GtkWidget *, DialogVariationsArgs *args) {
		args->update(true);
	}
};
}
void dialog_variations_show(GtkWindow *parent, ColorList &selectedColorList, GlobalState &gs) {
	DialogVariationsArgs args(selectedColorList, gs, parent);
	args.run();
}
