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

#include "uiListPalette.h"
#include "uiUtilities.h"
#include "gtk/ColorCell.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "ColorSource.h"
#include "DragDrop.h"
#include "GlobalState.h"
#include "Converters.h"
#include "Converter.h"
#include "GlobalState.h"
#include "dynv/Map.h"
#include "Vector2.h"
#include "I18N.h"
#include "common/Format.h"
#include "StandardMenu.h"
#include "StandardEventHandler.h"
#include <sstream>
#include <iomanip>
using namespace math;
using namespace std;

static void foreachSelectedItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback);
static void foreachItem(GtkTreeView *treeView, std::function<bool(ColorObject *)> callback);

struct ListPaletteArgs : public IReadonlyColorsUI {
	ColorSource source;
	GtkWidget *treeview;
	gint scroll_timeout;
	Vec2<int> last_click_position;
	bool disable_selection;
	GtkWidget* count_label;
	GlobalState* gs;

	virtual ~ListPaletteArgs() {
	}
	virtual void addToPalette(const ColorObject &) override {
		foreachSelectedItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			color_list_add_color_object(gs->getColorList(), colorObject, true);
			return true;
		});
	}
	virtual void addAllToPalette() override {
		foreachItem(GTK_TREE_VIEW(treeview), [this](ColorObject *colorObject) {
			color_list_add_color_object(gs->getColorList(), colorObject, true);
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
	virtual std::vector<ColorObject> getColors(bool selected) {
		std::vector<ColorObject> result;
		if (selected)
			foreachSelectedItem(GTK_TREE_VIEW(treeview), [&result](ColorObject *colorObject) {
				result.push_back(*colorObject);
				return true;
			});
		else
			foreachItem(GTK_TREE_VIEW(treeview), [&result](ColorObject *colorObject) {
				result.push_back(*colorObject);
				return true;
			});
		return result;
	}
	ColorObject colorObject;
};

static void destroy_arguments(gpointer data);
static ColorObject* get_color_object(struct DragDrop* dd);
static ColorObject** get_color_object_list(struct DragDrop* dd, size_t *color_object_n);

#define SCROLL_EDGE_SIZE 15 //SCROLL_EDGE_SIZE from gtktreeview.c

static void add_scroll_timeout(ListPaletteArgs *args);
static void remove_scroll_timeout(ListPaletteArgs *args);
static gboolean scroll_row_timeout(ListPaletteArgs *args);
static void palette_list_vertical_autoscroll(GtkTreeView *treeview);

static void update_counts(ListPaletteArgs *args);

static gboolean scroll_row_timeout(ListPaletteArgs *args){
	palette_list_vertical_autoscroll(GTK_TREE_VIEW(args->treeview));
	return true;
}

static void add_scroll_timeout(ListPaletteArgs *args){
	if (!args->scroll_timeout){
		args->scroll_timeout = gdk_threads_add_timeout(150, (GSourceFunc)scroll_row_timeout, args);
	}
}

static void remove_scroll_timeout(ListPaletteArgs *args){
	if (args->scroll_timeout){
		g_source_remove(args->scroll_timeout);
		args->scroll_timeout = 0;
	}
}

static bool drag_end(struct DragDrop* dd, GtkWidget *widget, GdkDragContext *context){
	remove_scroll_timeout((ListPaletteArgs*)dd->userdata);
	update_counts((ListPaletteArgs*)dd->userdata);
	return true;
}

typedef struct SelectionBoundsArgs{
	int min_index;
	int max_index;
	int last_index;
	bool discontinuous;
} SelectionBoundsArgs;


static void find_selection_bounds(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data){
	gint *indices = gtk_tree_path_get_indices(path);
	SelectionBoundsArgs *args = (SelectionBoundsArgs *) data;
	int index = indices[0]; // currently indices are all 1d.
	int diff = index - args->last_index;
	if ((args->last_index != 0x7fffffff) && (diff != 1) && (diff != -1)){
		args->discontinuous = true;
	}
	if (index > args->max_index){
		args->max_index = index;
	}
	if (index < args->min_index){
		args->min_index = index;
	}
	args->last_index = index;
}

static void update_counts(ListPaletteArgs *args){
	stringstream s;
	GtkTreeSelection *sel;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(args->treeview));
	SelectionBoundsArgs bounds;
	int selected_count;
	int total_colors;

	if (!args->count_label){
		return;
	}

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(args->treeview));
	selected_count = gtk_tree_selection_count_selected_rows(sel);
	total_colors = gtk_tree_model_iter_n_children(model, nullptr);

	bounds.discontinuous = false;
	bounds.min_index = 0x7fffffff;
	bounds.last_index = 0x7fffffff;
	bounds.max_index = 0;
	if (selected_count > 0){
		s.str("");
		s << "#";
		gtk_tree_selection_selected_foreach(sel, &find_selection_bounds, &bounds);
		if (bounds.min_index < bounds.max_index){
			s << bounds.min_index;
			if (bounds.discontinuous){
				s << "..";
			}else{
				s << "-";
			}
			s << bounds.max_index;
		}else{
			s << bounds.min_index;
		}
#ifdef ENABLE_NLS
		s << " (" << common::format(ngettext("{} color", "{} colors", selected_count), selected_count) << ")";
#else
		s << " (" << ((selected_count == 1) ? "color" : "colors") << ")";
#endif
		s << " " << _("selected") << ". ";
	}
#ifdef ENABLE_NLS
	s << common::format(ngettext("Total {} color", "Total {} colors", total_colors), total_colors);
#else
	s << "Total " << total_colors << ((total_colors == 1) ? " color" : " colors");
#endif
	auto message = s.str();
	gtk_label_set_text(GTK_LABEL(args->count_label), message.c_str());
}
static void palette_list_vertical_autoscroll(GtkTreeView *treeview)
{
	GdkRectangle visible_rect;
	gint y;
	gint offset;
	gdk_window_get_pointer(gtk_tree_view_get_bin_window(treeview), nullptr, &y, nullptr);
	gint dy;
	gtk_tree_view_convert_bin_window_to_tree_coords(treeview, 0, 0, 0, &dy);
	y += dy;
	gtk_tree_view_get_visible_rect(treeview, &visible_rect);
	offset = y - (visible_rect.y + 2 * SCROLL_EDGE_SIZE);
	if (offset > 0) {
		offset = y - (visible_rect.y + visible_rect.height - 2 * SCROLL_EDGE_SIZE);
		if (offset < 0)
			return;
	}
	GtkAdjustment *adjustment = gtk_tree_view_get_vadjustment(treeview);
	gtk_adjustment_set_value(adjustment, min(max(gtk_adjustment_get_value(adjustment) + offset, 0.0), gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size (adjustment)));
}

