/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#include "ColorVisionDeficiency.h"
#include "../MathUtil.h"
#include "../uiUtilities.h"
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

namespace transformation {

static const char * transformation_name = "color_vision_deficiency";
static const char * transformation_readable_name = "Color vision deficiency";

const char *ColorVisionDeficiency::getName()
{
	return transformation_name;
}

const char *ColorVisionDeficiency::getReadableName()
{
	return transformation_readable_name;
}

const char* ColorVisionDeficiency::deficiency_type_string[] = {
	"protanomaly",
	"deuteranomaly",
	"tritanomaly",
};

const float protanomaly[11][9] = {
{1.000000,0.000000,-0.000000,0.000000,1.000000,0.000000,-0.000000,-0.000000,1.000000},
{0.856167,0.182038,-0.038205,0.029342,0.955115,0.015544,-0.002880,-0.001563,1.004443},
{0.734766,0.334872,-0.069637,0.051840,0.919198,0.028963,-0.004928,-0.004209,1.009137},
{0.630323,0.465641,-0.095964,0.069181,0.890046,0.040773,-0.006308,-0.007724,1.014032},
{0.539009,0.579343,-0.118352,0.082546,0.866121,0.051332,-0.007136,-0.011959,1.019095},
{0.458064,0.679578,-0.137642,0.092785,0.846313,0.060902,-0.007494,-0.016807,1.024301},
{0.385450,0.769005,-0.154455,0.100526,0.829802,0.069673,-0.007442,-0.022190,1.029632},
{0.319627,0.849633,-0.169261,0.106241,0.815969,0.077790,-0.007025,-0.028051,1.035076},
{0.259411,0.923008,-0.182420,0.110296,0.804340,0.085364,-0.006276,-0.034346,1.040622},
{0.203876,0.990338,-0.194214,0.112975,0.794542,0.092483,-0.005222,-0.041043,1.046265},
{0.152286,1.052583,-0.204868,0.114503,0.786281,0.099216,-0.003882,-0.048116,1.051998},
};

const float deuteranomaly[11][9] = {
{1.000000,0.000000,-0.000000,0.000000,1.000000,0.000000,-0.000000,-0.000000,1.000000},
{0.866435,0.177704,-0.044139,0.049567,0.939063,0.011370,-0.003453,0.007233,0.996220},
{0.760729,0.319078,-0.079807,0.090568,0.889315,0.020117,-0.006027,0.013325,0.992702},
{0.675425,0.433850,-0.109275,0.125303,0.847755,0.026942,-0.007950,0.018572,0.989378},
{0.605511,0.528560,-0.134071,0.155318,0.812366,0.032316,-0.009376,0.023176,0.986200},
{0.547494,0.607765,-0.155259,0.181692,0.781742,0.036566,-0.010410,0.027275,0.983136},
{0.498864,0.674741,-0.173604,0.205199,0.754872,0.039929,-0.011131,0.030969,0.980162},
{0.457771,0.731899,-0.189670,0.226409,0.731012,0.042579,-0.011595,0.034333,0.977261},
{0.422823,0.781057,-0.203881,0.245752,0.709602,0.044646,-0.011843,0.037423,0.974421},
{0.392952,0.823610,-0.216562,0.263559,0.690210,0.046232,-0.011910,0.040281,0.971630},
{0.367322,0.860646,-0.227968,0.280085,0.672501,0.047413,-0.011820,0.042940,0.968881},
};

const float tritanomaly[11][9] = {
{1.000000,0.000000,-0.000000,0.000000,1.000000,0.000000,-0.000000,-0.000000,1.000000},
{0.926670,0.092514,-0.019184,0.021191,0.964503,0.014306,0.008437,0.054813,0.936750},
{0.895720,0.133330,-0.029050,0.029997,0.945400,0.024603,0.013027,0.104707,0.882266},
{0.905871,0.127791,-0.033662,0.026856,0.941251,0.031893,0.013410,0.148296,0.838294},
{0.948035,0.089490,-0.037526,0.014364,0.946792,0.038844,0.010853,0.193991,0.795156},
{1.017277,0.027029,-0.044306,-0.006113,0.958479,0.047634,0.006379,0.248708,0.744913},
{1.104996,-0.046633,-0.058363,-0.032137,0.971635,0.060503,0.001336,0.317922,0.680742},
{1.193214,-0.109812,-0.083402,-0.058496,0.979410,0.079086,-0.002346,0.403492,0.598854},
{1.257728,-0.139648,-0.118081,-0.078003,0.975409,0.102594,-0.003316,0.501214,0.502102},
{1.278864,-0.125333,-0.153531,-0.084748,0.957674,0.127074,-0.000989,0.601151,0.399838},
{1.255528,-0.076749,-0.178779,-0.078411,0.930809,0.147602,0.004733,0.691367,0.303900},
};

static void load_matrix(const float matrix_data[9], matrix3x3 *matrix)
{
	matrix->m[0][0] = matrix_data[0];
	matrix->m[1][0] = matrix_data[1];
	matrix->m[2][0] = matrix_data[2];
	matrix->m[0][1] = matrix_data[3];
	matrix->m[1][1] = matrix_data[4];
	matrix->m[2][1] = matrix_data[5];
	matrix->m[0][2] = matrix_data[6];
	matrix->m[1][2] = matrix_data[7];
	matrix->m[2][2] = matrix_data[8];
}

void ColorVisionDeficiency::apply(Color *input, Color *output)
{
	vector3 vi, vo1, vo2;
	vi.x = input->rgb.red;
	vi.y = input->rgb.green;
	vi.z = input->rgb.blue;
	matrix3x3 matrix1, matrix2;
	int index = floor(strength * 10);
	int index_secondary = std::min(index + 1, 10);
	float interpolation_factor = (strength * 10) - index;

	switch (type){
  case PROTANOMALY:
		load_matrix(protanomaly[index], &matrix1);
		load_matrix(protanomaly[index_secondary], &matrix2);
		break;
  case DEUTERANOMALY:
		load_matrix(deuteranomaly[index], &matrix1);
		load_matrix(deuteranomaly[index_secondary], &matrix2);
		break;
  case TRITANOMALY:
		load_matrix(tritanomaly[index], &matrix1);
		load_matrix(tritanomaly[index_secondary], &matrix2);
		break;
  default:
		color_copy(input, output);
		return;
	}

	vector3_multiply_matrix3x3(&vi, &matrix1, &vo1);
	vector3_multiply_matrix3x3(&vi, &matrix2, &vo2);

	output->rgb.red = vo1.x * interpolation_factor + vo2.x * (1 - interpolation_factor);
	output->rgb.green= vo1.y * interpolation_factor + vo2.y * (1 - interpolation_factor);
	output->rgb.blue = vo1.z * interpolation_factor + vo2.z * (1 - interpolation_factor);
}

ColorVisionDeficiency::ColorVisionDeficiency():Transformation(transformation_name, transformation_readable_name)
{
	type = PROTANOMALY;
	strength = 0.5;
}

ColorVisionDeficiency::ColorVisionDeficiency(DeficiencyType type_, float strength_):Transformation(transformation_name, transformation_readable_name)
{
	type = type_;
	strength = strength_;
}

ColorVisionDeficiency::~ColorVisionDeficiency()
{

}


void ColorVisionDeficiency::serialize(struct dynvSystem *dynv)
{
	dynv_set_float(dynv, "strength", strength);
	dynv_set_string(dynv, "type", deficiency_type_string[type]);
}

ColorVisionDeficiency::DeficiencyType ColorVisionDeficiency::typeFromString(const char *type_string)
{
	for (int i = 0; i < DEFICIENCY_TYPE_COUNT; i++){
		if (strcmp(type_string, deficiency_type_string[i]) == 0){
			return (DeficiencyType)i;
		}
	}
	return PROTANOMALY;
}

void ColorVisionDeficiency::deserialize(struct dynvSystem *dynv)
{
	strength = dynv_get_float_wd(dynv, "strength", 0.5);
	type = typeFromString(dynv_get_string_wd(dynv, "type", "protanomaly"));
}


static GtkWidget* create_type_list(void){
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkWidget *widget;
	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	gtk_combo_box_set_add_tearoffs(GTK_COMBO_BOX(widget), 0);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, 0);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer, "text", 0, NULL);
	g_object_unref(GTK_TREE_MODEL(store));
	GtkTreeIter iter1;

  struct {
		const char *name;
		int type;
	} types[] = {
		{"Protanomaly", ColorVisionDeficiency::PROTANOMALY},
		{"Deuteranomaly", ColorVisionDeficiency::DEUTERANOMALY},
		{"Tritanomaly", ColorVisionDeficiency::TRITANOMALY},
	};

	for (int i = 0; i < ColorVisionDeficiency::DEFICIENCY_TYPE_COUNT; ++i){
		gtk_list_store_append(store, &iter1);
		gtk_list_store_set(store, &iter1,
			0, types[i].name,
			1, types[i].type,
		-1);
	}

	return widget;
}



