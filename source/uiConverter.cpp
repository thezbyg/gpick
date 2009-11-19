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

#include "uiConverter.h"

#include "Converter.h"
#include "uiUtilities.h"
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"

#include <iostream>
using namespace std;

typedef enum{
	CONVERTERLIST_HUMAN_NAME = 0,
	CONVERTERLIST_EXAMPLE,
	CONVERTERLIST_CONVERTER_PTR,
	CONVERTERLIST_COPY,
	CONVERTERLIST_COPY_ENABLED,
	CONVERTERLIST_PASTE,
	CONVERTERLIST_PASTE_ENABLED,
	CONVERTERLIST_N_COLUMNS
}ConverterListColumns;

struct Arguments{
	GtkWidget* list;
	GtkWidget* combo;
	
	struct dynvSystem *params;
	GlobalState *gs;
};

static void converter_update_row(GtkTreeModel *model, GtkTreeIter *iter1, Converter *converter, struct Arguments *args) {
	gchar* converted;
	
	Color c;
	c.rgb.red=0.75;
	c.rgb.green=0.50;
	c.rgb.blue=0.25;
	struct ColorObject *color_object=color_list_new_color_object(args->gs->colors, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)"Test color");
	
	if (converters_color_serialize((Converters*)dynv_system_get(args->gs->params, "ptr", "Converters"), converter->function_name, color_object, &converted)==0) {
		gtk_list_store_set(GTK_LIST_STORE(model), iter1,
			CONVERTERLIST_HUMAN_NAME, converter->human_readable,
			CONVERTERLIST_EXAMPLE, converted,
			CONVERTERLIST_CONVERTER_PTR, converter,
			CONVERTERLIST_COPY, converter->copy,
			CONVERTERLIST_COPY_ENABLED, converter->serialize_available,
			CONVERTERLIST_PASTE, converter->paste,
			CONVERTERLIST_PASTE_ENABLED, converter->deserialize_available,
		-1);
		g_free(converted);
	}else{
		gtk_list_store_set(GTK_LIST_STORE(model), iter1,
			CONVERTERLIST_HUMAN_NAME, converter->human_readable,
			CONVERTERLIST_EXAMPLE, "error",
			CONVERTERLIST_CONVERTER_PTR, converter,
			CONVERTERLIST_COPY, converter->copy,
			CONVERTERLIST_COPY_ENABLED, converter->serialize_available,
			CONVERTERLIST_PASTE, converter->paste,
			CONVERTERLIST_PASTE_ENABLED, converter->deserialize_available,
		-1);
	}	
	
	color_object_release(color_object);
}



static void copy_toggled_cb(GtkCellRendererText *cell, gchar *path, struct Arguments *args) {
	GtkTreeIter iter1;
	GtkListStore *store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->list)));
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path );
	gboolean value;
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter1, CONVERTERLIST_COPY, &value, -1);
	gtk_list_store_set(store, &iter1, CONVERTERLIST_COPY, !value, -1);
}

static void paste_toggled_cb(GtkCellRendererText *cell, gchar *path, struct Arguments *args) {
	GtkTreeIter iter1;
	GtkListStore *store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->list)));
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path );
	gboolean value;
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter1, CONVERTERLIST_PASTE, &value, -1);	
	gtk_list_store_set(store, &iter1, CONVERTERLIST_PASTE, !value, -1);
}

static GtkWidget* converter_dropdown_new(struct Arguments *args, GtkTreeModel *model){
	
	GtkListStore  		*store = 0;
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;
	GtkWidget			*combo;
	
	if (model){
		combo = gtk_combo_box_new_with_model(model);
	}else{
		store = gtk_list_store_new (CONVERTERLIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	}
	
	renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, true);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", CONVERTERLIST_HUMAN_NAME, NULL);
	
	if (store) g_object_unref (store);
	
	return combo;
}

static GtkWidget* converter_list_new(struct Arguments *args){

	GtkListStore  		*store;
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;
	GtkWidget           *view;

	view = gtk_tree_view_new ();
	args->list=view;

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view),1);

	store = gtk_list_store_new (CONVERTERLIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Function name");
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", CONVERTERLIST_HUMAN_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Example");
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", CONVERTERLIST_EXAMPLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_title(col, "Copy");
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, false);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_signal_connect(renderer, "toggled", (GCallback) copy_toggled_cb, args);
	gtk_tree_view_column_set_attributes(col, renderer, "active", CONVERTERLIST_COPY, "activatable", CONVERTERLIST_COPY_ENABLED, (void*)0);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_title(col, "Paste");
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, false);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_signal_connect(renderer, "toggled", (GCallback) paste_toggled_cb, args);
	gtk_tree_view_column_set_attributes(col, renderer, "active", CONVERTERLIST_PASTE, "activatable", CONVERTERLIST_PASTE_ENABLED, (void*)0);


	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL(store));
	g_object_unref (GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(view) );
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW (view), TRUE);

	return view;
}

