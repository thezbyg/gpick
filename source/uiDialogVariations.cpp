/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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
#include "uiDialogBase.h"
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
struct VariationsDialog: public DialogBase {
	ColorList &selectedColorList;
	GtkWidget *multiplicationToggle, *linearizationToggle;
	GtkWidget *lightnessFromSpin, *lightnessToSpin, *stepsSpin;
	GtkWidget *saturationFromSpin, *saturationToSpin, *previewExpander;
	VariationsDialog(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.variations", _("Variations"), parent),
		selectedColorList(selectedColorList) {
		Grid grid(3, 5);
		grid.addLabel(_("Lightness:"));
		grid.add(lightnessFromSpin = gtk_spin_button_new_with_range(-100, 100, 0.001), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(lightnessFromSpin), options->getFloat("lightness_from", 1));
		g_signal_connect(G_OBJECT(lightnessFromSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.add(lightnessToSpin = gtk_spin_button_new_with_range(-100, 100, 0.001), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(lightnessToSpin), options->getFloat("lightness_to", 1));
		g_signal_connect(G_OBJECT(lightnessToSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Saturation:"));
		grid.add(saturationFromSpin = gtk_spin_button_new_with_range(-100, 100, 0.001), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(saturationFromSpin), options->getFloat("saturation_from", 0));
		g_signal_connect(G_OBJECT(saturationFromSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.add(saturationToSpin = gtk_spin_button_new_with_range(-100, 100, 0.001), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(saturationToSpin), options->getFloat("saturation_to", 1));
		g_signal_connect(G_OBJECT(saturationToSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Steps:"));
		grid.add(stepsSpin = gtk_spin_button_new_with_range(1, 255, 1), true, 2);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(stepsSpin), options->getInt32("steps", 3));
		g_signal_connect(G_OBJECT(stepsSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.nextColumn().add(multiplicationToggle = gtk_check_button_new_with_mnemonic(_("_Use multiplication")), true, 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(multiplicationToggle), options->getBool("multiplication", true));
		g_signal_connect(G_OBJECT(multiplicationToggle), "toggled", G_CALLBACK(onUpdate), this);
		grid.nextColumn().add(linearizationToggle = gtk_check_button_new_with_mnemonic(_("_Linearization")), true, 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linearizationToggle), options->getBool("linearization", false));
		g_signal_connect(G_OBJECT(linearizationToggle), "toggled", G_CALLBACK(onUpdate), this);
		grid.add(previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), true, 3, true);
		apply(true);
		setContent(grid);
	};
	virtual ~VariationsDialog() {
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
	}
	virtual void apply(bool preview) override {
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
};
}
void dialog_variations_show(GtkWindow *parent, GtkWidget *paletteWidget, GlobalState &gs) {
	ColorList colorList;
	palette_list_get_selected(paletteWidget, colorList);
	VariationsDialog(colorList, gs, parent).run();
}
