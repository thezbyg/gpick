/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#include "PaletteFromImage.h"
#include "../uiUtilities.h"
#include "../uiListPalette.h"
#include "../GlobalStateStruct.h"
#include "../DynvHelpers.h"

#include <string.h>
#include <iostream>
#include <stack>
#include <string>
using namespace std;

typedef struct Node{
	uint32_t n_pixels;
	uint32_t n_pixels_in;
	float color[3];
	float distance;

	Node *child[8];
	Node *parent;
}Node;

typedef struct Cube{
	float x, w;
	float y, h;
	float z, d;
}Cube;

struct Arguments{
	GtkWidget *file_browser;
	GtkWidget *range_colors;
	GtkWidget *merge_threshold;

    string previuos_filename;
    Node *previuos_node;

	struct ColorList *color_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
};


static Node* node_new(Node *parent){
	Node *n = new Node;
	n->color[0] = n->color[1] = n->color[2] = 0;
	n->distance = 0;
	n->n_pixels = 0;
	n->n_pixels_in = 0;
	n->parent = parent;
	for (int i = 0; i < 8; i++){
		n->child[i] = 0;
	}
	return n;
}

static void node_delete(Node *node){
	for (int i = 0; i < 8; i++){
		if (node->child[i]){
			node_delete(node->child[i]);
		}
	}

	delete node;
}

static Node* node_copy(Node *node, Node *parent){
	Node *n = node_new(0);
	memcpy(n, node, sizeof(Node));
	n->parent = parent;

	for (int i = 0; i < 8; i++){
		if (node->child[i]){
			n->child[i] = node_copy(node->child[i], n);
		}else{
			n->child[i] = 0;
		}
	}
	return n;
}

static uint32_t node_count_leafs(Node *node){
	uint32_t r = 0;
	if (node->n_pixels_in) r++;
	for (int i = 0; i < 8; i++){
		if (node->child[i])
			r += node_count_leafs(node->child[i]);
	}
	return r;
}

static void node_leaf_callback(Node *node, void (*leaf_cb)(Node* node, void* userdata), void* userdata){
	if (node->n_pixels_in > 0) leaf_cb(node, userdata);

	for (int i = 0; i < 8; i++){
		if (node->child[i])
			node_leaf_callback(node->child[i], leaf_cb, userdata);
	}
}

static void node_prune(Node *node){

	for (int i = 0; i < 8; i++){
		if (node->child[i]){
			node_prune(node->child[i]);
			node->child[i] = 0;

		}
	}

	if (node->parent){
		node->parent->n_pixels_in += node->n_pixels_in;

		node->parent->color[0] += node->color[0];
		node->parent->color[1] += node->color[1];
		node->parent->color[2] += node->color[2];
	}

	node_delete(node);
}

typedef struct PruneData{
	float threshold;
	float min_distance;
	uint32_t n_colors;
	uint32_t n_colors_target;

	Node *prune_node;
	uint32_t distant_nodes;
}PruneData;

static bool node_prune_threshold(Node *node, PruneData *prune_data){
	if (node->distance <= prune_data->threshold){
		uint32_t colors_removed = node_count_leafs(node);
		node_prune(node);
		prune_data->n_colors -= colors_removed;
		return true;
	}

	if (node->distance < prune_data->min_distance){
		prune_data->min_distance = node->distance;
	}

	uint32_t n = node->n_pixels_in;

	for (int i = 0; i < 8; i++){
		if (node->child[i]){
			if (node_prune_threshold(node->child[i], prune_data)){
				node->child[i] = 0;
			}
		}
	}

	if (node->n_pixels_in > 0 && n == 0) prune_data->n_colors++;

	return false;
}

static void node_reduce(Node *node, uint32_t colors){
    PruneData prune_data;
    prune_data.n_colors = node_count_leafs(node);
    prune_data.n_colors_target = colors;
    prune_data.threshold = 0;

    while (prune_data.n_colors > colors){
	    prune_data.min_distance = node->distance;

	    if (node_prune_threshold(node, &prune_data)) break;

	    prune_data.threshold = prune_data.min_distance;
	}

}

