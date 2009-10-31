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

#include "uiListPalette.h"
#include "uiUtilities.h"
#include "gtk/ColorCell.h"
#include "ColorSource.h"
#include "DragDrop.h"
#include "GlobalState.h"
#include "main.h"

#include <sstream>
#include <iomanip>

using namespace std;

struct Arguments{
	ColorSource source;
	
	GlobalState* gs;
};

static void destroy_arguments(gpointer data);

static void palette_list_entry_fill(GtkListStore* store, GtkTreeIter *iter, struct ColorObject* color_object, struct Arguments* args){
	Color color;
	color_object_get_color(color_object, &color);
	gchar* text = main_get_color_text(args->gs, &color, COLOR_TEXT_TYPE_COLOR_LIST);
	
	const char* name=(const char*)dynv_system_get(color_object->params, "string", "name");
	
	gtk_list_store_set(store, iter, 0, color_object_ref(color_object), 1, text, 2, name, -1);

	if (text) g_free(text);
}

static void palette_list_cell_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer user_data) {
	GtkTreeIter iter;
	GtkTreeModel *model=GTK_TREE_MODEL(user_data);

	gtk_tree_model_get_iter_from_string(model, &iter, path );

	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		2, new_text,
	-1);
	
	struct ColorObject *color_object;
	gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	dynv_system_set(color_object->params, "string", "name", (void*)new_text);
}

static void palette_list_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
	struct Arguments* args = (struct Arguments*)user_data;
	
	GtkTreeModel* model;
	GtkTreeIter iter;

	model=gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	struct ColorObject *color_object;
	gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	
	ColorSource* color_source=(ColorSource*)dynv_system_get(args->gs->params, "ptr", "CurrentColorSource");
	color_source_set_color(color_source, color_object);
}

static int palette_list_preview_on_insert(struct ColorList* color_list, struct ColorObject* color_object){
	palette_list_add_entry(GTK_WIDGET(color_list->userdata), color_object);
	return 0;
}

static int palette_list_preview_on_clear(struct ColorList* color_list){
	palette_list_remove_all_entries(GTK_WIDGET(color_list->userdata));
	return 0;
}

GtkWidget* palette_list_preview_new(GlobalState* gs, bool expanded, struct ColorList* color_list, struct ColorList** out_color_list){
	
	struct Arguments* args = new struct Arguments;
	args->gs = gs;
	
	GtkListStore  		*store;
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;
	GtkWidget           *view;

	view = gtk_tree_view_new ();

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), 0);
	
	store = gtk_list_store_new (3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
	
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, 0);
	renderer = custom_cell_renderer_color_new();
	custom_cell_renderer_color_set_size(renderer, 16, 16);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "color", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL(store));
	g_object_unref (GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(view) );
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);
	
	if (out_color_list) {
		struct dynvHandlerMap* handler_map=dynv_system_get_handler_map(color_list->params);
		struct ColorList* cl=color_list_new(handler_map);
		dynv_handler_map_release(handler_map);
		
		cl->userdata=view;
		cl->on_insert=palette_list_preview_on_insert;
		cl->on_clear=palette_list_preview_on_clear;
		*out_color_list=cl;
		
	}
	
	GtkWidget *scrolled_window;
	scrolled_window=gtk_scrolled_window_new (0,0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);
	
	GtkWidget *expander=gtk_expander_new("Preview");
	gtk_container_add(GTK_CONTAINER(expander), scrolled_window);
	gtk_expander_set_expanded(GTK_EXPANDER(expander), expanded);
	
	g_object_set_data_full(G_OBJECT(view), "arguments", args, destroy_arguments);	

	return expander;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){

	GtkTreeIter iter;
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dd->widget));
	GtkTreeModel* model;	
	GList *list = gtk_tree_selection_get_selected_rows ( selection, &model );
	
	if (list){
		GList *i = list;

		struct ColorObject* color_object;
		while (i) {
			gtk_tree_model_get_iter(model, &iter, (GtkTreePath*) (i->data));
			gtk_tree_model_get(model, &iter, 0, &color_object, -1);
			
			g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
			g_list_free (list);
			return color_object_ref(color_object);

			i = g_list_next(i);
		}

		g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free (list);
	}	
	
	return 0;
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	struct Arguments* args = (struct Arguments*)dd->userdata;

	GtkTreePath* path;
	GtkTreeViewColumn* column;
	GtkTreeViewDropPosition pos;
	GtkTreeIter iter, iter2;
	
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(dd->widget));
	
	if (move){
		if (colorobject->refcnt!=1){	//only one reference, can't be in palette
			color_list_remove_color_object(args->gs->colors, colorobject);
		}
	}else{
		colorobject = color_object_copy(colorobject);
	}
	
	if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(dd->widget), x, y, &path, &pos)){
		gtk_tree_model_get_iter(model, &iter, path);
		
		if (pos==GTK_TREE_VIEW_DROP_BEFORE || pos==GTK_TREE_VIEW_DROP_INTO_OR_BEFORE){
			gtk_list_store_insert_before(GTK_LIST_STORE(model), &iter2, &iter);
			palette_list_entry_fill(GTK_LIST_STORE(model), &iter2, colorobject, args);
		}else if (pos==GTK_TREE_VIEW_DROP_AFTER || pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER){
			gtk_list_store_insert_after(GTK_LIST_STORE(model), &iter2, &iter);
			palette_list_entry_fill(GTK_LIST_STORE(model), &iter2, colorobject, args);
		}
		
		color_list_add_color_object(args->gs->colors, colorobject, false);
	}else{
		color_list_add_color_object(args->gs->colors, colorobject, true);	
	}
	return 0;
}

