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

#include "uiConverter.h"
#include "Converters.h"
#include "Converter.h"
#include "uiUtilities.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "I18N.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "EventBus.h"
#include <iostream>
using namespace std;

typedef enum
{
	CONVERTERLIST_LABEL = 0,
	CONVERTERLIST_EXAMPLE,
	CONVERTERLIST_CONVERTER_PTR,
	CONVERTERLIST_COPY,
	CONVERTERLIST_COPY_ENABLED,
	CONVERTERLIST_PASTE,
	CONVERTERLIST_PASTE_ENABLED,
	CONVERTERLIST_N_COLUMNS
}ConverterListColumns;

struct ConverterArgs
{
	GtkWidget *list;
	dynv::Ref options;
	GlobalState *gs;
};

static void converter_update_row(GtkTreeModel *model, GtkTreeIter *iter1, Converter *converter, ConverterArgs *args)
{
	ConverterSerializePosition position(1);
	Color c;
	c.rgb.red = 0.75;
	c.rgb.green = 0.50;
	c.rgb.blue = 0.25;
	ColorObject *color_object = color_list_new_color_object(args->gs->getColorList(), &c);
	color_object->setName(_("Test color"));
	string text_line = converter->serialize(color_object, position);
	gtk_list_store_set(GTK_LIST_STORE(model), iter1,
		CONVERTERLIST_LABEL, converter->label().c_str(),
		CONVERTERLIST_EXAMPLE, text_line.c_str(),
		CONVERTERLIST_CONVERTER_PTR, converter,
		CONVERTERLIST_COPY, converter->copy(),
		CONVERTERLIST_COPY_ENABLED, converter->hasSerialize(),
		CONVERTERLIST_PASTE, converter->paste(),
		CONVERTERLIST_PASTE_ENABLED, converter->hasDeserialize(),
		-1);
	color_object->release();
}
static void copy_toggled_cb(GtkCellRendererText *cell, gchar *path, ConverterArgs *args)
{
	GtkTreeIter iter1;
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->list)));
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path );
	gboolean value;
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter1, CONVERTERLIST_COPY, &value, -1);
	gtk_list_store_set(store, &iter1, CONVERTERLIST_COPY, !value, -1);
}
static void paste_toggled_cb(GtkCellRendererText *cell, gchar *path, ConverterArgs *args)
{
	GtkTreeIter iter1;
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->list)));
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path );
	gboolean value;
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter1, CONVERTERLIST_PASTE, &value, -1);
	gtk_list_store_set(store, &iter1, CONVERTERLIST_PASTE, !value, -1);
}
static GtkWidget* converter_dropdown_new(ConverterArgs *args, GtkTreeModel *model)
{
	GtkListStore *store = 0;
	GtkCellRenderer *renderer;
	GtkWidget *combo;
	if (model){
		combo = gtk_combo_box_new_with_model(model);
	}else{
		store = gtk_list_store_new (CONVERTERLIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	}
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, true);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", CONVERTERLIST_LABEL, nullptr);
	if (store) g_object_unref (store);
	return combo;
}
static GtkWidget* converter_list_new(ConverterArgs *args)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	GtkWidget *view = gtk_tree_view_new();
	args->list = view;
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view),1);
	store = gtk_list_store_new (CONVERTERLIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, _("Function name"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", CONVERTERLIST_LABEL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, _("Example"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", CONVERTERLIST_EXAMPLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_title(col, _("Copy"));
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, false);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_signal_connect(renderer, "toggled", (GCallback) copy_toggled_cb, args);
	gtk_tree_view_column_set_attributes(col, renderer, "active", CONVERTERLIST_COPY, "activatable", CONVERTERLIST_COPY_ENABLED, (void*)0);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_title(col, _("Paste / Edit"));
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
void dialog_converter_show(GtkWindow *parent, GlobalState *gs)
{
	ConverterArgs *args = new ConverterArgs;
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick");
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Converters"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("converters.window.width", 640),
		args->options->getInt32("converters.window.height", 400));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	GtkWidget *list = converter_list_new(args);
	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX(vbox), scrolled, true, true, 0);
	gint table_y;
	GtkWidget* table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(vbox), table, false, false, 0);
	table_y=0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Displays:"),0,0.5,0,0), 0, 1, table_y, table_y+1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	GtkWidget *display = converter_dropdown_new(args, 0);
	GtkTreeModel *model2=gtk_combo_box_get_model(GTK_COMBO_BOX(display));
	gtk_table_attach(GTK_TABLE(table), display, 1, 2, table_y, table_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Color list:"),0,0.5,0,0), 0, 1, table_y, table_y+1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	GtkWidget *color_list = converter_dropdown_new(args, model2);
	gtk_table_attach(GTK_TABLE(table), color_list, 1, 2, table_y, table_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;

	GtkTreeIter iter1, iter2;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	Converter *display_converter = args->gs->converters().display();
	Converter *color_list_converter = args->gs->converters().colorList();
	bool display_converter_found = false, color_list_converter_found = false;
	for (auto &converter: args->gs->converters().all()){
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		converter_update_row(model, &iter1, converter, args);
		gtk_list_store_append(GTK_LIST_STORE(model2), &iter2);
		converter_update_row(model2, &iter2, converter, args);
		if (display_converter == converter){
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(display), &iter2);
			display_converter_found = true;
		}
		if (color_list_converter == converter){
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(color_list), &iter2);
			color_list_converter_found = true;
		}
	}
	if (!display_converter_found){
		gtk_combo_box_set_active(GTK_COMBO_BOX(display), 0);
	}
	if (!color_list_converter_found){
		gtk_combo_box_set_active(GTK_COMBO_BOX(color_list), 0);
	}
	gtk_widget_show_all(vbox);
	setDialogContent(dialog, vbox);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		GtkTreeIter iter;
		GtkListStore *store;
		gboolean valid;
		Converter *converter;
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(display), &iter2)){
			gtk_tree_model_get(GTK_TREE_MODEL(model2), &iter2, CONVERTERLIST_CONVERTER_PTR, &converter, -1);
			args->gs->converters().display(converter);
			args->options->set("converters.display", converter->name());
		}

		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(color_list), &iter2)){
			gtk_tree_model_get(GTK_TREE_MODEL(model2), &iter2, CONVERTERLIST_CONVERTER_PTR, &converter, -1);
			args->gs->converters().colorList(converter);
			args->options->set("converters.color_list", converter->name());
		}

		store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
		size_t count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), nullptr);
		if (count > 0) {
			std::vector<std::string> names(count);
			std::vector<bool> copyArray(count);
			std::vector<bool> pasteArray(count);
			size_t i = 0;
			while (valid) {
				Converter* converter;
				gboolean copy, paste;
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, CONVERTERLIST_CONVERTER_PTR, &converter, CONVERTERLIST_COPY, &copy, CONVERTERLIST_PASTE, &paste, -1);
				names[i] = converter->name();
				copyArray[i] = copy;
				pasteArray[i] = paste;
				converter->copy(copy);
				converter->paste(paste);
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
				++i;
			}
			args->gs->converters().reorder(names);
			args->options->set("converters.names", names);
			args->options->set("converters.copy", copyArray);
			args->options->set("converters.paste", pasteArray);
		}else{
			args->options->remove("converters.names");
			args->options->remove("converters.copy");
			args->options->remove("converters.paste");
		}
		args->gs->converters().rebuildCopyPasteArrays();
		args->gs->eventBus().trigger(EventType::convertersUpdate);
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("converters.window.width", width);
	args->options->set("converters.window.height", height);
	gtk_widget_destroy(dialog);
	delete args;
}
