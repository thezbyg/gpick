/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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


struct Arguments{
	GtkWidget *minimize_to_tray;
	GtkWidget *close_to_tray;
	GtkWidget *start_in_tray;
	GtkWidget *refresh_rate;
	GKeyFile* settings;
};

static void calc( struct Arguments *args, bool preview, int limit){

	gboolean minimize_to_tray=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->minimize_to_tray));
	gboolean close_to_tray=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->close_to_tray));
	gboolean start_in_tray=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->start_in_tray));
	gdouble refresh_rate=gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->refresh_rate));

	if (!preview){
		g_key_file_set_boolean(args->settings, "Window", "Minimize to tray", minimize_to_tray);
		g_key_file_set_boolean(args->settings, "Window", "Close to tray", close_to_tray);
		g_key_file_set_boolean(args->settings, "Window", "Start in tray", start_in_tray);	
		g_key_file_set_double(args->settings, "Sampler", "Refresh rate", refresh_rate);
	}
}


void dialog_options_show(GtkWindow* parent, GKeyFile* settings) {
	struct Arguments args;
	
	args.settings = settings;
	
	GtkWidget *table, *widget;


	GtkWidget *dialog = gtk_dialog_new_with_buttons("Options", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	
	gtk_window_set_default_size(GTK_WINDOW(dialog), g_key_file_get_integer_with_default(settings, "Options Dialog", "Width", -1), 
		g_key_file_get_integer_with_default(settings, "Options Dialog", "Height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);


	gint table_y;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;


	args.minimize_to_tray = widget = gtk_check_button_new_with_mnemonic ("_Minimize to tray");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(settings, "Window", "Minimize to tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	
	args.close_to_tray = widget = gtk_check_button_new_with_mnemonic ("_Close to tray");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(settings, "Window", "Close to tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	args.start_in_tray = widget = gtk_check_button_new_with_mnemonic ("_Start in tray");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), g_key_file_get_boolean_with_default(settings, "Window", "Start in tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new("_Refresh rate:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	args.refresh_rate = widget = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args.refresh_rate), g_key_file_get_double_with_default(args.settings, "Sampler", "Refresh rate", 30));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hz",0,0.5,0,0),2,3,table_y,table_y+1,GTK_FILL,GTK_FILL,5,0);
	table_y++;



	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(&args, false, 0);

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	g_key_file_set_integer(settings, "Options Dialog", "Width", width);
	g_key_file_set_integer(settings, "Options Dialog", "Height", height);
	
	
	gtk_widget_destroy(dialog);

}
