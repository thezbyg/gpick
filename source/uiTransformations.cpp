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
#include "I18N.h"
#include "transformation/Chain.h"
#include "transformation/Factory.h"
#include "transformation/ColorVisionDeficiency.h"
#include <iostream>
using namespace std;

enum TransformationsColumns {
	TRANSFORMATIONS_HUMAN_NAME = 0,
	TRANSFORMATIONS_TRANSFORMATION_PTR,
	TRANSFORMATIONS_N_COLUMNS
};
enum AvailableTransformationsColumns {
	AVAILABLE_TRANSFORMATIONS_HUMAN_NAME = 0,
	AVAILABLE_TRANSFORMATIONS_NAME,
	AVAILABLE_TRANSFORMATIONS_N_COLUMNS
};
struct TransformationsArgs {
	GtkWidget *available_transformations;
	GtkWidget *transformation_list;
	GtkWidget *config_vbox;
	GtkWidget *vpaned;
	GtkWidget *configuration_label;
	GtkWidget *enabled;
	transformation::Transformation *transformation;
	std::unique_ptr<transformation::IConfiguration> configuration;
	dynv::Ref options;
	GlobalState *gs;
};

static void configure_transformation(TransformationsArgs *args, transformation::Transformation *transformation);

static void tranformations_update_row(GtkTreeModel *model, GtkTreeIter *iter1, transformation::Transformation *transformation, TransformationsArgs *args) {
	gtk_list_store_set(GTK_LIST_STORE(model), iter1,
			TRANSFORMATIONS_HUMAN_NAME, transformation->getReadableName().c_str(),
			TRANSFORMATIONS_TRANSFORMATION_PTR, transformation,
			-1);
}

static void available_tranformations_update_row(GtkTreeModel *model, GtkTreeIter *iter1, transformation::Factory::TypeInfo *type_info, TransformationsArgs *args) {
	gtk_list_store_set(GTK_LIST_STORE(model), iter1,
			AVAILABLE_TRANSFORMATIONS_HUMAN_NAME, type_info->name,
			AVAILABLE_TRANSFORMATIONS_NAME, type_info->id,
			-1);
}

static void available_transformation_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, TransformationsArgs *args) {
	GtkTreeModel* model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	gchar *name = 0;
	gtk_tree_model_get(model, &iter, AVAILABLE_TRANSFORMATIONS_NAME, &name, -1);

	auto transformation = transformation::Factory::create(name);
	if (transformation){
		auto chain = args->gs->getTransformationChain();
		configure_transformation(args, transformation.get());
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->transformation_list));
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		tranformations_update_row(model, &iter, transformation.get(), args);
		chain->add(std::move(transformation));
	}
}

static void add_transformation_cb(GtkWidget *widget, TransformationsArgs *args)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->available_transformations));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->available_transformations));
	GtkTreeIter iter;
	if (gtk_tree_selection_count_selected_rows(selection) == 0){
		return;
	}
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	while (i) {
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data);
		gchar *name = 0;
		gtk_tree_model_get(model, &iter, AVAILABLE_TRANSFORMATIONS_NAME, &name, -1);
		auto transformation = transformation::Factory::create(name);
		if (transformation){
			auto chain = args->gs->getTransformationChain();
			configure_transformation(args, transformation.get());
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->transformation_list));
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			tranformations_update_row(model, &iter, transformation.get(), args);
			chain->add(std::move(transformation));
		}
		i = g_list_next(i);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
}