static void palette_list_entry_fill(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object, ListPaletteArgs* args)
{
	string text = args->gs->converters().serialize(color_object, Converters::Type::colorList);
	gtk_list_store_set(store, iter, 0, color_object->reference(), 1, text.c_str(), 2, color_object->getName().c_str(), -1);
}
static void palette_list_entry_update_row(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object, ListPaletteArgs* args)
{
	string text = args->gs->converters().serialize(color_object, Converters::Type::colorList);
	gtk_list_store_set(store, iter, 1, text.c_str(), 2, color_object->getName().c_str(), -1);
}
static void palette_list_entry_update_name(GtkListStore* store, GtkTreeIter *iter, ColorObject* color_object)
{
	gtk_list_store_set(store, iter, 2, color_object->getName().c_str(), -1);
}
static void palette_list_cell_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model=GTK_TREE_MODEL(user_data);
	gtk_tree_model_get_iter_from_string(model, &iter, path );
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		2, new_text,
	-1);
	ColorObject *color_object;
	gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	color_object->setName(new_text);
}
static void palette_list_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	ListPaletteArgs* args = (ListPaletteArgs*)user_data;
	GtkTreeModel* model = gtk_tree_view_get_model(tree_view);;
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	ColorObject *color_object;
	gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	ColorSource *color_source = args->gs->getCurrentColorSource();
	if (color_source != nullptr)
		color_source_set_color(color_source, color_object);
	update_counts(args);
}

static int palette_list_preview_on_insert(ColorList* color_list, ColorObject* color_object){
	palette_list_add_entry(GTK_WIDGET(color_list->userdata), color_object);
	return 0;
}

