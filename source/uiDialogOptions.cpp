/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#include "uiDialogOptions.h"
#include "uiUtilities.h"
#include "GlobalStateStruct.h"

#include "DynvHelpers.h"

typedef struct DialogOptionsArgs{
	GtkWidget *minimize_to_tray;
	GtkWidget *close_to_tray;
	GtkWidget *start_in_tray;
	GtkWidget *refresh_rate;
	GtkWidget *single_instance;
	GtkWidget *add_on_release;
	GtkWidget *add_to_palette;
	GtkWidget *copy_to_clipboard;
	GtkWidget *rotate_swatch;
	GtkWidget *copy_on_release;
	GtkWidget *zoom_size;
	GtkWidget *imprecision_postfix;

	struct dynvSystem *params;
	GlobalState* gs;
}DialogOptionsArgs;

static void calc( DialogOptionsArgs *args, bool preview, int limit){
	if (preview) return;

	dynv_set_bool(args->params, "main.minimize_to_tray", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->minimize_to_tray)));
	dynv_set_bool(args->params, "main.close_to_tray", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->close_to_tray)));
	dynv_set_bool(args->params, "main.start_in_tray", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->start_in_tray)));
	dynv_set_bool(args->params, "main.single_instance", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->single_instance)));

	dynv_set_float(args->params, "picker.refresh_rate", gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->refresh_rate)));
	dynv_set_int32(args->params, "picker.zoom_size", gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->zoom_size)));
	dynv_set_bool(args->params, "picker.sampler.add_on_release", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->add_on_release)));
	dynv_set_bool(args->params, "picker.sampler.copy_on_release", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->copy_on_release)));
	dynv_set_bool(args->params, "picker.sampler.add_to_palette", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->add_to_palette)));
	dynv_set_bool(args->params, "picker.sampler.copy_to_clipboard", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->copy_to_clipboard)));
	dynv_set_bool(args->params, "picker.sampler.rotate_swatch_after_sample", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->rotate_swatch)));

	dynv_set_bool(args->params, "color_names.imprecision_postfix", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->imprecision_postfix)));
}



void dialog_options_show(GtkWindow* parent, GlobalState* gs) {
	DialogOptionsArgs *args = new DialogOptionsArgs;

	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick");

	GtkWidget *table, *table_m, *widget;


	GtkWidget *dialog = gtk_dialog_new_with_buttons("Options", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);

	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "options.window.width", -1),
		dynv_get_int32_wd(args->params, "options.window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	GtkWidget *frame;
	GtkWidget* notebook = gtk_notebook_new();
	gint table_y, table_m_y;


	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new("System");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;

	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	args->single_instance = widget = gtk_check_button_new_with_mnemonic ("_Single instance");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "main.single_instance", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;


	frame = gtk_frame_new("System tray");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	args->minimize_to_tray = widget = gtk_check_button_new_with_mnemonic ("_Minimize to system tray");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "main.minimize_to_tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	args->close_to_tray = widget = gtk_check_button_new_with_mnemonic ("_Close to system tray");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "main.close_to_tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	args->start_in_tray = widget = gtk_check_button_new_with_mnemonic ("_Start in system tray");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "main.start_in_tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;


	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic("_Main"));



	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new("Display");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;

	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new("_Refresh rate:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	args->refresh_rate = widget = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->refresh_rate), dynv_get_float_wd(args->params, "picker.refresh_rate", 30));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,5);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hz",0,0.5,0,0),2,3,table_y,table_y+1,GTK_FILL,GTK_FILL,5,5);
	table_y++;


	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new("_Magnified area size:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	args->zoom_size = widget = gtk_spin_button_new_with_range(75, 300, 15);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->zoom_size), dynv_get_int32_wd(args->params, "picker.zoom_size", 150));
	gtk_table_attach(GTK_TABLE(table), widget,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,5);
	table_y++;

	frame = gtk_frame_new("Floating picker click behaviour");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	args->add_on_release = widget = gtk_check_button_new_with_mnemonic("_Add to palette");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "picker.sampler.add_on_release", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	args->copy_on_release = widget = gtk_check_button_new_with_mnemonic("_Copy to clipboard");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "picker.sampler.copy_on_release", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;


	frame = gtk_frame_new("'Spacebar' button behaviour");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	args->add_to_palette = widget = gtk_check_button_new_with_mnemonic("_Add to palette");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "picker.sampler.add_to_palette", false));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	args->copy_to_clipboard = widget = gtk_check_button_new_with_mnemonic("_Copy to clipboard");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "picker.sampler.copy_to_clipboard", false));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	args->rotate_swatch = widget = gtk_check_button_new_with_mnemonic("_Rotate swatch");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "picker.sampler.rotate_swatch_after_sample", false));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;


	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic("_Picker"));



	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new("Color name generation");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;

	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	args->imprecision_postfix = widget = gtk_check_button_new_with_mnemonic("_Imprecision postix");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), dynv_get_bool_wd(args->params, "color_names.imprecision_postfix", true));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic("_Color names"));


	gtk_widget_show_all(notebook);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), notebook);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(args, false, 0);

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);

	dynv_set_int32(args->params, "options.window.width", width);
	dynv_set_int32(args->params, "options.window.height", height);

	dynv_system_release(args->params);

	gtk_widget_destroy(dialog);

	delete args;

}