void dialog_converter_show(GtkWindow* parent, GlobalState* gs){

	struct Arguments *args = new struct Arguments;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick");
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Converters", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			NULL);
	
	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "converters.window.width", -1),
		dynv_get_int32_wd(args->params, "converters.window.height", -1));	

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	
	GtkWidget* vbox = gtk_vbox_new(false, 5);

	GtkWidget *list;
	list = converter_list_new(args);
	
	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), list);
	
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX(vbox), scrolled, true, true, 0);
	
	gint table_y;
	GtkWidget* table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(vbox), table, false, false, 0);
	table_y=0;
	

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Displays:",0,0.5,0,0), 0, 1, table_y, table_y+1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	GtkWidget *display = converter_dropdown_new(args, 0);
	GtkTreeModel *model2=gtk_combo_box_get_model(GTK_COMBO_BOX(display));
	gtk_table_attach(GTK_TABLE(table), display, 1, 2, table_y, table_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;
	
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Color list:",0,0.5,0,0), 0, 1, table_y, table_y+1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	GtkWidget *color_list = converter_dropdown_new(args, model2);
	gtk_table_attach(GTK_TABLE(table), color_list, 1, 2, table_y, table_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;

	
	Converters *converters = (Converters*)dynv_system_get(args->gs->params, "ptr", "Converters");
	
	Converter **converter_table;
	uint32_t total_converters, converter_i;
	converter_table = converters_get_all(converters, &total_converters);
	
	GtkTreeIter iter1;
	GtkTreeModel *model=gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	
	GtkTreeIter iter2;
	
	
	Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_DISPLAY);
	bool display_converter_found = false;
	Converter *list_converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COLOR_LIST);
	bool color_list_converter_found = false;
	
	converter_i = 0;
	while (converter_i<total_converters){
		
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		converter_update_row(model, &iter1, converter_table[converter_i], args);
		
		gtk_list_store_append(GTK_LIST_STORE(model2), &iter2);
		converter_update_row(model2, &iter2, converter_table[converter_i], args);
		
		
		
		if (converter == converter_table[converter_i]){
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(display), &iter2);
			display_converter_found = true;
		}
		
		if (list_converter == converter_table[converter_i]){
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(color_list), &iter2);
			color_list_converter_found = true;
		}	
		
		++converter_i;
	}
	
	if (!display_converter_found){
		gtk_combo_box_set_active(GTK_COMBO_BOX(display), 0);
	}
	if (!color_list_converter_found){
		gtk_combo_box_set_active(GTK_COMBO_BOX(color_list), 0);
	}
	
	delete [] converter_table;

	gtk_widget_show_all(vbox);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, true, true, 5);
	
	//gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 240);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
	
		GtkTreeIter iter;
		GtkListStore *store;
		gboolean valid;
		
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(display), &iter2)){
			gtk_tree_model_get(GTK_TREE_MODEL(model2), &iter2, CONVERTERLIST_CONVERTER_PTR, &converter, -1);
			converters_set(converters, converter, CONVERTERS_ARRAY_TYPE_DISPLAY);
			dynv_set_string(args->params, "converters.display", converter->function_name);
		}
		
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(color_list), &iter2)){
			gtk_tree_model_get(GTK_TREE_MODEL(model2), &iter2, CONVERTERLIST_CONVERTER_PTR, &converter, -1);
			converters_set(converters, converter, CONVERTERS_ARRAY_TYPE_COLOR_LIST);
			dynv_set_string(args->params, "converters.color_list", converter->function_name);
		}	

		store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
		
		unsigned int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
		if (count>0){
			char** name_array = new char*[count];
			bool* copy_array = new bool[count];	
			bool* paste_array = new bool[count];			
			unsigned int i=0;
			
			while (valid){
				gchar* function_name;
				Converter* converter;
				gboolean copy, paste;
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, CONVERTERLIST_CONVERTER_PTR, &converter, CONVERTERLIST_COPY, &copy, CONVERTERLIST_PASTE, &paste, -1);
			
				name_array[i] = converter->function_name;
				copy_array[i] = copy;
				paste_array[i] = paste;
				
				converter->copy = copy;
				converter->paste = paste;
			
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
				++i;
			}
			
			converters_reorder(converters, (const char**)name_array, count);
			
			dynv_set_string_array(args->params, "converters.names", (const char**)name_array, count);
			dynv_set_bool_array(args->params, "converters.copy", copy_array, count);
			dynv_set_bool_array(args->params, "converters.paste", paste_array, count);
		
			delete [] name_array;
			delete [] copy_array;
			delete [] paste_array;		
		}else{
			converters_reorder(converters, 0, 0);
			
			dynv_set_string_array(args->params, "converters.names", 0, 0);
			dynv_set_bool_array(args->params, "converters.copy", 0, 0);
			dynv_set_bool_array(args->params, "converters.paste", 0, 0);

		}
		
		
		converters_rebuild_arrays(converters, CONVERTERS_ARRAY_TYPE_COPY);
		converters_rebuild_arrays(converters, CONVERTERS_ARRAY_TYPE_PASTE);
	
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	dynv_set_int32(args->params, "converters.window.width", width);
	dynv_set_int32(args->params, "converters.window.height", height);
	
	gtk_widget_destroy(dialog);
	
	dynv_system_release(args->params);
	delete args;
}



