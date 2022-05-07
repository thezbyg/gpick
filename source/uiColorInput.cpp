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
#include <string.h>
#include <string>
#include <iostream>
using namespace std;

struct DialogInputArgs
{
	ColorObject *color_object;
	GtkWidget *color_widget;
	GtkWidget *text_input;
	GtkWidget *rgb_expander, *hsv_expander, *hsl_expander, *cmyk_expander, *xyz_expander, *lab_expander, *lch_expander;
	GtkWidget *rgb_control, *hsv_control, *hsl_control, *cmyk_control, *xyz_control, *lab_control, *lch_control;
	dynv::Ref options;
	GlobalState* gs;
};
static void updateComponentText(DialogInputArgs *args, GtkColorComponent *component, const char *type)
{
	Color transformed_color;
	gtk_color_component_get_transformed_color(component, &transformed_color);
	float alpha = gtk_color_component_get_alpha(component);
	lua::Script &script = args->gs->script();
	vector<string> values = color_space_color_to_text(type, transformed_color, alpha, script, args->gs);
	const char *text[5] = {0};
	int j = 0;
	for (auto &value: values){
		text[j++] = value.c_str();
		if (j > 4)
			break;
	}
	gtk_color_component_set_text(component, text);
}
static void update(DialogInputArgs *args, GtkWidget *except_widget)
{
	auto color = args->color_object->getColor();
	gtk_color_set_color(GTK_COLOR(args->color_widget), &color, "");
	if (except_widget != args->text_input){
		string text = args->gs->converters().serialize(args->color_object, Converters::Type::display);
		gtk_entry_set_text(GTK_ENTRY(args->text_input), text.c_str());
	}
	if (except_widget != args->hsl_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hsl_control), &color);
	if (except_widget != args->hsv_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->hsv_control), &color);
	if (except_widget != args->rgb_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->rgb_control), &color);
	if (except_widget != args->cmyk_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->cmyk_control), &color);
	if (except_widget != args->lab_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->lab_control), &color);
	if (except_widget != args->lch_control) gtk_color_component_set_color(GTK_COLOR_COMPONENT(args->lch_control), &color);
	updateComponentText(args, GTK_COLOR_COMPONENT(args->hsl_control), "hsl");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->hsv_control), "hsv");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->rgb_control), "rgb");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->cmyk_control), "cmyk");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lab_control), "lab");
	updateComponentText(args, GTK_COLOR_COMPONENT(args->lch_control), "lch");
}
static void onComponentChangeValue(GtkWidget *widget, Color *color, DialogInputArgs *args)
{
	args->color_object->setColor(*color);
	update(args, widget);
}
static void onComponentInputClicked(GtkWidget *widget, int component_id, DialogInputArgs *args)
{
	dialog_color_component_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_COLOR_COMPONENT(widget), component_id, args->options->getOrCreateMap("component_edit"));
}
static void setComponentHandlers(GtkWidget *widget, DialogInputArgs *args)
{
	g_signal_connect(G_OBJECT(widget), "color-changed", G_CALLBACK(onComponentChangeValue), args);
	g_signal_connect(G_OBJECT(widget), "input-clicked", G_CALLBACK(onComponentInputClicked), args);
}
static void addComponentEditor(GtkWidget *vbox, const char *label, const char *expand_option, GtkColorComponentComp type, const char **labels, DialogInputArgs *args, GtkWidget *&expander, GtkWidget *&control)
{
	expander = gtk_expander_new(label);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), args->options->getBool(expand_option, false));
	gtk_box_pack_start(GTK_BOX(vbox), expander, false, false, 0);
	control = gtk_color_component_new(type);
	gtk_color_component_set_label(GTK_COLOR_COMPONENT(control), labels);
	setComponentHandlers(control, args);
	gtk_container_add(GTK_CONTAINER(expander), control);
}
static void onTextChanged(GtkWidget *entry, DialogInputArgs *args)
{
	ColorObject *color_object;
	if (args->gs->converters().deserialize((char*)gtk_entry_get_text(GTK_ENTRY(entry)), &color_object)){
		args->color_object->setColor(color_object->getColor());
		color_object->release();
		update(args, entry);
	}
}

