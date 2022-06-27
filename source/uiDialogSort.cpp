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

#include "uiDialogSort.h"
#include "uiDialogBase.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "Channels.h"
#include "ColorSpaces.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "I18N.h"
#include "common/Match.h"
#include "common/Format.h"
#include "common/Unused.h"
#include "math/BinaryTreeQuantization.h"
#include <tuple>
#include <algorithm>
namespace {
static float toGrayscale(const Color &color) {
	Color r = color.linearRgb();
	return (r.red + r.green + r.blue) / 3;
}
static const ChannelDescription channelNone = { "none" };
static const ChannelDescription virtualChannels[] = {
	{ "rgb_grayscale", N_("RGB Grayscale"), ColorSpace::rgb, Channel::userDefined, ChannelFlags::useConvertTo, { .convertTo = toGrayscale }, 0, 1 },
};
struct SortDialog: public DialogBase {
	GtkWidget *groupComboBox, *groupSensitivitySpin, *maxGroupsSpin, *sortComboBox, *reverseCheck, *reverseGroupsCheck, *previewExpander;
	ColorList &selectedColors, &sortedColors;
	std::vector<const ChannelDescription *> sortChannelsInComboBox, groupChannelsInComboBox;
	const ChannelDescription *groupChannel, *sortChannel;
	SortDialog(ColorList &selectedColors, ColorList &sortedColors, GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.group_and_sort", _("Group and sort"), parent),
		selectedColors(selectedColors),
		sortedColors(sortedColors) {
		groupChannel = &common::matchById(channels(), options->getString("group_type", "rgb_red"), [](std::string_view id) -> const ChannelDescription & {
			if (id.empty() || id == "none")
				return channelNone;
			return common::matchById(virtualChannels, id, channels()[0]);
		});
		sortChannel = &common::matchById(channels(), options->getString("sort_type", "rgb_red"), [](std::string_view id) -> const ChannelDescription & {
			return common::matchById(virtualChannels, id, channels()[0]);
		});
		Grid grid(2, 8);
		grid.addLabel(_("Sort by:"));
		grid.add(sortComboBox = gtk_combo_box_text_new(), true);
		for (const auto &i: channels()) {
			sortChannelsInComboBox.emplace_back(&i);
			auto label = common::format("{} {}", colorSpace(i.colorSpace).name, _(i.name));
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sortComboBox), label.c_str());
			if (&i == sortChannel)
				gtk_combo_box_set_active(GTK_COMBO_BOX(sortComboBox), sortChannelsInComboBox.size() - 1);
		}
		for (const auto &i: virtualChannels) {
			sortChannelsInComboBox.emplace_back(&i);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sortComboBox), _(i.name));
			if (&i == sortChannel)
				gtk_combo_box_set_active(GTK_COMBO_BOX(sortComboBox), sortChannelsInComboBox.size() - 1);
		}
		g_signal_connect(G_OBJECT(sortComboBox), "changed", G_CALLBACK(onUpdate), this);

		grid.nextColumn();
		grid.add(reverseCheck = gtk_check_button_new_with_mnemonic(_("_Reverse sort order")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reverseCheck), options->getBool("reverse", false));
		g_signal_connect(G_OBJECT(reverseCheck), "toggled", G_CALLBACK(onUpdate), this);

		grid.addLabel(_("Group by:"));
		grid.add(groupComboBox = gtk_combo_box_text_new(), true);
		groupChannelsInComboBox.emplace_back(&channelNone);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(groupComboBox), _("None"));
		if (groupChannel == &channelNone)
			gtk_combo_box_set_active(GTK_COMBO_BOX(groupComboBox), groupChannelsInComboBox.size() - 1);
		for (const auto &i: channels()) {
			groupChannelsInComboBox.emplace_back(&i);
			auto label = common::format("{} {}", colorSpace(i.colorSpace).name, _(i.name));
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(groupComboBox), label.c_str());
			if (&i == groupChannel)
				gtk_combo_box_set_active(GTK_COMBO_BOX(groupComboBox), groupChannelsInComboBox.size() - 1);
		}
		for (const auto &i: virtualChannels) {
			groupChannelsInComboBox.emplace_back(&i);
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(groupComboBox), _(i.name));
			if (&i == groupChannel)
				gtk_combo_box_set_active(GTK_COMBO_BOX(groupComboBox), groupChannelsInComboBox.size() - 1);
		}
		g_signal_connect(G_OBJECT(groupComboBox), "changed", G_CALLBACK(onUpdate), this);

		grid.addLabel(_("Maximum number of groups:"));
		grid.add(maxGroupsSpin = gtk_spin_button_new_with_range(1, 255, 1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(maxGroupsSpin), options->getInt32("max_groups", 10));
		g_signal_connect(G_OBJECT(maxGroupsSpin), "value-changed", G_CALLBACK(onUpdate), this);

		grid.addLabel(_("Grouping sensitivity:"));
		grid.add(groupSensitivitySpin = gtk_spin_button_new_with_range(0, 100, 0.1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(groupSensitivitySpin), options->getFloat("group_sensitivity", 50));
		g_signal_connect(G_OBJECT(groupSensitivitySpin), "value-changed", G_CALLBACK(onUpdate), this);

		grid.nextColumn();
		grid.add(reverseGroupsCheck = gtk_check_button_new_with_mnemonic(_("_Reverse group order")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(reverseGroupsCheck), options->getBool("reverse_groups", false));
		g_signal_connect(G_OBJECT(reverseGroupsCheck), "toggled", G_CALLBACK(onUpdate), this);

		grid.add(previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), true, 2, true);
		apply(true);
		setContent(grid);
	}
	virtual ~SortDialog() {
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
	}
	void enableGroupInputs(bool enable) {
		gtk_widget_set_sensitive(maxGroupsSpin, enable);
		gtk_widget_set_sensitive(groupSensitivitySpin, enable);
		gtk_widget_set_sensitive(reverseGroupsCheck, enable);
	}
	virtual void apply(bool preview) override {
		groupChannel = groupChannelsInComboBox[gtk_combo_box_get_active(GTK_COMBO_BOX(groupComboBox))];
		sortChannel = sortChannelsInComboBox[gtk_combo_box_get_active(GTK_COMBO_BOX(sortComboBox))];
		float groupSensitivity = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(groupSensitivitySpin)));
		int maxGroups = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(maxGroupsSpin)));
		bool reverse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(reverseCheck));
		bool reverseGroups = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(reverseGroupsCheck));
		if (!preview) {
			options->set("group_type", groupChannel->id);
			options->set("group_sensitivity", groupSensitivity);
			options->set("max_groups", maxGroups);
			options->set("sort_type", sortChannel->id);
			options->set("reverse", reverse);
			options->set("reverse_groups", reverseGroups);
		} else {
			previewColorList->removeAll();
			enableGroupInputs(groupChannel != &channelNone);
		}
		ColorList &colorList = preview ? *previewColorList : sortedColors;
		using ColorWithProperties = std::tuple<float, float, ColorObject *>;
		std::vector<ColorWithProperties> colors;
		colors.reserve(selectedColors.size());
		if (maxGroups == 1 || groupChannel == &channelNone) {
			auto sortConvertTo = sortChannel->useConvertTo() ? nullptr : colorSpace(sortChannel->colorSpace).convertTo;
			for (auto *colorObject: selectedColors) {
				Color color = colorObject->getColor();
				float sortValue = sortConvertTo ? (std::invoke(sortConvertTo, color).data[sortChannel->index] - sortChannel->min) / (sortChannel->max - sortChannel->min) : sortChannel->convertTo(color);
				colors.emplace_back(0, sortValue, colorObject);
			}
			std::stable_sort(colors.begin(), colors.end(), [reverse](const ColorWithProperties &a, const ColorWithProperties &b) -> bool {
				float aSort, bSort;
				std::tie(std::ignore, aSort, std::ignore) = a;
				std::tie(std::ignore, bSort, std::ignore) = b;
				return (aSort < bSort) ^ reverse;
			});
		} else {
			math::BinaryTreeQuantization<float> tree;
			auto groupConvertTo = groupChannel->useConvertTo() ? nullptr : colorSpace(groupChannel->colorSpace).convertTo;
			for (auto *colorObject: selectedColors) {
				Color color = colorObject->getColor();
				float value = groupConvertTo ? (std::invoke(groupConvertTo, color).data[groupChannel->index] - groupChannel->min) / (groupChannel->max - groupChannel->min) : groupChannel->convertTo(color);
				tree.add(value);
			}
			tree.reduce(maxGroups);
			tree.reduceByMinDistance(groupSensitivity / 100.0f);
			auto sortConvertTo = sortChannel->useConvertTo() ? nullptr : colorSpace(sortChannel->colorSpace).convertTo;
			for (auto *colorObject: selectedColors) {
				Color color = colorObject->getColor();
				float groupValue = tree.find(groupConvertTo ? (std::invoke(groupConvertTo, color).data[groupChannel->index] - groupChannel->min) / (groupChannel->max - groupChannel->min) : groupChannel->convertTo(color));
				float sortValue = sortConvertTo ? (std::invoke(sortConvertTo, color).data[sortChannel->index] - sortChannel->min) / (sortChannel->max - sortChannel->min) : sortChannel->convertTo(color);
				colors.emplace_back(groupValue, sortValue, colorObject);
			}
			std::stable_sort(colors.begin(), colors.end(), [reverse, reverseGroups](const ColorWithProperties &a, const ColorWithProperties &b) -> bool {
				float aGroup, aSort, bGroup, bSort;
				std::tie(aGroup, aSort, std::ignore) = a;
				std::tie(bGroup, bSort, std::ignore) = b;
				if (aGroup != bGroup) {
					return (aGroup < bGroup) ^ reverseGroups;
				}
				return (aSort < bSort) ^ reverse;
			});
		}
		common::Guard colorListGuard = colorList.changeGuard();
		for (auto [group, sort, colorObject]: colors) {
			common::maybeUnused(group, sort);
			colorList.add(colorObject);
		}
	}
};
}
void dialog_sort_show(GtkWindow *parent, GtkWidget *paletteWidget, GlobalState &gs) {
	ColorList colorList, sortedColorList;
	palette_list_get_selected(paletteWidget, colorList);
	if (SortDialog(colorList, sortedColorList, gs, parent).run()) {
		auto i = sortedColorList.begin();
		palette_list_foreach(paletteWidget, true, [&i](ColorObject **colorObject) {
			*colorObject = *i;
			++i;
			return Update::none;
		});
		auto selected = palette_list_get_selected(paletteWidget);
		i = sortedColorList.begin();
		for (auto *&colorObject: gs.colorList()) {
			if (selected.count(colorObject) != 0) {
				colorObject = *i;
				++i;
			}
		}
	}
}
