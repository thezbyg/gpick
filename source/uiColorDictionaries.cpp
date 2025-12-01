/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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
#include "uiDialogBase.h"
#include "uiUtilities.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "EventBus.h"
#include "I18N.h"
#include "Names.h"
#include "common/Ref.h"
#include <boost/container/static_vector.hpp>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <gdk/gdkkeysyms.h>
namespace {
enum struct Column : int {
	label = 0,
	enable,
	internal,
	count,
	pointer,
	nColumns
};
struct ColorDictionary: public common::Ref<ColorDictionary>::Counter {
	ColorDictionary(std::string_view label, const Names::InternalDescription *internal, bool enable):
		label(label),
		internal(internal),
		enable(enable) {
	}
	virtual ~ColorDictionary() {
	}
	std::string label;
	const Names::InternalDescription *internal;
	size_t index;
	bool enable;
};
struct ColorDictionariesDialog: public DialogBase {
	GtkWidget *dictionaryList, *fileBrowser;
	std::vector<common::Ref<ColorDictionary>> colorDictionaries;
	ColorDictionariesDialog(GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.color_dictionaries", _("Color dictionaries"), parent) {
		Grid grid(2, 2);
		dictionaryList = newList();
		g_signal_connect(G_OBJECT(dictionaryList), "key_press_event", G_CALLBACK(onKeyPressEvent), this);
		GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
		gtk_container_add(GTK_CONTAINER(scrolled), dictionaryList);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		grid.add(scrolled, true, 2, true);
		grid.addLabel(_("Add color dictionary file"));
		grid.add(fileBrowser = gtk_file_chooser_button_new(_("Color dictionary file"), GTK_FILE_CHOOSER_ACTION_OPEN), true);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileBrowser), options->getString("current_folder", "").c_str());
		g_signal_connect(G_OBJECT(fileBrowser), "file-set", G_CALLBACK(onAddFile), this);
		if (!options->contains("items")) {
			bool first = true;
			for (const auto &description: Names::internalNames()) {
				colorDictionaries.emplace_back(new ColorDictionary(_(description.name), &description, first));
				first = false;
			}
		} else {
			std::unordered_set<std::string> usedInternals;
			for (auto item: options->getMaps("items")) {
				std::string path = item->getString("path", "");
				bool enable = item->getBool("enable", false);
				bool internal = item->getBool("built_in", false);
				if (internal) {
					usedInternals.emplace(path);
					for (const auto &description: Names::internalNames()) {
						if (description.id == path)
							colorDictionaries.emplace_back(new ColorDictionary(_(description.name), &description, enable));
					}
					if (path == "built_in_0") {
						const auto &description = Names::internalNames()[0];
						colorDictionaries.emplace_back(new ColorDictionary(_(description.name), &description, enable));
						usedInternals.emplace(description.id);
					}
				} else {
					colorDictionaries.emplace_back(new ColorDictionary(path, nullptr, enable));
				}
			}
			for (const auto &description: Names::internalNames()) {
				if (usedInternals.find(description.id) == usedInternals.end())
					colorDictionaries.emplace_back(new ColorDictionary(_(description.name), &description, false));
			}
		}
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(dictionaryList));
		for (auto &dictionary: colorDictionaries) {
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			updateRow(model, iter, dictionary);
		}
		setContent(grid);
	}
	virtual void apply(bool preview) override {
		if (preview)
			return;
		std::vector<dynv::Ref> items;
		reorder();
		for (auto &dictionary: colorDictionaries) {
			auto item = dynv::Map::create();
			if (dictionary->internal) {
				item->set("path", dictionary->internal->id);
			} else {
				item->set("path", dictionary->label);
			}
			item->set("enable", dictionary->enable);
			item->set("built_in", dictionary->internal != nullptr);
			items.push_back(item);
		}
		options->set("items", items);
		gs.names().load();
		gs.eventBus().trigger(EventType::colorDictionaryUpdate);
	}
	GtkWidget *newList() {
		GtkWidget *view = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), true);
		GtkListStore *store = gtk_list_store_new(static_cast<int>(Column::nColumns), G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_POINTER);
		GtkTreeViewColumn *col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_resizable(col, true);
		gtk_tree_view_column_set_title(col, _("Path"));
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		gtk_tree_view_column_add_attribute(col, renderer, "text", static_cast<int>(Column::label));
		col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_resizable(col, true);
		gtk_tree_view_column_set_title(col, _("Count"));
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		gtk_tree_view_column_add_attribute(col, renderer, "text", static_cast<int>(Column::count));
		gtk_tree_view_column_add_attribute(col, renderer, "visible", static_cast<int>(Column::internal));
		col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
		gtk_tree_view_column_set_title(col, _("Enable"));
		renderer = gtk_cell_renderer_toggle_new();
		gtk_tree_view_column_pack_start(col, renderer, false);
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		g_signal_connect(renderer, "toggled", G_CALLBACK(onEnableToggled), this);
		gtk_tree_view_column_set_attributes(col, renderer, "active", Column::enable, nullptr);
		gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
		g_object_unref(GTK_TREE_MODEL(store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
		gtk_tree_view_set_reorderable(GTK_TREE_VIEW(view), true);
		return view;
	}
	void reorder() {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(dictionaryList));
		GtkTreeIter iter;
		bool valid = gtk_tree_model_get_iter_first(model, &iter);
		size_t index = 0;
		while (valid) {
			ColorDictionary *colorDictionary;
			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, Column::pointer, &colorDictionary, -1);
			colorDictionary->index = index++;
			valid = gtk_tree_model_iter_next(model, &iter);
		}
		std::sort(colorDictionaries.begin(), colorDictionaries.end(), [](const common::Ref<ColorDictionary> &a, const common::Ref<ColorDictionary> &b) {
			return a->index < b->index;
		});
	}
	void updateRow(GtkTreeModel *model, GtkTreeIter &iter, common::Ref<ColorDictionary> &colorDictionary) {
		bool internal = colorDictionary->internal != nullptr;
		int count = internal ? colorDictionary->internal->count : 0;
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			Column::label, colorDictionary->label.c_str(),
			Column::enable, colorDictionary->enable,
			Column::internal, internal,
			Column::count, count,
			Column::pointer, colorDictionary.pointer(),
			-1);
	}
	static gboolean onKeyPressEvent(GtkWidget *widget, GdkEventKey *event, ColorDictionariesDialog *args) {
		switch (getKeyval(*event, args->gs.latinKeysGroup)) {
		case GDK_KEY_Delete:
			args->removeSelected();
			break;
		default:
			return false;
			break;
		}
		return false;
	}
	void addFile() {
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileBrowser));
		if (filename) {
			auto colorDictionary = common::Ref(new ColorDictionary(filename, nullptr, true));
			colorDictionaries.push_back(colorDictionary);
			g_free(filename);
			GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(dictionaryList));
			GtkTreeIter iter;
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			updateRow(model, iter, colorDictionary);
		}
		gchar *currentFolder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fileBrowser));
		if (currentFolder) {
			options->set("current_folder", currentFolder);
			g_free(currentFolder);
		}
	}
	static void onAddFile(GtkWidget *, ColorDictionariesDialog *args) {
		args->addFile();
	}
	void removeSelected() {
		GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(dictionaryList)));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dictionaryList));
		GtkTreeIter iter;
		GList *selectedList = gtk_tree_selection_get_selected_rows(selection, nullptr);
		std::vector<GtkTreeRowReference *> removeItems;
		GList *i = selectedList;
		while (i) {
			GtkTreeRowReference *rowReference;
			GtkTreePath *path;
			bool referenceInserted = false;
			if ((rowReference = gtk_tree_row_reference_new(GTK_TREE_MODEL(store), (GtkTreePath *)(i->data)))) {
				if ((path = gtk_tree_row_reference_get_path(rowReference))) {
					if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
						ColorDictionary *dictionary = nullptr;
						gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, static_cast<int>(Column::pointer), &dictionary, -1);
						if (!dictionary->internal) {
							removeItems.push_back(rowReference);
							referenceInserted = true;
							colorDictionaries.erase(std::find_if(colorDictionaries.begin(), colorDictionaries.end(), [dictionary](const common::Ref<ColorDictionary> &i) {
								return i.pointer() == dictionary;
							}));
						}
					}
					gtk_tree_path_free(path);
				}
				if (!referenceInserted)
					gtk_tree_row_reference_free(rowReference);
			}
			i = g_list_next(i);
		}
		for (auto i = removeItems.begin(); i != removeItems.end(); i++) {
			GtkTreePath *path = gtk_tree_row_reference_get_path(*i);
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path)) {
				gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			}
			gtk_tree_row_reference_free(*i);
		}
		g_list_foreach(selectedList, (GFunc)gtk_tree_path_free, nullptr);
		g_list_free(selectedList);
	}
	static void onEnableToggled(GtkCellRendererText *cell, gchar *path, ColorDictionariesDialog *args) {
		GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(args->dictionaryList)));
		GtkTreeIter iter;
		gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path);
		ColorDictionary *colorDictionary;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, Column::pointer, &colorDictionary, -1);
		colorDictionary->enable = !colorDictionary->enable;
		gtk_list_store_set(store, &iter, Column::enable, colorDictionary->enable, -1);
	}
};
}
void dialog_color_dictionaries_show(GtkWindow *parent, GlobalState &gs) {
	ColorDictionariesDialog(gs, parent).run();
}