boost::shared_ptr<Configuration> ColorVisionDeficiency::getConfig(){
	boost::shared_ptr<ColorVisionDeficiencyConfig> config = boost::shared_ptr<ColorVisionDeficiencyConfig>(new ColorVisionDeficiencyConfig(*this));
	return config;
}


ColorVisionDeficiencyConfig::ColorVisionDeficiencyConfig(ColorVisionDeficiency &transformation){

	GtkWidget *table = gtk_table_new(2, 2, false);
	GtkWidget *widget;
	int table_y = 0;

	table_y=0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Type:", 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	type = widget = create_type_list();
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), transformation.type);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Strength:",0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	strength = widget = gtk_hscale_new_with_range(0, 100, 1);
	gtk_range_set_value(GTK_RANGE(widget), transformation.strength * 100);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;

	main = table;
	gtk_widget_show_all(main);

	g_object_ref(main);

}

ColorVisionDeficiencyConfig::~ColorVisionDeficiencyConfig(){
	g_object_unref(main);
}

GtkWidget* ColorVisionDeficiencyConfig::getWidget(){
	return main;
}

void ColorVisionDeficiencyConfig::applyConfig(dynvSystem *dynv){
	dynv_set_float(dynv, "strength", gtk_range_get_value(GTK_RANGE(strength)) / 100.0f);

	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(type), &iter)) {
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(type));
		ColorVisionDeficiency::DeficiencyType type_id;
		gtk_tree_model_get(model, &iter, 2, &type_id, -1);
		dynv_set_string(dynv, "type", ColorVisionDeficiency::deficiency_type_string[type_id]);
	}
}













}

