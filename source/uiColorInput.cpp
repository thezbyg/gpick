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

#include "uiColorInput.h"
#include "Converter.h"
#include "main.h"
#include "uiUtilities.h"

int dialog_color_input_show(GtkWindow* parent, GlobalState* gs, struct ColorObject* color_object, struct ColorObject** new_color_object){
	
	gchar* text = 0;
	
	Converter *converter;
	Converters *converters = (Converters*)dynv_system_get(gs->params, "ptr", "Converters");
	converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_DISPLAY);
	if (converter){
		converter_get_text(converter->function_name, color_object, 0, gs->params, &text);
	}
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Edit color", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	
	GtkWidget* vbox = gtk_vbox_new(false, 5);
	
	GtkWidget* hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_aligned_new("Color:",0,0.5,0,0), false, false, 0);
	
	GtkWidget* entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
	
	if (text){
		gtk_entry_set_text(GTK_ENTRY(entry), text);
		g_free(text);
	}
	
	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		
		struct ColorObject* color_object;
		if (main_get_color_object_from_text(gs, (char*)gtk_entry_get_text(GTK_ENTRY(entry)), &color_object)==0){
			*new_color_object = color_object;
			gtk_widget_destroy(dialog);
			return 0;
		}
	}
	gtk_widget_destroy(dialog);	
	return -1;
}