int dialog_color_input_show(GtkWindow *parent, GlobalState *gs, ColorObject *color_object, ColorObject **new_color_object)
{
	auto args = new DialogInputArgs();
	args->gs = gs;
	args->options = gs->settings().getOrCreateMap("gpick.color_input");
	bool new_item = false;
	if (color_object) {
		args->color_object = color_object->copy();
	} else {
		new_item = true;
		args->color_object = new ColorObject();
	}
	GtkWidget *dialog = gtk_dialog_new_with_buttons(new_item ? _("Add color") : _("Edit color"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1), args->options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	GtkWidget *widget = args->color_widget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_widget_set_size_request(widget, 30, 30);
	if (!new_item) {
		gtk_color_enable_split(GTK_COLOR(widget), true);
		gtk_color_set_split_color(GTK_COLOR(widget), &color_object->getColor());
	}
	gtk_box_pack_start(GTK_BOX(vbox), widget, true, true, 0);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_aligned_new(_("Color:"), 0, 0.5, 0, 0), false, false, 0);

	GtkWidget* entry = args->text_input = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), true);
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
	gtk_widget_grab_focus(entry);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(onTextChanged), args);

	const char *hsv_labels[] = {"H", _("Hue"), "S", _("Saturation"), "V", _("Value"), "A", _("Alpha"), nullptr};
	addComponentEditor(vbox, "HSV", "expander.hsv", GtkColorComponentComp::hsv, hsv_labels, args, args->hsv_expander, args->hsv_control);
	const char *hsl_labels[] = {"H", _("Hue"), "S", _("Saturation"), "L", _("Lightness"), "A", _("Alpha"), nullptr};
	addComponentEditor(vbox, "HSL", "expander.hsl", GtkColorComponentComp::hsl, hsl_labels, args, args->hsl_expander, args->hsl_control);
	const char *rgb_labels[] = {"R", _("Red"), "G", _("Green"), "B", _("Blue"), "A", _("Alpha"), nullptr};
	addComponentEditor(vbox, "RGB", "expander.rgb", GtkColorComponentComp::rgb, rgb_labels, args, args->rgb_expander, args->rgb_control);
	const char *cmyk_labels[] = {"C", _("Cyan"), "M", _("Magenta"), "Y", _("Yellow"), "K", _("Key"), "A", _("Alpha"), nullptr};
	addComponentEditor(vbox, "CMYK", "expander.cmyk", GtkColorComponentComp::cmyk, cmyk_labels, args, args->cmyk_expander, args->cmyk_control);
	const char *lab_labels[] = {"L", _("Lightness"), "a", "a", "b", "b", "A", _("Alpha"), nullptr};
	addComponentEditor(vbox, "Lab", "expander.lab", GtkColorComponentComp::lab, lab_labels, args, args->lab_expander, args->lab_control);
	const char *lch_labels[] = {"L", _("Lightness"), "C", "Chroma", "H", "Hue", "A", _("Alpha"), nullptr};
	addComponentEditor(vbox, "LCH", "expander.lch", GtkColorComponentComp::lch, lch_labels, args, args->lch_expander, args->lch_control);

	if (new_item) {
		auto text = args->options->getString("text", "");
		gtk_entry_set_text(GTK_ENTRY(args->text_input), text.c_str());
	} else {
		update(args, nullptr);
	}

	gtk_widget_show_all(vbox);
	setDialogContent(dialog, vbox);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	int result = -1;
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		if (new_item) {
			string name = color_names_get(args->gs->getColorNames(), &args->color_object->getColor(), args->gs->settings().getBool("gpick.color_names.imprecision_postfix", false));
			args->color_object->setName(name);
		}
		*new_color_object = args->color_object->reference();
		result = 0;
	}
	if (new_item)
		args->options->set("text", gtk_entry_get_text(GTK_ENTRY(entry)));
	args->options->set<bool>("expander.rgb", gtk_expander_get_expanded(GTK_EXPANDER(args->rgb_expander)));
	args->options->set<bool>("expander.hsv", gtk_expander_get_expanded(GTK_EXPANDER(args->hsv_expander)));
	args->options->set<bool>("expander.hsl", gtk_expander_get_expanded(GTK_EXPANDER(args->hsl_expander)));
	args->options->set<bool>("expander.lab", gtk_expander_get_expanded(GTK_EXPANDER(args->lab_expander)));
	args->options->set<bool>("expander.lch", gtk_expander_get_expanded(GTK_EXPANDER(args->lch_expander)));
	args->options->set<bool>("expander.cmyk", gtk_expander_get_expanded(GTK_EXPANDER(args->cmyk_expander)));
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("window.width", width);
	args->options->set("window.height", height);
	gtk_widget_destroy(dialog);
	args->color_object->release();
	delete args;
	return result;
}

struct ColorPickerComponentEditArgs
{
	GtkWidget* value[5];
	GtkColorComponentComp component;
	int component_id;
	dynv::Ref options;
};

void dialog_color_component_input_show(GtkWindow *parent, GtkColorComponent *color_component, int component_id, dynv::Ref options)
{
	GtkColorComponentComp component = gtk_color_component_get_component(GTK_COLOR_COMPONENT(color_component));
	ColorPickerComponentEditArgs *args = new ColorPickerComponentEditArgs;
	args->options = options;
	args->component = component;
	args->component_id = component_id;
	memset(args->value, 0, sizeof(args->value));
	GtkWidget *table;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1), args->options->getInt32("window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	table = gtk_table_new(2, 2, FALSE);

	Color raw_color;
	gtk_color_component_get_raw_color(color_component, &raw_color);
	float alpha = gtk_color_component_get_alpha(color_component);

	const ColorSpaceType *color_space_type = 0;
	for (uint32_t i = 0; i < color_space_count_types(); i++){
		if (color_space_get_types()[i].comp_type == component){
			color_space_type = &color_space_get_types()[i];
			break;
		}
	}

	if (color_space_type){
		for (int i = 0; i < color_space_type->n_items; i++){
			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(color_space_type->items[i].name,0,0,0,0),0,1,i,i+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
			args->value[i] = gtk_spin_button_new_with_range(color_space_type->items[i].min_value, color_space_type->items[i].max_value, color_space_type->items[i].step);
			gtk_entry_set_activates_default(GTK_ENTRY(args->value[i]), true);
			if (i < 4)
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), raw_color[i] * color_space_type->items[i].raw_scale);
			else
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), alpha * color_space_type->items[i].raw_scale);
			gtk_table_attach(GTK_TABLE(table), args->value[i],1,2,i,i+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			if (i == component_id)
				gtk_widget_grab_focus(args->value[i]);
		}
	}

	gtk_widget_show_all(table);
	setDialogContent(dialog, table);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		if (color_space_type){
			for (int i = 0; i < color_space_type->n_items; i++){
				if (i < 4)
					raw_color[i] = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / color_space_type->items[i].raw_scale);
				else
					alpha = static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / color_space_type->items[i].raw_scale);
			}
			gtk_color_component_set_raw_color(color_component, raw_color, alpha);
		}
	}

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	options->set("window.width", width);
	options->set("window.height", height);
	gtk_widget_destroy(dialog);
	delete args;
}
