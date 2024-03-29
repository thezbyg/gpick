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

#include "ClosestColors.h"
#include "ColorObject.h"
#include "IColorSource.h"
#include "ColorSourceManager.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "EventBus.h"
#include "gtk/ColorWidget.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IMenuExtension.h"
#include "common/Format.h"
#include "common/Guard.h"
#include "common/Match.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>

struct ClosestColorsArgs;
namespace {
enum struct ColorSource {
	colorDictionaries = 1,
	palette,
};
struct Type {
	const char *id;
	const char *name;
	ColorSource colorSource;
};
const Type types[] = {
	{ "color_dictionaries", N_("Color dictionaries"), ColorSource::colorDictionaries },
	{ "palette", N_("Palette"), ColorSource::palette },
};
struct ClosestColorsColorNameAssigner: public ToolColorNameAssigner {
	ClosestColorsColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << color_names_get(m_gs.getColorNames(), &colorObject.getColor(), false) << " " << _("closest color") << " " << m_ident;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_ident;
};
}
struct ClosestColorsArgs: public IColorSource, public IEventHandler {
	GtkWidget *main, *statusBar, *targetColor, *lastFocusedColor, *colorPreviews, *closestColors[9];
	dynv::Ref options;
	GlobalState &gs;
	const Type *type;
	ColorNames *paletteColorNames;
	ClosestColorsArgs(GlobalState &gs, const dynv::Ref &options):
		options(options),
		gs(gs),
		type(nullptr),
		editable(*this) {
		statusBar = gs.getStatusBar();
		paletteColorNames = color_names_new();
		gs.eventBus().subscribe(EventType::displayFiltersUpdate, *this);
		gs.eventBus().subscribe(EventType::colorDictionaryUpdate, *this);
		gs.eventBus().subscribe(EventType::paletteChanged, *this);
	}
	virtual ~ClosestColorsArgs() {
		Color color;
		gtk_color_get_color(GTK_COLOR(targetColor), &color);
		options->set("color", color);
		options->set("type", type->id);
		color_names_destroy(paletteColorNames);
		gtk_widget_destroy(main);
		gs.eventBus().unsubscribe(*this);
	}
	virtual std::string_view name() const override {
		return "closest_colors";
	}
	virtual void activate() override {
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "empty"), "");
	}
	virtual void deactivate() override {
	}
	virtual GtkWidget *getWidget() override {
		return main;
	}
	void setTransformationChain() {
		auto chain = gs.getTransformationChain();
		gtk_color_set_transformation_chain(GTK_COLOR(targetColor), chain);
		for (int i = 0; i < 9; ++i) {
			gtk_color_set_transformation_chain(GTK_COLOR(closestColors[i]), chain);
		}
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::colorDictionaryUpdate:
			update();
			break;
		case EventType::displayFiltersUpdate:
			setTransformationChain();
			break;
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
			break;
		case EventType::paletteChanged:
			updatePaletteColorNames();
			update();
			break;
		}
	}
	void addToPalette() {
		gs.colorList().add(getColor());
	}
	void addAllToPalette() {
		common::Guard colorListGuard = gs.colorList().changeGuard();
		ClosestColorsColorNameAssigner nameAssigner(gs);
		Color color;
		gtk_color_get_color(GTK_COLOR(targetColor), &color);
		colorObject.setColor(color);
		auto widgetName = identifyColorWidget(targetColor);
		nameAssigner.assign(colorObject, widgetName);
		gs.colorList().add(colorObject);
		for (int i = 0; i < 9; ++i) {
			gtk_color_get_color(GTK_COLOR(closestColors[i]), &color);
			colorObject.setColor(color);
			widgetName = identifyColorWidget(closestColors[i]);
			nameAssigner.assign(colorObject, widgetName);
			gs.colorList().add(colorObject);
		}
	}
	virtual void setColor(const ColorObject &colorObject) override {
		gtk_color_set_color(GTK_COLOR(targetColor), colorObject.getColor());
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
		ClosestColorsColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(colorObject, widgetName);
		return colorObject;
	}
	virtual const ColorObject &getNthColor(size_t index) override {
		return colorObject;
	}
	std::string identifyColorWidget(GtkWidget *widget) {
		if (targetColor == widget)
			return _("target");
		for (int i = 0; i < 9; ++i) {
			if (closestColors[i] == widget) {
				return common::format(_("match {}"), i + 1);
			}
		}
		return "unknown";
	}
	void update() {
		Color color;
		gtk_color_get_color(GTK_COLOR(targetColor), &color);
		std::vector<std::pair<const char *, Color>> colors;
		if (type->colorSource == ColorSource::palette) {
			color_names_find_nearest(paletteColorNames, color, 9, colors);
		} else {
			color_names_find_nearest(gs.getColorNames(), color, 9, colors);
		}
		for (size_t i = 0; i < 9; ++i) {
			if (i < colors.size()) {
				colors[i].second.alpha = color.alpha;
				gtk_color_set_color(GTK_COLOR(closestColors[i]), &colors[i].second, colors[i].first);
				gtk_widget_set_sensitive(closestColors[i], true);
			} else {
				gtk_widget_set_sensitive(closestColors[i], false);
			}
		}
	}
	void updatePaletteColorNames() {
		color_names_clear(paletteColorNames);
		color_names_load_from_list(paletteColorNames, gs.colorList());
	}
	bool isEditable() {
		return lastFocusedColor == targetColor;
	}
	static gboolean onFocusEvent(GtkWidget *widget, GdkEventFocus *, ClosestColorsArgs *args) {
		args->lastFocusedColor = widget;
		return false;
	}
	static void onColorActivate(GtkWidget *, ClosestColorsArgs *args) {
		args->addToPalette();
	}
	void setType(const Type *type) {
		this->type = type;
		if (type->colorSource == ColorSource::palette) {
			updatePaletteColorNames();
		} else {
			color_names_clear(paletteColorNames);
		}
	}
	static void onTypeChange(GtkWidget *widget, ClosestColorsArgs *args) {
		if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
			return;
		args->setType(static_cast<const Type *>(g_object_get_data(G_OBJECT(widget), "type")));
		args->update();
	}
	struct Editable: public IEditableColorsUI, IMenuExtension {
		Editable(ClosestColorsArgs &args):
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
		virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
			args.setColor(colorObjects[0]);
		}
		virtual const ColorObject &getColor() override {
			return args.getColor();
		}
		virtual std::vector<ColorObject> getColors(bool selected) override {
			std::vector<ColorObject> colors;
			colors.push_back(getColor());
			return colors;
		}
		virtual bool isEditable() override {
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
			for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
				auto item = gtk_radio_menu_item_new_with_label(group, _(types[i].name));
				group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
				if (args.type == &types[i])
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
				g_object_set_data(G_OBJECT(item), "type", const_cast<Type *>(&types[i]));
				g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(ClosestColorsArgs::onTypeChange), &args);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			}
		}
	private:
		ClosestColorsArgs &args;
	};
	std::optional<Editable> editable;
};
std::unique_ptr<IColorSource> build(GlobalState &gs, const dynv::Ref &options) {
	auto args = std::make_unique<ClosestColorsArgs>(gs, options);
	GtkWidget *vbox, *hbox, *widget;
	hbox = gtk_hbox_new(false, 0);
	vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 5);
	args->colorPreviews = gtk_table_new(3, 3, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->colorPreviews, true, true, 0);
	widget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, 0, 3, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
	args->targetColor = widget;
	g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(ClosestColorsArgs::onColorActivate), args.get());
	g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(ClosestColorsArgs::onFocusEvent), args.get());
	StandardEventHandler::forWidget(widget, &args->gs, &*args->editable);
	StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable);
	gtk_widget_set_size_request(widget, 30, 30);

	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_color_set_roundness(GTK_COLOR(widget), 5);
			gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, i, i + 1, j + 1, j + 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
			args->closestColors[i + j * 3] = widget;
			g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(ClosestColorsArgs::onColorActivate), args.get());
			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(ClosestColorsArgs::onFocusEvent), args.get());
			gtk_widget_set_size_request(widget, 30, 30);
			StandardEventHandler::forWidget(widget, &args->gs, &*args->editable);
			StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable, StandardDragDropHandler::Options().allowDrop(false));
		}
	}
	gtk_color_set_color(GTK_COLOR(args->targetColor), options->getColor("color", Color(0.5f)));
	args->setType(&common::matchById(types, options->getString("type", "color_dictionaries")));
	auto hbox2 = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 0);
	gtk_widget_show_all(hbox);
	args->update();
	args->main = hbox;
	args->setTransformationChain();
	return args;
}
void registerClosestColors(ColorSourceManager &csm) {
	csm.add("closest_colors", _("Closest colors"), RegistrationFlags::none, GDK_KEY_c, build);
}
