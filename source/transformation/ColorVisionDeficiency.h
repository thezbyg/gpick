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

#ifndef TRANSFORMATION_COLOR_VISION_DEFICIENCY_H_
#define TRANSFORMATION_COLOR_VISION_DEFICIENCY_H_
#include "Transformation.h"
#include <gtk/gtk.h>
namespace transformation {
struct ColorVisionDeficiency;
struct ColorVisionDeficiency: public Transformation {
	struct Configuration: public IConfiguration {
		Configuration(ColorVisionDeficiency &transformation);
		virtual ~Configuration() override;
		virtual GtkWidget *getWidget() override;
		virtual void apply(dynv::Map &options) override;
	private:
		GtkWidget *main;
		GtkWidget *info_bar;
		GtkWidget *info_label;
		GtkWidget *type;
		GtkWidget *strength;
		static void type_combobox_change_cb(GtkWidget *widget, Configuration *this_);
		static void info_label_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, Configuration *this_);
	};
	enum DeficiencyType {
		PROTANOMALY,
		DEUTERANOMALY,
		TRITANOMALY,
		PROTANOPIA,
		DEUTERANOPIA,
		TRITANOPIA,
		DEFICIENCY_TYPE_COUNT,
	};
	static const char *deficiency_type_string[];
	static const char *getId();
	static const char *getName();
	ColorVisionDeficiency();
	ColorVisionDeficiency(DeficiencyType type, float strength);
	virtual ~ColorVisionDeficiency() override;
	virtual void serialize(dynv::Map &system) override;
	virtual void deserialize(const dynv::Map &system) override;
	virtual std::unique_ptr<IConfiguration> getConfiguration() override;
	DeficiencyType typeFromString(const std::string &type_string);
private:
	float strength;
	DeficiencyType type;
	virtual void apply(Color *input, Color *output);
	friend struct Configuration;
};

}

#endif /* TRANSFORMATION_COLOR_VISION_DEFICIENCY_H_ */