static bool test_at(struct DragDrop* dd, int x, int y){
	struct Arguments* args = (struct Arguments*)dd->userdata;
	
	GtkTreePath* path;
	GtkTreeViewColumn* column;
	GtkTreeViewDropPosition pos;
	GtkTreeIter iter, iter2;
	
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(dd->widget));
	
	if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(dd->widget), x, y, &path, &pos)){
		
		if (pos==GTK_TREE_VIEW_DROP_BEFORE || pos==GTK_TREE_VIEW_DROP_INTO_OR_BEFORE){
			pos=GTK_TREE_VIEW_DROP_BEFORE;
		}else if (pos==GTK_TREE_VIEW_DROP_AFTER || pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER){
			pos=GTK_TREE_VIEW_DROP_AFTER;
		}
		
		gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(dd->widget), path, pos);
	}else{
		gtk_tree_view_unset_rows_drag_dest(GTK_TREE_VIEW(dd->widget));
	}
	
	return true;
}

static void destroy_arguments(gpointer data){
	struct Arguments* args = (struct Arguments*)data;
	delete args;
}

GtkWidget* palette_list_new(GlobalState* gs){
	
	struct Arguments* args = new struct Arguments;
	args->gs = gs;

	GtkListStore  		*store;
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;
	GtkWidget           *view;

	view = gtk_tree_view_new ();

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), 1);

	store = gtk_list_store_new (3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Color");
	renderer = custom_cell_renderer_color_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "color", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Color");
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Name");
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", (GCallback) palette_list_cell_edited, store);

	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL(store));
	g_object_unref (GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(view) );

	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (G_OBJECT (view), "row-activated", G_CALLBACK(palette_list_row_activated), args);
	
	//gtk_tree_view_set_reorderable(GTK_TREE_VIEW (view), TRUE);
	gtk_drag_dest_set( view, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));
	gtk_drag_source_set( view, GDK_BUTTON1_MASK, 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));

	struct DragDrop dd;
	dragdrop_init(&dd, gs);
	
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.test_at = test_at;
	dd.userdata = args;
	
	dragdrop_widget_attach(view, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	g_object_set_data_full(G_OBJECT(view), "arguments", args, destroy_arguments);	

	return view;
}

void palette_list_remove_all_entries(GtkWidget* widget) {
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	while (valid){
		struct ColorObject* c;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &c, -1);
		color_object_release(c);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}

	gtk_list_store_clear(GTK_LIST_STORE(store));
}

