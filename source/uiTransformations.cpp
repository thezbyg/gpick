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

#include "uiTransformations.h"
#include "Converter.h"
#include "uiUtilities.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "EventBus.h"
#include "I18N.h"
#include "transformation/Chain.h"
#include "transformation/Transformation.h"
using namespace transformation;
struct TransformationsArgs: public transformation::IEventHandler {
	static constexpr int idColumn = 0;
	static constexpr int pointerColumn = 0;
	static constexpr int nameColumn = 1;
	GtkWidget *dialog, *available, *added, *paned, *enableToggle, *configurationBox, *configurationLabel;
	Transformation *selectedTransformation;
	std::unique_ptr<BaseConfiguration> configuration;
	dynv::Ref options;
	GlobalState &gs;
	Chain savedChainState;
	TransformationsArgs(GtkWindow *parent, GlobalState &gs, dynv::Ref options):
		selectedTransformation(nullptr),
		options(options),
		gs(gs) {
		dialog = gtk_dialog_new_with_buttons(_("Display filters"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			nullptr);
		gtk_window_set_default_size(GTK_WINDOW(dialog), options->getInt32("window.width", -1), options->getInt32("window.height", -1));
		gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
		GtkWidget *vbox = gtk_vbox_new(false, 5);
		enableToggle = gtk_check_button_new_with_mnemonic(_("_Enable display filters"));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enableToggle), options->getBool("enabled", false));
		g_signal_connect(G_OBJECT(enableToggle), "toggled", G_CALLBACK(onToggle), this);
		gtk_box_pack_start(GTK_BOX(vbox), enableToggle, false, false, 0);
		paned = gtk_vpaned_new();
		gtk_box_pack_start(GTK_BOX(vbox), paned, true, true, 0);
		GtkWidget *hbox = gtk_hbox_new(false, 5);
		gtk_paned_pack1(GTK_PANED(paned), hbox, false, false);
		available = newList(true);
		g_signal_connect(G_OBJECT(available), "row-activated", G_CALLBACK(onAvailableRowActivate), this);
		GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
		gtk_container_add(GTK_CONTAINER(scrolled), available);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(hbox), scrolled, true, true, 0);
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(available));
		for (const auto &description: descriptions()) {
			GtkTreeIter iterator;
			gtk_list_store_append(GTK_LIST_STORE(model), &iterator);
			updateRow(model, iterator, description);
		}
		GtkWidget *buttonVbox = gtk_vbox_new(5, true);
		gtk_box_pack_start(GTK_BOX(hbox), buttonVbox, false, false, 0);
		GtkWidget *button = gtk_button_new_from_stock(GTK_STOCK_ADD);
		gtk_box_pack_start(GTK_BOX(buttonVbox), button, false, false, 0);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(onAdd), this);
		button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
		gtk_box_pack_start(GTK_BOX(buttonVbox), button, false, false, 0);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(onRemove), this);
		added = newList(false);
		g_signal_connect(G_OBJECT(added), "row-activated", G_CALLBACK(onRowActivate), this);
		g_signal_connect(G_OBJECT(added), "cursor-changed", G_CALLBACK(onCursorChange), this);
		scrolled = gtk_scrolled_window_new(0, 0);
		gtk_container_add(GTK_CONTAINER(scrolled), added);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(hbox), scrolled, true, true, 0);

		GtkWidget *configurationVbox = gtk_vbox_new(false, 5);
		gtk_paned_pack2(GTK_PANED(paned), configurationVbox, false, false);
		configurationLabel = gtk_label_new(_("No filter selected"));
		gtk_box_pack_start(GTK_BOX(configurationVbox), gtk_widget_aligned_new(configurationLabel, 0, 0.5, 0, 0), false, false, 5);
		configurationBox = gtk_vbox_new(false, 5);
		gtk_box_pack_start(GTK_BOX(configurationVbox), configurationBox, true, true, 0);
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(added));
		g_signal_connect(G_OBJECT(model), "row-changed", G_CALLBACK(onRowChange), this);
		auto &chain = gs.transformationChain();
		savedChainState = chain;
		for (auto &transformation: chain) {
			GtkTreeIter iterator;
			gtk_list_store_append(GTK_LIST_STORE(model), &iterator);
			updateRow(model, iterator, transformation);
		}
		gtk_paned_set_position(GTK_PANED(paned), options->getInt32("paned_position", -1));
		gtk_widget_show_all(vbox);
		setDialogContent(dialog, vbox);
	}
	virtual ~TransformationsArgs() {
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
		options->set("window.width", width);
		options->set("window.height", height);
		options->set("paned_position", gtk_paned_get_position(GTK_PANED(paned)));
		gtk_widget_destroy(dialog);
	}
	void run() {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
			gs.transformationChain() = std::move(savedChainState);
			gs.eventBus().trigger(EventType::displayFiltersUpdate);
			return;
		}
		finishConfiguration();
		bool enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enableToggle));
		options->set("enabled", enabled);
		GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(added)));
		GtkTreeIter iterator;
		bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iterator);
		size_t count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), nullptr);
		if (count == 0) {
			options->remove("items");
			return;
		}
		std::vector<dynv::Ref> configurations;
		configurations.reserve(count);
		while (valid) {
			Transformation *transformation;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iterator, pointerColumn, &transformation, -1);
			auto configuration = dynv::Map::create();
			transformation->serialize(*configuration);
			configurations.emplace_back(std::move(configuration));
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iterator);
		}
		options->set("items", configurations);
	}
	GtkWidget *newList(bool selectionList) {
		GtkWidget *view = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), 1);
		GtkTreeViewColumn *column = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_resizable(column, 1);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, true);
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
		if (selectionList) {
			gtk_tree_view_column_set_title(column, _("Available filters"));
			gtk_tree_view_column_add_attribute(column, renderer, "text", nameColumn);
		} else {
			gtk_tree_view_column_set_title(column, _("Active filters"));
			gtk_tree_view_column_add_attribute(column, renderer, "text", nameColumn);
		}
		GtkListStore *store = gtk_list_store_new(2, selectionList ? G_TYPE_STRING : G_TYPE_POINTER, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
		g_object_unref(GTK_TREE_MODEL(store));
		if (!selectionList) {
			GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
			gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
			gtk_tree_view_set_reorderable(GTK_TREE_VIEW(view), true);
		}
		return view;
	}
	void finishConfiguration() {
		if (!configuration)
			return;
		gtk_container_remove(GTK_CONTAINER(configurationBox), configuration->widget());
		if (selectedTransformation)
			configuration->apply(*selectedTransformation);
		configuration.reset();
	}
	void configure(Transformation &transformation) {
		finishConfiguration();
		gtk_label_set_text(GTK_LABEL(configurationLabel), transformation.name());
		selectedTransformation = &transformation;
		configuration = transformation.configuration(*this);
		if (configuration)
			gtk_box_pack_start(GTK_BOX(configurationBox), configuration->widget(), true, true, 0);
	}
	void configureNoTransformation() {
		finishConfiguration();
		gtk_label_set_text(GTK_LABEL(configurationLabel), _("No filter selected"));
	}
	void updateRow(GtkTreeModel *model, GtkTreeIter &iterator, const Transformation &transformation) {
		gtk_list_store_set(GTK_LIST_STORE(model), &iterator,
			nameColumn, transformation.name(),
			pointerColumn, &transformation,
			-1);
	}
	void updateRow(GtkTreeModel *model, GtkTreeIter &iterator, const Description &description) {
		gtk_list_store_set(GTK_LIST_STORE(model), &iterator,
			nameColumn, description.name,
			idColumn, description.id,
			-1);
	}
	void add(GtkTreePath *path) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(available));
		GtkTreeIter iterator;
		gtk_tree_model_get_iter(model, &iterator, path);
		const char *id = nullptr;
		gtk_tree_model_get(model, &iterator, idColumn, &id, -1);
		auto transformation = Transformation::create(id);
		if (transformation) {
			configure(*transformation);
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(added));
			gtk_list_store_append(GTK_LIST_STORE(model), &iterator);
			updateRow(model, iterator, *transformation);
			gs.transformationChain().add(std::move(transformation));
			gs.eventBus().trigger(EventType::displayFiltersUpdate);
		}
	}
	void remove(GtkTreePath *path) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(added));
		GtkTreeIter iterator;
		gtk_tree_model_get_iter(model, &iterator, path);
		Transformation *transformation;
		gtk_tree_model_get(model, &iterator, pointerColumn, &transformation, -1);
		gs.transformationChain().remove(*transformation);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iterator);
	}
	static void onAvailableRowActivate(GtkTreeView *, GtkTreePath *path, GtkTreeViewColumn *, TransformationsArgs *args) {
		args->add(path);
	}
	static void onRowActivate(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, TransformationsArgs *args) {
		GtkTreeModel *model = gtk_tree_view_get_model(treeView);
		GtkTreeIter iterator;
		gtk_tree_model_get_iter(model, &iterator, path);
		Transformation *transformation;
		gtk_tree_model_get(model, &iterator, pointerColumn, &transformation, -1);
		args->configure(*transformation);
	}
	static void onAdd(GtkWidget *widget, TransformationsArgs *args) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->available));
		if (gtk_tree_selection_count_selected_rows(selection) == 0)
			return;
		GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
		GList *i = list;
		while (i) {
			args->add((GtkTreePath *)i->data);
			i = g_list_next(i);
		}
		g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
		g_list_free(list);
	}
	static void onRemove(GtkWidget *widget, TransformationsArgs *args) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->added));
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->added));
		if (gtk_tree_selection_count_selected_rows(selection) == 0)
			return;
		args->configureNoTransformation();
		GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
		GList *references = nullptr;
		GList *i = list;
		while (i) {
			references = g_list_prepend(references, gtk_tree_row_reference_new(model, (GtkTreePath *)(i->data)));
			i = g_list_next(i);
		}
		i = references;
		GtkTreePath *path;
		while (i) {
			path = gtk_tree_row_reference_get_path((GtkTreeRowReference *)i->data);
			if (path) {
				args->remove(path);
				gtk_tree_path_free(path);
			}
			i = g_list_next(i);
		}
		g_list_foreach(references, (GFunc)gtk_tree_row_reference_free, nullptr);
		g_list_free(references);
		g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
		g_list_free(list);
		args->gs.eventBus().trigger(EventType::displayFiltersUpdate);
	}
	static void onCursorChange(GtkWidget *widget, TransformationsArgs *args) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->added));
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->added));
		if (gtk_tree_selection_count_selected_rows(selection) == 0)
			return;
		args->configureNoTransformation();
		GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
		GList *i = list;
		if (i) {
			GtkTreeIter iterator;
			gtk_tree_model_get_iter(model, &iterator, (GtkTreePath *)i->data);
			Transformation *transformation;
			gtk_tree_model_get(model, &iterator, pointerColumn, &transformation, -1);
			args->configure(*transformation);
		}
		g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
		g_list_free(list);
	}
	static void onToggle(GtkWidget *widget, TransformationsArgs *args) {
		bool enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
		args->gs.transformationChain().enable(enabled);
		args->gs.eventBus().trigger(EventType::displayFiltersUpdate);
	}
	static void onRowChange(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, TransformationsArgs *args) {
		GtkTreeIter iterator;
		gtk_tree_model_get_iter(model, &iterator, path);
		Transformation *transformation;
		gtk_tree_model_get(model, &iterator, pointerColumn, &transformation, -1);
		auto *indices = gtk_tree_path_get_indices(path);
		args->gs.transformationChain().move(*transformation, indices[0]);
	}
	virtual void onConfigurationChange(BaseConfiguration &configuration, Transformation &transformation) override {
		configuration.apply(transformation);
		if (gs.transformationChain().enabled())
			gs.eventBus().trigger(EventType::displayFiltersUpdate);
	}
};
void dialog_transformations_show(GtkWindow *parent, GlobalState &gs) {
	TransformationsArgs dialog(parent, gs, gs.settings().getOrCreateMap("gpick.transformations"));
	dialog.run();
}
