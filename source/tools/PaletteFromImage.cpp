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
#include <string>
using namespace std;

struct Arguments{
	GtkWidget *file_browser;
	GtkWidget *range_colors;
	GtkWidget *merge_threshold;



	struct ColorList *color_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
};

typedef struct Node{
	uint32_t n_pixels;
	uint32_t n_pixels_in;
	float color[3];
	float distance;

	Node *child[8];
}Node;

typedef struct Cube{
	float x, w;
	float y, h;
	float z, d;
}Cube;

static Node* node_new(){
	Node *n = new Node;
	memset(n, 0, sizeof(Node));
	return n;
}

static void node_split(Node *node){
	for (int i = 0; i < 8; i++)
		node->child[i] = node_new();
}

static void node_update(Node *node, Color *color, Cube *cube, uint32_t max_depth){
	Cube new_cube;

	new_cube.w = cube->w / 2;
	new_cube.h = cube->h / 2;
	new_cube.d = cube->d / 2;

	if (!max_depth){
		node->color[0] += color->xyz.x;
		node->color[1] += color->xyz.y;
		node->color[2] += color->xyz.z;

		node->n_pixels++;
		node->n_pixels_in++;

		node->distance += (color->xyz.x - (cube->x + new_cube.w)) * (color->xyz.x - (cube->x + new_cube.w)) +
			(color->xyz.y - (cube->y + new_cube.h)) * (color->xyz.y - (cube->y + new_cube.h)) +
			(color->xyz.z - (cube->z + new_cube.d)) * (color->xyz.z - (cube->z + new_cube.d));

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
			node->child[i] = node_new();

		node->n_pixels++;

		node_update(node->child[x | (y<<1) | (z<<2)], color, &new_cube, max_depth - 1);
	}
}

static void calc(struct Arguments *args, bool preview, int limit){

	gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(args->file_browser));
	GError *error = NULL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
	if (error){
		cout << error->message << endl;
		g_error_free(error);
	}
	g_free(filename);


	int colors = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->range_colors));
	double threshold = gtk_range_get_value(GTK_RANGE(args->merge_threshold));

	if (!preview){


	}

	Node *root_node = node_new();



	//uint32_t *color_usage = new uint32_t [128*128*128];
	//memset(color_usage, 0, 128*128*128*sizeof(uint32_t));

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

	for (int y = 0; y < height; y++){
		ptr = image_data + rowstride * y;
		for (int x = 0; x < width; x++){

			Color color;
			color.xyz.x = ptr[0] / 255.0;
			color.xyz.y = ptr[1] / 255.0;
			color.xyz.z = ptr[2] / 255.0;

			node_update(root_node, &color, &cube, 8);

			//color_usage[((ptr[0]>>1)<<0) | ((ptr[1]>>1)<<7) | ((ptr[2]>>1)<<14)]++;

			ptr += channels;
		}
	}

	//for (int i = 0; i < 128*128*128; i++)
	//    if (color_usage[i]) cout << color_usage[i] << " ";

	//delete [] color_usage;
	g_object_unref(pixbuf);

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
	gtk_table_attach(GTK_TABLE(table), widget, 0, 3, table_y, table_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	g_signal_connect(G_OBJECT(args->file_browser), "file-set", G_CALLBACK(update), args);
	table_y++;


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



	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Threshold:",0,0,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->merge_threshold = widget = gtk_hscale_new_with_range(0, 100, 0.1);
	gtk_range_set_value(GTK_RANGE(widget), dynv_get_float_wd(args->params, "merge_threshold", 20));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(update), args);
	g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(format_threshold_value_cb), args);
	gtk_scale_set_value_pos(GTK_SCALE(widget), GTK_POS_RIGHT);
	gtk_table_attach(GTK_TABLE(table), widget,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;


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

	dynv_system_release(args->params);

	gtk_widget_destroy(dialog);

	delete args;

}

