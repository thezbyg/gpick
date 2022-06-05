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

#include "uiDialogEdit.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "ColorSpaces.h"
#include "Channels.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "I18N.h"
#include "common/Guard.h"
#include "common/Match.h"
#include "common/Format.h"
#include "math/Algorithms.h"
#include <array>
#include <algorithm>
namespace {
struct DialogEditArgs {
	ColorList &selectedColorList;
	GlobalState &gs;
	dynv::Ref options;
	ColorList *previewColorList;
	const ColorSpaceDescription *colorSpace;
	struct Channel {
		GtkWidget *label, *alignment, *adjustmentRange;
		const ChannelDescription *channel;
	};
	std::array<Channel, 5> channels;
	GtkWidget *dialog, *colorSpaceComboBox, *relativeCheck, *previewExpander;
	DialogEditArgs(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		selectedColorList(selectedColorList),
		gs(gs) {
		options = gs.settings().getOrCreateMap("gpick.edit");
		colorSpace = &common::matchById(colorSpaces(), options->getString("color_space", "rgb"));
		dialog = gtk_dialog_new_with_buttons(_("Edit colors"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
		gtk_window_set_default_size(GTK_WINDOW(dialog), options->getInt32("window.width", -1), options->getInt32("window.height", -1));
		gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
		Grid grid(2, 3 + channels.size());
		grid.add(gtk_label_aligned_new(_("Color space:"), 0, 0.5, 0, 0));
		grid.add(colorSpaceComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: colorSpaces()) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(colorSpaceComboBox), _(i.name));
			if (&i == colorSpace)
				gtk_combo_box_set_active(GTK_COMBO_BOX(colorSpaceComboBox), &i - colorSpaces().data());
		}
		g_signal_connect(G_OBJECT(colorSpaceComboBox), "changed", G_CALLBACK(DialogEditArgs::onColorSpaceChange), this);
		grid.nextColumn();
		grid.add(relativeCheck = gtk_check_button_new_with_mnemonic(_("_Relative adjustment")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(relativeCheck), options->getBool("relative", true));
		g_signal_connect(G_OBJECT(relativeCheck), "toggled", G_CALLBACK(DialogEditArgs::onRelativeChange), this);
		size_t index = 0;
		for (auto &channel: channels) {
			channel.label = gtk_label_new("");
			channel.alignment = gtk_alignment_new(0, 0.5, 0, 0);
			gtk_container_add(GTK_CONTAINER(channel.alignment), channel.label);
			grid.add(channel.alignment);
			grid.add(channel.adjustmentRange = gtk_hscale_new_with_range(-100, 100, 1), true);
			gtk_range_set_value(GTK_RANGE(channel.adjustmentRange), options->getFloat(common::format("adjustment{}", index), 0) * 100);
			g_signal_connect(G_OBJECT(channel.adjustmentRange), "value-changed", G_CALLBACK(DialogEditArgs::onAdjustmentChange), this);
			g_signal_connect(G_OBJECT(channel.adjustmentRange), "format-value", G_CALLBACK(DialogEditArgs::onFormat), this);
			++index;
		}
		grid.add(previewExpander = palette_list_preview_new(&gs, true, options->getBool("show_preview", true), gs.getColorList(), &previewColorList), true, 2);
		setDialogContent(dialog, grid);
		setupChannels();
		update(true);
	}
	~DialogEditArgs() {
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
			options->set("relative", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(relativeCheck)));
			size_t index = 0;
			for (auto &channel: channels) {
				options->set(common::format("adjustment{}", index), static_cast<float>(gtk_range_get_value(GTK_RANGE(channel.adjustmentRange)) / 100));
				++index;
			}
		}
	}
	void update(bool preview) {
		if (preview)
			color_list_remove_all(previewColorList);
		colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(colorSpaceComboBox))];
		bool relative = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(relativeCheck));
		size_t index = 0;
		float adjustments[channels.size()];
		for (auto &channel: channels) {
			adjustments[index] = static_cast<float>(gtk_range_get_value(GTK_RANGE(channel.adjustmentRange)) / 100);
			++index;
		}
		ColorList &colorList = preview ? *previewColorList : *gs.getColorList();
		common::Guard colorListGuard(color_list_start_changes(&colorList), color_list_end_changes, &colorList);
		for (auto i = selectedColorList.colors.begin(); i != selectedColorList.colors.end(); ++i) {
			Color color = (*i)->getColor();
			float alpha = color.alpha;
			color = std::invoke(colorSpace->convertTo, color);
			for (size_t j = 0, end = std::min<size_t>(channels.size(), Color::MemberCount); j != end; ++j) {
				const auto &channel = *channels[j].channel;
				if (relative) {
					if (channel.wrap)
						color.data[j] = math::wrap(color.data[j] + color.data[j] * adjustments[j], channel.min, channel.max);
					else
						color.data[j] = math::clamp(color.data[j] + color.data[j] * adjustments[j], channel.min, channel.max);
				} else  {
					if (channel.wrap)
						color.data[j] = math::wrap(color.data[j] + (channel.max - channel.min) * adjustments[j], channel.min, channel.max);
					else
						color.data[j] = math::clamp(color.data[j] + (channel.max - channel.min) * adjustments[j], channel.min, channel.max);
				}
			}
			color = std::invoke(colorSpace->convertFrom, color);
			if ((colorSpace->flags & ColorSpaceFlags::externalAlpha) == ColorSpaceFlags::externalAlpha) {
				color.alpha = math::clamp(color.alpha + alpha * adjustments[4], 0, 1);
				color.alpha = alpha;
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
	void setupChannels() {
		size_t index = 0;
		for (const auto &channel: ::channels()) {
			if (channel.allColorSpaces || channel.colorSpace == colorSpace->type) {
				auto text = std::string(_(channel.name)) + ':';
				gtk_label_set_text(GTK_LABEL(channels[index].label), text.c_str());
				channels[index].channel = &channel;
				gtk_widget_show(channels[index].alignment);
				gtk_widget_show(channels[index].adjustmentRange);
				++index;
			}
		}
		for (; index < channels.size(); ++index) {
			gtk_widget_hide(channels[index].alignment);
			gtk_widget_hide(channels[index].adjustmentRange);
		}
	}
	static void onColorSpaceChange(GtkWidget *, DialogEditArgs *args) {
		args->colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(args->colorSpaceComboBox))];
		args->setupChannels();
		args->update(true);
	}
	static void onAdjustmentChange(GtkWidget *, DialogEditArgs *args) {
		args->update(true);
	}
	static void onRelativeChange(GtkWidget *, DialogEditArgs *args) {
		args->update(true);
	}
	static gchar *onFormat(GtkScale *, gdouble value) {
		return g_strdup_printf("%d%%", static_cast<int>(value));
	}
};
}
void dialog_edit_show(GtkWindow *parent, ColorList &selectedColorList, GlobalState &gs) {
	DialogEditArgs args(selectedColorList, gs, parent);
	args.run();
}
