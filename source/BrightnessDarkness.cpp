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

#include "BrightnessDarkness.h"
#include "IColorSource.h"
#include "ColorSourceManager.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "gtk/Range2D.h"
#include "gtk/LayoutPreview.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "EventBus.h"
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
#include <optional>
using namespace layout;

struct BrightnessDarknessArgs;
struct BrightnessDarknessColorNameAssigner: public ToolColorNameAssigner {
	BrightnessDarknessColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << color_names_get(m_gs.getColorNames(), &colorObject.getColor(), false) << " " << _("brightness darkness") << " " << m_ident;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_ident;
};
struct BrightnessDarknessArgs: public IColorSource, public IEventHandler {
	Color color;
	GtkWidget *main, *statusBar, *brightnessDarkness, *layoutView;
	System *layoutSystem;
	dynv::Ref options;
	GlobalState &gs;
	BrightnessDarknessArgs(GlobalState &gs, const dynv::Ref &options):
		options(options),
		gs(gs),
		editable(*this) {
		statusBar = gs.getStatusBar();
		gs.eventBus().subscribe(EventType::displayFiltersUpdate, *this);
	}
	virtual ~BrightnessDarknessArgs() {
		if (layoutSystem)
			System::unref(layoutSystem);
		layoutSystem = nullptr;
		gtk_widget_destroy(main);
		gs.eventBus().unsubscribe(*this);
	}
	virtual std::string_view name() const {
		return "brightness_darkness";
	}
	virtual void activate() override {
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "empty"), "");
	}
	virtual void deactivate() override {
		options->set("color", color);
		update(true);
	}
	virtual GtkWidget *getWidget() override {
		return main;
	}
	void setTransformationChain() {
		auto chain = gs.getTransformationChain();
		gtk_layout_preview_set_transformation_chain(GTK_LAYOUT_PREVIEW(layoutView), chain);
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::displayFiltersUpdate:
			setTransformationChain();
			break;
		case EventType::colorDictionaryUpdate:
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
			break;
		}
	}
	void addToPalette() {
		if (!isSelected())
			return;
		color_list_add_color_object(gs.getColorList(), getColor(), true);
	}
	void addAllToPalette() {
		if (layoutSystem == nullptr)
			return;
		BrightnessDarknessColorNameAssigner nameAssigner(gs);
		for (auto &style: layoutSystem->styles()) {
			ColorObject colorObject(style->color);
			nameAssigner.assign(colorObject, style->label);
			color_list_add_color_object(gs.getColorList(), colorObject, true);
		}
	}
	virtual void setColor(const ColorObject &colorObject) override {
		color = colorObject.getColor();
		gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(layoutView), &color, "main");
		update(false);
	}
	virtual void setNthColor(size_t index, const ColorObject &colorObject) override {
	}
	ColorObject colorObject;
	virtual const ColorObject &getColor() override {
		Color color;
		if (gtk_layout_preview_get_current_color(GTK_LAYOUT_PREVIEW(layoutView), &color) != 0)
			return colorObject;
		Style *style = 0;
		if (gtk_layout_preview_get_current_style(GTK_LAYOUT_PREVIEW(layoutView), &style) != 0)
			return colorObject;
		colorObject.setColor(color);
		BrightnessDarknessColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(colorObject, style->label);
		return colorObject;
	}
	virtual const ColorObject &getNthColor(size_t index) override {
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
			for (auto &style: layoutSystem->styles()) {
				ColorObject colorObject(style->color);
				nameAssigner.assign(colorObject, style->label);
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
			box = layoutSystem->getNamedBox(name);
			if (box && box->style) {
				box->style->color = r;
			}
			hsl = hslOriginal;
			hsl.hsl.lightness = math::mix(hsl.hsl.lightness, math::mix(hsl.hsl.lightness, 0.0f, darkness), i / 4.0f);
			r = hsl.hslToRgb();
			name = format('c', i);
			box = layoutSystem->getNamedBox(name);
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
		Editable(BrightnessDarknessArgs &args):
			args(args) {
		}
		virtual ~Editable() = default;
		virtual void addToPalette(const ColorObject &) override {
			args.addToPalette();
		}
		virtual void addAllToPalette() override {
			args.addAllToPalette();
		}
		virtual void setColor(const ColorObject &colorObject) override {
			args.setColor(colorObject);
		}
		virtual void setColorAt(const ColorObject &colorObject, int x, int y) override {
			args.setColor(colorObject);
		}
		virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
			args.setColor(colorObjects[0]);
		}
		virtual const ColorObject &getColor() override {
			return args.getColor();
		}
		virtual std::vector<ColorObject> getColors(bool selected) override {
			return args.getColors(selected);
		}
		virtual bool isEditable() override {
			return args.isEditable();
		}
		virtual bool hasColor() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return args.isSelected();
		}
		virtual bool testDropAt(int, int) override {
			return args.selectMain();
		}
		virtual void dropEnd(bool move) override {
		}
	private:
		BrightnessDarknessArgs &args;
	};
	std::optional<Editable> editable;
};
static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, BrightnessDarknessArgs *args) {
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		args->addToPalette();
		return true;
	}
	return false;
}
static std::unique_ptr<IColorSource> build(GlobalState &gs, const dynv::Ref &options) {
	auto args = std::make_unique<BrightnessDarknessArgs>(gs, options);
	args->layoutSystem = nullptr;
	GtkWidget *hbox, *widget;
	hbox = gtk_hbox_new(false, 0);
	args->brightnessDarkness = widget = gtk_range_2d_new();
	gtk_range_2d_set_values(GTK_RANGE_2D(widget), options->getFloat("brightness", 0.5f), options->getFloat("darkness", 0.5f));
	gtk_range_2d_set_axis(GTK_RANGE_2D(widget), _("Brightness"), _("Darkness"));
	g_signal_connect(G_OBJECT(widget), "values_changed", G_CALLBACK(BrightnessDarknessArgs::onChange), args.get());
	gtk_box_pack_start(GTK_BOX(hbox), widget, false, false, 0);
	args->layoutView = widget = gtk_layout_preview_new();
	gtk_layout_preview_set_fill(GTK_LAYOUT_PREVIEW(widget), true);
	g_signal_connect_after(G_OBJECT(widget), "button-press-event", G_CALLBACK(onButtonPress), args.get());
	StandardEventHandler::forWidget(widget, &args->gs, &*args->editable);
	StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable);
	gtk_box_pack_start(GTK_BOX(hbox), widget, true, true, 0);
	auto layout = gs.layouts().byName("std_layout_brightness_darkness");
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
	args->setTransformationChain();
	return args;
}
void registerBrightnessDarkness(ColorSourceManager &csm) {
	csm.add("brightness_darkness", _("Brightness Darkness"), RegistrationFlags::none, GDK_KEY_d, build);
}
