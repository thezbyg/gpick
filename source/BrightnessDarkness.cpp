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

#include "BrightnessDarkness.h"
#include "ColorSource.h"
#include "ColorSourceManager.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "gtk/Range2D.h"
#include "gtk/LayoutPreview.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include "layout/Layout.h"
#include "layout/Layouts.h"
#include "layout/Style.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IDroppableColorUI.h"
#include "common/Format.h"
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <sstream>
#include <boost/optional.hpp>
using namespace layout;

struct BrightnessDarknessArgs;
struct BrightnessDarknessColorNameAssigner: public ToolColorNameAssigner {
	BrightnessDarknessColorNameAssigner(GlobalState *gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject *colorObject, Color *color, const char *ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject, color);
	}
	virtual std::string getToolSpecificName(ColorObject *colorObject, const Color *color) {
		m_stream.str("");
		m_stream << color_names_get(m_gs->getColorNames(), color, false) << " " << _("brightness darkness") << " " << m_ident;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	const char *m_ident;
};
struct BrightnessDarknessArgs {
	ColorSource source;
	Color color;
	GtkWidget *main, *statusBar, *brightnessDarkness, *layoutView;
	System *layoutSystem;
	dynv::Ref options;
	GlobalState *gs;
	void addToPalette() {
		if (!isSelected())
			return;
		color_list_add_color_object(gs->getColorList(), getColor(), true);
	}
	void addAllToPalette() {
		if (layoutSystem == nullptr)
			return;
		BrightnessDarknessColorNameAssigner nameAssigner(gs);
		for (auto &style: layoutSystem->styles) {
			ColorObject colorObject(style->color);
			nameAssigner.assign(&colorObject, &style->color, style->label.c_str());
			color_list_add_color_object(gs->getColorList(), colorObject, true);
		}
	}
	void setColor(const ColorObject &colorObject) {
		color = colorObject.getColor();
		gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(layoutView), &color, "main");
		update(false);
	}
	ColorObject colorObject;
	const ColorObject &getColor() {
		Color color;
		if (gtk_layout_preview_get_current_color(GTK_LAYOUT_PREVIEW(layoutView), &color) != 0)
			return colorObject;
		Style *style = 0;
		if (gtk_layout_preview_get_current_style(GTK_LAYOUT_PREVIEW(layoutView), &style) != 0)
			return colorObject;
		colorObject.setColor(color);
		BrightnessDarknessColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(&colorObject, &color, style->label.c_str());
		return colorObject;
	}
	std::vector<ColorObject> getColors(bool selected) {
		if (layoutSystem == nullptr)
			return std::vector<ColorObject>();
		BrightnessDarknessColorNameAssigner nameAssigner(gs);
		std::vector<ColorObject> colors;
		if (selected) {
			auto colorObject = getColor();
			colors.push_back(colorObject);
		} else {
			for (auto &style: layoutSystem->styles) {
				ColorObject colorObject(style->color);
				nameAssigner.assign(&colorObject, &style->color, style->label.c_str());
				colors.push_back(colorObject);
			}
		}
		return colors;
	}
	bool selectMain() {
		gtk_layout_preview_set_focus_named(GTK_LAYOUT_PREVIEW(layoutView), "main");
		return gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(layoutView));
	}
	bool isSelected() {
		return gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(layoutView));
	}
	bool isEditable() {
		if (!gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(layoutView)))
			return false;
		return gtk_layout_preview_is_editable(GTK_LAYOUT_PREVIEW(layoutView));
	}
	static std::string format(char prefix, int index) {
		return prefix + common::as_string(index);
	}
	void update(bool saveSettings) {
		float brightness = static_cast<float>(gtk_range_2d_get_x(GTK_RANGE_2D(brightnessDarkness)));
		float darkness = static_cast<float>(gtk_range_2d_get_y(GTK_RANGE_2D(brightnessDarkness)));
		if (saveSettings) {
			options->set("brightness", brightness);
			options->set("darkness", brightness);
		}
		Color hslOriginal = color.rgbToHsl(), hsl, r;
		Box *box;
		std::string name;
		if (layoutSystem == nullptr)
			return;
		for (int i = 1; i <= 4; i++) {
			hsl = hslOriginal;
			hsl.hsl.lightness = math::mix(hsl.hsl.lightness, math::mix(hsl.hsl.lightness, 1.0f, brightness), i / 4.0f);
			r = hsl.hslToRgb();
			name = format('b', i);
			box = layoutSystem->GetNamedBox(name.c_str());
			if (box && box->style) {
				box->style->color = r;
			}
			hsl = hslOriginal;
			hsl.hsl.lightness = math::mix(hsl.hsl.lightness, math::mix(hsl.hsl.lightness, 0.0f, darkness), i / 4.0f);
			r = hsl.hslToRgb();
			name = format('c', i);
			box = layoutSystem->GetNamedBox(name.c_str());
			if (box && box->style) {
				box->style->color = r;
			}
		}
		gtk_widget_queue_draw(GTK_WIDGET(layoutView));
	}
	static void onChange(GtkWidget *, BrightnessDarknessArgs *args) {
		args->update(false);
	}
	struct Editable: public IEditableColorsUI, public IDroppableColorUI {
		Editable(BrightnessDarknessArgs *args):
			args(args) {
		}
		virtual ~Editable() = default;
		virtual void addToPalette(const ColorObject &) override {
			args->addToPalette();
		}
		virtual void addAllToPalette() override {
			args->addAllToPalette();
		}
		virtual void setColor(const ColorObject &colorObject) override {
			args->setColor(colorObject);
		}
		virtual void setColorAt(const ColorObject &colorObject, int x, int y) override {
			args->setColor(colorObject);
		}
		virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
			args->setColor(colorObjects[0]);
		}
		virtual const ColorObject &getColor() override {
			return args->getColor();
		}
		virtual std::vector<ColorObject> getColors(bool selected) override {
			return args->getColors(selected);
		}
		virtual bool isEditable() override {
			return args->isEditable();
		}
		virtual bool hasColor() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return args->isSelected();
		}
		virtual bool testDropAt(int, int) override {
			return args->selectMain();
		}
		virtual void dropEnd(bool move) override {
		}
	private:
		BrightnessDarknessArgs *args;
	};
	boost::optional<Editable> editable;
};
static int getColor(BrightnessDarknessArgs *args, ColorObject **color) {
	if (!args->isSelected())
		return -1;
	auto colorObject = args->getColor();
	*color = colorObject.copy();
	return 0;
}
static int setColor(BrightnessDarknessArgs *args, ColorObject *colorObject) {
	args->setColor(*colorObject);
	return 0;
}
static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, BrightnessDarknessArgs *args) {
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		args->addToPalette();
		return true;
	}
	return false;
}
static int destroy(BrightnessDarknessArgs *args) {
	if (args->layoutSystem) System::unref(args->layoutSystem);
	args->layoutSystem = nullptr;
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}
static int activate(BrightnessDarknessArgs *args) {
	auto chain = args->gs->getTransformationChain();
	gtk_layout_preview_set_transformation_chain(GTK_LAYOUT_PREVIEW(args->layoutView), chain);
	gtk_statusbar_push(GTK_STATUSBAR(args->statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusBar), "empty"), "");
	return 0;
}
static int deactivate(BrightnessDarknessArgs *args) {
	args->options->set("color", args->color);
	args->update(true);
	return 0;
}
static ColorSource *implement(ColorSource *source, GlobalState *gs, const dynv::Ref &options) {
	auto *args = new BrightnessDarknessArgs;
	args->editable = BrightnessDarknessArgs::Editable(args);
	args->options = options;
	args->statusBar = gs->getStatusBar();
	args->gs = gs;
	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source)) destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject **color)) getColor;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject *color)) setColor;
	args->source.deactivate = (int (*)(ColorSource *source)) deactivate;
	args->source.activate = (int (*)(ColorSource *source)) activate;
	args->layoutSystem = nullptr;
	GtkWidget *hbox, *widget;
	hbox = gtk_hbox_new(false, 0);
	args->brightnessDarkness = widget = gtk_range_2d_new();
	gtk_range_2d_set_values(GTK_RANGE_2D(widget), options->getFloat("brightness", 0.5f), options->getFloat("darkness", 0.5f));
	gtk_range_2d_set_axis(GTK_RANGE_2D(widget), _("Brightness"), _("Darkness"));
	g_signal_connect(G_OBJECT(widget), "values_changed", G_CALLBACK(BrightnessDarknessArgs::onChange), args);
	gtk_box_pack_start(GTK_BOX(hbox), widget, false, false, 0);
	args->layoutView = widget = gtk_layout_preview_new();
	g_signal_connect_after(G_OBJECT(widget), "button-press-event", G_CALLBACK(onButtonPress), args);
	StandardEventHandler::forWidget(widget, args->gs, &*args->editable);
	StandardDragDropHandler::forWidget(widget, args->gs, &*args->editable);
	gtk_box_pack_start(GTK_BOX(hbox), widget, false, false, 0);
	auto layout = gs->layouts().byName("std_layout_brightness_darkness");
	if (layout != nullptr) {
		System *layoutSystem = layout->build();
		gtk_layout_preview_set_system(GTK_LAYOUT_PREVIEW(args->layoutView), layoutSystem);
		if (args->layoutSystem) System::unref(args->layoutSystem);
		args->layoutSystem = layoutSystem;
	} else {
		if (args->layoutSystem) System::unref(args->layoutSystem);
		args->layoutSystem = nullptr;
	}
	args->color = options->getColor("color", Color(0.5f));
	gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(args->layoutView), &args->color, "main");
	args->update(false);
	gtk_widget_show_all(hbox);
	args->main = hbox;
	args->source.widget = hbox;
	return (ColorSource *)args;
}
int brightness_darkness_source_register(ColorSourceManager *csm) {
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "brightness_darkness", _("Brightness Darkness"));
	color_source->implement = implement;
	color_source->default_accelerator = GDK_KEY_d;
	color_source_manager_add_source(csm, color_source);
	return 0;
}
