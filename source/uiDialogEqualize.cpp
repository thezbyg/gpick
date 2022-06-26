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
#include "ColorSpaces.h"
#include "Channels.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "I18N.h"
#include "common/Match.h"
#include "math/Algorithms.h"
#include <vector>
#include <algorithm>
#include <functional>
namespace {
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
	common::Ref<ColorList> previewColorList;
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
		colorSpace = &common::matchById(colorSpaces(), options->getString("color_space", "lab"));
		channel = &common::matchById(channels(), options->getString("channel", "lab_lightness"));
		type = &common::matchById(types, options->getString("type", "average"));
		selectChannel();
		dialog = gtk_dialog_new_with_buttons(_("Equalize colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
		gtk_window_set_default_size(GTK_WINDOW(dialog), options->getInt32("window.width", -1), options->getInt32("window.height", -1));
		gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
		Grid grid(2, 5);
		grid.addLabel(_("Color space:"));
		grid.add(colorSpaceComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: colorSpaces()) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(colorSpaceComboBox), _(i.name));
			if (&i == colorSpace)
				gtk_combo_box_set_active(GTK_COMBO_BOX(colorSpaceComboBox), &i - colorSpaces().data());
		}
		g_signal_connect(G_OBJECT(colorSpaceComboBox), "changed", G_CALLBACK(DialogEqualizeArgs::onColorSpaceChange), this);
		grid.addLabel(_("Channel:"));
		grid.add(channelComboBox = gtk_combo_box_text_new(), true);
		buildChannelComboBox();
		g_signal_connect(G_OBJECT(channelComboBox), "changed", G_CALLBACK(DialogEqualizeArgs::onChannelChange), this);
		grid.addLabel(_("Type:"));
		grid.add(typeComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: types) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(typeComboBox), _(i.name));
			if (&i == type)
				gtk_combo_box_set_active(GTK_COMBO_BOX(typeComboBox), &i - types);
		}
		g_signal_connect(G_OBJECT(typeComboBox), "changed", G_CALLBACK(DialogEqualizeArgs::onTypeChange), this);
		grid.addLabel(_("Strength:"));
		grid.add(strengthSpinButton = gtk_spin_button_new_with_range(0, 100, 1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(strengthSpinButton), options->getFloat("strength", 1) * 100);
		g_signal_connect(G_OBJECT(strengthSpinButton), "value-changed", G_CALLBACK(DialogEqualizeArgs::onStrengthUpdate), this);
		grid.add(previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), true, 2, true);
		update(true);
		setDialogContent(dialog, grid);
	}
	~DialogEqualizeArgs() {
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
			options->set("color_space", colorSpace->id);
			options->set("channel", channel->id);
			options->set("type", type->id);
			options->set("strength", static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(strengthSpinButton))) / 100);
		}
	}
	void update(bool preview) {
		if (preview)
			previewColorList->removeAll();
		colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(colorSpaceComboBox))];
		channel = channelsInComboBox[gtk_combo_box_get_active(GTK_COMBO_BOX(channelComboBox))];
		type = &types[gtk_combo_box_get_active(GTK_COMBO_BOX(typeComboBox))];
		float strength = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(strengthSpinButton)) / 100);
		ColorList &colorList = preview ? *previewColorList : gs.colorList();
		common::Guard colorListGuard = colorList.changeGuard();
		size_t count = 0;
		bool first = true;
		float commonValue = 0;
		for (auto *colorObject: selectedColorList) {
			++count;
			Color color = colorObject->getColor();
			float value = std::invoke(colorSpace->convertTo, color).data[channel->index];
			if (first) {
				commonValue = value;
				first = false;
			} else {
				switch (type->type) {
				case Type::average:
					commonValue += value;
					break;
				case Type::minimum:
					commonValue = std::min(commonValue, value);
					break;
				case Type::maximum:
					commonValue = std::max(commonValue, value);
					break;
				}
			}
		}
		if (count == 0)
			return;
		switch (type->type) {
		case Type::average:
			commonValue /= count;
			break;
		case Type::minimum:
			break;
		case Type::maximum:
			break;
		}
		for (auto *colorObject: selectedColorList) {
			Color color = colorObject->getColor();
			float alpha = color.alpha;
			color = std::invoke(colorSpace->convertTo, color);
			float value = color.data[channel->index];
			value = math::mix(value, commonValue, strength);
			color.data[channel->index] = value;
			color = std::invoke(colorSpace->convertFrom, color);
			if (colorSpace->externalAlpha()) {
				color.alpha = alpha;
			}
			if (preview) {
				colorList.add(ColorObject(colorObject->getName(), color));
			} else {
				colorObject->setColor(color);
			}
		}
	}
	void selectChannel() {
		if (!channel->allColorSpaces() && colorSpace->type != channel->colorSpace) {
			for (const auto &i: channels()) {
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
		for (const auto &i: channels()) {
			if (!i.allColorSpaces() && i.colorSpace != colorSpace->type)
				continue;
			channelsInComboBox.push_back(&i);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(channelComboBox), _(i.name));
			if (&i == channel)
				gtk_combo_box_set_active(GTK_COMBO_BOX(channelComboBox), channelsInComboBox.size() - 1);
		}
		blockChannelChange = false;
	}
	static void onColorSpaceChange(GtkWidget *, DialogEqualizeArgs *args) {
		args->colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(args->colorSpaceComboBox))];
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
