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

#include "ColorMixer.h"
#include "ColorObject.h"
#include "IColorSource.h"
#include "ColorSourceManager.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "gtk/ColorWidget.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "EventBus.h"
#include "Names.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IMenuExtension.h"
#include "common/Format.h"
#include "common/Guard.h"
#include "common/Match.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>
#include <cmath>

namespace {
const int Rows = 5;
enum struct Mode {
	normal = 1,
	multiply,
	difference,
	add,
	hue,
	saturation,
	lightness,
};
struct Type {
	const char *id;
	const char *name;
	Mode mode;
};
const Type types[] = {
	{ "normal", N_("Normal"), Mode::normal },
	{ "multiply", N_("Multiply"), Mode::multiply },
	{ "add", N_("Add"), Mode::add },
	{ "difference", N_("Difference"), Mode::difference },
	{ "hue", N_("Hue"), Mode::hue },
	{ "saturation", N_("Saturation"), Mode::saturation },
	{ "lightness", N_("Lightness"), Mode::lightness },
};
struct ColorMixerColorNameAssigner: public ToolColorNameAssigner {
	ColorMixerColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << m_gs.names().get(colorObject.getColor()) << " " << _("color mixer") << " " << m_ident;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_ident;
};
}
struct ColorMixerArgs: public IColorSource, public IEventHandler {
	GtkWidget *main, *statusBar, *secondaryColor, *opacityRange, *lastFocusedColor, *colorPreviews;
	const Type *mixerType;
	struct {
		GtkWidget *input;
		GtkWidget *output;
	} rows[Rows];
	dynv::Ref options;
	GlobalState &gs;
	ColorMixerArgs(GlobalState &gs, const dynv::Ref &options):
		options(options),
		gs(gs) {
		statusBar = gs.getStatusBar();
		gs.eventBus().subscribe(EventType::displayFiltersUpdate, *this);
	}
	virtual ~ColorMixerArgs() {
		Color c;
		char tmp[32];
		options->set("mixer_type", mixerType->id);
		for (int i = 0; i < Rows; ++i) {
			sprintf(tmp, "color%d", i);
			gtk_color_get_color(GTK_COLOR(rows[i].input), &c);
			options->set(tmp, c);
		}
		gtk_color_get_color(GTK_COLOR(secondaryColor), &c);
		options->set("secondary_color", c);
		gtk_widget_destroy(main);
		gs.eventBus().unsubscribe(*this);
	}
	virtual std::string_view name() const override {
		return "color_mixer";
	}
	virtual void activate() override {
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "empty"), "");
	}
	virtual void deactivate() override {
		update(true);
	}
	virtual GtkWidget *getWidget() override {
		return main;
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::displayFiltersUpdate:
			gtk_widget_queue_draw(secondaryColor);
			for (int i = 0; i < Rows; ++i) {
				gtk_widget_queue_draw(rows[i].input);
				gtk_widget_queue_draw(rows[i].output);
			}
			break;
		case EventType::colorDictionaryUpdate:
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
		case EventType::paletteChanged:
			break;
		}
	}
	void addToPalette() {
		gs.colorList().add(getColor());
	}
	void addToPalette(ColorMixerColorNameAssigner &nameAssigner, Color &color, GtkWidget *widget) {
		gtk_color_get_color(GTK_COLOR(widget), &color);
		colorObject.setColor(color);
		auto widgetName = identifyColorWidget(widget);
		nameAssigner.assign(colorObject, widgetName);
		gs.colorList().add(colorObject);
	}
	void addAllToPalette() {
		common::Guard colorListGuard = gs.colorList().changeGuard();
		ColorMixerColorNameAssigner nameAssigner(gs);
		Color color;
		for (int i = 0; i < Rows; ++i)
			addToPalette(nameAssigner, color, rows[i].input);
		addToPalette(nameAssigner, color, secondaryColor);
		for (int i = 0; i < Rows; ++i)
			addToPalette(nameAssigner, color, rows[i].output);
	}
	virtual void setColor(const ColorObject &colorObject) override {
		gtk_color_set_color(GTK_COLOR(lastFocusedColor), colorObject.getColor());
		update();
	}
	virtual void setNthColor(size_t index, const ColorObject &colorObject) override {
	}
	ColorObject colorObject;
	virtual const ColorObject &getColor() override {
		Color color;
		gtk_color_get_color(GTK_COLOR(lastFocusedColor), &color);
		auto widgetName = identifyColorWidget(lastFocusedColor);
		colorObject.setColor(color);
		ColorMixerColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(colorObject, widgetName);
		return colorObject;
	}
	virtual const ColorObject &getNthColor(size_t index) override {
		return colorObject;
	}
	std::string identifyColorWidget(GtkWidget *widget) {
		if (secondaryColor == widget) {
			return _("secondary");
		} else
			for (int i = 0; i < Rows; ++i) {
				if (rows[i].input == widget) {
					return common::format(_("primary {}"), i + 1);
				} else if (rows[i].output == widget) {
					return common::format(_("result {}"), i + 1);
				}
			}
		return "unknown";
	}
	static gboolean onFocusEvent(GtkWidget *widget, GdkEventFocus *, ColorMixerArgs *args) {
		args->lastFocusedColor = widget;
		return false;
	}
	void setActiveWidget(GtkWidget *widget) {
		lastFocusedColor = widget;
	}
	void setMode(const Type *type) {
		mixerType = type;
		gtk_color_set_text(GTK_COLOR(secondaryColor), _(mixerType->name));
		update();
	}
	static void onModeChange(GtkWidget *widget, ColorMixerArgs *args) {
		if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
			return;
		return args->setMode(static_cast<const Type *>(g_object_get_data(G_OBJECT(widget), "color_mixer_type")));
	}
	static void onColorActivate(GtkWidget *, ColorMixerArgs *args) {
		args->addToPalette();
	}
	static void onChange(GtkWidget *, ColorMixerArgs *args) {
		args->update();
	}
	bool isEditable() {
		if (lastFocusedColor == secondaryColor)
			return true;
		for (int i = 0; i < Rows; ++i)
			if (lastFocusedColor == rows[i].input)
				return true;
		return false;
	}
	void update(bool saveSettings = false) {
		float opacity = static_cast<float>(gtk_range_get_value(GTK_RANGE(opacityRange)));
		if (saveSettings) {
			options->set("opacity", opacity);
		}
		Color color, color2, r, hsv1, hsv2, hsl1, hsl2;
		gtk_color_get_color(GTK_COLOR(secondaryColor), &color2);
		for (int i = 0; i < Rows; ++i) {
			gtk_color_get_color(GTK_COLOR(rows[i].input), &color);
			switch (mixerType->mode) {
			case Mode::normal:
				r = math::mix(color.linearRgb(), color2.linearRgb(), opacity / 100.0f).nonLinearRgbInplace();
				break;
			case Mode::multiply:
				r = math::mix(color.linearRgb(), color.linearRgb() * color2.linearRgb(), opacity / 100.0f).nonLinearRgbInplace();
				break;
			case Mode::add:
				r = math::mix(color.linearRgb(), color.linearRgb() + color2.linearRgb(), opacity / 100.0f).nonLinearRgbInplace();
				break;
			case Mode::difference:
				r = math::mix(color.linearRgb(), (color.linearRgb() - color2.linearRgb()).absoluteInplace(), opacity / 100.0f).nonLinearRgbInplace();
				break;
			case Mode::hue:
				hsv1 = color.rgbToHsv();
				hsv2 = color2.rgbToHsv();
				hsv1.hsv.hue = math::mix(hsv1.hsv.hue, hsv2.hsv.hue, opacity / 100.0f);
				r = hsv1.hsvToRgb();
				break;
			case Mode::saturation:
				hsv1 = color.rgbToHsv();
				hsv2 = color2.rgbToHsv();
				hsv1.hsv.saturation = math::mix(hsv1.hsv.saturation, hsv2.hsv.saturation, opacity / 100.0f);
				r = hsv1.hsvToRgb();
				break;
			case Mode::lightness:
				hsl1 = color.rgbToHsl();
				hsl2 = color2.rgbToHsl();
				hsl1.hsl.lightness = math::mix(hsl1.hsl.lightness, hsl2.hsl.lightness, opacity / 100.0f);
				r = hsl1.hslToRgb();
				break;
			}
			r.normalizeRgbInplace();
			gtk_color_set_color(GTK_COLOR(rows[i].output), r);
		}
	}
	struct Editable: IEditableColorsUI, IMenuExtension {
		Editable(ColorMixerArgs &args, GtkWidget *widget):
			args(args),
			widget(widget) {
		}
		virtual ~Editable() = default;
		virtual void addToPalette(const ColorObject &) override {
			args.setActiveWidget(widget);
			args.addToPalette();
		}
		virtual void addAllToPalette() override {
			args.addAllToPalette();
		}
		virtual void setColor(const ColorObject &colorObject) override {
			args.setActiveWidget(widget);
			args.setColor(colorObject);
		}
		virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
			args.setActiveWidget(widget);
			args.setColor(colorObjects[0]);
		}
		virtual const ColorObject &getColor() override {
			args.setActiveWidget(widget);
			return args.getColor();
		}
		virtual std::vector<ColorObject> getColors(bool selected) override {
			std::vector<ColorObject> colors;
			colors.push_back(getColor());
			return colors;
		}
		virtual bool isEditable() override {
			args.setActiveWidget(widget);
			return args.isEditable();
		}
		virtual bool hasColor() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return true;
		}
		virtual void extendMenu(GtkWidget *menu, Position position) override {
			if (position != Position::end || !isEditable())
				return;
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			GSList *group = nullptr;
			for (size_t i = 0; i < sizeof(types) / sizeof(Type); i++) {
				auto item = gtk_radio_menu_item_new_with_label(group, _(types[i].name));
				group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
				if (args.mixerType == &types[i])
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
				g_object_set_data(G_OBJECT(item), "color_mixer_type", const_cast<Type *>(&types[i]));
				g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(ColorMixerArgs::onModeChange), &args);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			}
		}
	private:
		ColorMixerArgs &args;
		GtkWidget *widget;
	};
	std::optional<Editable> editable[1 + Rows * 2];
};
static std::unique_ptr<IColorSource> build(GlobalState &gs, const dynv::Ref &options) {
	auto args = std::make_unique<ColorMixerArgs>(gs, options);
	GtkWidget *table, *vbox, *hbox, *widget, *hbox2;
	hbox = gtk_hbox_new(false, 0);
	vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 5);
	args->colorPreviews = gtk_table_new(Rows, 3, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->colorPreviews, true, true, 0);
	widget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, 1, 2, 0, Rows, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
	args->secondaryColor = widget;
	g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(ColorMixerArgs::onColorActivate), args.get());
	g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(ColorMixerArgs::onFocusEvent), args.get());
	args->editable[0].emplace(*args, widget);
	StandardEventHandler::forWidget(widget, &args->gs, &*args->editable[0]);
	StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[0]);
	gtk_widget_set_size_request(widget, 50, 50);
	auto *chain = &gs.transformationChain();
	gtk_color_set_transformation_chain(GTK_COLOR(args->secondaryColor), chain);
	for (intptr_t i = 0; i < Rows; ++i) {
		for (intptr_t j = 0; j < 2; ++j) {
			widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_color_set_roundness(GTK_COLOR(widget), 5);
			gtk_color_set_transformation_chain(GTK_COLOR(widget), chain);
			gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, j * 2, j * 2 + 1, i, i + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
			if (j) {
				args->rows[i].output = widget;
			} else {
				args->rows[i].input = widget;
			}
			g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(ColorMixerArgs::onColorActivate), args.get());
			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(ColorMixerArgs::onFocusEvent), args.get());
			gtk_widget_set_size_request(widget, 30, 30);
			args->editable[1 + i + j * Rows].emplace(*args, widget);
			StandardEventHandler::forWidget(widget, &args->gs, &*args->editable[1 + i + j * Rows]);
			if (j == 0) {
				StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[1 + i + j * Rows]);
			} else {
				StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[1 + i + j * Rows], StandardDragDropHandler::Options().allowDrop(false));
			}
		}
	}
	args->mixerType = &common::matchById(types, options->getString("mixer_type", "normal"));
	Color c = { 0.5f };
	for (int i = 0; i < Rows; ++i) {
		gtk_color_set_color(GTK_COLOR(args->rows[i].input), options->getColor(common::format("color{}", i), c));
	}
	gtk_color_set_color(GTK_COLOR(args->secondaryColor), options->getColor("secondary_color", c), _(args->mixerType->name));
	hbox2 = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 0);
	gint table_y;
	table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(hbox2), table, true, true, 0);
	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Opacity:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	args->opacityRange = gtk_hscale_new_with_range(1, 100, 1);
	gtk_range_set_value(GTK_RANGE(args->opacityRange), options->getFloat("opacity", 50));
	g_signal_connect(G_OBJECT(args->opacityRange), "value-changed", G_CALLBACK(ColorMixerArgs::onChange), args.get());
	gtk_table_attach(GTK_TABLE(table), args->opacityRange, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;
	gtk_widget_show_all(hbox);
	args->update();
	args->main = hbox;
	return args;
}
void registerColorMixer(ColorSourceManager &csm) {
	csm.add("color_mixer", _("Color mixer"), RegistrationFlags::none, GDK_KEY_m, build);
}
