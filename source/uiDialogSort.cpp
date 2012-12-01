/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#include "uiDialogSort.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "MathUtil.h"
#include "DynvHelpers.h"
#include "GlobalStateStruct.h"
#include "ColorRYB.h"
#include "Noise.h"
#include "GenerateScheme.h"
#include "Internationalisation.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;

typedef struct DialogSortArgs{
	GtkWidget *group_type;
	GtkWidget *group_sensitivity;
	GtkWidget *max_groups;
	GtkWidget *sort_type;
	GtkWidget *toggle_reverse;
	GtkWidget *toggle_reverse_groups;

	struct ColorList *sorted_color_list;
	struct ColorList *selected_color_list;
	struct ColorList *preview_color_list;

	struct dynvSystem *params;
	GlobalState* gs;
}DialogSortArgs;

typedef struct SortType{
	const char *name;
	double (*get_value)(Color *color);
}SortType;

typedef struct GroupType{
	const char *name;
	double (*get_group)(Color *color);
}GroupType;

static double sort_rgb_red(Color *color)
{
	return color->rgb.red;
}
static double sort_rgb_green(Color *color)
{
	return color->rgb.green;
}
static double sort_rgb_blue(Color *color)
{
	return color->rgb.blue;
}
static double sort_rgb_grayscale(Color *color)
{
	return (color->rgb.red + color->rgb.green + color->rgb.blue) / 3.0;
}
static double sort_hsl_hue(Color *color)
{
	Color hsl;
	color_rgb_to_hsl(color, &hsl);
	return hsl.hsl.hue;
}
static double sort_hsl_saturation(Color *color)
{
	Color hsl;
	color_rgb_to_hsl(color, &hsl);
	return hsl.hsl.saturation;
}
static double sort_hsl_lightness(Color *color)
{
	Color hsl;
	color_rgb_to_hsl(color, &hsl);
	return hsl.hsl.lightness;
}
static double sort_lab_lightness(Color *color)
{
	Color lab;
	color_rgb_to_lab_d50(color, &lab);
	return lab.lab.L;
}
static double sort_lab_a(Color *color)
{
	Color lab;
	color_rgb_to_lab_d50(color, &lab);
	return lab.lab.a;
}
static double sort_lab_b(Color *color)
{
	Color lab;
	color_rgb_to_lab_d50(color, &lab);
	return lab.lab.b;
}
static double sort_lch_lightness(Color *color)
{
	Color lch;
	color_rgb_to_lch_d50(color, &lch);
	return lch.lch.L;
}
static double sort_lch_chroma(Color *color)
{
	Color lch;
	color_rgb_to_lch_d50(color, &lch);
	return lch.lch.C;
}
static double sort_lch_hue(Color *color)
{
	Color lch;
	color_rgb_to_lch_d50(color, &lch);
	return lch.lch.h;
}

const SortType sort_types[] = {
	{"RGB Red", sort_rgb_red},
	{"RGB Green", sort_rgb_green},
	{"RGB Blue", sort_rgb_blue},
	{"RGB Grayscale", sort_rgb_grayscale},
	{"HSL Hue", sort_hsl_hue},
	{"HSL Saturation", sort_hsl_saturation},
	{"HSL Lightness", sort_hsl_lightness},
	{"Lab Lightness", sort_lab_lightness},
	{"Lab A", sort_lab_a},
	{"Lab B", sort_lab_b},
	{"LCh Lightness", sort_lch_lightness},
	{"LCh Chroma", sort_lch_chroma},
	{"LCh Hue", sort_lch_hue},
};

static double group_rgb_red(Color *color)
{
	return color->rgb.red;
}
static double group_rgb_green(Color *color)
{
	return color->rgb.green;
}
static double group_rgb_blue(Color *color)
{
	return color->rgb.blue;
}
static double group_rgb_grayscale(Color *color)
{
	return (color->rgb.red + color->rgb.green + color->rgb.blue) / 3.0;
}
static double group_hsl_hue(Color *color)
{
	Color hsl;
	color_rgb_to_hsl(color, &hsl);
	return hsl.hsl.hue;
}
static double group_hsl_saturation(Color *color)
{
	Color hsl;
	color_rgb_to_hsl(color, &hsl);
	return hsl.hsl.saturation;
}
static double group_hsl_lightness(Color *color)
{
	Color hsl;
	color_rgb_to_hsl(color, &hsl);
	return hsl.hsl.lightness;
}
static double group_lab_lightness(Color *color)
{
	Color lab;
	color_rgb_to_lab_d50(color, &lab);
	return lab.lab.L / 100.0;
}
static double group_lab_a(Color *color)
{
	Color lab;
	color_rgb_to_lab_d50(color, &lab);
	return (lab.lab.a + 145) / 290.0;
}
static double group_lab_b(Color *color)
{
	Color lab;
	color_rgb_to_lab_d50(color, &lab);
	return (lab.lab.b + 145) / 290.0;
}
static double group_lch_lightness(Color *color)
{
	Color lch;
	color_rgb_to_lch_d50(color, &lch);
	return lch.lch.L / 100.0;
}
static double group_lch_chroma(Color *color)
{
	Color lch;
	color_rgb_to_lch_d50(color, &lch);
	return lch.lch.C / 136.0;
}
static double group_lch_hue(Color *color)
{
	Color lch;
	color_rgb_to_lch_d50(color, &lch);
	return lch.lch.h / 360.0;
}

