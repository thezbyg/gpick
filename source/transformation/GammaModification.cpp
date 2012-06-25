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

#include "GammaModification.h"
#include "../MathUtil.h"
#include "../uiUtilities.h"
#include "../Internationalisation.h"
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

namespace transformation {

static const char * transformation_name = "gamma_modification";

const char *GammaModification::getName()
{
	return transformation_name;
}

const char *GammaModification::getReadableName()
{
	return _("Gamma modification");
}

void GammaModification::apply(Color *input, Color *output)
{
	Color linear_input, linear_output;
	color_rgb_get_linear(input, &linear_input);
	linear_output.rgb.red = pow(linear_input.rgb.red, value);
	linear_output.rgb.green= pow(linear_input.rgb.green, value);
	linear_output.rgb.blue = pow(linear_input.rgb.blue, value);
	color_linear_get_rgb(&linear_output, output);
	color_rgb_normalize(output);
}

GammaModification::GammaModification():Transformation(transformation_name, getReadableName())
{
	value = 1;
}

GammaModification::GammaModification(float value_):Transformation(transformation_name, getReadableName())
{
	value = value_;
}

GammaModification::~GammaModification()
{
}

void GammaModification::serialize(struct dynvSystem *dynv)
{
	dynv_set_float(dynv, "value", value);
	Transformation::serialize(dynv);
}

void GammaModification::deserialize(struct dynvSystem *dynv)
{
	value = dynv_get_float_wd(dynv, "value", 1);
}

boost::shared_ptr<Configuration> GammaModification::getConfig(){
	boost::shared_ptr<GammaModificationConfig> config = boost::shared_ptr<GammaModificationConfig>(new GammaModificationConfig(*this));
	return config;
}

GammaModificationConfig::GammaModificationConfig(GammaModification &transformation){
	GtkWidget *table = gtk_table_new(2, 2, false);
	GtkWidget *widget;
	int table_y = 0;

	table_y=0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Value:"),0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	value = widget = gtk_spin_button_new_with_range(0, 100, 0.01);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), transformation.value);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;

	main = table;
	gtk_widget_show_all(main);

	g_object_ref(main);
}

GammaModificationConfig::~GammaModificationConfig(){
	g_object_unref(main);
}

GtkWidget* GammaModificationConfig::getWidget(){
	return main;
}

void GammaModificationConfig::applyConfig(dynvSystem *dynv){
	dynv_set_float(dynv, "value", gtk_spin_button_get_value(GTK_SPIN_BUTTON(value)));
}

}