static int palette_list_preview_on_clear(ColorList* color_list){
	palette_list_remove_all_entries(GTK_WIDGET(color_list->userdata));
	return 0;
}

static void destroy_cb(GtkWidget* widget, ListPaletteArgs *args){
	remove_scroll_timeout(args);
	palette_list_remove_all_entries(widget);
}

GtkWidget* palette_list_get_widget(ColorList *color_list){
	return (GtkWidget*)color_list->userdata;
}

static gboolean controlSelection(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, ListPaletteArgs *args) {
	return args->disable_selection;
}
static void disablePaletteSelection(GtkTreeView *treeView, gboolean disable, int x, int y, ListPaletteArgs *args) {
	auto selection = gtk_tree_view_get_selection(treeView);
	args->disable_selection = disable;
	gtk_tree_selection_set_select_function(selection, (GtkTreeSelectionFunc)controlSelection, args, nullptr);
	args->last_click_position.x = x;
	args->last_click_position.y = y;
}
static void onPreviewActivate(GtkTreeView *treeView, GtkTreePath *path, GtkTreeViewColumn *column, ListPaletteArgs *args) {
	auto model = gtk_tree_view_get_model(treeView);
	GtkTreeIter iter;
	gtk_tree_model_get_iter(model, &iter, path);
	ColorObject *colorObject;
	gtk_tree_model_get(model, &iter, 0, &colorObject, -1);
	if (colorObject)
		color_list_add_color_object(args->gs->getColorList(), colorObject, true);
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
	if (event->button == 3) {
		return false;
	}
	disablePaletteSelection(treeView, true, -1, -1, args);
	if (event->button != 1)
		return false;
	if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
		return false;
	GtkTreePath *path = nullptr;
	if (!gtk_tree_view_get_path_at_pos(treeView, static_cast<int>(event->x), static_cast<int>(event->y), &path, nullptr, nullptr, nullptr))
		return false;
	auto selection = gtk_tree_view_get_selection(treeView);
	if (gtk_tree_selection_path_is_selected(selection, path))
		disablePaletteSelection(treeView, false, static_cast<int>(event->x), static_cast<int>(event->y), args);
	if (path)
		gtk_tree_path_free(path);
	return false;
}
static gboolean onPreviewButtonRelease(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	if (args->last_click_position != Vec2<int>(-1, -1)) {
		Vec2<int> clickPosition = args->last_click_position;
		disablePaletteSelection(treeView, true, -1, -1, args);
		if (clickPosition.x == static_cast<int>(event->x) && clickPosition.y == static_cast<int>(event->y)) {
			GtkTreePath *path = nullptr;
			GtkTreeViewColumn *column;
			if (gtk_tree_view_get_path_at_pos(treeView, static_cast<int>(event->x), static_cast<int>(event->y), &path, &column, nullptr, nullptr)) {
				gtk_tree_view_set_cursor(treeView, path, column, false);
			}
			if (path)
				gtk_tree_path_free(path);
		}
	}
	return false;
}
GtkWidget* palette_list_preview_new(GlobalState* gs, bool expander, bool expanded, ColorList* color_list, ColorList** out_color_list) {
	auto args = new ListPaletteArgs;
	args->gs = gs;
	args->scroll_timeout = 0;
	args->count_label = nullptr;
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
	StandardEventHandler::forWidget(view, args->gs, args, StandardEventHandler::Options().afterEvents(false));
	gtk_drag_source_set(view, GDK_BUTTON1_MASK, 0, 0, GdkDragAction(GDK_ACTION_COPY));

	struct DragDrop dd;
	dragdrop_init(&dd, gs);
	dd.converterType = Converters::Type::colorList;
	dd.get_color_object = get_color_object;
	dd.get_color_object_list = get_color_object_list;
	dd.drag_end = drag_end;
	dd.userdata = args;
	dragdrop_widget_attach(view, DragDropFlags(DRAGDROP_SOURCE), &dd);

	if (out_color_list) {
		ColorList* cl=color_list_new();
		cl->userdata=view;
		cl->on_insert=palette_list_preview_on_insert;
		cl->on_clear=palette_list_preview_on_clear;
		*out_color_list=cl;
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

static ColorObject** get_color_object_list(struct DragDrop* dd, size_t *color_object_n){

	GtkTreeIter iter;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dd->widget));
	GtkTreeModel* model;
	gint selected = gtk_tree_selection_count_selected_rows(selection);
	if (selected <= 1){
		if (color_object_n) *color_object_n = selected;
		return 0;
	}

	GList *list = gtk_tree_selection_get_selected_rows(selection, &model);

	ColorObject** color_objects = new ColorObject*[selected];
	if (color_object_n) *color_object_n = selected;

	if (list){
		GList *i = list;

		ColorObject* color_object;
		uint32_t j = 0;
		while (i) {
			gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) (i->data));
			gtk_tree_model_get(model, &iter, 0, &color_object, -1);

			color_objects[j] = color_object->reference();

			i = g_list_next(i);
			j++;
		}

		g_list_foreach (list, (GFunc)gtk_tree_path_free, nullptr);
		g_list_free (list);
	}

	return color_objects;
}

