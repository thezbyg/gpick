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

#include "ColorVisionDeficiency.h"
#include "dynv/Map.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#include "uiUtilities.h"
#include "I18N.h"
#include <gtk/gtk.h>
using namespace std;
namespace transformation {
static const char *transformationId = "color_vision_deficiency";
const char *ColorVisionDeficiency::getId() {
	return transformationId;
}
const char *ColorVisionDeficiency::getName() {
	return _("Color vision deficiency");
}
const char *ColorVisionDeficiency::m_deficiencyTypeStrings[] = {
	"protanomaly",
	"deuteranomaly",
	"tritanomaly",
	"protanopia",
	"deuteranopia",
	"tritanopia",
};
const size_t ColorVisionDeficiency::m_typeCount = sizeof(m_deficiencyTypeStrings) / sizeof(m_deficiencyTypeStrings[0]);
const math::Matrix3d protanomaly[11] = {
	{ 1.000000, 0.000000, -0.000000, 0.000000, 1.000000, 0.000000, -0.000000, -0.000000, 1.000000 },
	{ 0.856167, 0.182038, -0.038205, 0.029342, 0.955115, 0.015544, -0.002880, -0.001563, 1.004443 },
	{ 0.734766, 0.334872, -0.069637, 0.051840, 0.919198, 0.028963, -0.004928, -0.004209, 1.009137 },
	{ 0.630323, 0.465641, -0.095964, 0.069181, 0.890046, 0.040773, -0.006308, -0.007724, 1.014032 },
	{ 0.539009, 0.579343, -0.118352, 0.082546, 0.866121, 0.051332, -0.007136, -0.011959, 1.019095 },
	{ 0.458064, 0.679578, -0.137642, 0.092785, 0.846313, 0.060902, -0.007494, -0.016807, 1.024301 },
	{ 0.385450, 0.769005, -0.154455, 0.100526, 0.829802, 0.069673, -0.007442, -0.022190, 1.029632 },
	{ 0.319627, 0.849633, -0.169261, 0.106241, 0.815969, 0.077790, -0.007025, -0.028051, 1.035076 },
	{ 0.259411, 0.923008, -0.182420, 0.110296, 0.804340, 0.085364, -0.006276, -0.034346, 1.040622 },
	{ 0.203876, 0.990338, -0.194214, 0.112975, 0.794542, 0.092483, -0.005222, -0.041043, 1.046265 },
	{ 0.152286, 1.052583, -0.204868, 0.114503, 0.786281, 0.099216, -0.003882, -0.048116, 1.051998 },
};
const math::Matrix3d deuteranomaly[11] = {
	{ 1.000000, 0.000000, -0.000000, 0.000000, 1.000000, 0.000000, -0.000000, -0.000000, 1.000000 },
	{ 0.866435, 0.177704, -0.044139, 0.049567, 0.939063, 0.011370, -0.003453, 0.007233, 0.996220 },
	{ 0.760729, 0.319078, -0.079807, 0.090568, 0.889315, 0.020117, -0.006027, 0.013325, 0.992702 },
	{ 0.675425, 0.433850, -0.109275, 0.125303, 0.847755, 0.026942, -0.007950, 0.018572, 0.989378 },
	{ 0.605511, 0.528560, -0.134071, 0.155318, 0.812366, 0.032316, -0.009376, 0.023176, 0.986200 },
	{ 0.547494, 0.607765, -0.155259, 0.181692, 0.781742, 0.036566, -0.010410, 0.027275, 0.983136 },
	{ 0.498864, 0.674741, -0.173604, 0.205199, 0.754872, 0.039929, -0.011131, 0.030969, 0.980162 },
	{ 0.457771, 0.731899, -0.189670, 0.226409, 0.731012, 0.042579, -0.011595, 0.034333, 0.977261 },
	{ 0.422823, 0.781057, -0.203881, 0.245752, 0.709602, 0.044646, -0.011843, 0.037423, 0.974421 },
	{ 0.392952, 0.823610, -0.216562, 0.263559, 0.690210, 0.046232, -0.011910, 0.040281, 0.971630 },
	{ 0.367322, 0.860646, -0.227968, 0.280085, 0.672501, 0.047413, -0.011820, 0.042940, 0.968881 },
};
const math::Matrix3d tritanomaly[11] = {
	{ 1.000000, 0.000000, -0.000000, 0.000000, 1.000000, 0.000000, -0.000000, -0.000000, 1.000000 },
	{ 0.926670, 0.092514, -0.019184, 0.021191, 0.964503, 0.014306, 0.008437, 0.054813, 0.936750 },
	{ 0.895720, 0.133330, -0.029050, 0.029997, 0.945400, 0.024603, 0.013027, 0.104707, 0.882266 },
	{ 0.905871, 0.127791, -0.033662, 0.026856, 0.941251, 0.031893, 0.013410, 0.148296, 0.838294 },
	{ 0.948035, 0.089490, -0.037526, 0.014364, 0.946792, 0.038844, 0.010853, 0.193991, 0.795156 },
	{ 1.017277, 0.027029, -0.044306, -0.006113, 0.958479, 0.047634, 0.006379, 0.248708, 0.744913 },
	{ 1.104996, -0.046633, -0.058363, -0.032137, 0.971635, 0.060503, 0.001336, 0.317922, 0.680742 },
	{ 1.193214, -0.109812, -0.083402, -0.058496, 0.979410, 0.079086, -0.002346, 0.403492, 0.598854 },
	{ 1.257728, -0.139648, -0.118081, -0.078003, 0.975409, 0.102594, -0.003316, 0.501214, 0.502102 },
	{ 1.278864, -0.125333, -0.153531, -0.084748, 0.957674, 0.127074, -0.000989, 0.601151, 0.399838 },
	{ 1.255528, -0.076749, -0.178779, -0.078411, 0.930809, 0.147602, 0.004733, 0.691367, 0.303900 },
};
const math::Matrix3d rgbToLms = {
	0.05059983, 0.08585369, 0.00952420, 0.01893033, 0.08925308, 0.01370054, 0.00292202, 0.00975732, 0.07145979,
};
const math::Matrix3d lmsToRgb = {
	30.830854, -29.832659, 1.610474, -6.481468, 17.715578, -2.532642, -0.375690, -1.199062, 14.273846,
};
const double anchor[] = {
	0.080080, 0.157900, 0.589700,
	0.128400, 0.223700, 0.363600,
	0.985600, 0.732500, 0.001079,
	0.091400, 0.007009, 0.000000,
};
const double rgbAnchor[] = {
	rgbToLms[0] + rgbToLms[1] + rgbToLms[2],
	rgbToLms[3] + rgbToLms[4] + rgbToLms[5],
	rgbToLms[6] + rgbToLms[7] + rgbToLms[8],
};
const math::Vector3d protanopiaAbc[2] = {
	{
		rgbAnchor[1] * anchor[8] - rgbAnchor[2] * anchor[7],
		rgbAnchor[2] * anchor[6] - rgbAnchor[0] * anchor[8],
		rgbAnchor[0] * anchor[7] - rgbAnchor[1] * anchor[6],
	},
	{
		rgbAnchor[1] * anchor[2] - rgbAnchor[2] * anchor[1],
		rgbAnchor[2] * anchor[0] - rgbAnchor[0] * anchor[2],
		rgbAnchor[0] * anchor[1] - rgbAnchor[1] * anchor[0],
	},
};
const math::Vector3d deuteranopiaAbc[2] = {
	{
		rgbAnchor[1] * anchor[8] - rgbAnchor[2] * anchor[7],
		rgbAnchor[2] * anchor[6] - rgbAnchor[0] * anchor[8],
		rgbAnchor[0] * anchor[7] - rgbAnchor[1] * anchor[6],
	},
	{
		rgbAnchor[1] * anchor[2] - rgbAnchor[2] * anchor[1],
		rgbAnchor[2] * anchor[0] - rgbAnchor[0] * anchor[2],
		rgbAnchor[0] * anchor[1] - rgbAnchor[1] * anchor[0],
	},
};
const math::Vector3d tritanopiaAbc[2] = {
	{
		rgbAnchor[1] * anchor[11] - rgbAnchor[2] * anchor[10],
		rgbAnchor[2] * anchor[9] - rgbAnchor[0] * anchor[11],
		rgbAnchor[0] * anchor[10] - rgbAnchor[1] * anchor[9],
	},
	{
		rgbAnchor[1] * anchor[5] - rgbAnchor[2] * anchor[4],
		rgbAnchor[2] * anchor[3] - rgbAnchor[0] * anchor[5],
		rgbAnchor[0] * anchor[4] - rgbAnchor[1] * anchor[3],
	},
};
void ColorVisionDeficiency::apply(Color *input, Color *output) {
	Color linearInput, linearOutput;
	linearInput = input->linearRgb();
	math::Vector3d vi, vo1, vo2;
	vi = linearInput.rgbVector<double>();
	int index = static_cast<int>(std::floor(m_strength * 10));
	int indexSecondary = std::min(index + 1, 10);
	float interpolationFactor = (m_strength * 10) - index;
	math::Vector3d lms;
	switch (m_type) {
	case Type::protanomaly:
		linearOutput = vi * math::mix(protanomaly[index], protanomaly[indexSecondary], interpolationFactor);
		break;
	case Type::deuteranomaly:
		linearOutput = vi * math::mix(deuteranomaly[index], deuteranomaly[indexSecondary], interpolationFactor);
		break;
	case Type::tritanomaly:
		linearOutput = vi * math::mix(tritanomaly[index], tritanomaly[indexSecondary], interpolationFactor);
		break;
	case Type::protanopia:
		lms = rgbToLms * vi;
		if (lms.z / lms.y < rgbAnchor[2] / rgbAnchor[1]) {
			lms.x = -(protanopiaAbc[0].y * lms.y + protanopiaAbc[0].z * lms.z) / protanopiaAbc[0].x;
		} else {
			lms.x = -(protanopiaAbc[1].y * lms.y + protanopiaAbc[1].z * lms.z) / protanopiaAbc[1].x;
		}
		linearOutput = math::mix(vi, lmsToRgb * lms, m_strength);
		break;
	case Type::deuteranopia:
		lms = rgbToLms * vi;
		if (lms.z / lms.x < rgbAnchor[2] / rgbAnchor[0]) {
			lms.y = -(deuteranopiaAbc[0].x * lms.x + deuteranopiaAbc[0].z * lms.z) / deuteranopiaAbc[0].y;
		} else {
			lms.y = -(deuteranopiaAbc[1].x * lms.x + deuteranopiaAbc[1].z * lms.z) / deuteranopiaAbc[1].y;
		}
		linearOutput = math::mix(vi, lmsToRgb * lms, m_strength);
		break;
	case Type::tritanopia:
		lms = rgbToLms * vi;
		if (lms.y / lms.x < rgbAnchor[1] / rgbAnchor[0]) {
			lms.z = -(tritanopiaAbc[0].x * lms.x + tritanopiaAbc[0].y * lms.y) / tritanopiaAbc[0].z;
		} else {
			lms.z = -(tritanopiaAbc[1].x * lms.x + tritanopiaAbc[1].y * lms.y) / tritanopiaAbc[1].z;
		}
		linearOutput = math::mix(vi, lmsToRgb * lms, m_strength);
		break;
	default:
		*output = *input;
		return;
	}
	*output = linearOutput.nonLinearRgbInplace().normalizeRgbInplace();
}
ColorVisionDeficiency::ColorVisionDeficiency():
	Transformation(transformationId, getName()),
	m_type(Type::protanomaly),
	m_strength(0.5f) {
}
ColorVisionDeficiency::ColorVisionDeficiency(Type type, float strength):
	Transformation(transformationId, getName()),
	m_type(type),
	m_strength(strength) {
}
ColorVisionDeficiency::~ColorVisionDeficiency() {
}
void ColorVisionDeficiency::serialize(dynv::Map &system) {
	system.set("strength", m_strength);
	system.set("type", m_deficiencyTypeStrings[static_cast<int>(m_type)]);
	Transformation::serialize(system);
}
ColorVisionDeficiency::Type ColorVisionDeficiency::typeFromString(const std::string &typeString) {
	for (size_t i = 0; i < m_typeCount; i++) {
		if (typeString == m_deficiencyTypeStrings[i]) {
			return static_cast<Type>(i);
		}
	}
	return Type::protanomaly;
}
void ColorVisionDeficiency::deserialize(const dynv::Map &system) {
	m_strength = system.getFloat("strength", 0.5f);
	m_type = typeFromString(system.getString("type", "protanomaly"));
}
GtkWidget *ColorVisionDeficiency::createTypeList() {
	GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkWidget *widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	gtk_combo_box_set_add_tearoffs(GTK_COMBO_BOX(widget), 0);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, 0);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer, "text", 0, nullptr);
	g_object_unref(GTK_TREE_MODEL(store));
	struct {
		const char *name;
		Type type;
	} types[] = {
		{ _("Protanomaly"), Type::protanomaly },
		{ _("Deuteranomaly"), Type::deuteranomaly },
		{ _("Tritanomaly"), Type::tritanomaly },
		{ _("Protanopia"), Type::protanopia },
		{ _("Deuteranopia"), Type::deuteranopia },
		{ _("Tritanopia"), Type::tritanopia },
	};
	GtkTreeIter iter1;
	for (size_t i = 0; i < ColorVisionDeficiency::m_typeCount; ++i) {
		gtk_list_store_append(store, &iter1);
		gtk_list_store_set(store, &iter1, 0, types[i].name, 1, static_cast<int>(types[i].type), -1);
	}
	return widget;
}
std::unique_ptr<IConfiguration> ColorVisionDeficiency::getConfiguration() {
	return std::move(std::make_unique<Configuration>(*this));
}
ColorVisionDeficiency::Configuration::Configuration(ColorVisionDeficiency &transformation) {
	GtkWidget *table = gtk_table_new(2, 2, false);
	GtkWidget *widget;
	int table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Type:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	m_type = widget = createTypeList();
	g_signal_connect(G_OBJECT(m_type), "changed", G_CALLBACK(onTypeComboBoxChange), this);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	m_infoBar = widget = gtk_info_bar_new();
	m_infoLabel = gtk_label_new("");
	gtk_label_set_line_wrap(GTK_LABEL(m_infoLabel), true);
	gtk_label_set_justify(GTK_LABEL(m_infoLabel), GTK_JUSTIFY_LEFT);
	gtk_label_set_single_line_mode(GTK_LABEL(m_infoLabel), false);
	gtk_misc_set_alignment(GTK_MISC(m_infoLabel), 0, 0.5);
	gtk_widget_set_size_request(m_infoLabel, 1, -1);
	GtkWidget *contentArea = gtk_info_bar_get_content_area(GTK_INFO_BAR(m_infoBar));
	gtk_container_add(GTK_CONTAINER(contentArea), m_infoLabel);
	gtk_widget_show_all(m_infoBar);
	g_signal_connect(G_OBJECT(m_infoLabel), "size-allocate", G_CALLBACK(onInfoLabelSizeAllocate), this);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	gtk_combo_box_set_active(GTK_COMBO_BOX(m_type), static_cast<gint>(transformation.m_type));
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Strength:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	m_strength = widget = gtk_hscale_new_with_range(0, 100, 1);
	gtk_range_set_value(GTK_RANGE(widget), transformation.m_strength * 100);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	m_main = table;
	gtk_widget_show_all(m_main);
	g_object_ref(m_main);
}
ColorVisionDeficiency::Configuration::~Configuration() {
	g_object_unref(m_main);
}
GtkWidget *ColorVisionDeficiency::Configuration::getWidget() {
	return m_main;
}
void ColorVisionDeficiency::Configuration::apply(dynv::Map &options) {
	options.set("strength", static_cast<float>(gtk_range_get_value(GTK_RANGE(m_strength)) / 100.0f));
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(m_type), &iter)) {
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(m_type));
		Type type;
		gtk_tree_model_get(model, &iter, 1, &type, -1);
		options.set("type", ColorVisionDeficiency::m_deficiencyTypeStrings[static_cast<size_t>(type)]);
	}
}
void ColorVisionDeficiency::Configuration::onTypeComboBoxChange(GtkWidget *widget, ColorVisionDeficiency::Configuration *configuration) {
	const char *descriptions[] = {
		_("Altered spectral sensitivity of red receptors"),
		_("Altered spectral sensitivity of green receptors"),
		_("Altered spectral sensitivity of blue receptors"),
		_("Absence of red receptors"),
		_("Absence of green receptors"),
		_("Absence of blue receptors"),
	};
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(configuration->m_type), &iter)) {
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(configuration->m_type));
		Type type;
		gtk_tree_model_get(model, &iter, 1, &type, -1);
		gtk_label_set_text(GTK_LABEL(configuration->m_infoLabel), descriptions[static_cast<size_t>(type)]);
	} else {
		gtk_label_set_text(GTK_LABEL(configuration->m_infoLabel), "");
	}
	gtk_info_bar_set_message_type(GTK_INFO_BAR(configuration->m_infoBar), GTK_MESSAGE_INFO);
}
void ColorVisionDeficiency::Configuration::onInfoLabelSizeAllocate(GtkWidget *widget, GtkAllocation *allocation, ColorVisionDeficiency::Configuration *configuration) {
	gtk_widget_set_size_request(configuration->m_infoLabel, allocation->width - 16, -1);
}
}
