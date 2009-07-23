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
#include "ColorNames.h"
#include "gtk/uiSwatch.h"

#include <sstream>
#include <iomanip>

using namespace std;

void palette_list_cell_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer user_data) {
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

void palette_list_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
	GtkTreeModel* model;
	GtkTreeIter iter;

	model=gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get_iter(model, &iter, path);

	struct ColorObject *color_object;
	gtk_tree_model_get(model, &iter, 0, &color_object, -1);
	Color c;
	color_object_get_color(color_object, &c);
	gtk_swatch_set_active_color(GTK_SWATCH(user_data), &c);


}

GtkWidget* palette_list_new(GtkWidget* swatch) {

	GtkListStore  		*store;
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;
	GtkWidget           *view;

	view = gtk_tree_view_new ();



	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view),1);

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

	g_signal_connect (G_OBJECT (view), "row-activated", G_CALLBACK(palette_list_row_activated) , swatch);
	
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW (view), TRUE);

	return view;
}

/*
static GdkPixbuf* palette_list_pixbuf_from_color(Color* c, gint32 width, gint32 height){
	GdkPixmap* pixmap=gdk_pixmap_new (gdk_get_default_root_window(), width, height, -1);
	cairo_t *cr;
	cr = gdk_cairo_create (pixmap);

	cairo_set_source_rgb(cr,c->rgb.red, c->rgb.green, c->rgb.blue);
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_fill (cr);

	cairo_destroy (cr);

	GdkPixbuf* pixbuf = gdk_pixbuf_get_from_drawable(0, pixmap, 0, 0,0, 0,0, width, height);

	g_object_unref (pixmap);

	return pixbuf;
}
*/

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

	while (i) {
		gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, (GtkTreePath*)i->data);

		struct ColorObject* c;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &c, -1);
		color_object_get_color(c, color);
		//color_copy(c, color);

		i = g_list_next(i);
	}

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);

	return 0;
}

void palette_list_remove_selected_entries(GtkWidget* widget) {

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

}

/*
void palette_list_add_entry(GtkWidget* widget, ColorNames* color_names, Color* color) {
	string color_name = color_names_get(color_names, color);
	palette_list_add_entry_name(widget, color_name.c_str(), color);
}
*/

void palette_list_add_entry(GtkWidget* widget, struct ColorObject* color_object){
	GtkTreeIter iter1;
	GtkListStore *store;

	store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(widget)));

	//GdkPixbuf* pixbuf = palette_list_pixbuf_from_color(color, 24,16);

	Color color;
	color_object_get_color(color_object, &color);

	stringstream s;
	s<<int(color.rgb.red * 255)<<", "<<int(color.rgb.green * 255)<<", "<<int(color.rgb.blue * 255);

	gchar* hex_text = g_strdup(s.str().c_str());//g_strdup_printf("%d %d %d", int(color->rgb.red*255), int(color->rgb.green*255), int(color->rgb.blue*255));

	//Color* c=color_new();
	//color_copy(color, c );
	
	const char* name=(const char*)dynv_system_get(color_object->params, "string", "name");

	gtk_list_store_append(store, &iter1);
	gtk_list_store_set(store, &iter1,
		0, color_object_ref(color_object),
		1, hex_text,
		2, name,
	-1);

	g_free(hex_text);
	//g_object_unref (pixbuf);
}

#if 0
static void palette_list_menu_value_copy(GtkWidget *widget,  gpointer item) {
	const gchar *text = (const gchar *)g_object_get_data(G_OBJECT(widget), "copy-value");
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, -1);
}

void palette_list_color_value_destroy(gpointer data) {
	g_free(data);
}


/*
 * Color printing functions are temporary, we are going to add user configurable ones
 */
static string print_color_hex_rgb(Color* color){
	stringstream s;
	s.str("");
	s<<hex<<"#"<< setfill('0')<<setw(2)<<int(color->rgb.red * 255)<<setw(2)<<int(color->rgb.green * 255)<<setw(2)<<int(color->rgb.blue * 255);
	return s.str();
}

