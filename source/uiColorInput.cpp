/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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

#include "uiColorInput.h"
#include "Converters.h"
#include "Converter.h"
#include "dynv/Map.h"
#include "uiUtilities.h"
#include "GlobalState.h"
#include "gtk/ColorWheel.h"
#include "I18N.h"
#include "gtk/ColorComponent.h"
#include "gtk/ColorWidget.h"
#include "ColorObject.h"
#include "ColorSpaces.h"
#include "color_names/ColorNames.h"
#include "common/Unused.h"
#include <array>
#include <string>
using namespace std::string_literals;
namespace {
struct DialogInputArgs {
	common::Ref<ColorObject> colorObject;
	GlobalState &gs;
	dynv::Ref options;
	GtkWidget *colorWidget, *textInput, *nameInput, *automaticName;
	GtkWidget *expanders[maxColorSpaces];
	GtkWidget *controls[maxColorSpaces];
	bool ignoreTextChange;
	DialogInputArgs(GlobalState &gs):
		gs(gs),
		ignoreTextChange(false) {
		options = gs.settings().getOrCreateMap("gpick.color_input");
	}
	void addComponentEditor(ColorSpace type, size_t index, GtkWidget *vbox) {
		const auto &description = colorSpace(type);
		auto &expander = expanders[index], &control = controls[index];
		expander = gtk_expander_new(description.name);
		gtk_expander_set_expanded(GTK_EXPANDER(expander), options->getBool("expander."s + description.id, false));
		gtk_box_pack_start(GTK_BOX(vbox), expander, false, false, 0);
		control = gtk_color_component_new(type);
		std::array<const char *, ColorSpaceDescription::maxChannels * 2 + 1> labels;
		for (int i = 0; i < description.channelCount; ++i) {
			labels[i * 2 + 0] = description.channels[i].shortName;
			labels[i * 2 + 1] = _(description.channels[i].name);
		}
		labels[description.channelCount * 2] = nullptr;
		gtk_color_component_set_labels(GTK_COLOR_COMPONENT(control), labels.data());
		g_signal_connect(G_OBJECT(control), "color-changed", G_CALLBACK(onComponentChangeValue), this);
		g_signal_connect(G_OBJECT(control), "input-clicked", G_CALLBACK(onComponentInputClicked), this);
		gtk_container_add(GTK_CONTAINER(expander), control);
	}
	static void onComponentChangeValue(GtkWidget *widget, Color *color, DialogInputArgs *args) {
		args->colorObject->setColor(*color);
		args->update(widget);
	}
	static void onComponentInputClicked(GtkWidget *widget, int channel, DialogInputArgs *args) {
		dialog_color_component_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_COLOR_COMPONENT(widget), channel, args->options->getOrCreateMap("component_edit"));
	}
	void update(GtkWidget *exceptWidget) {
		auto color = colorObject->getColor();
		gtk_color_set_color(GTK_COLOR(colorWidget), &color, "");
		if (exceptWidget != textInput) {
			std::string text = gs.converters().serialize(*colorObject, Converters::Type::display);
			ignoreTextChange = true;
			gtk_entry_set_text(GTK_ENTRY(textInput), text.c_str());
			ignoreTextChange = false;
		}
		int i = 0;
		for (const auto &colorSpace: colorSpaces()) {
			common::maybeUnused(colorSpace);
			if (exceptWidget != controls[i])
				gtk_color_component_set_color(GTK_COLOR_COMPONENT(controls[i]), color);
			updateComponentText(GTK_COLOR_COMPONENT(controls[i]));
			++i;
		}
	}
	void updateComponentText(GtkColorComponent *component) {
		Color transformedColor;
		gtk_color_component_get_transformed_color(component, transformedColor);
		float alpha = gtk_color_component_get_alpha(component);
		std::vector<std::string> values = toTexts(gtk_color_component_get_color_space(component), transformedColor, alpha);
		const char *texts[5] = { 0 };
		int j = 0;
		for (auto &value: values) {
			texts[j++] = value.c_str();
			if (j > 4)
				break;
		}
		gtk_color_component_set_texts(component, texts);
	}
};
static void onTextChanged(GtkWidget *entry, DialogInputArgs *args) {
	if (args->ignoreTextChange)
		return;
	ColorObject colorObject;
	if (args->gs.converters().deserialize(gtk_entry_get_text(GTK_ENTRY(entry)), colorObject)) {
		args->colorObject->setColor(colorObject.getColor());
		args->update(entry);
	}
}
static void onAutomaticNameToggled(GtkWidget *entry, DialogInputArgs *args) {
	gtk_widget_set_sensitive(args->nameInput, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->automaticName)));
}
}
int dialog_color_input_show(GtkWindow *parent, GlobalState &gs, common::OptionalReference<ColorObject> colorObject, bool editableName, common::Ref<ColorObject> &newColorObject) {
	auto args = new DialogInputArgs(gs);
	bool newItem = false;
	if (colorObject) {
		args->colorObject = colorObject->get().copy();
	} else {
		newItem = true;
		args->colorObject = common::Ref(new ColorObject());
	}
	GtkWidget *dialog = gtk_dialog_new_with_buttons(newItem ? _("Add color") : _("Edit color"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1), args->options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	GtkWidget *widget = args->colorWidget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_color_set_transformation_chain(GTK_COLOR(widget), &gs.transformationChain());
	gtk_widget_set_size_request(widget, 30, 30);
	if (!newItem) {
		gtk_color_enable_split(GTK_COLOR(widget), true);
		gtk_color_set_split_color(GTK_COLOR(widget), &args->colorObject->getColor());
	}
	gtk_box_pack_start(GTK_BOX(vbox), widget, true, true, 0);

	Grid grid(2, 3);
	gtk_box_pack_start(GTK_BOX(vbox), grid, false, false, 0);
	grid.addLabel(_("Color:"));
	GtkWidget *entry = args->textInput = grid.add(gtk_entry_new(), true);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), true);
	gtk_widget_grab_focus(entry);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(onTextChanged), args);
	if (editableName) {
		grid.addLabel(_("Name:"));
		args->nameInput = grid.add(gtk_entry_new(), true);
		gtk_widget_set_sensitive(args->nameInput, !args->options->getBool("automatic_name", false));
		grid.nextColumn();
		args->automaticName = grid.add(gtk_check_button_new_with_mnemonic(_("_Automatic name")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->automaticName), args->options->getBool("automatic_name", false));
		g_signal_connect(G_OBJECT(args->automaticName), "toggled", G_CALLBACK(onAutomaticNameToggled), args);
	}
	int i = 0;
	for (const auto &colorSpace: colorSpaces()) {
		args->addComponentEditor(colorSpace.type, i++, vbox);
	}
	if (newItem) {
		auto text = args->options->getString("text", "");
		gtk_entry_set_text(GTK_ENTRY(args->textInput), text.c_str());
	} else {
		args->update(nullptr);
		if (editableName)
			gtk_entry_set_text(GTK_ENTRY(args->nameInput), args->colorObject->getName().c_str());
	}

	gtk_widget_show_all(vbox);
	setDialogContent(dialog, vbox);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	int result = -1;
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		if (editableName) {
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->automaticName))) {
				std::string name = color_names_get(gs.getColorNames(), &args->colorObject->getColor(), gs.settings().getBool("gpick.color_names.imprecision_postfix", false));
				args->colorObject->setName(name);
			} else {
				args->colorObject->setName(gtk_entry_get_text(GTK_ENTRY(args->nameInput)));
			}
		}
		newColorObject = args->colorObject;
		result = 0;
	}
	if (newItem)
		args->options->set("text", gtk_entry_get_text(GTK_ENTRY(entry)));
	if (editableName)
		args->options->set<bool>("automatic_name", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->automaticName)));
	i = 0;
	for (const auto &colorSpace: colorSpaces()) {
		args->options->set<bool>("expander."s + colorSpace.id, gtk_expander_get_expanded(GTK_EXPANDER(args->expanders[i])));
		++i;
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("window.width", width);
	args->options->set("window.height", height);
	gtk_widget_destroy(dialog);
	delete args;
	return result;
}