static void node_update(Node *node, Color *color, Cube *cube, uint32_t max_depth){
	Cube new_cube;

	new_cube.w = cube->w / 2;
	new_cube.h = cube->h / 2;
	new_cube.d = cube->d / 2;

	node->n_pixels++;

	node->distance += (color->xyz.x - (cube->x + new_cube.w)) * (color->xyz.x - (cube->x + new_cube.w)) +
		(color->xyz.y - (cube->y + new_cube.h)) * (color->xyz.y - (cube->y + new_cube.h)) +
		(color->xyz.z - (cube->z + new_cube.d)) * (color->xyz.z - (cube->z + new_cube.d));

	if (!max_depth){
		node->n_pixels_in++;

		node->color[0] += color->xyz.x;
		node->color[1] += color->xyz.y;
		node->color[2] += color->xyz.z;

	}else{
		int x, y, z;

		if (color->xyz.x - cube->x < new_cube.w)
			x = 0;
		else
			x = 1;

		if (color->xyz.y - cube->y < new_cube.h)
			y = 0;
		else
			y = 1;

		if (color->xyz.z - cube->z < new_cube.d)
			z = 0;
		else
			z = 1;

		new_cube.x = cube->x + new_cube.w * x;
		new_cube.y = cube->y + new_cube.h * y;
		new_cube.z = cube->z + new_cube.d * z;

		int i = x | (y<<1) | (z<<2);

		if (!node->child[i])
			node->child[i] = node_new(node);

		node->n_pixels++;

		node_update(node->child[x | (y<<1) | (z<<2)], color, &new_cube, max_depth - 1);
	}
}

static void leaf_cb(Node *node, void *userdata){
	list<Color> *l = static_cast<list<Color>*>(userdata);

	Color c;
	c.xyz.x = node->color[0] / node->n_pixels_in;
	c.xyz.y = node->color[1] / node->n_pixels_in;
	c.xyz.z = node->color[2] / node->n_pixels_in;

	l->push_back(c);
}

static Node* process_image(struct Arguments *args, const char *filename, Node* node){

	if (args->previuos_filename == filename){
		if (args->previuos_node)
			return node_copy(args->previuos_node, 0);
		else
			return 0;
	}

	args->previuos_filename = filename;
	if (args->previuos_node){
		node_delete(args->previuos_node);
		args->previuos_node = 0;
	}

	GError *error = NULL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
	if (error){
		cout << error->message << endl;
		g_error_free(error);
		return 0;
	}

	int channels = gdk_pixbuf_get_n_channels(pixbuf);
	int width = gdk_pixbuf_get_width(pixbuf);
	int height = gdk_pixbuf_get_height(pixbuf);
	int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	guchar *image_data = gdk_pixbuf_get_pixels(pixbuf);
	guchar *ptr = image_data;

	Cube cube;
	cube.x = 0;
	cube.y = 0;
	cube.z = 0;
	cube.w = 1;
	cube.h = 1;
	cube.d = 1;

	Color color;

	args->previuos_node = node_new(0);

	for (int y = 0; y < height; y++){
		ptr = image_data + rowstride * y;
		for (int x = 0; x < width; x++){

			color.xyz.x = ptr[0] / 255.0;
			color.xyz.y = ptr[1] / 255.0;
			color.xyz.z = ptr[2] / 255.0;

			node_update(args->previuos_node, &color, &cube, 5);

			ptr += channels;
		}
	}

	g_object_unref(pixbuf);

	node_reduce(args->previuos_node, 200);

	return node_copy(args->previuos_node, 0);
}