static ColorObject* get_color_object(struct DragDrop* dd){

	GtkTreeIter iter;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dd->widget));
	GtkTreeModel* model;
	GList *list = gtk_tree_selection_get_selected_rows ( selection, &model );

	if (list){
		GList *i = list;

		ColorObject* color_object;
		while (i) {
			gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) (i->data));
			gtk_tree_model_get(model, &iter, 0, &color_object, -1);

			g_list_foreach (list, (GFunc)gtk_tree_path_free, nullptr);
			g_list_free (list);
			return color_object->reference();

			i = g_list_next(i);
		}

		g_list_foreach (list, (GFunc)gtk_tree_path_free, nullptr);
		g_list_free (list);
	}

	return 0;
}

static bool getPathAt(GtkTreeView *tree_view, int x, int y, GtkTreePath *&path, GtkTreeViewDropPosition &position) {
	if (gtk_tree_view_get_dest_row_at_pos(tree_view, x, y, &path, &position)) {
		GdkModifierType mask;
		gdk_window_get_pointer(gtk_tree_view_get_bin_window(tree_view), nullptr, nullptr, &mask);
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
		gtk_tree_view_convert_widget_to_tree_coords(tree_view, x, y, &tx, &ty);
		GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
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
static int set_color_object_list_at(DragDrop* dd, ColorObject** color_objects, size_t color_object_n, int x, int y, bool move, bool sameWidget) {
	ListPaletteArgs* args = (ListPaletteArgs*)dd->userdata;
	color_list_set_selected(args->gs->getColorList(), false);
	remove_scroll_timeout(args);
	auto model = gtk_tree_view_get_model(GTK_TREE_VIEW(dd->widget));
	boost::optional<GtkTreeIter> insertIterator;
	GtkTreePath* path;
	GtkTreeViewDropPosition pos;
	if (getPathAt(GTK_TREE_VIEW(dd->widget), x, y, path, pos)) {
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_path_free(path);
		insertIterator = iter;
	}
	if (sameWidget && move) {
		for (size_t i = 0; i != color_object_n; i++) {
			ColorObject *color_object = nullptr;
			if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
				color_object = color_objects[i];
			} else if (pos == GTK_TREE_VIEW_DROP_AFTER || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
				color_object = color_objects[color_object_n - i - 1];
			}
			ColorObject *reference_color_object = nullptr;
			if (insertIterator) {
				gtk_tree_model_get(GTK_TREE_MODEL(model), &*insertIterator, 0, &reference_color_object, -1);
			}
			if (reference_color_object == color_object){
				// Reference item is going to be removed, so any further inserts
				// will fail if the same iterator is used. Iterator is moved forward
				// to avoid that.
				GtkTreeIter iter = *insertIterator;
				if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
					if (gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter)){
						insertIterator = iter;
					} else {
						insertIterator.reset();
					}
				}else if (pos == GTK_TREE_VIEW_DROP_AFTER || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
					GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
					if (gtk_tree_path_prev(path)) {
						if (gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path)){
							insertIterator = iter;
						} else {
							insertIterator.reset();
						}
					}else{
						insertIterator.reset();
					}
					gtk_tree_path_free(path);
				}
			}
			color_list_remove_color_object(args->gs->getColorList(), color_object);
			color_object->setSelected(true);
			if (insertIterator) {
				GtkTreeIter iter;
				if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
					gtk_list_store_insert_before(GTK_LIST_STORE(model), &iter, &*insertIterator);
				} else {
					gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter, &*insertIterator);
				}
				palette_list_entry_fill(GTK_LIST_STORE(model), &iter, color_object, args);
				color_list_add_color_object(args->gs->getColorList(), color_object, false);
			}else{
				color_list_add_color_object(args->gs->getColorList(), color_object, true);
			}
		}
	} else {
		for (size_t i = 0; i != color_object_n; i++) {
			ColorObject *color_object = nullptr;
			if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
				color_object = color_objects[i]->copy();
			} else if (pos == GTK_TREE_VIEW_DROP_AFTER || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
				color_object = color_objects[color_object_n - i - 1]->copy();
			}
			color_object->setSelected(true);
			if (insertIterator) {
				GtkTreeIter iter;
				if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
					gtk_list_store_insert_before(GTK_LIST_STORE(model), &iter, &*insertIterator);
				} else {
					gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter, &*insertIterator);
				}
				palette_list_entry_fill(GTK_LIST_STORE(model), &iter, color_object, args);
				color_list_add_color_object(args->gs->getColorList(), color_object, false);
			}else{
				color_list_add_color_object(args->gs->getColorList(), color_object, true);
				color_object->release();
			}
		}
	}
	auto *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dd->widget));
	gtk_tree_selection_set_select_function(selection, nullptr, nullptr, nullptr);
	gtk_tree_selection_unselect_all(selection);
	GtkTreeIter iter;
	bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid) {
		ColorObject *colorObject;
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &colorObject, -1);
		if (colorObject->isSelected()) {
			gtk_tree_selection_select_iter(selection, &iter);
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
	update_counts(args);
	return 0;
}
static int set_color_object_at(struct DragDrop* dd, ColorObject* color_object, int x, int y, bool move, bool sameWidget) {
	ListPaletteArgs* args = (ListPaletteArgs*)dd->userdata;
	color_list_set_selected(args->gs->getColorList(), false);
	remove_scroll_timeout(args);
	auto model = gtk_tree_view_get_model(GTK_TREE_VIEW(dd->widget));
	boost::optional<GtkTreeIter> insertIterator;
	GtkTreePath* path;
	GtkTreeViewDropPosition pos;
	if (getPathAt(GTK_TREE_VIEW(dd->widget), x, y, path, pos)) {
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_path_free(path);
		insertIterator = iter;
	}
	if (sameWidget && move) {
		ColorObject *reference_color_object = nullptr;
		if (insertIterator) {
			gtk_tree_model_get(GTK_TREE_MODEL(model), &*insertIterator, 0, &reference_color_object, -1);
		}
		if (reference_color_object == color_object){
			// Reference item is going to be removed, so any further inserts
			// will fail if the same iterator is used. Iterator is moved forward
			// to avoid that.
			GtkTreeIter iter = *insertIterator;
			if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
				if (gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter)){
					insertIterator = iter;
				} else {
					insertIterator.reset();
				}
			}else if (pos == GTK_TREE_VIEW_DROP_AFTER || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER) {
				GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
				if (gtk_tree_path_prev(path)) {
					if (gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path)){
						insertIterator = iter;
					} else {
						insertIterator.reset();
					}
				}else{
					insertIterator.reset();
				}
				gtk_tree_path_free(path);
			}
		}
		color_list_remove_color_object(args->gs->getColorList(), color_object);
		color_object->setSelected(true);
		if (insertIterator) {
			GtkTreeIter iter;
			if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
				gtk_list_store_insert_before(GTK_LIST_STORE(model), &iter, &*insertIterator);
			} else {
				gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter, &*insertIterator);
			}
			palette_list_entry_fill(GTK_LIST_STORE(model), &iter, color_object, args);
			color_list_add_color_object(args->gs->getColorList(), color_object, false);
		}else{
			color_list_add_color_object(args->gs->getColorList(), color_object, true);
		}
	} else {
		color_object = color_object->copy();
		color_object->setSelected(true);
		if (insertIterator) {
			GtkTreeIter iter;
			if (pos == GTK_TREE_VIEW_DROP_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE) {
				gtk_list_store_insert_before(GTK_LIST_STORE(model), &iter, &*insertIterator);
			} else {
				gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter, &*insertIterator);
			}
			palette_list_entry_fill(GTK_LIST_STORE(model), &iter, color_object, args);
			color_list_add_color_object(args->gs->getColorList(), color_object, false);
		}else{
			color_list_add_color_object(args->gs->getColorList(), color_object, true);
			color_object->release();
		}
	}
	auto *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dd->widget));
	gtk_tree_selection_set_select_function(selection, nullptr, nullptr, nullptr);
	gtk_tree_selection_unselect_all(selection);
	GtkTreeIter iter;
	bool valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid) {
		ColorObject *colorObject;
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &colorObject, -1);
		if (colorObject->isSelected()) {
			gtk_tree_selection_select_iter(selection, &iter);
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
	update_counts(args);
	return 0;
}
static bool test_at(struct DragDrop* dd, int x, int y) {
	GtkTreePath* path;
	GtkTreeViewDropPosition pos;
	if (getPathAt(GTK_TREE_VIEW(dd->widget), x, y, path, pos)) {
		gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(dd->widget), path, pos);
		gtk_tree_path_free(path);
	} else {
		gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(dd->widget), nullptr, pos);
	}
	add_scroll_timeout((ListPaletteArgs*)dd->userdata);
	return true;
}

