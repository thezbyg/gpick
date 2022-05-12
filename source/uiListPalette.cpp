/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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

#include "uiListPalette.h"
#include "uiUtilities.h"
#include "gtk/ColorCell.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "IColorSource.h"
#include "GlobalState.h"
#include "Converters.h"
#include "Converter.h"
#include "GlobalState.h"
#include "EventBus.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "common/Format.h"
#include "common/Guard.h"
#include "StandardMenu.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IDroppableColorUI.h"
#include <sstream>
#include <iomanip>
using namespace math;

struct ListPaletteArgs;
static void foreachSelectedItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback);
static void foreachItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback);
static void updateAll(GtkTreeView *treeView, GlobalState &gs);
static void palette_list_entry_fill(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object, ListPaletteArgs* args);
const int ScrollEdgeSize = 15; //SCROLL_EDGE_SIZE from gtktreeview.c
struct ListPaletteArgs : public IEditableColorsUI, public IDroppableColorsUI, public IDraggableColorUI, public IEventHandler {
	GtkWidget *treeview;
	gint scrollTimeout;
	bool allowSelection;
	GtkWidget* countLabel;
	GlobalState &gs;
	bool countUpdateBlocked;
	bool mainPalette;
	ListPaletteArgs(GlobalState &gs, GtkWidget *countLabel, bool mainPalette):
		scrollTimeout(0),
		countLabel(countLabel),
		gs(gs),
		countUpdateBlocked(false),
		mainPalette(mainPalette) {
		gs.eventBus().subscribe(EventType::optionsUpdate, *this);
		gs.eventBus().subscribe(EventType::convertersUpdate, *this);
		gs.eventBus().subscribe(EventType::paletteChanged, *this);
	}
	virtual ~ListPaletteArgs() {
		gs.eventBus().unsubscribe(*this);
	}
	virtual void addToPalette(const ColorObject &) override {
		common::Guard colorListGuard(color_list_start_changes(gs.getColorList()), color_list_end_changes, gs.getColorList());
		foreachSelectedItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			color_list_add_color_object(gs.getColorList(), colorObject, true);
			return true;
		});
	}
	virtual void addAllToPalette() override {
		common::Guard colorListGuard(color_list_start_changes(gs.getColorList()), color_list_end_changes, gs.getColorList());
		foreachItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			color_list_add_color_object(gs.getColorList(), colorObject, true);
			return true;
		});
	}
	virtual const ColorObject &getColor() override {
		foreachSelectedItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			this->colorObject = *colorObject;
			return false;
		});
		return colorObject;
	}
	virtual bool hasColor() override {
		bool result = false;
		foreachItem(GTK_TREE_VIEW(treeview), [&result](ColorObject *) {
			result = true;
			return false;
		});
		return result;
	}
	virtual bool hasSelectedColor() override {
		bool result = false;
		foreachSelectedItem(GTK_TREE_VIEW(treeview), [&result](ColorObject *) {
			result = true;
			return false;
		});
		return result;
	}
	virtual bool isEditable() override {
		return false;
	}
	virtual void setColor(const ColorObject &colorObject) override {
	}
	virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
	}
	virtual void setColorAt(const ColorObject &colorObject, int x, int y) override {
		setColorsAt(std::vector<ColorObject>{ colorObject }, x, y);
	}
	virtual void setColorsAt(const std::vector<ColorObject> &colorObjects, int x, int y) override {
		color_list_reset_selected(gs.getColorList());
		removeScrollTimeout();
		auto model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		std::optional<GtkTreeIter> insertIterator;
		GtkTreePath* path;
		GtkTreeViewDropPosition pos;
		if (getPathAt(GTK_TREE_VIEW(treeview), x, y, path, pos)) {
			GtkTreeIter iter;
			gtk_tree_model_get_iter(model, &iter, path);
			gtk_tree_path_free(path);
			insertIterator = iter;
		}
		common::Guard colorListGuard(color_list_start_changes(gs.getColorList()), color_list_end_changes, gs.getColorList());
		ColorObject *colorObject = nullptr;
		for (size_t i = 0, count = colorObjects.size(); i < count; i++) {
			if (insertIterator) {
				if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
					colorObject = colorObjects[i].copy();
				} else if (pos == GTK_TREE_VIEW_DROP_AFTER || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
					colorObject = colorObjects[count - i - 1].copy();
				}
				colorObject->setSelected(true);
				GtkTreeIter iter;
				if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
					gtk_list_store_insert_before(GTK_LIST_STORE(model), &iter, &*insertIterator);
				} else {
					gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter, &*insertIterator);
				}
				palette_list_entry_fill(GTK_LIST_STORE(model), &iter, colorObject, this);
				color_list_add_color_object(gs.getColorList(), colorObject, false);
			} else {
				colorObject = colorObjects[i].copy();
				colorObject->setSelected(true);
				color_list_add_color_object(gs.getColorList(), colorObject, true);
			}
			colorObject->release();
		}
	}
	virtual std::vector<ColorObject> getColors(bool selected) override {
		std::vector<ColorObject> result;
		color_list_reset_all(gs.getColorList());
		if (selected)
			foreachSelectedItem(GTK_TREE_VIEW(treeview), [&result](ColorObject *colorObject) {
				result.push_back(*colorObject);
				colorObject->setVisited(true);
				return true;
			});
		else
			foreachItem(GTK_TREE_VIEW(treeview), [&result](ColorObject *colorObject) {
				result.push_back(*colorObject);
				colorObject->setVisited(true);
				return true;
			});
		return result;
	}
	static gboolean scrollRowTimeout(ListPaletteArgs *args){
		GdkRectangle visibleRect;
		gint y, offset, dy;
		gdk_window_get_pointer(gtk_tree_view_get_bin_window(GTK_TREE_VIEW(args->treeview)), nullptr, &y, nullptr);
		gtk_tree_view_convert_bin_window_to_tree_coords(GTK_TREE_VIEW(args->treeview), 0, 0, 0, &dy);
		y += dy;
		gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(args->treeview), &visibleRect);
		offset = y - (visibleRect.y + 2 * ScrollEdgeSize);
		if (offset > 0) {
			offset = y - (visibleRect.y + visibleRect.height - 2 * ScrollEdgeSize);
			if (offset < 0)
				return true;
		}
		GtkAdjustment *adjustment = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(args->treeview));
		gtk_adjustment_set_value(adjustment, min(max(gtk_adjustment_get_value(adjustment) + offset, 0.0), gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size (adjustment)));
		return true;
	}
	static bool getPathAt(GtkTreeView *treeView, int x, int y, GtkTreePath *&path, GtkTreeViewDropPosition &position) {
		if (gtk_tree_view_get_dest_row_at_pos(treeView, x, y, &path, &position)) {
			GdkModifierType mask;
			gdk_window_get_pointer(gtk_tree_view_get_bin_window(treeView), nullptr, nullptr, &mask);
			if ((mask & GDK_CONTROL_MASK) && (position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER || position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)) {
				position = GTK_TREE_VIEW_DROP_INTO_OR_BEFORE;
			} else if (position == GTK_TREE_VIEW_DROP_BEFORE || position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
				position = GTK_TREE_VIEW_DROP_BEFORE;
			} else {
				position = GTK_TREE_VIEW_DROP_AFTER;
			}
			return true;
		}else{
			int tx, ty;
			gtk_tree_view_convert_widget_to_tree_coords(treeView, x, y, &tx, &ty);
			GtkTreeModel *model = gtk_tree_view_get_model(treeView);
			GtkTreeIter last, iter;
			if (!gtk_tree_model_get_iter_first(model, &iter)) {
				position = GTK_TREE_VIEW_DROP_AFTER;
				return false;
			}
			if (ty >= 0) {
				do {
					last = iter;
				} while (gtk_tree_model_iter_next(model, &iter));
				position = GTK_TREE_VIEW_DROP_AFTER;
				iter = last;
			} else {
				position = GTK_TREE_VIEW_DROP_BEFORE;
			}
			path = gtk_tree_model_get_path(model, &iter);
			return true;
		}
	}
	virtual bool testDropAt(int x, int y) override {
		GtkTreePath* path;
		GtkTreeViewDropPosition pos;
		if (getPathAt(GTK_TREE_VIEW(treeview), x, y, path, pos)) {
			gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(treeview), path, pos);
			gtk_tree_path_free(path);
		} else {
			gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(treeview), nullptr, pos);
		}
		if (!scrollTimeout) {
			scrollTimeout = gdk_threads_add_timeout(150, GSourceFunc(scrollRowTimeout), this);
		}
		return true;
	}
	void removeScrollTimeout() {
		if (scrollTimeout) {
			g_source_remove(scrollTimeout);
			scrollTimeout = 0;
		}
	}
	virtual void dropEnd(bool move) override {
		countUpdateBlocked = true;
		removeScrollTimeout();
		auto *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_set_select_function(selection, nullptr, nullptr, nullptr);
		gtk_tree_selection_unselect_all(selection);
		auto model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		GtkTreeIter iter;
		bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
		bool first = true;
		while (valid) {
			ColorObject *colorObject;
			gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
			if (colorObject->isSelected()) {
				gtk_tree_selection_select_iter(selection, &iter);
				if (first) {
					first = false;
					auto path = gtk_tree_model_get_path(model, &iter);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(treeview), path, nullptr, false);
					gtk_tree_path_free(path);
				}
			}
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
		}
		if (move) {
			GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));;
			GtkTreeIter iter;
			gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
			ColorObject* colorObject;
			while (valid) {
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &colorObject, -1);
				if (colorObject->isVisited()) {
					valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
					colorObject->release();
				}else{
					valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
				}
			}
			color_list_remove_visited(gs.getColorList());
			color_list_reset_selected(gs.getColorList());
			if (mainPalette)
				gs.eventBus().trigger(EventType::paletteChanged);
		} else {
			color_list_reset_all(gs.getColorList());
			if (mainPalette)
				gs.eventBus().trigger(EventType::paletteChanged);
		}
		countUpdateBlocked = false;
		updateCounts();
	}
	virtual void dragEnd(bool move) override {
		removeScrollTimeout();
		updateCounts();
	}
	struct SelectionBoundsArgs {
		int minIndex;
		int maxIndex;
		int lastIndex;
		bool discontinuous;
	};
	static void findSelectionBounds(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data){
		gint *indices = gtk_tree_path_get_indices(path);
		SelectionBoundsArgs *args = (SelectionBoundsArgs *) data;
		int index = indices[0]; // currently indices are all 1d.
		int diff = index - args->lastIndex;
		if ((args->lastIndex != 0x7fffffff) && (diff != 1) && (diff != -1)){
			args->discontinuous = true;
		}
		if (index > args->maxIndex){
			args->maxIndex = index;
		}
		if (index < args->minIndex){
			args->minIndex = index;
		}
		args->lastIndex = index;
	}
	void updateCounts() {
		if (countUpdateBlocked)
			return;
		std::stringstream ss;
		GtkTreeSelection *sel;
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
		SelectionBoundsArgs bounds;
		int selected_count;
		int total_colors;
		if (!countLabel){
			return;
		}
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		selected_count = gtk_tree_selection_count_selected_rows(sel);
		total_colors = gtk_tree_model_iter_n_children(model, nullptr);
		bounds.discontinuous = false;
		bounds.minIndex = 0x7fffffff;
		bounds.lastIndex = 0x7fffffff;
		bounds.maxIndex = 0;
		if (selected_count > 0){
			ss.str("");
			ss << "#";
			gtk_tree_selection_selected_foreach(sel, findSelectionBounds, &bounds);
			if (bounds.minIndex < bounds.maxIndex){
				ss << bounds.minIndex;
				if (bounds.discontinuous){
					ss << "..";
				}else{
					ss << "-";
				}
				ss << bounds.maxIndex;
			}else{
				ss << bounds.minIndex;
			}
#ifdef ENABLE_NLS
			ss << " (" << common::format(ngettext("{} color", "{} colors", selected_count), selected_count) << ")";
#else
			ss << " (" << ((selected_count == 1) ? "color" : "colors") << ")";
#endif
			ss << " " << _("selected") << ". ";
		}
#ifdef ENABLE_NLS
		ss << common::format(ngettext("Total {} color", "Total {} colors", total_colors), total_colors);
#else
		ss << "Total " << total_colors << ((total_colors == 1) ? " color" : " colors");
#endif
		auto message = ss.str();
		gtk_label_set_text(GTK_LABEL(countLabel), message.c_str());
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
			updateAll(GTK_TREE_VIEW(treeview), gs);
			break;
		case EventType::displayFiltersUpdate:
		case EventType::colorDictionaryUpdate:
		case EventType::paletteChanged:
			break;
		}
	}
	static gboolean controlSelectionCallback(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, ListPaletteArgs *args) {
		return args->allowSelection;
	}
	void controlSelection(bool allow) {
		auto selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		allowSelection = allow;
		gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)controlSelectionCallback, this, nullptr);
	}
	void onChange() {
		if (mainPalette)
			gs.eventBus().trigger(EventType::paletteChanged);
	}
	ColorObject colorObject;
};
static void palette_list_entry_fill(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object, ListPaletteArgs* args)
{
	std::string text = args->gs.converters().serialize(color_object, Converters::Type::colorList);
	gtk_list_store_set(store, iter, 0, color_object->reference(), 1, text.c_str(), 2, color_object->getName().c_str(), -1);
}
static void palette_list_entry_update_row(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object, ListPaletteArgs* args)
{
	std::string text = args->gs.converters().serialize(color_object, Converters::Type::colorList);
	gtk_list_store_set(store, iter, 1, text.c_str(), 2, color_object->getName().c_str(), -1);
}
static void palette_list_entry_update_name(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object)
{
	gtk_list_store_set(store, iter, 2, color_object->getName().c_str(), -1);
}
static void palette_list_cell_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer userData)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(userData);
	ListPaletteArgs *args = reinterpret_cast<ListPaletteArgs *>(g_object_get_data(G_OBJECT(model), "arguments"));
	gtk_tree_model_get_iter_from_string(model, &iter, path );
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		2, new_text,
	-1);
	ColorObject *color_object;
	gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	color_object->setName(new_text);
	args->onChange();
}
static void palette_list_row_activated(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, gpointer userData)
{
	ListPaletteArgs* args = (ListPaletteArgs*)userData;
	GtkTreeModel* model = gtk_tree_view_get_model(treeView);;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	ColorObject *colorObject;
	gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
	auto *colorSource = args->gs.getCurrentColorSource();
	if (colorSource != nullptr)
		colorSource->setColor(*colorObject);
	args->updateCounts();
	args->onChange();
}