const GroupType group_types[] = {
	{"None", NULL},
	{"RGB Red", group_rgb_red},
	{"RGB Green", group_rgb_green},
	{"RGB Blue", group_rgb_blue},
	{"RGB Grayscale", group_rgb_grayscale},
	{"HSL Hue", group_hsl_hue},
	{"HSL Saturation", group_hsl_saturation},
	{"HSL Lightness", group_hsl_lightness},
	{"Lab Lightness", group_lab_lightness},
	{"Lab A", group_lab_a},
	{"Lab B", group_lab_b},
	{"LCh Lightness", group_lch_lightness},
	{"LCh Chroma", group_lch_chroma},
	{"LCh Hue", group_lch_hue},
};



typedef struct Node{
	uint32_t n_values;
	uint32_t n_values_in;
	double value_sum;
	double distance;

	Node *child[2];
	Node *parent;
}Node;

typedef struct Range{
	double x;
	double w;
}Range;

static Node* node_new(Node *parent){
	Node *n = new Node;
	n->value_sum = 0;
	n->distance = 0;
	n->n_values = 0;
	n->n_values_in = 0;
	n->parent = parent;
	for (int i = 0; i < 2; i++){
		n->child[i] = 0;
	}
	return n;
}

static void node_delete(Node *node){
	for (int i = 0; i < 2; i++){
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

	for (int i = 0; i < 2; i++){
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
	if (node->n_values_in) r++;
	for (int i = 0; i < 2; i++){
		if (node->child[i])
			r += node_count_leafs(node->child[i]);
	}
	return r;
}

static void node_leaf_callback(Node *node, void (*leaf_cb)(Node* node, void* userdata), void* userdata){
	if (node->n_values_in > 0) leaf_cb(node, userdata);

	for (int i = 0; i < 2; i++){
		if (node->child[i])
			node_leaf_callback(node->child[i], leaf_cb, userdata);
	}
}

static void node_prune(Node *node){
	for (int i = 0; i < 2; i++){
		if (node->child[i]){
			node_prune(node->child[i]);
			node->child[i] = 0;
		}
	}
	if (node->parent){
		node->parent->n_values_in += node->n_values_in;
		node->parent->value_sum += node->value_sum;
	}
	node_delete(node);
}

typedef struct PruneData{
	double threshold;
	double min_distance;
	uint32_t n_values;
	uint32_t n_values_target;

	Node *prune_node;
	uint32_t distant_nodes;
}PruneData;

static bool node_prune_threshold(Node *node, PruneData *prune_data, bool leave_node){
	if (node->distance <= prune_data->threshold){
		uint32_t values_removed;
		if (leave_node){
			values_removed = 0;
			for (int i = 0; i < 2; i++){
				if (node->child[i]){
					values_removed += node_count_leafs(node->child[i]);
					node_prune(node->child[i]);
					node->child[i] = 0;
				}
			}
		}else{
			values_removed = node_count_leafs(node);
			node_prune(node);
		}
		prune_data->n_values -= values_removed;
		return true;
	}
	if (node->distance < prune_data->min_distance){
		prune_data->min_distance = node->distance;
	}
	uint32_t n = node->n_values_in;
	for (int i = 0; i < 2; i++){
		if (node->child[i]){
			if (node_prune_threshold(node->child[i], prune_data, false)){
				node->child[i] = 0;
			}
		}
	}
	if (node->n_values_in > 0 && n == 0) prune_data->n_values++;
	return false;
}

static void node_reduce(Node *node, double threshold, uintptr_t max_values){
	PruneData prune_data;
	prune_data.n_values = node_count_leafs(node);
	prune_data.n_values_target = max_values;
	prune_data.threshold = threshold;

	prune_data.min_distance = node->distance;
	node_prune_threshold(node, &prune_data, true);
	prune_data.threshold = prune_data.min_distance;

	while (prune_data.n_values > max_values){
		prune_data.min_distance = node->distance;
		if (node_prune_threshold(node, &prune_data, true)) break;
		prune_data.threshold = prune_data.min_distance;
	}
}

static Node* node_find(Node *node, Range *range, double value)
{
	Range new_range;
	new_range.w = range->w / 2;
	int x;
	if (value - range->x < new_range.w)
		x = 0;
	else
		x = 1;
	new_range.x = range->x + new_range.w * x;
	if (node->child[x]){
		return node_find(node->child[x], &new_range, value);
	}else return node;
}

static void node_update(Node *node, Range *range, double value, uint32_t max_depth){
	Range new_range;
	new_range.w = range->w / 2;
	node->n_values++;
	node->distance += (value - (range->x + new_range.w)) * (value - (range->x + new_range.w));

	if (!max_depth){
		node->n_values_in++;
		node->value_sum += value;
	}else{
		int x;
		if (value - range->x < new_range.w)
			x = 0;
		else
			x = 1;

		new_range.x = range->x + new_range.w * x;
		if (!node->child[x])
			node->child[x] = node_new(node);

		node->n_values++;
		node_update(node->child[x], &new_range, value, max_depth - 1);
	}
}


static void calc(DialogSortArgs *args, bool preview, int limit){
	int32_t group_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->group_type));
	double group_sensitivity = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->group_sensitivity));
	int max_groups = gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->max_groups));
	int32_t sort_type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->sort_type));
	bool reverse = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_reverse));
	bool reverse_groups = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->toggle_reverse_groups));

	if (!preview){
		dynv_set_int32(args->params, "group_type", group_type);
		dynv_set_float(args->params, "group_sensitivity", group_sensitivity);
		dynv_set_int32(args->params, "max_groups", max_groups);
		dynv_set_int32(args->params, "sort_type", sort_type);
		dynv_set_bool(args->params, "reverse", reverse);
		dynv_set_bool(args->params, "reverse_groups", reverse_groups);
	}

	struct ColorList *color_list;
	if (preview)
		color_list = args->preview_color_list;
	else
		color_list = args->sorted_color_list;

	typedef std::multimap<double, struct ColorObject*> SortedColors;
	typedef std::map<uintptr_t, SortedColors> GroupedSortedColors;
	GroupedSortedColors grouped_sorted_colors;

	typedef std::multimap<double, uintptr_t> SortedGroups;
	SortedGroups sorted_groups;

	const GroupType *group = &group_types[group_type];
	const SortType *sort = &sort_types[sort_type];

	Color in;
	Node *group_nodes = node_new(0);
	Range range;
	range.x = 0;
	range.w = 1;
	int tmp_limit = limit;
	if (group->get_group){
		for (ColorList::iter i = args->selected_color_list->colors.begin(); i != args->selected_color_list->colors.end(); ++i){
			color_object_get_color(*i, &in);
			node_update(group_nodes, &range, group->get_group(&in), 8);

			if (preview){
				if (tmp_limit <= 0)
					break;
				tmp_limit--;
			}
		}
	}

	node_reduce(group_nodes, group_sensitivity / 100.0, max_groups);

	tmp_limit = limit;
	for (ColorList::iter i = args->selected_color_list->colors.begin(); i != args->selected_color_list->colors.end(); ++i){
		color_object_get_color(*i, &in);

		uintptr_t node_ptr = 0;
		if (group->get_group){
			node_ptr = reinterpret_cast<uintptr_t>(node_find(group_nodes, &range, group->get_group(&in)));
		}
		grouped_sorted_colors[node_ptr].insert(std::pair<double, struct ColorObject*>(sort->get_value(&in), *i));

		if (preview){
			if (tmp_limit <= 0)
				break;
			tmp_limit--;
		}
	}

	node_delete(group_nodes);

	for (GroupedSortedColors::iterator i = grouped_sorted_colors.begin(); i != grouped_sorted_colors.end(); ++i){
		color_object_get_color((*(*i).second.begin()).second, &in);
		sorted_groups.insert(std::pair<double, uintptr_t>(sort->get_value(&in), (*i).first));
	}

	if (reverse_groups){
		for (SortedGroups::reverse_iterator i = sorted_groups.rbegin(); i != sorted_groups.rend(); ++i){
			GroupedSortedColors::iterator a, b;
			a = grouped_sorted_colors.lower_bound((*i).second);
			b = grouped_sorted_colors.upper_bound((*i).second);
			for (GroupedSortedColors::iterator j = a; j != b; ++j){
				if (reverse){
					for (SortedColors::reverse_iterator k = (*j).second.rbegin(); k != (*j).second.rend(); ++k){
						color_list_add_color_object(color_list, (*k).second, true);
					}
				}else{
					for (SortedColors::iterator k = (*j).second.begin(); k != (*j).second.end(); ++k){
						color_list_add_color_object(color_list, (*k).second, true);
					}
				}
			}
		}
	}else{
		for (SortedGroups::iterator i = sorted_groups.begin(); i != sorted_groups.end(); ++i){
			GroupedSortedColors::iterator a, b;
			a = grouped_sorted_colors.lower_bound((*i).second);
			b = grouped_sorted_colors.upper_bound((*i).second);
			for (GroupedSortedColors::iterator j = a; j != b; ++j){
				if (reverse){
					for (SortedColors::reverse_iterator k = (*j).second.rbegin(); k != (*j).second.rend(); ++k){
						color_list_add_color_object(color_list, (*k).second, true);
					}
				}else{
					for (SortedColors::iterator k = (*j).second.begin(); k != (*j).second.end(); ++k){
						color_list_add_color_object(color_list, (*k).second, true);
					}
				}
			}
		}
	}
}

