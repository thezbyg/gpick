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

#include "uiColorDictionaries.h"
#include "uiUtilities.h"
#include "DynvHelpers.h"
#include "GlobalState.h"
#include "color_names/ColorNames.h"
#include "Internationalisation.h"
#include <list>
#include <string>
#include <vector>
#include <algorithm>
#include <gdk/gdkkeysyms.h>
using namespace std;

enum class ColorDictionaryList: int
{
	path = 0,
	built_in,
	enable,
	pointer,
	n_columns
};
struct ColorDictionary
{
	ColorDictionary():
		built_in(false),
		enable(false)
	{
	}
	ColorDictionary(const char *path, bool built_in, bool enable):
		path(path),
		built_in(built_in),
		enable(enable)
	{
	}
	string path;
	bool built_in, enable;
	size_t index;
	bool operator==(const ColorDictionary &color_dictionary) const
	{
		if (this == &color_dictionary) return true;
		if (path == color_dictionary.path && built_in == color_dictionary.built_in && enable == color_dictionary.enable) return true;
		return false;
	}
};
struct ColorDictionarySort
{
	bool operator()(const ColorDictionary &a, const ColorDictionary &b)
	{
		return a.index < b.index;
	}
};
struct ColorDictionariesArgs
{
	GtkWidget *dictionary_list, *file_browser;
	list<ColorDictionary> color_dictionaries;
	struct dynvSystem *params;
	GlobalState *gs;
};
static void color_dictionaries_update_row(GtkTreeModel *model, GtkTreeIter *iter1, ColorDictionary *color_dictionary, ColorDictionariesArgs *args)
{
	gtk_list_store_set(GTK_LIST_STORE(model), iter1,
		ColorDictionaryList::path, color_dictionary->path.c_str(),
		ColorDictionaryList::enable, color_dictionary->enable,
		ColorDictionaryList::built_in, color_dictionary->built_in,
		ColorDictionaryList::pointer, color_dictionary,
	-1);
}
static void enable_toggled_cb(GtkCellRendererText *cell, gchar *path, ColorDictionariesArgs *args)
{
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->dictionary_list)));
	GtkTreeIter iter1;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path);
	ColorDictionary *color_dictionary;
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter1, ColorDictionaryList::pointer, &color_dictionary, -1);
	color_dictionary->enable = !color_dictionary->enable;
	gtk_list_store_set(store, &iter1, ColorDictionaryList::enable, color_dictionary->enable, -1);
}
static GtkWidget* color_dictionaries_list_new(ColorDictionariesArgs *args)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	GtkWidget *view = args->dictionary_list = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), true);
	GtkListStore *store = gtk_list_store_new(static_cast<int>(ColorDictionaryList::n_columns), G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, true);
	gtk_tree_view_column_set_title(col, _("Path"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", static_cast<int>(ColorDictionaryList::path));

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_title(col, _("Enable"));
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, false);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_signal_connect(renderer, "toggled", reinterpret_cast<GCallback>(enable_toggled_cb), args);
	gtk_tree_view_column_set_attributes(col, renderer, "active", ColorDictionaryList::enable, nullptr);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
	g_object_unref(GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(view), true);
	return view;
}
static void add_file(GtkWidget *widget, ColorDictionariesArgs *args)
{
	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(args->file_browser));
	if (filename){
		args->color_dictionaries.push_back(ColorDictionary(filename, false, true));
		g_free(filename);
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->dictionary_list));
		GtkTreeIter iter1;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		color_dictionaries_update_row(model, &iter1, &args->color_dictionaries.back(), args);
	}
	gchar *current_folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(args->file_browser));
	if (current_folder){
		dynv_set_string(args->params, "current_folder", current_folder);
		g_free(current_folder);
	}
}
static void remove_selected(ColorDictionariesArgs *args)
{
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->dictionary_list)));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->dictionary_list));
	GtkTreeIter iter1;
	GList *selected_list = gtk_tree_selection_get_selected_rows(selection, nullptr);
	list<GtkTreeRowReference*> remove_items;
	{
		GList *i = selected_list;
		while (i){
			GtkTreeRowReference *row_reference;
			GtkTreePath *path;
			bool reference_inserted = false;
			if ((row_reference = gtk_tree_row_reference_new(GTK_TREE_MODEL(store), (GtkTreePath*) (i->data)))){
				if ((path = gtk_tree_row_reference_get_path(row_reference))){
					if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter1, path)){
						ColorDictionary *dictionary = nullptr;
						gtk_tree_model_get(GTK_TREE_MODEL(store), &iter1, static_cast<int>(ColorDictionaryList::pointer), &dictionary, -1);
						if (!dictionary->built_in){
							remove_items.push_back(row_reference);
							reference_inserted = true;
							args->color_dictionaries.remove(*dictionary);
						}
					}
					gtk_tree_path_free(path);
				}
				if (!reference_inserted)
					gtk_tree_row_reference_free(row_reference);
			}
			i = g_list_next(i);
		}
	}
	for (auto i = remove_items.begin(); i != remove_items.end(); i++){
		GtkTreePath *path = gtk_tree_row_reference_get_path(*i);
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter1, path)){
			gtk_list_store_remove(GTK_LIST_STORE(store), &iter1);
		}
		gtk_tree_row_reference_free(*i);
	}
	g_list_foreach(selected_list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(selected_list);
}
static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, ColorDictionariesArgs *args)
{
	switch (event->keyval){
		case GDK_KEY_Delete:
			remove_selected(args);
			break;
		default:
			return false;
		break;
	}
	return false;
}
static void reorder(ColorDictionariesArgs *args)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->dictionary_list));
	GtkTreeIter iter1;
	bool valid = gtk_tree_model_get_iter_first(model, &iter1);
	size_t index = 0;
	while (valid){
		ColorDictionary *color_dictionary;
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter1, ColorDictionaryList::pointer, &color_dictionary, -1);
		color_dictionary->index = index++;
		valid = gtk_tree_model_iter_next(model, &iter1);
	}
	args->color_dictionaries.sort(ColorDictionarySort());
}
void dialog_color_dictionaries_show(GtkWindow* parent, GlobalState* gs)
{
	ColorDictionariesArgs *args = new ColorDictionariesArgs;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->getSettings(), "gpick");
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Color dictionaries"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "color_dictionaries.window.width", -1),
		dynv_get_int32_wd(args->params, "color_dictionaries.window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	GtkWidget* vbox = gtk_vbox_new(false, 5);
	GtkWidget *list = color_dictionaries_list_new(args);
	g_signal_connect(G_OBJECT(list), "key_press_event", G_CALLBACK(key_press_event), args);

	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled, true, true, 0);

	gint table_y;
	GtkWidget* table = gtk_table_new(3, 1, false);
	gtk_box_pack_start(GTK_BOX(vbox), table, false, false, 0);
	table_y = 0;

	GtkWidget *frame = gtk_frame_new(_("Add color dictionary file"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table), frame, 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);

	GtkWidget *widget = args->file_browser = gtk_file_chooser_button_new(_("Color dictionary file"), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), dynv_get_string_wd(args->params, "current_folder", ""));
	gtk_container_add(GTK_CONTAINER(frame), widget);
	g_signal_connect(G_OBJECT(args->file_browser), "file-set", G_CALLBACK(add_file), args);
	table_y++;

	bool built_in_found = false;
	uint32_t dictionary_count = 0;
	struct dynvSystem** dictionaries = dynv_get_dynv_array_wd(args->params, "color_dictionaries.items", nullptr, 0, &dictionary_count);
	if (dictionaries){
		for (uint32_t i = 0; i < dictionary_count; i++){
			ColorDictionary dictionary;
			dictionary.path = dynv_get_string_wd(dictionaries[i], "path", "");
			dictionary.enable = dynv_get_bool_wd(dictionaries[i], "enable", "false");
			dictionary.built_in = dynv_get_bool_wd(dictionaries[i], "built_in", "false");
			if (dictionary.built_in){
				if (dictionary.path == "built_in_0"){
					built_in_found = true;
					dictionary.path = _("Built in");
				}else{
					dynv_system_release(dictionaries[i]);
					continue;
				}
			}
			args->color_dictionaries.push_back(dictionary);
			dynv_system_release(dictionaries[i]);
		}
		if (dictionaries) delete [] dictionaries;
	}
	if (!built_in_found){
		args->color_dictionaries.push_back(ColorDictionary(_("Built in"), true, true));
	}

	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	for (auto &dictionary: args->color_dictionaries){
		GtkTreeIter iter1;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		color_dictionaries_update_row(model, &iter1, &dictionary, args);
	}

	gtk_widget_show_all(vbox);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, true, true, 5);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		size_t item_count = args->color_dictionaries.size();
		if (item_count){
			reorder(args);
			vector<dynvSystem*> serialized;
			serialized.resize(item_count);
			size_t i = 0;
			for (auto &dictionary: args->color_dictionaries){
				serialized[i] = dynv_system_create(args->params);
				if (dictionary.built_in){
					dynv_set_string(serialized[i], "path", "built_in_0");
				}else{
					dynv_set_string(serialized[i], "path", dictionary.path.c_str());
				}
				dynv_set_bool(serialized[i], "enable", dictionary.enable);
				dynv_set_bool(serialized[i], "built_in", dictionary.built_in);
				i++;
			}
			dynv_set_dynv_array(args->params, "color_dictionaries.items", const_cast<const dynvSystem**>(serialized.data()), serialized.size());
			for (auto i: serialized){
				dynv_system_release(i);
			}
		}else{
			dynv_set_dynv_array(args->params, "color_dictionaries.items", nullptr, 0);
		}
		color_names_clear(args->gs->getColorNames());
		color_names_load(args->gs->getColorNames(), args->params);
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	dynv_set_int32(args->params, "color_dictionaries.window.width", width);
	dynv_set_int32(args->params, "color_dictionaries.window.height", height);
	gtk_widget_destroy(dialog);
	dynv_system_release(args->params);
	delete args;
}