static void remove_transformation_cb(GtkWidget *widget, TransformationsArgs *args)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->transformation_list));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->transformation_list));
	GtkTreeIter iter;

	if (gtk_tree_selection_count_selected_rows(selection) == 0){
		return;
	}

	configure_transformation(args, nullptr);

	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *ref_list = nullptr;

	GList *i = list;
	while (i) {
		ref_list = g_list_prepend(ref_list, gtk_tree_row_reference_new(model, (GtkTreePath*) (i->data)));
		i = g_list_next(i);
	}

	auto chain = args->gs->getTransformationChain();

	i = ref_list;
	GtkTreePath *path;
	while (i) {
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)i->data);
		if (path) {
			gtk_tree_model_get_iter(model, &iter, path);
			gtk_tree_path_free(path);

			transformation::Transformation *transformation;
			gtk_tree_model_get(model, &iter, TRANSFORMATIONS_TRANSFORMATION_PTR, &transformation, -1);

			chain->remove(transformation);

			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
		}
		i = g_list_next(i);
	}
	g_list_foreach(ref_list, (GFunc)gtk_tree_row_reference_free, nullptr);
	g_list_free(ref_list);

	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
}

static GtkWidget* transformations_list_new(bool selection_list)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	GtkWidget *view;

	view = gtk_tree_view_new();

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), 1);

	if (selection_list){
		store = gtk_list_store_new(AVAILABLE_TRANSFORMATIONS_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	}else{
		store = gtk_list_store_new(TRANSFORMATIONS_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
	}

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, 1);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	if (selection_list){
		gtk_tree_view_column_set_title(col, _("Available filters"));
		gtk_tree_view_column_add_attribute(col, renderer, "text", AVAILABLE_TRANSFORMATIONS_HUMAN_NAME);
	}else{
		gtk_tree_view_column_set_title(col, _("Active filters"));
		gtk_tree_view_column_add_attribute(col, renderer, "text", TRANSFORMATIONS_HUMAN_NAME);
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
	g_object_unref(GTK_TREE_MODEL(store));

	if (!selection_list){
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
		gtk_tree_view_set_reorderable(GTK_TREE_VIEW(view), true);
	}

	return view;
}

static void apply_configuration(TransformationsArgs *args) {
	if (args->configuration && args->transformation) {
		dynv::Map options;
		args->configuration->apply(options);
		args->transformation->deserialize(options);
	}
}

static void configure_transformation(TransformationsArgs *args, transformation::Transformation *transformation)
{
	if (args->configuration){
		gtk_container_remove(GTK_CONTAINER(args->config_vbox), args->configuration->getWidget());
		apply_configuration(args);
		args->configuration.reset();
	}
	if (transformation){
		gtk_label_set_text(GTK_LABEL(args->configuration_label), transformation->getReadableName().c_str());
		args->configuration = transformation->getConfiguration();
		args->transformation = transformation;
		gtk_box_pack_start(GTK_BOX(args->config_vbox), args->configuration->getWidget(), true, true, 0);
	}else{
		gtk_label_set_text(GTK_LABEL(args->configuration_label), _("No filter selected"));
	}
}

static void	transformation_chain_cursor_changed(GtkWidget *widget, TransformationsArgs *args)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->transformation_list));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->transformation_list));
	GtkTreeIter iter;

	if (gtk_tree_selection_count_selected_rows(selection) == 0){
		return;
	}

	configure_transformation(args, nullptr);

	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);

	GList *i = list;
	if (i) {
		gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)i->data);

		transformation::Transformation *transformation;
		gtk_tree_model_get(model, &iter, TRANSFORMATIONS_TRANSFORMATION_PTR, &transformation, -1);
		configure_transformation(args, transformation);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
}

static void transformation_chain_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, TransformationsArgs *args)
{
	GtkTreeModel* model;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	transformation::Transformation *transformation;
	gtk_tree_model_get(model, &iter, TRANSFORMATIONS_TRANSFORMATION_PTR, &transformation, -1);
	configure_transformation(args, transformation);
}