static string print_color_css_rgb(Color* color){
	stringstream s;
	s<<dec<<"rgb("<< int(color->rgb.red * 255)<<", "<<int(color->rgb.green * 255)<<", "<<int(color->rgb.blue * 255)<<")";
	return s.str();
}

static string print_color_css_hsl(Color* color){
	stringstream s;
	s<<dec<<"hsl("<< int(color->hsl.hue*360)<<", "<<int(color->hsl.saturation*100)<<"%, "<<int(color->hsl.lightness*100)<<"%)";
	return s.str();
}

/*
 * This is also must be redone in the future, because all colors are converted to all posible text forms when we actualy need only one
 */
GtkWidget* palette_list_create_copy_menu_list (GList* colors){

	struct{
		gint is_hsl;
		string (*print)(Color* color);
	}entries[]={
		{0, print_color_hex_rgb},
		{0, print_color_css_rgb},
		{1, print_color_css_hsl},
	};

	GtkWidget *menu;
	GtkWidget* item;
	gchar* tmp;
	menu = gtk_menu_new();

	for (guint i=0; i<sizeof(entries)/sizeof(entries[0]); ++i){



		GList *colors_i;
		stringstream s;
		string menu_text;
		string tmp_val;
		gint first=1;

		colors_i=colors;
		while (colors_i){

			if (entries[i].is_hsl){
				Color hsl;
				color_rgb_to_hsl(((struct NamedColor *)colors_i->data)->color, &hsl);
				tmp_val=entries[i].print(&hsl);
			}else{
				tmp_val=entries[i].print(((struct NamedColor *)colors_i->data)->color);
			}

			if (first){
				menu_text=tmp_val;
				first=0;
			}else s<<endl;
			s<<tmp_val;

			colors_i=g_list_next(colors_i);
		}

		tmp = g_strdup(s.str().c_str());
		item = gtk_menu_item_new_with_image(menu_text.c_str(), gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_list_menu_value_copy), 0);
		g_object_set_data_full(G_OBJECT(item), "copy-value", tmp, palette_list_color_value_destroy);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	}

	return menu;
}

GtkWidget* palette_list_create_copy_menu (Color* color) {
	GtkWidget *menu;
	GtkWidget* item;
	gchar* tmp;
	menu = gtk_menu_new();

	Color hsl;
	color_rgb_to_hsl(color, &hsl);

	tmp = g_strdup(print_color_hex_rgb(color).c_str());
	item = gtk_menu_item_new_with_image(tmp, gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_list_menu_value_copy), 0);
	g_object_set_data_full(G_OBJECT(item), "copy-value", tmp, palette_list_color_value_destroy);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	tmp = g_strdup(print_color_css_rgb(color).c_str());
	item = gtk_menu_item_new_with_image(tmp, gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_list_menu_value_copy), 0);
	g_object_set_data_full(G_OBJECT(item), "copy-value", tmp, palette_list_color_value_destroy);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	tmp = g_strdup(print_color_css_hsl(&hsl).c_str());
	item = gtk_menu_item_new_with_image(tmp, gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_list_menu_value_copy), 0);
	g_object_set_data_full(G_OBJECT(item), "copy-value", tmp, palette_list_color_value_destroy);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	return menu;
}
#endif

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


#if 0
gint32 palette_list_make_color_list_add(struct ColorObject* color_object, const gchar *name, void *userdata){
	struct NamedColor *c=new NamedColor;
	c->color=color_object;
	c->name=g_strdup(name);
	*((GList**)userdata) = g_list_append(*((GList**)userdata), c);
	return 0;
}

GList* palette_list_make_color_list(GtkWidget* widget){
	GList* colors=NULL;
	palette_list_foreach_selected(widget, palette_list_make_color_list_add, &colors);
	return colors;
}

void palette_list_free_color_list(GList *colors){
	GList *i;
	i=colors;
	while (i){
		g_free(((struct NamedColor *)i->data)->name);
		delete (struct NamedColor *)i->data;
		i=g_list_next(i);
	}
	g_list_free(colors);
}
#endif