static void update(GtkWidget *widget, DialogSortArgs *args ){
	color_list_remove_all(args->preview_color_list);
	calc(args, true, 100);
}

bool dialog_sort_show(GtkWindow* parent, struct ColorList *selected_color_list, struct ColorList *sorted_color_list, GlobalState* gs)
{
	DialogSortArgs *args = new DialogSortArgs;
	args->gs = gs;
	args->params = dynv_get_dynv(args->gs->params, "gpick.group_and_sort");
	args->sorted_color_list = sorted_color_list;

	GtkWidget *table;

	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Group and sort"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(args->params, "window.width", -1), dynv_get_int32_wd(args->params, "window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(4, 4, FALSE);
	table_y = 0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Group type:"),0,0.5,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->group_type = gtk_combo_box_new_text();
	for (uint32_t i = 0; i < sizeof(group_types) / sizeof(GroupType); i++){
		gtk_combo_box_append_text(GTK_COMBO_BOX(args->group_type), group_types[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->group_type), dynv_get_int32_wd(args->params, "group_type", 0));
	g_signal_connect(G_OBJECT(args->group_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->group_type,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Grouping sensitivity:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->group_sensitivity = gtk_spin_button_new_with_range(0, 100, 0.01);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->group_sensitivity), dynv_get_float_wd(args->params, "group_sensitivity", 50));
	gtk_table_attach(GTK_TABLE(table), args->group_sensitivity,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->group_sensitivity), "value-changed", G_CALLBACK(update), args);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Maximum number of groups:"),0,0,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->max_groups = gtk_spin_button_new_with_range(1, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->max_groups), dynv_get_int32_wd(args->params, "max_groups", 10));
	gtk_table_attach(GTK_TABLE(table), args->max_groups,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect(G_OBJECT(args->max_groups), "value-changed", G_CALLBACK(update), args);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Sort type:"),0,0.5,0,0),2,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);
	args->sort_type = gtk_combo_box_new_text();
	for (uint32_t i = 0; i < sizeof(sort_types) / sizeof(SortType); i++){
		gtk_combo_box_append_text(GTK_COMBO_BOX(args->sort_type), sort_types[i].name);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(args->sort_type), dynv_get_int32_wd(args->params, "sort_type", 0));
	g_signal_connect (G_OBJECT (args->sort_type), "changed", G_CALLBACK(update), args);
	gtk_table_attach(GTK_TABLE(table), args->sort_type,3,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	table_y++;

	args->toggle_reverse_groups = gtk_check_button_new_with_mnemonic(_("_Reverse group order"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_reverse_groups), dynv_get_bool_wd(args->params, "reverse_groups", false));
	gtk_table_attach(GTK_TABLE(table), args->toggle_reverse_groups,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT(args->toggle_reverse_groups), "toggled", G_CALLBACK(update), args);
	table_y++;

	args->toggle_reverse = gtk_check_button_new_with_mnemonic(_("_Reverse order inside groups"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(args->toggle_reverse), dynv_get_bool_wd(args->params, "reverse", false));
	gtk_table_attach(GTK_TABLE(table), args->toggle_reverse,1,4,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,0);
	g_signal_connect (G_OBJECT(args->toggle_reverse), "toggled", G_CALLBACK(update), args);
	table_y++;


	GtkWidget* preview_expander;
	struct ColorList* preview_color_list = NULL;
	gtk_table_attach(GTK_TABLE(table), preview_expander = palette_list_preview_new(gs, true, dynv_get_bool_wd(args->params, "show_preview", true), gs->colors, &preview_color_list), 0, 4, table_y, table_y + 1 , GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;

	args->selected_color_list = selected_color_list;
	args->preview_color_list = preview_color_list;

	update(0, args);

	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

	bool retval = false;
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
		calc(args, false, 0);
		retval = true;
	}

	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	dynv_set_int32(args->params, "window.width", width);
	dynv_set_int32(args->params, "window.height", height);
	dynv_set_bool(args->params, "show_preview", gtk_expander_get_expanded(GTK_EXPANDER(preview_expander)));

	gtk_widget_destroy(dialog);

	color_list_destroy(args->preview_color_list);
	dynv_system_release(args->params);
	delete args;

	return retval;
}


