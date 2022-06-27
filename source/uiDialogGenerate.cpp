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

#include "uiDialogGenerate.h"
#include "uiDialogBase.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "color_names/ColorNames.h"
#include "ColorRYB.h"
#include "Noise.h"
#include "GenerateScheme.h"
#include "I18N.h"
#include "Random.h"
#include <cmath>
#include <sstream>
namespace {
struct ColorWheelType {
	const char *name;
	void (*hueToHsl)(double hue, Color &hsl);
	void (*rgbHueToHue)(double rgbhue, double *hue);
};
static void rgb_hue2hue(double hue, Color &hsl) {
	hsl.hsl.hue = static_cast<float>(hue);
	hsl.hsl.saturation = 1;
	hsl.hsl.lightness = 0.5f;
}
static void rgb_rgbhue2hue(double rgbhue, double *hue) {
	*hue = rgbhue;
}
static void ryb1_hue2hue(double hue, Color &hsl) {
	Color color;
	color_rybhue_to_rgb(hue, &color);
	hsl = color.rgbToHsl();
}
static void ryb1_rgbhue2hue(double rgbhue, double *hue) {
	color_rgbhue_to_rybhue(rgbhue, hue);
}
static void ryb2_hue2hue(double hue, Color &hsl) {
	hsl.hsl.hue = static_cast<float>(color_rybhue_to_rgbhue_f(hue));
	hsl.hsl.saturation = 1;
	hsl.hsl.lightness = 0.5f;
}
static void ryb2_rgbhue2hue(double rgbhue, double *hue) {
	color_rgbhue_to_rybhue_f(rgbhue, hue);
}
static const ColorWheelType colorWheelTypes[] = {
	{ "RGB", rgb_hue2hue, rgb_rgbhue2hue },
	{ "RYB v1", ryb1_hue2hue, ryb1_rgbhue2hue },
	{ "RYB v2", ryb2_hue2hue, ryb2_rgbhue2hue },
};
struct GenerateColorNameAssigner: public ToolColorNameAssigner {
	GenerateColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, size_t index, std::string_view schemeName) {
		m_index = index;
		m_schemeName = schemeName;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << _("scheme") << " " << m_schemeName << " #" << m_index << "[" << color_names_get(m_gs.getColorNames(), &colorObject.getColor(), false) << "]";
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_schemeName;
	size_t m_index;
};
struct GenerateDialog: public DialogBase {
	ColorList &selectedColorList;
	GtkWidget *typeCombo, *wheelTypeCombo, *colorsSpin, *chaosSpin, *additionalRotationSpin, *chaosSeedSpin, *reverseToggle, *previewExpander;
	GenerateDialog(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.generate", _("Generate colors"), parent),
		selectedColorList(selectedColorList) {
		Grid grid(4, 6);
		grid.addLabel(_("Colors:"));
		grid.add(colorsSpin = gtk_spin_button_new_with_range(1, 1000, 1), true, 3);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(colorsSpin), options->getInt32("colors", 1));
		g_signal_connect(G_OBJECT(colorsSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Type:"));
		grid.add(typeCombo = gtk_combo_box_text_new(), true);
		for (uint32_t i = 0; i < generate_scheme_get_n_scheme_types(); i++) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(typeCombo), _(generate_scheme_get_scheme_type(i)->name));
		}
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(typeCombo), _("Static"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(typeCombo), options->getInt32("type", 0));
		g_signal_connect(G_OBJECT(typeCombo), "changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Color wheel:"));
		grid.add(wheelTypeCombo = gtk_combo_box_text_new(), true);
		for (size_t i = 0; i < sizeof(colorWheelTypes) / sizeof(ColorWheelType); i++) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wheelTypeCombo), colorWheelTypes[i].name);
		}
		gtk_combo_box_set_active(GTK_COMBO_BOX(wheelTypeCombo), options->getInt32("wheel_type", 0));
		g_signal_connect(G_OBJECT(wheelTypeCombo), "changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Chaos:"));
		grid.add(chaosSpin = gtk_spin_button_new_with_range(0, 1, 0.001), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(chaosSpin), options->getFloat("chaos", 0));
		g_signal_connect(G_OBJECT(chaosSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Seed:"));
		grid.add(chaosSeedSpin = gtk_spin_button_new_with_range(0, 0xFFFF, 1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(chaosSeedSpin), options->getInt32("chaos_seed", 0));
		g_signal_connect(G_OBJECT(chaosSeedSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Rotation:"));
		grid.add(additionalRotationSpin = gtk_spin_button_new_with_range(-360, 360, 0.1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(additionalRotationSpin), options->getFloat("additional_rotation", 0));
		g_signal_connect(G_OBJECT(additionalRotationSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.nextRow().nextColumn().add(reverseToggle = gtk_check_button_new_with_mnemonic(_("_Reverse")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reverseToggle), options->getBool("reverse", false));
		g_signal_connect(G_OBJECT(reverseToggle), "toggled", G_CALLBACK(onUpdate), this);
		grid.nextRow().add(previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), true, 4, true);
		apply(true);
		setContent(grid);
	}
	virtual ~GenerateDialog() {
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
	}
	virtual void apply(bool preview) override {
		if (preview)
			previewColorList->removeAll();
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(typeCombo));
		int wheelType = gtk_combo_box_get_active(GTK_COMBO_BOX(wheelTypeCombo));
		int32_t colorCount = static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(colorsSpin)));
		float chaos = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(chaosSpin)));
		int32_t chaosSeed = static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(chaosSeedSpin)));
		bool reverse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(reverseToggle));
		float additionalRotation = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(additionalRotationSpin)));
		if (!preview) {
			options->set("type", type);
			options->set("wheel_type", wheelType);
			options->set("colors", colorCount);
			options->set("chaos", chaos);
			options->set("chaos_seed", chaosSeed);
			options->set("reverse", reverse);
			options->set("additional_rotation", additionalRotation);
		}
		GenerateColorNameAssigner nameAssigner(gs);
		Color r, hsl, hslResults;
		double hue, hueStep;
		ColorList &colorList = preview ? *previewColorList : gs.colorList();
		const ColorWheelType *wheel = &colorWheelTypes[wheelType];
		struct Random *random = random_new("SHR3", chaosSeed);
		const SchemeType *schemeType;
		static SchemeType staticSchemeType = { _("Static"), 1, 1, { 0 } };
		if (static_cast<size_t>(type) >= generate_scheme_get_n_scheme_types()) {
			schemeType = &staticSchemeType;
		} else {
			schemeType = generate_scheme_get_scheme_type(type);
		}
		common::Guard colorListGuard = colorList.changeGuard();
		for (auto *colorObject: selectedColorList) {
			Color in = colorObject->getColor();
			hsl = in.rgbToHsl();
			wheel->rgbHueToHue(hsl.hsl.hue, &hue);
			wheel->hueToHsl(hue, hslResults);
			double saturation = hsl.hsl.saturation * 1 / hslResults.hsl.saturation;
			double lightness = hsl.hsl.lightness - hslResults.hsl.lightness;
			for (int32_t i = 0; i < colorCount; i++) {
				if (preview) {
					if (i >= 100) {
						random_destroy(random);
						return;
					}
				}
				wheel->hueToHsl(hue, hsl);
				hsl.hsl.lightness = math::clamp(static_cast<float>(hsl.hsl.lightness + lightness), 0.0f, 1.0f);
				hsl.hsl.saturation = math::clamp(static_cast<float>(hsl.hsl.saturation * saturation), 0.0f, 1.0f);
				r = hsl.hslToRgb();
				ColorObject colorObject(r);
				nameAssigner.assign(colorObject, i, _(schemeType->name));
				colorList.add(colorObject);
				hueStep = (schemeType->turn[i % schemeType->turnCount]) / (360.0f) + chaos * (random_get_double(random) - 0.5f) + additionalRotation / 360.0f;
				if (reverse) {
					hue = math::wrap(static_cast<float>(hue - hueStep));
				} else {
					hue = math::wrap(static_cast<float>(hue + hueStep));
				}
			}
		}
		random_destroy(random);
	}
};
}
void dialog_generate_show(GtkWindow *parent, GtkWidget *paletteWidget, GlobalState &gs) {
	ColorList colorList;
	palette_list_get_selected(paletteWidget, colorList);
	GenerateDialog(colorList, gs, parent).run();
}