struct ColorPickerComponentEditArgs {
	GtkWidget *value[5];
	ColorSpace colorSpace;
	int channel;
	dynv::Ref options;
};

void dialog_color_component_input_show(GtkWindow *parent, GtkColorComponent *colorComponent, int channel, dynv::Ref options) {
	ColorSpace colorSpace = gtk_color_component_get_color_space(GTK_COLOR_COMPONENT(colorComponent));
	ColorPickerComponentEditArgs *args = new ColorPickerComponentEditArgs;
	args->options = options;
	args->colorSpace = colorSpace;
	args->channel = channel;
	memset(args->value, 0, sizeof(args->value));
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1), args->options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	GtkWidget *table = gtk_table_new(2, 2, FALSE);
	Color rawColor;
	gtk_color_component_get_raw_color(colorComponent, rawColor);
	float alpha = gtk_color_component_get_alpha(colorComponent);
	auto &colorSpaceType = ::colorSpace(colorSpace);
	for (int i = 0; i < colorSpaceType.channelCount; i++) {
		gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_(colorSpaceType.channels[i].name), 0, 0, 0, 0), 0, 1, i, i + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
		args->value[i] = gtk_spin_button_new_with_range(colorSpaceType.channels[i].minValue, colorSpaceType.channels[i].maxValue, colorSpaceType.channels[i].step);
		gtk_entry_set_activates_default(GTK_ENTRY(args->value[i]), true);
		if (i < 4)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), rawColor[i] * colorSpaceType.channels[i].rawScale);
		else
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), alpha * colorSpaceType.channels[i].rawScale);
		gtk_table_attach(GTK_TABLE(table), args->value[i], 1, 2, i, i + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
		if (i == channel)
			gtk_widget_grab_focus(args->value[i]);
	}
	setDialogContent(dialog, table);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		for (int i = 0; i < colorSpaceType.channelCount; i++) {
			if (i < 4)
				rawColor[i] = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / colorSpaceType.channels[i].rawScale);
			else
				alpha = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / colorSpaceType.channels[i].rawScale);
		}
		gtk_color_component_set_raw_color(colorComponent, rawColor, alpha);
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	options->set("window.width", width);
	options->set("window.height", height);
	gtk_widget_destroy(dialog);
	delete args;
}
