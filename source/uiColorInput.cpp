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
#include "ColorSpaceType.h"
#include "color_names/ColorNames.h"
#include "common/SetOnScopeEnd.h"
#include <string>
namespace {
struct DialogInputArgs {
	common::Ref<ColorObject> colorObject;
	GlobalState &gs;
	dynv::Ref options;
	GtkWidget *colorWidget, *textInput, *nameInput, *automaticName;
	GtkWidget *rgbExpander, *hsvExpander, *hslExpander, *cmykExpander, *labExpander, *lchExpander;
	GtkWidget *rgbControl, *hsvControl, *hslControl, *cmykControl, *labControl, *lchControl;
	bool ignoreTextChange;
	DialogInputArgs(GlobalState &gs):
		gs(gs),
		ignoreTextChange(false) {
		options = gs.settings().getOrCreateMap("gpick.color_input");
	}
};
static void updateComponentText(DialogInputArgs *args, GtkColorComponent *component, const char *type) {
	Color transformedColor;
	gtk_color_component_get_transformed_color(component, transformedColor);
	float alpha = gtk_color_component_get_alpha(component);
	lua::Script &script = args->gs.script();
	std::vector<std::string> values = color_space_color_to_text(type, transformedColor, alpha, script, &args->gs);
	const char *texts[5] = { 0 };
	int j = 0;
	for (auto &value: values) {
		texts[j++] = value.c_str();
		if (j > 4)
			break;
	}
	gtk_color_component_set_texts(component, texts);
}
static void update(DialogInputArgs *args, GtkWidget *exceptWidget) {
	auto color = args->colorObject->getColor();
	gtk_color_set_color(GTK_COLOR(args->colorWidget), &color, "");
	if (exceptWidget != args->textInput) {
		std::string text = args->gs.converters().serialize(*args->colorObject, Converters::Type::display);
		common::SetOnScopeEnd ignoreTextChange(args->ignoreTextChange = true, false);
		gtk_entry_set_text(GTK_ENTRY(args->textInput), text.c_str());
	}
	if (exceptWidget != args->hslControl) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hslControl), color);
	if (exceptWidget != args->hsvControl) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hsvControl), color);
	if (exceptWidget != args->rgbControl) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->rgbControl), color);
	if (exceptWidget != args->cmykControl) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->cmykControl), color);
	if (exceptWidget != args->labControl) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->labControl), color);
	if (exceptWidget != args->lchControl) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->lchControl), color);
	updateComponentText(args, GTK_COLOR_COMPONENT(args->hslControl), "hsl");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->hsvControl), "hsv");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->rgbControl), "rgb");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->cmykControl), "cmyk");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->labControl), "lab");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lchControl), "lch");
}
static void onComponentChangeValue(GtkWidget *widget, Color *color, DialogInputArgs *args) {
	args->colorObject->setColor(*color);
	update(args, widget);
}
static void onComponentInputClicked(GtkWidget *widget, int channel, DialogInputArgs *args) {
	dialog_color_component_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_COLOR_COMPONENT(widget), channel, args->options->getOrCreateMap("component_edit"));
}
static void setComponentHandlers(GtkWidget *widget, DialogInputArgs *args) {
	g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(onComponentChangeValue), args);
	g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(onComponentInputClicked), args);
}
static void addComponentEditor(GtkWidget *vbox, const char *label, const char *expandOption, ColorSpace type, const char **labels, DialogInputArgs *args, GtkWidget *&expander, GtkWidget *&control) {
	expander = gtk_expander_new(label);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), args->options->getBool(expandOption, false));
	gtk_box_pack_start(GTK_BOX(vbox), expander, false, false, 0);
	control = gtk_color_component_new(type);
	gtk_color_component_set_labels(GTK_COLOR_COMPONENT(control), labels);
	setComponentHandlers(control, args);
	gtk_container_add(GTK_CONTAINER(expander), control);
}
static void onTextChanged(GtkWidget *entry, DialogInputArgs *args) {
	if (args->ignoreTextChange)
		return;
	ColorObject colorObject;
	if (args->gs.converters().deserialize(gtk_entry_get_text(GTK_ENTRY(entry)), colorObject)) {
		args->colorObject->setColor(colorObject.getColor());
		update(args, entry);
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

	const char *hsvLabels[] = { "H", _("Hue"), "S", _("Saturation"), "V", _("Value"), "A", _("Alpha"), nullptr };
	addComponentEditor(vbox, "HSV", "expander.hsv", ColorSpace::hsv, hsvLabels, args, args->hsvExpander, args->hsvControl);
	const char *hslLabels[] = { "H", _("Hue"), "S", _("Saturation"), "L", _("Lightness"), "A", _("Alpha"), nullptr };
	addComponentEditor(vbox, "HSL", "expander.hsl", ColorSpace::hsl, hslLabels, args, args->hslExpander, args->hslControl);
	const char *rgbLabels[] = { "R", _("Red"), "G", _("Green"), "B", _("Blue"), "A", _("Alpha"), nullptr };
	addComponentEditor(vbox, "RGB", "expander.rgb", ColorSpace::rgb, rgbLabels, args, args->rgbExpander, args->rgbControl);
	const char *cmykLabels[] = { "C", _("Cyan"), "M", _("Magenta"), "Y", _("Yellow"), "K", _("Key"), "A", _("Alpha"), nullptr };
	addComponentEditor(vbox, "CMYK", "expander.cmyk", ColorSpace::cmyk, cmykLabels, args, args->cmykExpander, args->cmykControl);
	const char *labLabels[] = { "L", _("Lightness"), "a", "a", "b", "b", "A", _("Alpha"), nullptr };
	addComponentEditor(vbox, "Lab", "expander.lab", ColorSpace::lab, labLabels, args, args->labExpander, args->labControl);
	const char *lchLabels[] = { "L", _("Lightness"), "C", "Chroma", "H", "Hue", "A", _("Alpha"), nullptr };
	addComponentEditor(vbox, "LCH", "expander.lch", ColorSpace::lch, lchLabels, args, args->lchExpander, args->lchControl);

	if (newItem) {
		auto text = args->options->getString("text", "");
		gtk_entry_set_text(GTK_ENTRY(args->textInput), text.c_str());
	} else {
		update(args, nullptr);
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
	args->options->set<bool>("expander.rgb", gtk_expander_get_expanded(GTK_EXPANDER(args->rgbExpander)));
	args->options->set<bool>("expander.hsv", gtk_expander_get_expanded(GTK_EXPANDER(args->hsvExpander)));
	args->options->set<bool>("expander.hsl", gtk_expander_get_expanded(GTK_EXPANDER(args->hslExpander)));
	args->options->set<bool>("expander.lab", gtk_expander_get_expanded(GTK_EXPANDER(args->labExpander)));
	args->options->set<bool>("expander.lch", gtk_expander_get_expanded(GTK_EXPANDER(args->lchExpander)));
	args->options->set<bool>("expander.cmyk", gtk_expander_get_expanded(GTK_EXPANDER(args->cmykExpander)));
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
	GtkWidget *table;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1), args->options->getInt32("window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	table = gtk_table_new(2, 2, FALSE);

	Color rawColor;
	gtk_color_component_get_raw_color(colorComponent, rawColor);
	float alpha = gtk_color_component_get_alpha(colorComponent);

	const ColorSpaceType *colorSpaceType = 0;
	for (uint32_t i = 0; i < color_space_count_types(); i++) {
		if (color_space_get_types()[i].colorSpace == colorSpace) {
			colorSpaceType = &color_space_get_types()[i];
			break;
		}
	}

	if (colorSpaceType) {
		for (int i = 0; i < colorSpaceType->channelCount; i++) {
			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(colorSpaceType->channels[i].name, 0, 0, 0, 0), 0, 1, i, i + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
			args->value[i] = gtk_spin_button_new_with_range(colorSpaceType->channels[i].minValue, colorSpaceType->channels[i].maxValue, colorSpaceType->channels[i].step);
			gtk_entry_set_activates_default(GTK_ENTRY(args->value[i]), true);
			if (i < 4)
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), rawColor[i] * colorSpaceType->channels[i].rawScale);
			else
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), alpha * colorSpaceType->channels[i].rawScale);
			gtk_table_attach(GTK_TABLE(table), args->value[i], 1, 2, i, i + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
			if (i == channel)
				gtk_widget_grab_focus(args->value[i]);
		}
	}

	gtk_widget_show_all(table);
	setDialogContent(dialog, table);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		if (colorSpaceType) {
			for (int i = 0; i < colorSpaceType->channelCount; i++) {
				if (i < 4)
					rawColor[i] = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / colorSpaceType->channels[i].rawScale);
				else
					alpha = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / colorSpaceType->channels[i].rawScale);
			}
			gtk_color_component_set_raw_color(colorComponent, rawColor, alpha);
		}
	}

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	options->set("window.width", width);
	options->set("window.height", height);
	gtk_widget_destroy(dialog);
	delete args;
}
