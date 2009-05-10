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

#include "uiConverter.h"
#include "uiUtilities.h"
#include "uiListPalette.h"

#include "LuaSystem.h"
#include "LuaExt.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;

struct ConverterParams{
	gchar* function_name;
	struct LuaSystem* lua;
	GtkWidget* palette_widget;
	struct ColorObject* color_object;
};

static void converter_destroy_params(struct ConverterParams* data){
	color_object_release(data->color_object);
	g_free(data->function_name);
	delete data;
}

static gint32 color_list_selected(struct ColorObject* color_object, void *userdata){
	color_list_add_color_object((struct ColorList *)userdata, color_object);
	return 0;
}


static int converter_get_result(const gchar* function, struct ColorObject* color_object, struct LuaSystem* lua, gchar** result){

	size_t st;
	int status;
	int stack_top = lua_gettop(lua->l);
	
	lua_getglobal(lua->l, function);
	if (lua_type(lua->l, 1)!=LUA_TNIL){

		lua_pushcolorobject (lua->l, color_object);
	
		status=lua_pcall(lua->l, 1, 1, 0);
		if (status==0){
			if (lua_type(lua->l, 1)==LUA_TSTRING){
				const char* converted = luaL_checklstring(lua->l,-1, &st);
				*result = g_strdup(converted);
				lua_settop(lua->l, stack_top);
				return 0;
			}else{
				cerr<<"converter_get_result: returned not a string value \""<<function<<"\""<<endl;
			}
		}else{
			char* message=0;
			luasys_report(lua->l,status,&message);
			if (message){
				cerr<<"converter_get_result: "<<message<<endl;
				free(message);
			}
		}
	}else{
		cerr<<"converter_get_result: no such function \""<<function<<"\""<<endl;
	}
	
	lua_settop(lua->l, stack_top);	
	return -1;
}

static void converter_callback_copy(GtkWidget *widget,  gpointer item) {
	struct ConverterParams* params=(struct ConverterParams*)g_object_get_data(G_OBJECT(widget), "params");
	
	stringstream text(ios::out);

	int first=TRUE;
	
	struct ColorList *color_list = color_list_new(NULL);
	if (params->palette_widget){
		palette_list_foreach_selected(params->palette_widget, color_list_selected, color_list);
	}else{
		color_list_add_color_object(color_list, params->color_object);
	}

	for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
	
		gchar* converted;
	
		if (converter_get_result(params->function_name, *i, params->lua, &converted)==0){
			if (first){
				text<<converted;
				first=FALSE;
			}else{
				text<<endl<<converted;
			}
			g_free(converted);
		}
	}
	
	color_list_destroy(color_list);
	
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text.str().c_str(), -1);
}


static void converter_create_copy_menu_item (GtkWidget *menu, const gchar* function, struct ColorObject* color_object, GtkWidget* palette_widget, struct LuaSystem* lua){
	GtkWidget* item;
	gchar* converted;
	
	if (converter_get_result(function, color_object, lua, &converted)==0){
		item = gtk_menu_item_new_with_image(converted, gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(converter_callback_copy), 0);

		struct ConverterParams* params=new struct ConverterParams;
		params->lua=lua;
		params->function_name=g_strdup(function);
		params->palette_widget=palette_widget;
		params->color_object=color_object_ref(color_object);

		g_object_set_data_full(G_OBJECT(item), "params", params, (GDestroyNotify)converter_destroy_params);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		
		g_free(converted);
	}																						
}

GtkWidget* converter_create_copy_menu (struct ColorObject* color_object, GtkWidget* palette_widget, GKeyFile* settings, struct LuaSystem* lua){

	GtkWidget *menu;
	menu = gtk_menu_new();
	
	/*converter_create_copy_menu_item(menu, "color_web_hex", color_object, palette_widget, lua);
	converter_create_copy_menu_item(menu, "color_css_rgb", color_object, palette_widget, lua);
	converter_create_copy_menu_item(menu, "color_css_hsl", color_object, palette_widget, lua);*/
	
	gchar** source_array;
	gsize source_array_size;
	if ((source_array = g_key_file_get_string_list(settings, "Converter", "Names", &source_array_size, 0))){
		for (gsize i=0; i<source_array_size; ++i){
			converter_create_copy_menu_item(menu, source_array[i], color_object, palette_widget, lua);		
		}
		g_strfreev(source_array);	
	}

	return menu;
}

struct ConverterDialog{
	struct LuaSystem *lua;
	struct ColorList *color_list;
	GtkListStore *store;
	GtkWidget* list;
};

static void converter_cell_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer user_data) {
	struct ConverterDialog* params=(struct ConverterDialog*)user_data;
	
	GtkTreeIter iter1;
	GtkListStore *store=params->store;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path );

	gchar* converted;
	
	Color c;
	c.rgb.red=0.75;
	c.rgb.green=0.50;
	c.rgb.blue=0.25;
	struct ColorObject *color_object=color_list_new_color_object(params->color_list, &c);
	dynv_system_set(color_object->params, "string", "name", (void*)"Test color");

	if (converter_get_result(new_text, color_object, params->lua, &converted)==0) {
		gtk_list_store_set(store, &iter1,
			0, new_text,
			1, converted,
		-1);
		g_free(converted);
	}else{
		gtk_list_store_set(store, &iter1,
			0, new_text,
			1, "error",
		-1);
	}	
	
	color_object_release(color_object);

}