static void calc(struct Arguments *args, bool preview, int limit){

	Node *root_node = 0;

	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(args->file_browser));
	if (filename){
		root_node = process_image(args, filename, root_node);
		g_free(filename);
	}


	int colors = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_colors));
	//double threshold = gtk_range_get_value(GTK_RANGE(args->merge_threshold));

	if (!preview){
		dynv_set_int32(args->params, "colors", colors);
		gchar *current_folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(args->file_browser));
		if (current_folder){
			dynv_set_string(args->params, "current_folder", current_folder);
			g_free(current_folder);
		}
	}

	struct ColorList *color_list;

	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->gs->colors;

	list<Color> tmp_list;

	if (root_node){
		node_reduce(root_node, colors);
		node_leaf_callback(root_node, leaf_cb, &tmp_list);
		node_delete(root_node);
	}

    for (list<Color>::iterator i = tmp_list.begin(); i != tmp_list.end(); i++){
	    struct ColorObject *color_object = color_list_new_color_object(color_list, &(*i));
	    dynv_set_string(color_object->params, "name", "");
	    color_list_add_color_object(color_list, color_object, 1);
	    color_object_release(color_object);
	}

}

static void update(GtkWidget *widget, struct Arguments *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}


static gchar* format_threshold_value_cb(GtkScale *scale, gdouble value){
	return g_strdup_printf("%0.01f%%", value);
}


void tools_palette_from_image_show(GtkWindow* parent, GlobalState* gs){
	struct Arguments *args = new struct Arguments;


	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick.tools.palette_from_image");
	args->previuos_node = 0;

	GtkWidget *table, *table_m, *widget;

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Palette from image", parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,	GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1),
		dynv_get_int32_wd(args->params, "window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);



	GtkWidget *frame;
	gint table_y, table_m_y;

	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new("Image");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;

	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);

	args->file_browser = widget = gtk_file_chooser_button_new("Image file", GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), dynv_get_string_wd(args->params, "current_folder", ""));
	gtk_table_attach(GTK_TABLE(table), widget, 0, 3, table_y, table_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	g_signal_connect(G_OBJECT(args->file_browser), "file-set", G_CALLBACK(update), args);
	table_y++;




	/* TODO:
	 *  use gdk_pixbuf_get_formats to query supported image types
	 *  save selected filter
	 */

	struct filter_format{
		const gchar* name;
		const gchar* pattern[16];
	};
	filter_format formats[] = {
		{ "PNG image (*.png)", {"*.png", 0}},
		{ "JPEG image (*.jpg, *.jpeg, *.jpe)", {"*.jpg", "*.jpeg", "*.jpe", 0}},

	};

	GtkFileFilter *filter;
	for (gint i = 0; i != sizeof(formats) / sizeof(struct filter_format); ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		for (int j = 0; formats[i].pattern[j]; j++)
			gtk_file_filter_add_pattern(filter, formats[i].pattern[j]);

		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);

		/*if (g_strcmp0(formats[i].pattern, selected_filter)==0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}*/
	}


	frame = gtk_frame_new("Options");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);



	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Colors:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->range_colors = widget = gtk_spin_button_new_with_range (1, 100, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->range_colors), dynv_get_int32_wd(args->params, "colors", 3));
	gtk_table_attach(GTK_TABLE(table), widget,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	g_signal_connect(G_OBJECT(args->range_colors), "value-changed", G_CALLBACK(update), args);
	table_y++;


    /*
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Threshold:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->merge_threshold = widget = gtk_hscale_new_with_range(0, 100, 0.1);
	gtk_range_set_value(GTK_RANGE(widget), dynv_get_float_wd(args->params, "merge_threshold", 20));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(update), args);
	g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(format_threshold_value_cb), args);
	gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_RIGHT);
	gtk_table_attach(GTK_TABLE(table), widget,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
    */

	GtkWidget* preview_expander;
	struct ColorList* preview_color_list = NULL;
	gtk_table_attach(GTK_TABLE(table_m), preview_expander = palette_list_preview_new(gs, dynv_get_bool_wd(args->params, "show_preview", true), gs->colors, &preview_color_list), 0, 1, table_m_y, table_m_y+1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_m_y++;

	args->preview_color_list = preview_color_list;

	gtk_widget_show_all(table_m);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table_m);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) calc(args, false, 0);

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);

	dynv_set_int32(args->params, "window.width", width);
	dynv_set_int32(args->params, "window.height", height);

	if (args->previuos_node) node_delete(args->previuos_node);

	dynv_system_release(args->params);

	gtk_widget_destroy(dialog);

	delete args;

}

