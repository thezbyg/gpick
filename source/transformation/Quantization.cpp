/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
 * All rights reserved.
 * Copyright (c) 2012, David Gowers (Portions regarding adaptation of GammaModification.(cpp|h) to quantise colors instead.)
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

#include "Quantization.h"
#include "../MathUtil.h"
#include "../uiUtilities.h"
#include "../Internationalisation.h"
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <boost/math/special_functions/round.hpp>

namespace transformation {

static const char * transformation_name = "quantization";

const char *Quantization::getName()
{
	return transformation_name;
}

const char *Quantization::getReadableName()
{
	return _("Quantization");
}

void Quantization::apply(Color *input, Color *output)
{
	if (clip_top) { 
		float max_intensity = (value - 1) / value;
		output->rgb.red = MIN(max_intensity, boost::math::round(input->rgb.red * value) / value);
		output->rgb.green = MIN(max_intensity, boost::math::round(input->rgb.green * value) / value);
		output->rgb.blue = MIN(max_intensity, boost::math::round(input->rgb.blue * value) / value);
	}else{
		float actualmax = value - 1;
		output->rgb.red = boost::math::round(input->rgb.red * actualmax) / actualmax;
		output->rgb.green = boost::math::round(input->rgb.green * actualmax) / actualmax;
		output->rgb.blue = boost::math::round(input->rgb.blue * actualmax) / actualmax;
	}
}

Quantization::Quantization():Transformation(transformation_name, getReadableName())
{
	value = 16;
}

Quantization::Quantization(float value_):Transformation(transformation_name, getReadableName())
{
	value = value_;
}

Quantization::~Quantization()
{
}

void Quantization::serialize(struct dynvSystem *dynv)
{
	dynv_set_float(dynv, "value", value);
	dynv_set_bool(dynv, "clip-top", clip_top);
	Transformation::serialize(dynv);
}

void Quantization::deserialize(struct dynvSystem *dynv)
{
	value = dynv_get_float_wd(dynv, "value", 16);
	clip_top = dynv_get_bool_wd(dynv, "clip-top", 0);
}

boost::shared_ptr<Configuration> Quantization::getConfig(){
	boost::shared_ptr<QuantizationConfig> config = boost::shared_ptr<QuantizationConfig>(new QuantizationConfig(*this));
	return config;
}

QuantizationConfig::QuantizationConfig(Quantization &transformation){
	GtkWidget *table = gtk_table_new(2, 3, false);
	GtkWidget *widget;
	int table_y = 0;

	table_y=0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Value:"),0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_FILL, 5, 5);
	value = widget = gtk_spin_button_new_with_range(2, 256, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), transformation.value);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	clip_top = widget = gtk_check_button_new_with_label("Clip top-end");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), transformation.clip_top);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);

	main = table;
	gtk_widget_show_all(main);

	g_object_ref(main);
}

QuantizationConfig::~QuantizationConfig(){
	g_object_unref(main);
}

GtkWidget* QuantizationConfig::getWidget(){
	return main;
}

void QuantizationConfig::applyConfig(dynvSystem *dynv){
	dynv_set_float(dynv, "value", gtk_spin_button_get_value(GTK_SPIN_BUTTON(value)));
	dynv_set_bool(dynv, "clip-top", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(clip_top)));
}

}