void dialog_transformations_show(GtkWindow* parent, GlobalState* gs)
{
	TransformationsArgs *args = new TransformationsArgs;
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick.transformations");
	args->transformation = 0;

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Display filters"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			nullptr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1), args->options->getInt32("window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	GtkWidget* vbox = gtk_vbox_new(false, 5);
	GtkWidget *widget = args->enabled = gtk_check_button_new_with_mnemonic(_("_Enable display filters"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("enabled", false));
	gtk_box_pack_start(GTK_BOX(vbox), args->enabled, false, false, 0);
	args->vpaned = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), args->vpaned, true, true, 0);

	GtkWidget *hbox = gtk_hbox_new(false, 5);

	GtkWidget *list = args->available_transformations = transformations_list_new(true);
	g_signal_connect(G_OBJECT(list), "row-activated", G_CALLBACK(available_transformation_row_activated), args);
	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(hbox), scrolled, true, true, 0);

	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	vector<transformation::Factory::TypeInfo> types = transformation::Factory::getAllTypes();
	for (size_t i = 0; i != types.size(); i++){
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		available_tranformations_update_row(model, &iter, &types[i], args);
	}

	GtkWidget *vbox3 = gtk_vbox_new(5, true);
	GtkWidget *button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(vbox3), button, false, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(add_transformation_cb), args);
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	gtk_box_pack_start(GTK_BOX(vbox3), button, false, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remove_transformation_cb), args);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, false, false, 0);
	args->transformation_list = list = transformations_list_new(false);
	g_signal_connect(G_OBJECT(list), "row-activated", G_CALLBACK(transformation_chain_row_activated), args);
	g_signal_connect(G_OBJECT(list), "cursor-changed", G_CALLBACK(transformation_chain_cursor_changed), args);
	scrolled = gtk_scrolled_window_new(0, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(hbox), scrolled, true, true, 0);

	gtk_paned_pack1(GTK_PANED(args->vpaned), hbox, false, false);

	GtkWidget *config_wrap_vbox = gtk_vbox_new(false, 5);
	args->configuration_label = gtk_label_new(_("No filter selected"));
	gtk_box_pack_start(GTK_BOX(config_wrap_vbox), gtk_widget_aligned_new(args->configuration_label, 0, 0.5, 0, 0), false, false, 5);

	args->config_vbox = gtk_vbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(config_wrap_vbox), args->config_vbox, true, true, 0);
	gtk_paned_pack2(GTK_PANED(args->vpaned), config_wrap_vbox, false, false);

	auto chain = args->gs->getTransformationChain();
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
	for (auto &transformation: chain->getAll()) {
		GtkTreeIter iter;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter);
		tranformations_update_row(model, &iter, transformation.get(), args);
	}
	gtk_paned_set_position(GTK_PANED(args->vpaned), args->options->getInt32("paned_position", -1));
	gtk_widget_show_all(vbox);
	setDialogContent(dialog, vbox);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		apply_configuration(args);
		GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
		GtkTreeIter iter;
		bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
		bool enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->enabled));
		args->options->set("enabled", enabled);
		chain->setEnabled(enabled);
		size_t count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), nullptr);
		if (count > 0) {
			std::vector<dynv::Ref> configurations(count);
			size_t i = 0;
			while (valid) {
				transformation::Transformation* transformation;
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, TRANSFORMATIONS_TRANSFORMATION_PTR, &transformation, -1);
				auto configuration = dynv::Map::create();
				transformation->serialize(*configuration);
				configurations[i] = configuration;
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
				++i;
			}
			args->options->set("items", configurations);
		} else {
			args->options->remove("items");
		}
	}
	chain->clear();
	chain->setEnabled(args->options->getBool("enabled", false));
	auto configurations = args->options->getMaps("items");
	for (auto &configuration: configurations) {
		if (!configuration)
			continue;
		auto name = configuration->getString("name", "");
		if (name.empty())
			continue;
		auto transformation = transformation::Factory::create(name.c_str());
		if (!transformation)
			continue;
		transformation->deserialize(*configuration);
		chain->add(std::move(transformation));
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("window.width", width);
	args->options->set("window.height", height);
	args->options->set("paned_position", gtk_paned_get_position(GTK_PANED(args->vpaned)));
	gtk_widget_destroy(dialog);
	delete args;
}