static int palette_list_preview_on_insert(ColorList* color_list, ColorObject* color_object, void *userdata) {
	palette_list_add_entry(GTK_WIDGET(userdata), color_object, false);
	return 0;
}

static int palette_list_preview_on_clear(ColorList* color_list, void *userdata) {
	palette_list_remove_all_entries(GTK_WIDGET(userdata), false);
	return 0;
}

static void destroy_cb(GtkWidget* widget, ListPaletteArgs *args){
	args->removeScrollTimeout();
	palette_list_remove_all_entries(widget, false);
}

GtkWidget* palette_list_get_widget(ColorList *color_list){
	return (GtkWidget*)color_list->userdata;
}

static void onPreviewActivate(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, ListPaletteArgs *args) {
	auto model = gtk_tree_view_get_model(treeView);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	ColorObject *colorObject;
	gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
	if (colorObject)
		color_list_add_color_object(args->gs.getColorList(), colorObject, true);
}
static void foreachItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback) {
	auto model = gtk_tree_view_get_model(treeView);
	GtkTreeIter iter;
	auto valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		if (!callback(colorObject))
			break;
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}
static void updateAll(GtkTreeView *treeView, GlobalState &gs) {
	auto model = gtk_tree_view_get_model(treeView);
	GtkTreeIter iter;
	auto valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid) {
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		std::string text = gs.converters().serialize(colorObject, Converters::Type::colorList);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, text.c_str(), -1);
		valid = gtk_tree_model_iter_next(model, &iter);
	}
}
static void foreachSelectedItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback) {
	auto model = gtk_tree_view_get_model(treeView);
	auto selection = gtk_tree_view_get_selection(treeView);
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	while (i) {
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, reinterpret_cast<GtkTreePath *>(i->data));
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		if (!callback(colorObject))
			break;
		i = g_list_next(i);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
}
#if GTK_MAJOR_VERSION >= 3
static void onExpanderStateChange(GtkExpander *expander, GParamSpec *, ListPaletteArgs *args) {
	bool expanded = gtk_expander_get_expanded(expander);
	gtk_widget_set_hexpand(args->treeview, expanded);
	gtk_widget_set_vexpand(args->treeview, expanded);
}
#endif
static gboolean onPreviewButtonPress(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	if (event->button != 1)
		return false;
	if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
		return false;
	args->controlSelection(true);
	GtkTreePath *path = nullptr;
	if (!gtk_tree_view_get_path_at_pos(treeView, static_cast<int>(event->x), static_cast<int>(event->y), &path, nullptr, nullptr, nullptr))
		return false;
	auto selection = gtk_tree_view_get_selection(treeView);
	if (gtk_tree_selection_path_is_selected(selection, path))
		args->controlSelection(false);
	if (path)
		gtk_tree_path_free(path);
	return false;
}
static gboolean onPreviewButtonRelease(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	if (event->button == 1 && !args->allowSelection) {
		args->controlSelection(true);
	}
	return false;
}
static void destroy_arguments(gpointer data);
GtkWidget* palette_list_preview_new(GlobalState* gs, bool expander, bool expanded, ColorList* color_list, ColorList** out_color_list) {
	auto args = new ListPaletteArgs(*gs, nullptr, false);
	auto view = args->treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), 0);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), true);
	auto store = gtk_list_store_new(3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
	auto col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(col, 0);
	auto renderer = custom_cell_renderer_color_new();
	custom_cell_renderer_color_set_size(renderer, 16, 16);
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "color", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), false);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
	g_object_unref(GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect(G_OBJECT(view), "button-press-event", G_CALLBACK(onPreviewButtonPress), args);
	g_signal_connect(G_OBJECT(view), "button-release-event", G_CALLBACK(onPreviewButtonRelease), args);
	g_signal_connect(G_OBJECT(view), "row-activated", G_CALLBACK(onPreviewActivate), args);
	StandardEventHandler::forWidget(view, &args->gs, args, StandardEventHandler::Options().afterEvents(false));
	StandardDragDropHandler::forWidget(view, &args->gs, args, StandardDragDropHandler::Options().allowDrop(false).converterType(Converters::Type::colorList));

	if (out_color_list) {
		ColorList *colorList = color_list_new();
		colorList->onInsert = palette_list_preview_on_insert;
		colorList->onClear = palette_list_preview_on_clear;
		colorList->userdata = view;
		*out_color_list = colorList;
	}

	GtkWidget *scrolledWindow = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), view);
	GtkWidget *mainWidget = scrolledWindow;
	if (expander){
		GtkWidget *expander = gtk_expander_new(_("Preview"));
		gtk_container_add(GTK_CONTAINER(expander), scrolledWindow);
#if GTK_MAJOR_VERSION >= 3
		g_signal_connect(expander, "notify::expanded", G_CALLBACK(onExpanderStateChange), args);
#endif
		gtk_expander_set_expanded(GTK_EXPANDER(expander), expanded);
		mainWidget = expander;
	}
	g_object_set_data_full(G_OBJECT(view), "arguments", args, destroy_arguments);
	g_signal_connect(G_OBJECT(view), "destroy", G_CALLBACK(destroy_cb), args);
	return mainWidget;
}
static void destroy_arguments(gpointer data){
	ListPaletteArgs* args = (ListPaletteArgs*)data;
	delete args;
}
static gboolean on_palette_button_press(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	if (event->button == 1 && !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))) {
		args->controlSelection(true);
		GtkTreePath *path = nullptr;
		if (!gtk_tree_view_get_path_at_pos(treeView, static_cast<int>(event->x), static_cast<int>(event->y), &path, nullptr, nullptr, nullptr))
			return false;
		auto selection = gtk_tree_view_get_selection(treeView);
		if (gtk_tree_selection_path_is_selected(selection, path)) {
			args->controlSelection(false);
		}
		if (path)
			gtk_tree_path_free(path);
	}
	args->updateCounts();
	return false;
}

