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

#include "uiDialogEqualize.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "I18N.h"
#include "common/Guard.h"
#include "common/Match.h"
#include <vector>
#include <optional>
#include <algorithm>
namespace {
enum struct ColorSpace {
	rgb = 1,
	hsl,
	hsv,
	cmyk,
	lab,
	lch,
	all,
};
struct ColorSpaceDescription {
	const char *id;
	const char *name;
	ColorSpace type;
};
const ColorSpaceDescription colorSpaces[] = {
	{ "rgb", "RGB", ColorSpace::rgb },
	{ "hsl", "HSL", ColorSpace::hsl },
	{ "hsv", "HSV", ColorSpace::hsv },
	{ "cmyk", "CMYK", ColorSpace::cmyk },
	{ "lab", "LAB", ColorSpace::lab },
	{ "lch", "LCH", ColorSpace::lch },
};
enum struct Channel {
	rgbRed = 1,
	rgbGreen,
	rgbBlue,
	hslHue,
	hslSaturation,
	hslLightness,
	hsvHue,
	hsvSaturation,
	hsvValue,
	cmykCyan,
	cmykMagenta,
	cmykYellow,
	cmykKey,
	labLightness,
	labA,
	labB,
	lchLightness,
	lchChroma,
	lchHue,
	alpha,
};
struct ChannelDescription {
	const char *id;
	const char *name;
	ColorSpace colorSpace;
	Channel type;
};
const ChannelDescription channels[] = {
	{ "rgb_red", N_("Red"), ColorSpace::rgb, Channel::rgbRed },
	{ "rgb_green", N_("Green"), ColorSpace::rgb, Channel::rgbGreen },
	{ "rgb_blue", N_("Blue"), ColorSpace::rgb, Channel::rgbBlue },
	{ "hsl_hue", N_("Hue"), ColorSpace::hsl, Channel::hslHue },
	{ "hsl_saturation", N_("Saturation"), ColorSpace::hsl, Channel::hslSaturation },
	{ "hsl_lightness", N_("Lightness"), ColorSpace::hsl, Channel::hslLightness },
	{ "hsv_hue", N_("Hue"), ColorSpace::hsv, Channel::hsvHue },
	{ "hsv_saturation", N_("Saturation"), ColorSpace::hsv, Channel::hsvSaturation },
	{ "hsv_value", N_("Value"), ColorSpace::hsv, Channel::hsvValue },
	{ "cmyk_cyan", N_("Cyan"), ColorSpace::cmyk, Channel::cmykCyan },
	{ "cmyk_magenta", N_("Magenta"), ColorSpace::cmyk, Channel::cmykMagenta },
	{ "cmyk_yellow", N_("Yellow"), ColorSpace::cmyk, Channel::cmykYellow },
	{ "cmyk_key", N_("Key"), ColorSpace::cmyk, Channel::cmykKey },
	{ "lab_lightness", N_("Lightness"), ColorSpace::lab, Channel::labLightness },
	{ "lab_a", "a", ColorSpace::lab, Channel::labA },
	{ "lab_b", "b", ColorSpace::lab, Channel::labB },
	{ "lch_lightness", N_("Lightness"), ColorSpace::lch, Channel::lchLightness },
	{ "lch_chroma", N_("Chroma"), ColorSpace::lch, Channel::lchChroma },
	{ "lch_hue", N_("Hue"), ColorSpace::lch, Channel::lchHue },
	{ "alpha", N_("Alpha"), ColorSpace::all, Channel::alpha },
};
enum struct Type {
	average = 1,
	minimum,
	maximum,
};
struct TypeDescription {
	const char *id;
	const char *name;
	Type type;
};
const TypeDescription types[] = {
	{ "average", N_("Average"), Type::average },
	{ "minimum", N_("Minimum"), Type::minimum },
	{ "maximum", N_("Maximum"), Type::maximum },
};
struct DialogEqualizeArgs {
	ColorList &selectedColorList;
	GlobalState &gs;
	dynv::Ref options;
	ColorList *previewColorList;
	const ColorSpaceDescription *colorSpace;
	const ChannelDescription *channel;
	const TypeDescription *type;
	std::vector<const ChannelDescription *> channelsInComboBox;
	GtkWidget *dialog, *colorSpaceComboBox, *channelComboBox, *typeComboBox, *strengthSpinButton, *previewExpander;
	bool blockChannelChange;
	DialogEqualizeArgs(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		selectedColorList(selectedColorList),
		gs(gs),
		blockChannelChange(false) {
		options = gs.settings().getOrCreateMap("gpick.equalize");
		colorSpace = &common::matchById(colorSpaces, options->getString("color_space", "lab"));
		channel = &common::matchById(channels, options->getString("channel", "lab_lightness"));
		type = &common::matchById(types, options->getString("type", "average"));
		selectChannel();
		dialog = gtk_dialog_new_with_buttons(_("Equalize colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
		gtk_window_set_default_size(GTK_WINDOW(dialog), options->getInt32("window.width", -1), options->getInt32("window.height", -1));
		gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
		Grid grid(2, 5);
		grid.add(gtk_label_aligned_new(_("Color space:"), 0, 0.5, 0, 0));
		grid.add(colorSpaceComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: colorSpaces) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(colorSpaceComboBox), _(i.name));
			if (&i == colorSpace)
				gtk_combo_box_set_active(GTK_COMBO_BOX(colorSpaceComboBox), &i - colorSpaces);
		}
		g_signal_connect(G_OBJECT(colorSpaceComboBox), "changed", G_CALLBACK(DialogEqualizeArgs::onColorSpaceChange), this);
		grid.add(gtk_label_aligned_new(_("Channel:"), 0, 0.5, 0, 0));
		grid.add(channelComboBox = gtk_combo_box_text_new(), true);
		buildChannelComboBox();
		g_signal_connect(G_OBJECT(channelComboBox), "changed", G_CALLBACK(DialogEqualizeArgs::onChannelChange), this);
		grid.add(gtk_label_aligned_new(_("Type:"), 0, 0.5, 0, 0));
		grid.add(typeComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: types) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(typeComboBox), _(i.name));
			if (&i == type)
				gtk_combo_box_set_active(GTK_COMBO_BOX(typeComboBox), &i - types);
		}
		g_signal_connect(G_OBJECT(typeComboBox), "changed", G_CALLBACK(DialogEqualizeArgs::onTypeChange), this);
		grid.add(gtk_label_aligned_new(_("Strength:"), 0, 0.5, 0, 0));
		grid.add(strengthSpinButton = gtk_spin_button_new_with_range(0, 100, 1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(strengthSpinButton), options->getFloat("strength", 1) * 100);
		g_signal_connect(G_OBJECT(strengthSpinButton), "value-changed", G_CALLBACK(DialogEqualizeArgs::onStrengthUpdate), this);
		grid.add(previewExpander = palette_list_preview_new(&gs, true, options->getBool("show_preview", true), gs.getColorList(), &previewColorList), true, 2);
		update(true);
		gtk_widget_show_all(grid);
		setDialogContent(dialog, grid);
	}
	~DialogEqualizeArgs() {
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
		options->set("window.width", width);
		options->set("window.height", height);
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
		gtk_widget_destroy(dialog);
		color_list_destroy(previewColorList);
	}
	void run() {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			update(false);
			options->set("color_space", colorSpace->id);
			options->set("channel", channel->id);
			options->set("type", type->id);
			options->set("strength", static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(strengthSpinButton))) / 100);
		}
	}
	static void apply(float &now, float value, float strength) {
		now = now * (1 - strength) + value * strength;
	}
	void update(bool preview) {
		if (preview)
			color_list_remove_all(previewColorList);
		colorSpace = &colorSpaces[gtk_combo_box_get_active(GTK_COMBO_BOX(colorSpaceComboBox))];
		channel = channelsInComboBox[gtk_combo_box_get_active(GTK_COMBO_BOX(channelComboBox))];
		type = &types[gtk_combo_box_get_active(GTK_COMBO_BOX(typeComboBox))];
		float strength = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(strengthSpinButton)) / 100);
		ColorList &colorList = preview ? *previewColorList : *gs.getColorList();
		common::Guard colorListGuard(color_list_start_changes(&colorList), color_list_end_changes, &colorList);
		size_t count = 0;
		std::optional<float> commonValue;
		for (auto i = selectedColorList.colors.begin(); i != selectedColorList.colors.end(); ++i) {
			++count;
			Color color = (*i)->getColor();
			float value = 0;
			switch (channel->type) {
			case Channel::rgbRed:
				value = color.linearRgb().red;
				break;
			case Channel::rgbGreen:
				value = color.linearRgb().green;
				break;
			case Channel::rgbBlue:
				value = color.linearRgb().blue;
				break;
			case Channel::hslHue:
				value = color.rgbToHsl().hsl.hue;
				break;
			case Channel::hslSaturation:
				value = color.rgbToHsl().hsl.saturation;
				break;
			case Channel::hslLightness:
				value = color.rgbToHsl().hsl.lightness;
				break;
			case Channel::hsvHue:
				value = color.rgbToHsv().hsv.hue;
				break;
			case Channel::hsvSaturation:
				value = color.rgbToHsv().hsv.saturation;
				break;
			case Channel::hsvValue:
				value = color.rgbToHsv().hsv.value;
				break;
			case Channel::cmykCyan:
				value = color.rgbToCmyk().cmyk.c;
				break;
			case Channel::cmykMagenta:
				value = color.rgbToCmyk().cmyk.m;
				break;
			case Channel::cmykYellow:
				value = color.rgbToCmyk().cmyk.y;
				break;
			case Channel::cmykKey:
				value = color.rgbToCmyk().cmyk.k;
				break;
			case Channel::labLightness:
				value = color.rgbToLabD50().lab.L;
				break;
			case Channel::labA:
				value = color.rgbToLabD50().lab.a;
				break;
			case Channel::labB:
				value = color.rgbToLabD50().lab.b;
				break;
			case Channel::lchLightness:
				value = color.rgbToLchD50().lch.L;
				break;
			case Channel::lchChroma:
				value = color.rgbToLchD50().lch.C;
				break;
			case Channel::lchHue:
				value = color.rgbToLchD50().lch.h;
				break;
			case Channel::alpha:
				value = color.alpha;
				break;
			}
			switch (type->type) {
			case Type::average:
				if (!commonValue)
					commonValue = value;
				else
					commonValue = *commonValue + value;
				break;
			case Type::minimum:
				if (!commonValue)
					commonValue = value;
				else
					commonValue = std::min(*commonValue, value);
				break;
			case Type::maximum:
				if (!commonValue)
					commonValue = value;
				else
					commonValue = std::max(*commonValue, value);
				break;
			}
		}
		if (count == 0)
			return;
		switch (type->type) {
		case Type::average:
			if (commonValue)
				commonValue = *commonValue / count;
			break;
		case Type::minimum:
			break;
		case Type::maximum:
			break;
		}
		for (auto i = selectedColorList.colors.begin(); i != selectedColorList.colors.end(); ++i) {
			Color color = (*i)->getColor();
			switch (channel->type) {
			case Channel::rgbRed:
				color.linearRgbInplace();
				apply(color.red, *commonValue, strength);
				color.nonLinearRgbInplace();
				break;
			case Channel::rgbGreen:
				color.linearRgbInplace();
				apply(color.green, *commonValue, strength);
				color.nonLinearRgbInplace();
				break;
			case Channel::rgbBlue:
				color.linearRgbInplace();
				apply(color.blue, *commonValue, strength);
				color.nonLinearRgbInplace();
				break;
			case Channel::hslHue:
				color = color.rgbToHsl();
				apply(color.hsl.hue, *commonValue, strength);
				color = color.hslToRgb();
				break;
			case Channel::hslSaturation:
				color = color.rgbToHsl();
				apply(color.hsl.saturation, *commonValue, strength);
				color = color.hslToRgb();
				break;
			case Channel::hslLightness:
				color = color.rgbToHsl();
				apply(color.hsl.lightness, *commonValue, strength);
				color = color.hslToRgb();
				break;
			case Channel::hsvHue:
				color = color.rgbToHsv();
				apply(color.hsv.hue, *commonValue, strength);
				color = color.hsvToRgb();
				break;
			case Channel::hsvSaturation:
				color = color.rgbToHsv();
				apply(color.hsv.saturation, *commonValue, strength);
				color = color.hsvToRgb();
				break;
			case Channel::hsvValue:
				color = color.rgbToHsv();
				apply(color.hsv.value, *commonValue, strength);
				color = color.hsvToRgb();
				break;
			case Channel::cmykCyan:
				color = color.rgbToCmyk();
				apply(color.cmyk.c, *commonValue, strength);
				color = color.cmykToRgb();
				break;
			case Channel::cmykMagenta:
				color = color.rgbToCmyk();
				apply(color.cmyk.m, *commonValue, strength);
				color = color.cmykToRgb();
				break;
			case Channel::cmykYellow:
				color = color.rgbToCmyk();
				apply(color.cmyk.y, *commonValue, strength);
				color = color.cmykToRgb();
				break;
			case Channel::cmykKey:
				color = color.rgbToCmyk();
				apply(color.cmyk.k, *commonValue, strength);
				color = color.cmykToRgb();
				break;
			case Channel::labLightness:
				color = color.rgbToLabD50();
				apply(color.lab.L, *commonValue, strength);
				color = color.labToRgbD50();
				break;
			case Channel::labA:
				color = color.rgbToLabD50();
				apply(color.lab.a, *commonValue, strength);
				color = color.labToRgbD50();
				break;
			case Channel::labB:
				color = color.rgbToLabD50();
				apply(color.lab.b, *commonValue, strength);
				color = color.labToRgbD50();
				break;
			case Channel::lchLightness:
				color = color.rgbToLchD50();
				apply(color.lch.L, *commonValue, strength);
				color = color.lchToRgbD50();
				break;
			case Channel::lchChroma:
				color = color.rgbToLchD50();
				apply(color.lch.C, *commonValue, strength);
				color = color.lchToRgbD50();
				break;
			case Channel::lchHue:
				color = color.rgbToLchD50();
				apply(color.lch.h, *commonValue, strength);
				color = color.lchToRgbD50();
				break;
			case Channel::alpha:
				apply(color.alpha, *commonValue, strength);
				break;
			}
			if (preview) {
				ColorObject *colorObject = color_list_new_color_object(&colorList, &color);
				colorObject->setName((*i)->getName());
				color_list_add_color_object(&colorList, colorObject, 1);
				colorObject->release();
			} else {
				(*i)->setColor(color);
			}
		}
	}
	void selectChannel() {
		if (colorSpace->type != channel->colorSpace && channel->colorSpace != ColorSpace::all) {
			for (const auto &i: channels) {
				if (i.colorSpace == colorSpace->type) {
					channel = &i;
					break;
				}
			}
		}
	}
	void buildChannelComboBox() {
		blockChannelChange = true;
		channelsInComboBox.clear();
		gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(channelComboBox))));
		for (const auto &i: channels) {
			if (i.colorSpace != colorSpace->type && i.colorSpace != ColorSpace::all)
				continue;
			channelsInComboBox.push_back(&i);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(channelComboBox), _(i.name));
			if (&i == channel)
				gtk_combo_box_set_active(GTK_COMBO_BOX(channelComboBox), channelsInComboBox.size() - 1);
		}
		blockChannelChange = false;
	}
	static void onColorSpaceChange(GtkWidget *, DialogEqualizeArgs *args) {
		args->colorSpace = &colorSpaces[gtk_combo_box_get_active(GTK_COMBO_BOX(args->colorSpaceComboBox))];
		args->selectChannel();
		args->buildChannelComboBox();
		args->update(true);
	}
	static void onChannelChange(GtkWidget *, DialogEqualizeArgs *args) {
		if (args->blockChannelChange)
			return;
		args->update(true);
	}
	static void onTypeChange(GtkWidget *, DialogEqualizeArgs *args) {
		args->update(true);
	}
	static void onStrengthUpdate(GtkWidget *, DialogEqualizeArgs *args) {
		args->update(true);
	}
};
}
void dialog_equalize_show(GtkWindow *parent, ColorList &selectedColorList, GlobalState &gs) {
	DialogEqualizeArgs args(selectedColorList, gs, parent);
	args.run();
}