static void destroy_arguments(gpointer data){
	ListPaletteArgs* args = (ListPaletteArgs*)data;
	delete args;
}

static gboolean on_palette_button_press(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	disablePaletteSelection(treeView, true, -1, -1, args);
	if (event->button != 1)
		return false;
	if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
		return false;
	GtkTreePath *path = nullptr;
	if (!gtk_tree_view_get_path_at_pos(treeView, static_cast<int>(event->x), static_cast<int>(event->y), &path, nullptr, nullptr, nullptr))
		return false;
	auto selection = gtk_tree_view_get_selection(treeView);
	if (gtk_tree_selection_path_is_selected(selection, path)) {
		disablePaletteSelection(treeView, false, static_cast<int>(event->x), static_cast<int>(event->y), args);
	}
	if (path)
		gtk_tree_path_free(path);
	update_counts(args);
	return false;
}

static gboolean on_palette_button_release(GtkTreeView *treeView, GdkEventButton *event, ListPaletteArgs *args) {
	if (args->last_click_position != Vec2<int>(-1, -1)) {
		Vec2<int> clickPosition = args->last_click_position;
		disablePaletteSelection(treeView, true, -1, -1, args);
		if (clickPosition.x == static_cast<int>(event->x) && clickPosition.y == static_cast<int>(event->y)) {
			GtkTreePath *path = nullptr;
			GtkTreeViewColumn *column;
			if (gtk_tree_view_get_path_at_pos(treeView, static_cast<int>(event->x), static_cast<int>(event->y), &path, &column, nullptr, nullptr)) {
				gtk_tree_view_set_cursor(treeView, path, column, FALSE);
			}
			if (path)
				gtk_tree_path_free(path);
		}
	}
	update_counts(args);
	return false;
}