static gboolean on_palette_button_release(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	if (event->button == 1 && !args->allowSelection) {
		args->controlSelection(true);
	}
	args->updateCounts();
	return false;
}

static void on_palette_cursor_changed(GtkTreeView *treeview, ListPaletteArgs *args){
	args->updateCounts();
}

static gboolean on_palette_select_all(GtkTreeView *treeview, ListPaletteArgs *args){
	args->updateCounts();
	return false;
}

static gboolean on_palette_unselect_all(GtkTreeView *treeview, ListPaletteArgs *args){
	args->updateCounts();
	return false;
}

GtkWidget* palette_list_new(GlobalState* gs, GtkWidget* countLabel) {
	auto args = new ListPaletteArgs(*gs, countLabel, true);
	auto view = args->treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), true);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), true);
	auto store = gtk_list_store_new (3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
	g_object_set_data_full(G_OBJECT(store), "arguments", args, nullptr);
	auto col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, _("Color"));
	auto renderer = custom_cell_renderer_color_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "color", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, _("Color"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, _("Name"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_object_set(renderer, "editable", TRUE, nullptr);
	g_signal_connect(renderer, "edited", (GCallback) palette_list_cell_edited, store);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), false);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
	g_object_unref(GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect(G_OBJECT(view), "row-activated", G_CALLBACK(palette_list_row_activated), args);
	g_signal_connect(G_OBJECT(view), "button-press-event", G_CALLBACK(on_palette_button_press), args);
	g_signal_connect(G_OBJECT(view), "button-release-event", G_CALLBACK(on_palette_button_release), args);
	g_signal_connect(G_OBJECT(view), "cursor-changed", G_CALLBACK(on_palette_cursor_changed), args);
	g_signal_connect_after(G_OBJECT(view), "select-all", G_CALLBACK(on_palette_select_all), args);
	g_signal_connect_after(G_OBJECT(view), "unselect-all", G_CALLBACK(on_palette_unselect_all), args);

	///gtk_tree_view_set_reorderable(GTK_TREE_VIEW (view), TRUE);
	gtk_drag_dest_set( view, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));
	gtk_drag_source_set( view, GDK_BUTTON1_MASK, 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));

	g_object_set_data_full(G_OBJECT(view), "arguments", args, destroy_arguments);
	g_signal_connect(G_OBJECT(view), "destroy", G_CALLBACK(destroy_cb), args);
	StandardDragDropHandler::forWidget(view, &args->gs, args, StandardDragDropHandler::Options().supportsMove(true).converterType(Converters::Type::colorList));

	if (countLabel){
		args->updateCounts();
	}
	return view;
}

