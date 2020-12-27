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

#include "GammaModification.h"
#include "dynv/Map.h"
#include "../uiUtilities.h"
#include "../I18N.h"
#include <gtk/gtk.h>
#include <cmath>
namespace transformation {
static const char *transformationId = "gamma_modification";
const char *GammaModification::getId() {
	return transformationId;
}
const char *GammaModification::getName() {
	return _("Gamma modification");
}
void GammaModification::apply(Color *input, Color *output) {
	Color linear_input, linear_output;
	linear_input = input->linearRgb();
	linear_output.rgb.red = std::pow(linear_input.rgb.red, value);
	linear_output.rgb.green = std::pow(linear_input.rgb.green, value);
	linear_output.rgb.blue = std::pow(linear_input.rgb.blue, value);
	*output = linear_output.nonLinearRgbInplace().normalizeRgbInplace();
}
GammaModification::GammaModification():
	Transformation(transformationId, getName()) {
	value = 1;
}
GammaModification::GammaModification(float value_):
	Transformation(transformationId, getName()) {
	value = value_;
}
GammaModification::~GammaModification() {
}
void GammaModification::serialize(dynv::Map &system) {
	system.set("value", value);
	Transformation::serialize(system);
}
void GammaModification::deserialize(const dynv::Map &system) {
	value = system.getFloat("value", 1);
}
std::unique_ptr<IConfiguration> GammaModification::getConfiguration() {
	return std::move(std::make_unique<Configuration>(*this));
}
GammaModification::Configuration::Configuration(GammaModification &transformation) {
	GtkWidget *table = gtk_table_new(2, 2, false);
	GtkWidget *widget;
	int table_y = 0;

	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Value:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	value = widget = gtk_spin_button_new_with_range(0, 100, 0.01);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), transformation.value);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;

	main = table;
	gtk_widget_show_all(main);

	g_object_ref(main);
}
GammaModification::Configuration::~Configuration() {
	g_object_unref(main);
}
GtkWidget *GammaModification::Configuration::getWidget() {
	return main;
}
void GammaModification::Configuration::apply(dynv::Map &system) {
	system.set("value", static_cast<float>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(value))));
}
}
