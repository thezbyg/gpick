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

#include "ColorMixer.h"
#include "ColorObject.h"
#include "ColorSource.h"
#include "ColorSourceManager.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "gtk/ColorWidget.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IMenuExtension.h"
#include "common/Format.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>
#include <cmath>

const int Rows = 5;
enum class Mode {
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
	ColorMixerColorNameAssigner(GlobalState *gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject *colorObject, const Color *color, const char *ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject, color);
	}
	virtual std::string getToolSpecificName(ColorObject *colorObject, const Color *color) {
		m_stream.str("");
		m_stream << color_names_get(m_gs->getColorNames(), color, false) << " " << _("color mixer") << " " << m_ident;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	const char *m_ident;
};
struct ColorMixerArgs;
struct ColorMixerArgs {
	ColorSource source;
	GtkWidget *main, *statusBar, *secondaryColor, *opacityRange, *lastFocusedColor, *colorPreviews;
	const Type *mixerType;
	struct {
		GtkWidget *input;
		GtkWidget *output;
	} rows[Rows];
	dynv::Ref options;
	GlobalState *gs;
	void addToPalette() {
		color_list_add_color_object(gs->getColorList(), getColor(), true);
	}
	void addToPalette(ColorMixerColorNameAssigner &nameAssigner, Color &color, GtkWidget *widget) {
		gtk_color_get_color(GTK_COLOR(widget), &color);
		colorObject.setColor(color);
		auto widgetName = identifyColorWidget(widget);
		nameAssigner.assign(&colorObject, &color, widgetName.c_str());
		color_list_add_color_object(gs->getColorList(), colorObject, true);
	}
	void addAllToPalette() {
		ColorMixerColorNameAssigner nameAssigner(gs);
		Color color;
		for (int i = 0; i < Rows; ++i)
			addToPalette(nameAssigner, color, rows[i].input);
		addToPalette(nameAssigner, color, secondaryColor);
		for (int i = 0; i < Rows; ++i)
			addToPalette(nameAssigner, color, rows[i].output);
	}
	void setColor(const ColorObject &colorObject) {
		gtk_color_set_color(GTK_COLOR(lastFocusedColor), colorObject.getColor());
		update();
	}
	ColorObject colorObject;
	const ColorObject &getColor() {
		Color color;
		gtk_color_get_color(GTK_COLOR(lastFocusedColor), &color);
		auto widgetName = identifyColorWidget(lastFocusedColor);
		colorObject.setColor(color);
		ColorMixerColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(&colorObject, &color, widgetName.c_str());
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
			gtk_color_set_color(GTK_COLOR(rows[i].output), r);
		}
	}
	struct Editable: IEditableColorsUI, IMenuExtension {
		Editable(ColorMixerArgs *args, GtkWidget *widget):
			args(args),
			widget(widget) {
		}
		virtual ~Editable() = default;
		virtual void addToPalette(const ColorObject &) override {
			args->setActiveWidget(widget);
			args->addToPalette();
		}
		virtual void addAllToPalette() override {
			args->addAllToPalette();
		}
		virtual void setColor(const ColorObject &colorObject) override {
			args->setActiveWidget(widget);
			args->setColor(colorObject);
		}
		virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
			args->setActiveWidget(widget);
			args->setColor(colorObjects[0]);
		}
		virtual const ColorObject &getColor() override {
			args->setActiveWidget(widget);
			return args->getColor();
		}
		virtual std::vector<ColorObject> getColors(bool selected) override {
			std::vector<ColorObject> colors;
			colors.push_back(getColor());
			return colors;
		}
		virtual bool isEditable() override {
			args->setActiveWidget(widget);
			return args->isEditable();
		}
		virtual bool hasColor() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return true;
		}
		virtual void extendMenu(GtkWidget *menu, Position position) {
			if (position != Position::end || !isEditable())
				return;
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			GSList *group = nullptr;
			for (size_t i = 0; i < sizeof(types) / sizeof(Type); i++) {
				auto item = gtk_radio_menu_item_new_with_label(group, _(types[i].name));
				group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
				if (args->mixerType == &types[i])
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
				g_object_set_data(G_OBJECT(item), "color_mixer_type", const_cast<Type *>(&types[i]));
				g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(ColorMixerArgs::onModeChange), args);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			}
		}
	private:
		ColorMixerArgs *args;
		GtkWidget *widget;
	};
	boost::optional<Editable> editable[1 + Rows * 2];
};
static int destroy(ColorMixerArgs *args) {
	Color c;
	char tmp[32];
	args->options->set("mixer_type", args->mixerType->id);
	for (int i = 0; i < Rows; ++i) {
		sprintf(tmp, "color%d", i);
		gtk_color_get_color(GTK_COLOR(args->rows[i].input), &c);
		args->options->set(tmp, c);
	}
	gtk_color_get_color(GTK_COLOR(args->secondaryColor), &c);
	args->options->set("secondary_color", c);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}