void palette_list_remove_all_entries(GtkWidget* widget, bool allowUpdate) {
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	while (valid){
		ColorObject* c;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &c, -1);
		c->release();
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	gtk_list_store_clear(GTK_LIST_STORE(store));
	if (allowUpdate) {
		args->updateCounts();
		args->onChange();
	}
}

gint32 palette_list_get_selected_count(GtkWidget* widget) {
	return gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
}

gint32 palette_list_get_count(GtkWidget* widget){
	GtkTreeModel *store;
	store=gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	return gtk_tree_model_iter_n_children(store, nullptr);
}

gint32 palette_list_get_selected_color(GtkWidget* widget, Color* color)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(widget) );
	GtkListStore *store;
	GtkTreeIter iter;
	if (gtk_tree_selection_count_selected_rows(selection) != 1){
		return -1;
	}
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GList *list = gtk_tree_selection_get_selected_rows ( selection, 0 );
	GList *i = list;
	if (i){
		ColorObject* color_object;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*)i->data);
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		*color = color_object->getColor();
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	return 0;
}

void palette_list_remove_selected_entries(GtkWidget* widget, bool allowUpdate)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	ColorObject* color_object;
	while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		if (color_object->isSelected()){
			valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			color_object->release();
		}else{
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
		}
	}
	if (allowUpdate) {
		args->updateCounts();
		args->onChange();
	}
}