void converter_add(GtkToolButton *toolbutton, struct ConverterDialog* params){
	GtkTreeIter iter1;
	gtk_list_store_append(params->store, &iter1);
	gtk_list_store_set(params->store, &iter1,
		0, "edit me",
		1, "error",
	-1);
}

void converter_remove_selected(GtkToolButton *toolbutton, struct ConverterDialog* params){

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(params->list));
	GtkListStore *store;
	GtkTreeIter iter;

	if (gtk_tree_selection_count_selected_rows(selection) == 0){
		return;
	}

	store=params->store;

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
			gtk_list_store_remove(GTK_LIST_STORE(store), &iter);
		}
		i = g_list_next(i);
	}
	g_list_foreach (ref_list, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free (ref_list);

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
}


static GtkWidget* converter_list_new(struct ConverterDialog* params) {

	GtkListStore  		*store;
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn   *col;
	GtkWidget           *view;

	view = gtk_tree_view_new ();
	params->list=view;

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view),1);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Function name");
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", (GCallback) converter_cell_edited, params);
	params->store=store;

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col,1);
	gtk_tree_view_column_set_title(col, "Example");
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL(store));
	g_object_unref (GTK_TREE_MODEL(store));

	GtkTreeSelection *selection = gtk_tree_view_get_selection ( GTK_TREE_VIEW(view) );

	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

	//g_signal_connect (G_OBJECT (view), "row-activated", G_CALLBACK(converter_row_activated) , 0);
	
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW (view), TRUE);

	return view;
}

void dialog_converter_show(GtkWindow* parent, GKeyFile* settings, struct LuaSystem* lua, struct ColorList *color_list ){
	
	GtkWidget *dialog = gtk_dialog_new_with_buttons("Converters", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
			
	struct ConverterDialog params;
	params.lua=lua;
	params.color_list=color_list;
			
	GtkWidget* vbox = gtk_vbox_new(0, 0);
	
	GtkWidget* toolbar;
	toolbar = gtk_toolbar_new ();
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	
		GtkToolItem *tool;
	
		tool = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON),"Add");
		g_signal_connect(G_OBJECT(tool), "clicked", G_CALLBACK(converter_add), &params);
		gtk_toolbar_insert(GTK_TOOLBAR (toolbar),tool,-1);
	
		tool = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON),"Remove");
		g_signal_connect(G_OBJECT(tool), "clicked", G_CALLBACK(converter_remove_selected), &params);
		gtk_toolbar_insert(GTK_TOOLBAR (toolbar),tool,-1);
		
	
	GtkWidget* list;
	list = converter_list_new(&params);
	gtk_box_pack_start(GTK_BOX(vbox), list, TRUE, TRUE, 0);
	
		gchar** source_array;
		gsize source_array_size;
		if ((source_array = g_key_file_get_string_list(settings, "Converter", "Names", &source_array_size, 0))){
			GtkTreeIter iter1;
			GtkListStore *store;

			store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
			
			gchar* converted;
			
			Color c;
			c.rgb.red=0.75;
			c.rgb.green=0.50;
			c.rgb.blue=0.25;
			struct ColorObject *color_object=color_list_new_color_object(color_list, &c);
			dynv_system_set(color_object->params, "string", "name", (void*)"Test color");
			
			for (gsize i=0; i<source_array_size; ++i){
			
				gtk_list_store_append(store, &iter1);
				
				if (converter_get_result(source_array[i], color_object, lua, &converted)==0) {
					gtk_list_store_set(store, &iter1,
						0, source_array[i],
						1, converted,
					-1);
					g_free(converted);
				}else{
					gtk_list_store_set(store, &iter1,
						0, source_array[i],
						1, "error",
					-1);
				}			
				
			}
			g_strfreev(source_array);	
			
			color_object_release(color_object);
		}
	

	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);
	
	gtk_window_set_default_size(GTK_WINDOW(dialog), 320, 240);
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
	
		GtkTreeIter iter;
		GtkListStore *store;
		gboolean valid;

		store=GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
		
		unsigned int count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);
		if (count>0){
			gchar** name_array = new gchar*[count];
			unsigned int i=0;

			while (valid){
				gchar* function_name;
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &function_name, -1);
			
				name_array[i]=function_name;
			
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
				++i;
			}
		
			g_key_file_set_string_list(settings, "Converter", "Names", name_array, count);
		
			for (i=0; i<count; ++i){
				if (name_array[i]) g_free(name_array[i]);
			}
			delete name_array;
		}else{
			g_key_file_set_string_list(settings, "Converter", "Names", NULL, 0);
		}
	
	}
	
	gtk_widget_destroy(dialog);
}