static int getColor(ColorMixerArgs *args, ColorObject **color) {
	auto colorObject = args->getColor();
	*color = colorObject.copy();
	return 0;
}
static int setColor(ColorMixerArgs *args, ColorObject *colorObject) {
	args->setColor(*colorObject);
	return 0;
}
static int activate(ColorMixerArgs *args) {
	auto chain = args->gs->getTransformationChain();
	gtk_color_set_transformation_chain(GTK_COLOR(args->secondaryColor), chain);
	for (int i = 0; i < Rows; ++i) {
		gtk_color_set_transformation_chain(GTK_COLOR(args->rows[i].input), chain);
		gtk_color_set_transformation_chain(GTK_COLOR(args->rows[i].output), chain);
	}
	gtk_statusbar_push(GTK_STATUSBAR(args->statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusBar), "empty"), "");
	return 0;
}
static int deactivate(ColorMixerArgs *args) {
	args->update(true);
	return 0;
}
static ColorSource *source_implement(ColorSource *source, GlobalState *gs, const dynv::Ref &options) {
	auto *args = new ColorMixerArgs;
	args->options = options;
	args->statusBar = gs->getStatusBar();
	args->gs = gs;
	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *))destroy;
	args->source.get_color = (int (*)(ColorSource *, ColorObject **))getColor;
	args->source.set_color = (int (*)(ColorSource *, ColorObject *))setColor;
	args->source.deactivate = (int (*)(ColorSource *))deactivate;
	args->source.activate = (int (*)(ColorSource *))activate;
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
	g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(ColorMixerArgs::onColorActivate), args);
	g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(ColorMixerArgs::onFocusEvent), args);
	args->editable[0] = ColorMixerArgs::Editable(args, widget);
	StandardEventHandler::forWidget(widget, args->gs, &*args->editable[0]);
	StandardDragDropHandler::forWidget(widget, args->gs, &*args->editable[0]);
	gtk_widget_set_size_request(widget, 50, 50);
	for (intptr_t i = 0; i < Rows; ++i) {
		for (intptr_t j = 0; j < 2; ++j) {
			widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_color_set_roundness(GTK_COLOR(widget), 5);
			gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, j * 2, j * 2 + 1, i, i + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
			if (j) {
				args->rows[i].output = widget;
			} else {
				args->rows[i].input = widget;
			}
			g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(ColorMixerArgs::onColorActivate), args);
			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(ColorMixerArgs::onFocusEvent), args);
			gtk_widget_set_size_request(widget, 30, 30);
			args->editable[1 + i + j * Rows] = ColorMixerArgs::Editable(args, widget);
			StandardEventHandler::forWidget(widget, args->gs, &*args->editable[1 + i + j * Rows]);
			if (j == 0) {
				StandardDragDropHandler::forWidget(widget, args->gs, &*args->editable[1 + i + j * Rows]);
			} else {
				StandardDragDropHandler::forWidget(widget, args->gs, &*args->editable[1 + i + j * Rows], StandardDragDropHandler::Options().allowDrop(false));
			}
		}
	}
	Color c = { 0.5f };
	char tmp[32];
	auto type_name = options->getString("mixer_type", "normal");
	for (uint32_t j = 0; j < sizeof(types) / sizeof(Type); j++) {
		if (types[j].id == type_name) {
			args->mixerType = &types[j];
			break;
		}
	}
	for (gint i = 0; i < Rows; ++i) {
		sprintf(tmp, "color%d", i);
		gtk_color_set_color(GTK_COLOR(args->rows[i].input), options->getColor(tmp, c));
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
	g_signal_connect(G_OBJECT(args->opacityRange), "value-changed", G_CALLBACK(ColorMixerArgs::onChange), args);
	gtk_table_attach(GTK_TABLE(table), args->opacityRange, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;
	gtk_widget_show_all(hbox);
	args->update();
	args->main = hbox;
	args->source.widget = hbox;
	return (ColorSource *)args;
}
int color_mixer_source_register(ColorSourceManager *csm) {
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "color_mixer", _("Color mixer"));
	color_source->implement = source_implement;
	color_source->default_accelerator = GDK_KEY_m;
	color_source_manager_add_source(csm, color_source);
	return 0;
}