void palette_list_add_entry(GtkWidget* widget, ColorObject* color_object, bool allowUpdate)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter1;
	GtkListStore *store;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	gtk_list_store_append(store, &iter1);
	palette_list_entry_fill(store, &iter1, color_object, args);
	if (allowUpdate) {
		args->updateCounts();
		args->onChange();
	}
}
int palette_list_remove_entry(GtkWidget* widget, ColorObject* r_color_object, bool allowUpdate)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	ColorObject* color_object;
	while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		if (color_object == r_color_object){
			valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			color_object->release();
			if (allowUpdate) {
				args->updateCounts();
				args->onChange();
			}
			return 0;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	return -1;
}
template<typename Callback>
struct ExecuteContext {
	ListPaletteArgs *args;
	Callback callback;
	void *userdata;
	bool changed;
	ExecuteContext(ListPaletteArgs *args, Callback callback, void *userdata):
		args(args),
		callback(callback),
		userdata(userdata),
		changed(false) {
	}
};
static void executeCallback(GtkListStore *store, GtkTreeIter *iter, ExecuteContext<PaletteListCallback> &executeContext) {
	ColorObject *colorObject;
	gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &colorObject, -1);
	switch (executeContext.callback(colorObject, executeContext.userdata)) {
	case PaletteListCallbackResult::updateName:
		palette_list_entry_update_name(store, iter, colorObject);
		executeContext.changed = true;
		break;
	case PaletteListCallbackResult::updateRow:
		palette_list_entry_update_row(store, iter, colorObject, executeContext.args);
		executeContext.changed = true;
		break;
	case PaletteListCallbackResult::noUpdate:
		break;
	}
}
static void executeReplaceCallback(GtkListStore *store, GtkTreeIter *iter, ExecuteContext<PaletteListReplaceCallback> &executeContext) {
	ColorObject *colorObject, *originalColorObject;
	gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &colorObject, -1);
	originalColorObject = colorObject->reference();
	auto result = executeContext.callback(&colorObject, executeContext.userdata);
	if (colorObject != originalColorObject){
		gtk_list_store_set(store, iter, 0, colorObject, -1);
		executeContext.changed = true;
	}
	switch (result) {
	case PaletteListCallbackResult::updateName:
		palette_list_entry_update_name(store, iter, colorObject);
		executeContext.changed = true;
		break;
	case PaletteListCallbackResult::updateRow:
		palette_list_entry_update_row(store, iter, colorObject, executeContext.args);
		executeContext.changed = true;
		break;
	case PaletteListCallbackResult::noUpdate:
		break;
	}
	originalColorObject->release();
}
void palette_list_foreach(GtkWidget *widget, PaletteListCallback callback, void *userdata, bool allowUpdate) {
	ListPaletteArgs *args = (ListPaletteArgs *)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GtkTreeIter iter;
	bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	ExecuteContext executeContext(args, callback, userdata);
	while (valid) {
		executeCallback(store, &iter, executeContext);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	if (executeContext.changed && allowUpdate)
		args->onChange();
}
void palette_list_foreach_selected(GtkWidget *widget, PaletteListCallback callback, void *userdata, bool allowUpdate) {
	ListPaletteArgs *args = (ListPaletteArgs *)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GtkTreeIter iter;
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	ExecuteContext executeContext(args, callback, userdata);
	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		executeCallback(store, &iter, executeContext);
		i = g_list_next(i);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	if (executeContext.changed && allowUpdate)
		args->onChange();
}
void palette_list_foreach_selected(GtkWidget *widget, PaletteListReplaceCallback callback, void *userdata, bool allowUpdate) {
	ListPaletteArgs *args = (ListPaletteArgs *)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GtkTreeIter iter;
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	ExecuteContext executeContext(args, callback, userdata);
	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		executeReplaceCallback(store, &iter, executeContext);
		i = g_list_next(i);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	if (executeContext.changed && allowUpdate)
		args->onChange();
}
void palette_list_forfirst_selected(GtkWidget* widget, PaletteListCallback callback, void *userdata, bool allowUpdate) {
	ListPaletteArgs *args = (ListPaletteArgs *)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GtkTreeIter iter;
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	ExecuteContext executeContext(args, callback, userdata);
	if (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		executeCallback(store, &iter, executeContext);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	if (executeContext.changed && allowUpdate)
		args->onChange();
}
ColorObject *palette_list_get_first_selected(GtkWidget* widget)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	ColorObject *color_object = nullptr;
	if (i){
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, reinterpret_cast<GtkTreePath*>(i->data));
		gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	return color_object;
}
void palette_list_update_first_selected(GtkWidget* widget, bool only_name)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	ColorObject *color_object = nullptr;
	if (i){
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, reinterpret_cast<GtkTreePath*>(i->data));
		gtk_tree_model_get(model, &iter, 0, &color_object, -1);
		if (only_name)
			palette_list_entry_update_name(GTK_LIST_STORE(model), &iter, color_object);
		else
			palette_list_entry_update_row(GTK_LIST_STORE(model), &iter, color_object, args);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	args->onChange();
}
void palette_list_append_copy_menu(GtkWidget* widget, GtkWidget *menu) {
	auto args = reinterpret_cast<ListPaletteArgs *>(g_object_get_data(G_OBJECT(widget), "arguments"));
	StandardMenu::appendMenu(menu, args, &args->gs);
}
void palette_list_after_update(GtkWidget* widget) {
	auto args = reinterpret_cast<ListPaletteArgs *>(g_object_get_data(G_OBJECT(widget), "arguments"));
	args->updateCounts();
	args->onChange();
}