gint32 palette_list_get_selected_count(GtkWidget* widget) {
	return gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)));
}

gint32 palette_list_get_count(GtkWidget* widget){
	GtkTreeModel *store;
	store=gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
	return gtk_tree_model_iter_n_children(store, NULL);
}

gint32 palette_list_get_selected_color(GtkWidget* widget, Color* color) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(widget) );
	GtkListStore *store;
	GtkTreeIter iter;

	if (gtk_tree_selection_count_selected_rows(selection)!=1){
		return -1;
	}

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	GList *list = gtk_tree_selection_get_selected_rows ( selection, 0 );
	GList *i=list;

	struct ColorObject* c;
	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*)i->data);

		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &c, -1);
		color_object_get_color(c, color);
		break;

		i = g_list_next(i);
	}

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);

	return 0;
}

void palette_list_remove_selected_entries(GtkWidget* widget) {
	
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	struct ColorObject* color_object;

    while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		if (color_object->selected){
			valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			color_object_release(color_object);
		}else{
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
		}
    }
	
/*
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(widget) );
	GtkListStore *store;
	GtkTreeIter iter;

	if (gtk_tree_selection_count_selected_rows(selection) == 0){
		return;
	}

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	GList *list = gtk_tree_selection_get_selected_rows ( selection, 0 );
	GList *ref_list = NULL;

	GList *i = list;
	while (i) {
		ref_list = g_list_prepend(ref_list, gtk_tree_row_reference_new(GTK_TREE_MODEL(store), (GtkTreePath*) (i->data)));
		i = g_list_next(i);
	}

	i = ref_list;
	GtkTreePath *path;
	while (i) {
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference*) i->data);
		if (path) {
			gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
			gtk_tree_path_free(path);

			struct ColorObject* c;
			gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &c, -1);
			color_object_release(c);

			gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
		}
		i = g_list_next(i);
	}
	g_list_foreach (ref_list, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free (ref_list);

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
*/
}


void palette_list_add_entry(GtkWidget* widget, struct ColorObject* color_object){
	struct Arguments* args = (struct Arguments*)g_object_get_data(G_OBJECT(widget), "arguments");
	
	GtkTreeIter iter1;
	GtkListStore *store;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
	
	gtk_list_store_append(store, &iter1);
	palette_list_entry_fill(store, &iter1, color_object, args);
}

int palette_list_remove_entry(GtkWidget* widget, struct ColorObject* r_color_object){
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	struct ColorObject* color_object;

    while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		if (color_object == r_color_object){
			valid = gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
			color_object_release(color_object);
			return 0;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
	return -1;
}

gint32 palette_list_foreach(GtkWidget* widget, gint32 (*callback)(struct ColorObject* color_object, void *userdata), void *userdata){
	GtkTreeIter iter;
	GtkListStore *store;
	gboolean valid;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));
    valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	struct ColorObject* color_object;
	//gchar* color_name;

    while (valid){
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		callback(color_object, userdata);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }
	return 0;
}

gint32 palette_list_foreach_selected(GtkWidget* widget, gint32 (*callback)(struct ColorObject* color_object, void *userdata), void *userdata){
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(widget) );
	GtkListStore *store;
	GtkTreeIter iter;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	GList *list = gtk_tree_selection_get_selected_rows ( selection, 0 );
	GList *i = list;

	struct ColorObject* color_object;
	//gchar* color_name;

	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		callback(color_object, userdata);
		i = g_list_next(i);
	}

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	return 0;
}

gint32 palette_list_forfirst_selected(GtkWidget* widget, gint32 (*callback)(struct ColorObject* color_object, void *userdata), void *userdata){
	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(widget) );
	GtkListStore *store;
	GtkTreeIter iter;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	GList *list = gtk_tree_selection_get_selected_rows ( selection, 0 );
	GList *i = list;

	struct ColorObject* color_object;
	//gchar* color_name;

	if (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*) (i->data));
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &color_object, -1);
		callback(color_object, userdata);
	}

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
	return 0;
}