static void on_palette_cursor_changed(GtkTreeView *treeview, ListPaletteArgs *args){
	update_counts(args);
}

static gboolean on_palette_select_all(GtkTreeView *treeview, ListPaletteArgs *args){
	update_counts(args);
	return FALSE;
}

static gboolean on_palette_unselect_all(GtkTreeView *treeview, ListPaletteArgs *args){
	update_counts(args);
	return FALSE;
}

GtkWidget* palette_list_new(GlobalState* gs, GtkWidget* count_label) {
	auto args = new ListPaletteArgs;
	args->gs = gs;
	args->count_label = count_label;
	args->scroll_timeout = 0;
	auto view = args->treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), true);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(view), true);
	auto store = gtk_list_store_new (3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
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

	struct DragDrop dd;
	dragdrop_init(&dd, gs);
	dd.get_color_object = get_color_object;
	dd.get_color_object_list = get_color_object_list;
	dd.set_color_object_at = set_color_object_at;
	dd.set_color_object_list_at = set_color_object_list_at;
	dd.test_at = test_at;
	dd.drag_end = drag_end;
	dd.converterType = Converters::Type::colorList;
	dd.userdata = args;
	dragdrop_widget_attach(view, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	g_object_set_data_full(G_OBJECT(view), "arguments", args, destroy_arguments);
	g_signal_connect(G_OBJECT(view), "destroy", G_CALLBACK(destroy_cb), args);

	if (count_label){
		update_counts(args);
	}

	return view;
}

void palette_list_remove_all_entries(GtkWidget* widget) {
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

	update_counts(args);
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

void palette_list_remove_selected_entries(GtkWidget* widget)
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
	update_counts(args);
}

void palette_list_add_entry(GtkWidget* widget, ColorObject* color_object)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter1;
	GtkListStore *store;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	gtk_list_store_append(store, &iter1);
	palette_list_entry_fill(store, &iter1, color_object, args);
	update_counts(args);
}
int palette_list_remove_entry(GtkWidget* widget, ColorObject* r_color_object)
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
			return 0;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	update_counts(args);
	return -1;
}
static void execute_callback(GtkListStore *store, GtkTreeIter *iter, ListPaletteArgs* args, PaletteListCallback callback, void *userdata)
{
	ColorObject* color_object;
	gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &color_object, -1);
	PaletteListCallbackReturn r = callback(color_object, userdata);
	switch (r){
		case PALETTE_LIST_CALLBACK_UPDATE_NAME:
			palette_list_entry_update_name(store, iter, color_object);
			break;
		case PALETTE_LIST_CALLBACK_UPDATE_ROW:
			palette_list_entry_update_row(store, iter, color_object, args);
			break;
		case PALETTE_LIST_CALLBACK_NO_UPDATE:
			break;
	}
}
static void execute_replace_callback(GtkListStore *store, GtkTreeIter *iter, ListPaletteArgs* args, PaletteListReplaceCallback callback, void *userdata)
{
	ColorObject *color_object, *orig_color_object;
	gtk_tree_model_get(GTK_TREE_MODEL(store), iter, 0, &color_object, -1);
	orig_color_object = color_object;

	color_object->reference();
	PaletteListCallbackReturn r = callback(&color_object, userdata);
	if (color_object != orig_color_object){
		gtk_list_store_set(store, iter, 0, color_object, -1);
	}
	switch (r){
		case PALETTE_LIST_CALLBACK_UPDATE_NAME:
			palette_list_entry_update_name(store, iter, color_object);
			break;
		case PALETTE_LIST_CALLBACK_UPDATE_ROW:
			palette_list_entry_update_row(store, iter, color_object, args);
			break;
		case PALETTE_LIST_CALLBACK_NO_UPDATE:
			break;
	}
	color_object->release();
}
gint32 palette_list_foreach(GtkWidget* widget, PaletteListCallback callback, void *userdata)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	while (valid){
		execute_callback(store, &iter, args, callback, userdata);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
	return 0;
}
gint32 palette_list_foreach_selected(GtkWidget* widget, PaletteListCallback callback, void *userdata)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store;
	GtkTreeIter iter;
	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;
	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		execute_callback(store, &iter, args, callback, userdata);
		i = g_list_next(i);
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	return 0;
}

gint32 palette_list_foreach_selected(GtkWidget* widget, PaletteListReplaceCallback callback, void *userdata){
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store;
	GtkTreeIter iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;

	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		execute_replace_callback(store, &iter, args, callback, userdata);
		i = g_list_next(i);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	return 0;
}

gint32 palette_list_forfirst_selected(GtkWidget* widget, PaletteListCallback callback, void *userdata)
{
	ListPaletteArgs* args = (ListPaletteArgs*)g_object_get_data(G_OBJECT(widget), "arguments");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
	GtkListStore *store;
	GtkTreeIter iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	GList *list = gtk_tree_selection_get_selected_rows(selection, 0);
	GList *i = list;

	if (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		execute_callback(store, &iter, args, callback, userdata);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, nullptr);
	g_list_free(list);
	return 0;
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
}
void palette_list_append_copy_menu(GtkWidget* widget, GtkWidget *menu) {
	auto args = reinterpret_cast<ListPaletteArgs *>(g_object_get_data(G_OBJECT(widget), "arguments"));
	StandardMenu::appendMenu(menu, args, args->gs);
}
