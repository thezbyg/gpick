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
#include "uiDialogBase.h"
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
#include "common/Format.h"
#include "math/Algorithms.h"
#include <array>
#include <algorithm>
namespace {
struct EditDialog: public DialogBase {
	ColorList &selectedColorList;
	const ColorSpaceDescription *colorSpace;
	struct Channel {
		GtkWidget *label, *alignment, *adjustmentRange;
		const ChannelDescription *channel;
	};
	std::array<Channel, 5> channels;
	GtkWidget  *colorSpaceComboBox, *relativeCheck, *previewExpander;
	EditDialog(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.edit", _("Edit colors"), parent),
		selectedColorList(selectedColorList) {
		colorSpace = &common::matchById(colorSpaces(), options->getString("color_space", "rgb"));
		Grid grid(2, 3 + channels.size());
		grid.addLabel(_("Color space:"));
		grid.add(colorSpaceComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: colorSpaces()) {
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(colorSpaceComboBox), _(i.name));
			if (&i == colorSpace)
				gtk_combo_box_set_active(GTK_COMBO_BOX(colorSpaceComboBox), &i - colorSpaces().data());
		}
		g_signal_connect(G_OBJECT(colorSpaceComboBox), "changed", G_CALLBACK(onColorSpaceChange), this);
		grid.nextColumn().add(relativeCheck = gtk_check_button_new_with_mnemonic(_("_Relative adjustment")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(relativeCheck), options->getBool("relative", true));
		g_signal_connect(G_OBJECT(relativeCheck), "toggled", G_CALLBACK(onRelativeChange), this);
		size_t index = 0;
		for (auto &channel: channels) {
			channel.label = gtk_label_new("");
			channel.alignment = gtk_alignment_new(0, 0.5, 0, 0);
			gtk_container_add(GTK_CONTAINER(channel.alignment), channel.label);
			grid.add(channel.alignment);
			grid.add(channel.adjustmentRange = gtk_hscale_new_with_range(-100, 100, 1), true);
			gtk_range_set_value(GTK_RANGE(channel.adjustmentRange), options->getFloat(common::format("adjustment{}", index), 0) * 100);
			g_signal_connect(G_OBJECT(channel.adjustmentRange), "value-changed", G_CALLBACK(onAdjustmentChange), this);
			g_signal_connect(G_OBJECT(channel.adjustmentRange), "format-value", G_CALLBACK(onFormat), this);
			++index;
		}
		grid.add(previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), true, 2, true);
		setContent(grid);
		setupChannels();
		apply(true);
	}
	virtual ~EditDialog() {
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
		options->set("color_space", colorSpace->id);
		options->set("relative", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(relativeCheck)));
		size_t index = 0;
		for (auto &channel: channels) {
			options->set(common::format("adjustment{}", index), static_cast<float>(gtk_range_get_value(GTK_RANGE(channel.adjustmentRange)) / 100));
			++index;
		}
	}
	virtual void apply(bool preview) override {
		if (preview)
			previewColorList->removeAll();
		colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(colorSpaceComboBox))];
		bool relative = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(relativeCheck));
		size_t index = 0;
		float adjustments[channels.size()];
		for (auto &channel: channels) {
			adjustments[index] = static_cast<float>(gtk_range_get_value(GTK_RANGE(channel.adjustmentRange)) / 100);
			++index;
		}
		ColorList &colorList = preview ? *previewColorList : gs.colorList();
		common::Guard colorListGuard = colorList.changeGuard();
		for (auto *colorObject: selectedColorList) {
			Color color = colorObject->getColor();
			float alpha = color.alpha;
			color = std::invoke(colorSpace->convertTo, color);
			for (size_t j = 0, end = std::min<size_t>(channels.size(), Color::MemberCount); j != end; ++j) {
				const auto &channel = *channels[j].channel;
				if (relative) {
					if (channel.wrap())
						color.data[j] = math::wrap(color.data[j] + color.data[j] * adjustments[j], channel.min, channel.max);
					else
						color.data[j] = math::clamp(color.data[j] + color.data[j] * adjustments[j], channel.min, channel.max);
				} else  {
					if (channel.wrap())
						color.data[j] = math::wrap(color.data[j] + (channel.max - channel.min) * adjustments[j], channel.min, channel.max);
					else
						color.data[j] = math::clamp(color.data[j] + (channel.max - channel.min) * adjustments[j], channel.min, channel.max);
				}
			}
			color = std::invoke(colorSpace->convertFrom, color);
			if (colorSpace->externalAlpha()) {
				color.alpha = math::clamp(color.alpha + alpha * adjustments[4], 0, 1);
				color.alpha = alpha;
			}
			color.normalizeRgbInplace();
			if (preview) {
				colorList.add(ColorObject(colorObject->getName(), color));
			} else {
				colorObject->setColor(color);
			}
		}
	}
	void setupChannels() {
		size_t index = 0;
		for (const auto &channel: ::channels()) {
			if (channel.allColorSpaces() || channel.colorSpace == colorSpace->type) {
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
	static void onColorSpaceChange(GtkWidget *, EditDialog *dialog) {
		dialog->colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(dialog->colorSpaceComboBox))];
		dialog->setupChannels();
		dialog->apply(true);
	}
	static void onAdjustmentChange(GtkWidget *, EditDialog *dialog) {
		dialog->apply(true);
	}
	static void onRelativeChange(GtkWidget *, EditDialog *dialog) {
		dialog->apply(true);
	}
	static gchar *onFormat(GtkScale *, gdouble value) {
		return g_strdup_printf("%d%%", static_cast<int>(value));
	}
};
}
void dialog_edit_show(GtkWindow *parent, GtkWidget *paletteWidget, GlobalState &gs) {
	ColorList colorList;
	palette_list_get_selected(paletteWidget, colorList);
	EditDialog(colorList, gs, parent).run();
	palette_list_update_selected(paletteWidget, false, true);
}
