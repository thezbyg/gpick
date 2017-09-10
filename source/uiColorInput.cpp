/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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
#include "DynvHelpers.h"
#include "uiUtilities.h"
#include "GlobalState.h"
#include "gtk/ColorWheel.h"
#include "I18N.h"
#include "gtk/ColorComponent.h"
#include "gtk/ColorWidget.h"
#include "ColorObject.h"
#include "ColorSpaceType.h"
#include <string.h>
#include <string>
using namespace std;

int dialog_color_input_show(GtkWindow *parent, GlobalState *gs, ColorObject *color_object, ColorObject **new_color_object)
{
	string text = gs->converters().serialize(color_object, Converters::Type::display);
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit color"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	GtkWidget* vbox = gtk_vbox_new(false, 5);
	GtkWidget* hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, true, true, 0);

	GtkWidget *widget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);

	Color color = color_object->getColor();
	gtk_color_set_color(GTK_COLOR(widget), &color, "");

	gtk_box_pack_start(GTK_BOX(hbox), widget, false, true, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_aligned_new(_("Color:"),0,0.5,0,0), false, false, 0);

	GtkWidget* entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(entry), true);
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);

	gtk_entry_set_text(GTK_ENTRY(entry), text.c_str());

	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		ColorObject* color_object;
		if (gs->converters().deserialize((char*)gtk_entry_get_text(GTK_ENTRY(entry)), &color_object)){
			*new_color_object = color_object;
			gtk_widget_destroy(dialog);
			return 0;
		}
	}
	gtk_widget_destroy(dialog);
	return -1;
}

struct ColorPickerComponentEditArgs
{
	GtkWidget* value[4];
	GtkColorComponentComp component;
	int component_id;
	struct dynvSystem *params;
};

void dialog_color_component_input_show(GtkWindow *parent, GtkColorComponent *color_component, int component_id, dynvSystem *params)
{
	GtkColorComponentComp component = gtk_color_component_get_component(GTK_COLOR_COMPONENT(color_component));
	ColorPickerComponentEditArgs *args = new ColorPickerComponentEditArgs;
	args->params = params;
	args->component = component;
	args->component_id = component_id;
	memset(args->value, 0, sizeof(args->value));
	GtkWidget *table;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Edit"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1), dynv_get_int32_wd(args->params, "window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	table = gtk_table_new(2, 2, FALSE);

	Color raw_color;
	gtk_color_component_get_raw_color(color_component, &raw_color);

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
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->value[i]), raw_color.ma[i] * color_space_type->items[i].raw_scale);
			gtk_table_attach(GTK_TABLE(table), args->value[i],1,2,i,i+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
			if (i == component_id)
				gtk_widget_grab_focus(args->value[i]);
		}
	}

	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		if (color_space_type){
			for (int i = 0; i < color_space_type->n_items; i++){
				raw_color.ma[i] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->value[i])) / color_space_type->items[i].raw_scale;
			}
			gtk_color_component_set_raw_color(color_component, &raw_color);
		}
	}

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	dynv_set_int32(args->params, "window.width", width);
	dynv_set_int32(args->params, "window.height", height);

	gtk_widget_destroy(dialog);

	dynv_system_release(args->params);
	delete args;
}
