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
#include "uiDialogEdit.h"
#include "uiColorInput.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "common/Format.h"
#include "common/Guard.h"
#include "StandardMenu.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IDroppableColorUI.h"
#include "IContainerUI.h"
#include "IPalette.h"
#include <sstream>
#include <iomanip>
using namespace math;
namespace {
enum struct Type {
	main,
	preview,
	temporary,
};
struct ListPaletteArgs;
static void foreachSelectedItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback);
static void foreachItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback);
static void updateSelected(GtkTreeView *treeView, GlobalState &gs);
static void updateAll(GtkTreeView *treeView, GlobalState &gs);
static void set(GtkListStore* store, GtkTreeIter *iter, ColorObject* colorObject, ListPaletteArgs* args);
const int scrollEdgeSize = 15; //SCROLL_EDGE_SIZE from gtktreeview.c
struct ListPaletteArgs : public IEditableColorsUI, public IContainerUI, public IDroppableColorsUI, public IDraggableColorUI, public IEventHandler, public IPalette {
	GtkWidget *treeview;
	gint scrollTimeout;
	bool allowSelection;
	GtkWidget* countLabel;
	GlobalState &gs;
	bool countUpdateBlocked;
	Type type;
	ColorList &colorList;
	ListPaletteArgs(GlobalState &gs, GtkWidget *countLabel, Type type, ColorList &colorList):
		scrollTimeout(0),
		countLabel(countLabel),
		gs(gs),
		countUpdateBlocked(false),
		type(type),
		colorList(colorList) {
		gs.eventBus().subscribe(EventType::optionsUpdate, *this);
		gs.eventBus().subscribe(EventType::convertersUpdate, *this);
		gs.eventBus().subscribe(EventType::paletteChanged, *this);
		if (type == Type::main || type == Type::temporary)
			buildPalette();
		else if (type == Type::preview)
			buildPreviewPalette();
	}
	ListPaletteArgs(GlobalState &gs, GtkWidget *countLabel, Type type):
		scrollTimeout(0),
		countLabel(countLabel),
		gs(gs),
		countUpdateBlocked(false),
		type(type),
		colorList(gs.initializeColorList(*this)) {
		gs.eventBus().subscribe(EventType::optionsUpdate, *this);
		gs.eventBus().subscribe(EventType::convertersUpdate, *this);
		gs.eventBus().subscribe(EventType::paletteChanged, *this);
		if (type == Type::main || type == Type::temporary)
			buildPalette();
		else if (type == Type::preview)
			buildPreviewPalette();
	}
	virtual ~ListPaletteArgs() {
		gs.eventBus().unsubscribe(*this);
	}
	void buildPalette() {
		treeview = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), true);
		gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), true);
		auto store = gtk_list_store_new (3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
		g_object_set_data_full(G_OBJECT(store), "arguments", this, nullptr);
		auto col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable(col,1);
		gtk_tree_view_column_set_title(col, _("Color"));
		auto renderer = custom_cell_renderer_color_new();
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_column_add_attribute(col, renderer, "color", 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

		col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable(col,1);
		gtk_tree_view_column_set_title(col, _("Color"));
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

		col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable(col,1);
		gtk_tree_view_column_set_title(col, _("Name"));
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_column_add_attribute(col, renderer, "text", 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);
		g_object_set(renderer, "editable", TRUE, nullptr);
		g_signal_connect(renderer, "edited", G_CALLBACK(onCellEdited), store);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), false);
		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
		g_object_unref(GTK_TREE_MODEL(store));

		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
		g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(onRowActivated), this);
		g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(onButtonPress), this);
		g_signal_connect(G_OBJECT(treeview), "button-release-event", G_CALLBACK(onButtonRelease), this);
		g_signal_connect(G_OBJECT(treeview), "cursor-changed", G_CALLBACK(onCursorChanged), this);
		g_signal_connect_after(G_OBJECT(treeview), "select-all", G_CALLBACK(onSelectAll), this);
		g_signal_connect_after(G_OBJECT(treeview), "unselect-all", G_CALLBACK(onUnselectAll), this);
		gtk_drag_dest_set(treeview, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));
		gtk_drag_source_set(treeview, GDK_BUTTON1_MASK, 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));
		g_object_set_data_full(G_OBJECT(treeview), "arguments", this, reinterpret_cast<GDestroyNotify>(onDestroyArgs));
		g_signal_connect(G_OBJECT(treeview), "destroy", G_CALLBACK(onDestroy), this);
		if (type != Type::main)
			StandardEventHandler::forWidget(treeview, &gs, this, StandardEventHandler::Options().afterEvents(false));
		StandardDragDropHandler::forWidget(treeview, &gs, this, StandardDragDropHandler::Options().supportsMove(true).converterType(Converters::Type::colorList));
		if (countLabel){
			updateCounts();
		}
	}
	void buildPreviewPalette() {
		treeview = gtk_tree_view_new();
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), 0);
		gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), true);
		auto store = gtk_list_store_new(3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
		auto col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_resizable(col, 0);
		auto renderer = custom_cell_renderer_color_new();
		custom_cell_renderer_color_set_size(renderer, 16, 16);
		gtk_tree_view_column_pack_start(col, renderer, true);
		gtk_tree_view_column_add_attribute(col, renderer, "color", 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), false);
		gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
		g_object_unref(GTK_TREE_MODEL(store));

		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
		g_signal_connect(G_OBJECT(treeview), "button-press-event", G_CALLBACK(onPreviewButtonPress), this);
		g_signal_connect(G_OBJECT(treeview), "button-release-event", G_CALLBACK(onPreviewButtonRelease), this);
		g_signal_connect(G_OBJECT(treeview), "row-activated", G_CALLBACK(onPreviewActivate), this);
		g_object_set_data_full(G_OBJECT(treeview), "arguments", this, reinterpret_cast<GDestroyNotify>(onDestroyArgs));
		g_signal_connect(G_OBJECT(treeview), "destroy", G_CALLBACK(onDestroy), this);
		StandardEventHandler::forWidget(treeview, &gs, this, StandardEventHandler::Options().afterEvents(false));
		StandardDragDropHandler::forWidget(treeview, &gs, this, StandardDragDropHandler::Options().allowDrop(false).converterType(Converters::Type::colorList));
	}
	virtual void addToPalette(const ColorObject &) override {
		common::Guard colorListGuard = gs.colorList().changeGuard();
		foreachSelectedItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			gs.colorList().add(colorObject, true);
			return true;
		});
	}
	virtual void addAllToPalette() override {
		common::Guard colorListGuard = gs.colorList().changeGuard();
		foreachItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			gs.colorList().add(colorObject, true);
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
		return type != Type::preview;
	}
	virtual bool isContainer() override {
		return type != Type::preview;
	}
	virtual void addColors(const std::vector<ColorObject> &colorObjects) override {
		for (auto &colorObject: colorObjects) {
			colorList.add(colorObject, true);
		}
	}
	virtual void editColors() override {
		auto selectedCount = palette_list_get_selected_count(treeview);
		if (selectedCount == 0)
			return;
		if (selectedCount == 1) {
			ColorObject *colorObject = palette_list_get_first_selected(treeview)->reference();
			common::Ref<ColorObject> newColorObject;
			if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(treeview)), gs, *colorObject, true, newColorObject) == 0) {
				colorObject->setColor(newColorObject->getColor());
				colorObject->setName(newColorObject->getName());
				palette_list_update_first_selected(treeview, false);
			}
			colorObject->release();
		} else {
			ColorList colorList;
			foreachSelectedItem(GTK_TREE_VIEW(treeview), [&colorList](ColorObject *colorObject) {
				colorList.add(colorObject, false);
				return true;
			});
			dialog_edit_show(GTK_WINDOW(gtk_widget_get_toplevel(treeview)), colorList, gs);
			updateSelected(GTK_TREE_VIEW(treeview), gs);
		}
	}
	virtual void removeColors(bool selected) override {
		if (selected) {
			colorList.resetSelected();
			foreachSelectedItem(GTK_TREE_VIEW(treeview), [](ColorObject *colorObject) {
				colorObject->setSelected(true);
				return true;
			});
			colorList.removeSelected();
		} else {
			palette_list_remove_all_entries(treeview, true);
		}
	}
	virtual void setColor(const ColorObject &colorObject) override {
		foreachSelectedItem(GTK_TREE_VIEW(treeview), [&colorObject](ColorObject *old) {
			old->setName(colorObject.getName());
			old->setColor(colorObject.getColor());
			return false;
		});
	}
	virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
	}
	virtual void setColorAt(const ColorObject &colorObject, int x, int y) override {
		setColorsAt(std::vector<ColorObject>{ colorObject }, x, y);
	}
	virtual void setColorsAt(const std::vector<ColorObject> &colorObjects, int x, int y) override {
		colorList.resetSelected();
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
		common::Guard colorListGuard = colorList.changeGuard();
		common::Ref<ColorObject> colorObject;
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
				set(GTK_LIST_STORE(model), &iter, colorObject.pointer(), this);
				colorList.add(colorObject.pointer(), false);
			} else {
				colorObject = colorObjects[i].copy();
				colorObject->setSelected(true);
				colorList.add(colorObject.pointer(), true);
			}
		}
	}
	virtual std::vector<ColorObject> getColors(bool selected) override {
		std::vector<ColorObject> result;
		colorList.resetAll();
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
		gint x, y, offset, dx, dy;
		gdk_window_get_pointer(gtk_tree_view_get_bin_window(GTK_TREE_VIEW(args->treeview)), &x, &y, nullptr);
		gtk_tree_view_convert_bin_window_to_tree_coords(GTK_TREE_VIEW(args->treeview), 0, 0, &dx, &dy);
		x += dx;
		y += dy;
		gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(args->treeview), &visibleRect);
		if (x < visibleRect.x || x > visibleRect.x + visibleRect.width)
			return true;
		if (y < visibleRect.y - 50 || y  > visibleRect.y + visibleRect.height + 50)
			return true;
		offset = y - (visibleRect.y + 2 * scrollEdgeSize);
		if (offset > 0) {
			offset = y - (visibleRect.y + visibleRect.height - 2 * scrollEdgeSize);
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
		removeScrollTimeout();
		updateCounts();
	}
	virtual void dragEnd(bool move) override {
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
			colorList.removeVisited();
			colorList.resetSelected();
			if (type == Type::main)
				gs.eventBus().trigger(EventType::paletteChanged);
		} else {
			colorList.resetAll();
			if (type == Type::main)
				gs.eventBus().trigger(EventType::paletteChanged);
		}
		countUpdateBlocked = false;
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
		if (type == Type::main)
			gs.eventBus().trigger(EventType::paletteChanged);
	}
	static void onRowActivated(GtkTreeView *, GtkTreePath *path, GtkTreeViewColumn *, ListPaletteArgs *args) {
		GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->treeview));
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		if (colorObject && args->type == Type::main) {
			auto *colorSource = args->gs.getCurrentColorSource();
			if (colorSource != nullptr)
				colorSource->setColor(*colorObject);
		} else if (colorObject) {
			args->gs.colorList().add(colorObject, true);
		}
		args->updateCounts();
		args->onChange();
	}
	virtual void add(ColorList &colorList, ColorObject *colorObject) override {
		palette_list_add_entry(treeview, colorObject, !colorList.blocked());
	}
	virtual void remove(ColorList &colorList, ColorObject *colorObject) override {
		palette_list_remove_entry(treeview, colorObject, !colorList.blocked());
	}
	virtual void removeSelected(ColorList &colorList) override {
		palette_list_remove_selected_entries(treeview, !colorList.blocked());
	}
	virtual void clear(ColorList &colorList) override {
		palette_list_remove_all_entries(treeview, !colorList.blocked());
	}
	static PaletteListCallbackResult getPositions(ColorObject *colorObject, void *userdata) {
		auto &position = *reinterpret_cast<size_t *>(userdata);
		colorObject->setPosition(position);
		++position;
		return PaletteListCallbackResult::noUpdate;
	}
	virtual void getPositions(ColorList &colorList) override {
		size_t position = 0;
		palette_list_foreach(treeview, getPositions, &position, false);
	}
	virtual void update(ColorList &) override {
		palette_list_after_update(treeview);
	}
	static gboolean onButtonRelease(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
		if (event->button == 1 && !args->allowSelection) {
			args->controlSelection(true);
		}
		args->updateCounts();
		return false;
	}
	static void onCursorChanged(GtkTreeView *treeview, ListPaletteArgs *args) {
		args->updateCounts();
	}
	static gboolean onSelectAll(GtkTreeView *treeview, ListPaletteArgs *args) {
		args->updateCounts();
		return false;
	}
	static gboolean onUnselectAll(GtkTreeView *treeview, ListPaletteArgs *args) {
		args->updateCounts();
		return false;
	}
	static void onCellEdited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer userData) {
		GtkTreeIter iter;
		GtkTreeModel *model = GTK_TREE_MODEL(userData);
		ListPaletteArgs *args = reinterpret_cast<ListPaletteArgs *>(g_object_get_data(G_OBJECT(model), "arguments"));
		gtk_tree_model_get_iter_from_string(model, &iter, path);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter,
				2, new_text,
				-1);
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		colorObject->setName(new_text);
		args->onChange();
	}
	static void onPreviewActivate(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, ListPaletteArgs *args) {
		auto model = gtk_tree_view_get_model(treeView);
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		if (colorObject)
			args->gs.colorList().add(colorObject, true);
	}
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
	static void onDestroyArgs(ListPaletteArgs *args){
		delete args;
	}
	static void onDestroy(GtkWidget* widget, ListPaletteArgs *args){
		args->removeScrollTimeout();
		palette_list_remove_all_entries(widget, false);
	}
	static gboolean onButtonPress(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
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
	ColorObject colorObject;
};
static void set(GtkListStore* store, GtkTreeIter *iter, ColorObject* colorObject, ListPaletteArgs* args)
{
	std::string text = args->gs.converters().serialize(*colorObject, Converters::Type::colorList);
	gtk_list_store_set(store, iter, 0, colorObject->reference(), 1, text.c_str(), 2, colorObject->getName().c_str(), -1);
}
static void palette_list_entry_update_row(GtkListStore* store, GtkTreeIter *iter, ColorObject* colorObject, ListPaletteArgs* args)
{
	std::string text = args->gs.converters().serialize(*colorObject, Converters::Type::colorList);
	gtk_list_store_set(store, iter, 1, text.c_str(), 2, colorObject->getName().c_str(), -1);
}
static void palette_list_entry_update_name(GtkListStore* store, GtkTreeIter *iter, ColorObject* colorObject)
{
	gtk_list_store_set(store, iter, 2, colorObject->getName().c_str(), -1);
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
		std::string text = gs.converters().serialize(*colorObject, Converters::Type::colorList);
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
static void updateSelected(GtkTreeView *treeView, GlobalState &gs) {
	auto model = gtk_tree_view_get_model(treeView);
	auto selection = gtk_tree_view_get_selection(treeView);
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	while (i) {
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, reinterpret_cast<GtkTreePath *>(i->data));
		ColorObject *colorObject;
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		std::string text = gs.converters().serialize(*colorObject, Converters::Type::colorList);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, text.c_str(), -1);
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
}
GtkWidget* palette_list_new(GlobalState &gs, GtkWidget *countLabel) {
	auto args = new ListPaletteArgs(gs, countLabel, Type::main);
	return args->treeview;
}
GtkWidget* palette_list_temporary_new(GlobalState &gs, GtkWidget* countLabel, ColorList &colorList) {
	auto args = new ListPaletteArgs(gs, countLabel, Type::temporary, colorList);
	return args->treeview;
}
GtkWidget* palette_list_preview_new(GlobalState &gs, bool expander, bool expanded, common::Ref<ColorList> &outColorList) {
	auto args = new ListPaletteArgs(gs, nullptr, Type::preview, gs.colorList());
	outColorList = ColorList::newList(*args);
	GtkWidget *scrolledWindow = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolledWindow), args->treeview);
	GtkWidget *mainWidget = scrolledWindow;
	if (expander) {
		GtkWidget *expander = gtk_expander_new(_("Preview"));
		gtk_container_add(GTK_CONTAINER(expander), scrolledWindow);
#if GTK_MAJOR_VERSION >= 3
		g_signal_connect(expander, "notify::expanded", G_CALLBACK(onExpanderStateChange), args);
#endif
		gtk_expander_set_expanded(GTK_EXPANDER(expander), expanded);
		mainWidget = expander;
	}
	return mainWidget;
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
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store;
	GtkTreeIter iter;
	if (gtk_tree_selection_count_selected_rows(selection) != 1){
		return -1;
	}
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	if (i){
		ColorObject* colorObject;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*)i->data);
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &colorObject, -1);
		*color = colorObject->getColor();
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
	ColorObject* colorObject;
	while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &colorObject, -1);
		if (colorObject->isSelected()){
			valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			colorObject->release();
		}else{
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
		}
	}
	if (allowUpdate) {
		args->updateCounts();
		args->onChange();
	}
}

void palette_list_add_entry(GtkWidget* widget, ColorObject* colorObject, bool allowUpdate)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter1;
	GtkListStore *store;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	gtk_list_store_append(store, &iter1);
	set(store, &iter1, colorObject, args);
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
	ColorObject* colorObject;
	while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &colorObject, -1);
		if (colorObject == r_color_object){
			valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			colorObject->release();
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
	ColorObject *colorObject = nullptr;
	if (i){
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, reinterpret_cast<GtkTreePath*>(i->data));
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	return colorObject;
}
void palette_list_update_first_selected(GtkWidget* widget, bool only_name)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	ColorObject *colorObject = nullptr;
	if (i){
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, reinterpret_cast<GtkTreePath*>(i->data));
		gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
		if (only_name)
			palette_list_entry_update_name(GTK_LIST_STORE(model), &iter, colorObject);
		else
			palette_list_entry_update_row(GTK_LIST_STORE(model), &iter, colorObject, args);
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
