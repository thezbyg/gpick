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

#include "Variations.h"
#include "ColorObject.h"
#include "IColorSource.h"
#include "ColorSourceManager.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "gtk/ColorWidget.h"
#include "Converter.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IMenuExtension.h"
#include "common/Format.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>

const int Rows = 5;
const int VariantWidgets = 8;
enum class Component {
	hslHue = 1,
	hslSaturation,
	hslLightness,
	labLightness,
	rgbRed,
	rgbGreen,
	rgbBlue,
};
struct Type {
	const char *id;
	const char *name;
	const char *symbol;
	Component component;
	float strengthMultiplier;
};
const Type types[] = {
	{ "rgb_red", N_("Red"), "R<span font='8' rise='8000'>RGB</span>", Component::rgbRed, 1 },
	{ "rgb_green", N_("Green"), "G<span font='8' rise='8000'>RGB</span>", Component::rgbGreen, 1 },
	{ "rgb_blue", N_("Blue"), "B<span font='8' rise='8000'>RGB</span>", Component::rgbBlue, 1 },
	{ "hsl_hue", N_("Hue"), "H<span font='8' rise='8000'>HSL</span>", Component::hslHue, 1 },
	{ "hsl_saturation", N_("Saturation"), "S<span font='8' rise='8000'>HSL</span>", Component::hslSaturation, 1 },
	{ "hsl_lightness", N_("Lightness"), "L<span font='8' rise='8000'>HSL</span>", Component::hslLightness, 1 },
	{ "lab_lightness", N_("Lightness (Lab)"), "L<span font='8' rise='8000'>Lab</span>", Component::labLightness, 1 },
};
struct VariationsColorNameAssigner: public ToolColorNameAssigner {
	VariationsColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << color_names_get(m_gs.getColorNames(), &colorObject.getColor(), false) << " " << _("variations") << " " << m_ident;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_ident;
};
struct VariationsArgs: public IColorSource {
	GtkWidget *main, *statusBar, *strengthRange, *lastFocusedColor, *colorPreviews, *allColors;
	struct {
		GtkWidget *primary;
		GtkWidget *variants[VariantWidgets];
		const Type *type;
	} rows[Rows];
	dynv::Ref options;
	GlobalState &gs;
	VariationsArgs(GlobalState &gs, const dynv::Ref &options):
		options(options),
		gs(gs) {
		statusBar = gs.getStatusBar();
	}
	~VariationsArgs() {
		Color color;
		char tmp[32];
		for (int i = 0; i < Rows; ++i) {
			sprintf(tmp, "type%d", i);
			options->set(tmp, rows[i].type->id);
			sprintf(tmp, "color%d", i);
			gtk_color_get_color(GTK_COLOR(rows[i].primary), &color);
			options->set(tmp, color);
		}
		gtk_color_get_color(GTK_COLOR(allColors), &color);
		options->set("all_colors", color);
		gtk_widget_destroy(main);
	}
	virtual std::string_view name() const {
		return "variations";
	}
	virtual void activate() override {
		auto chain = gs.getTransformationChain();
		gtk_color_set_transformation_chain(GTK_COLOR(allColors), chain);
		for (int i = 0; i < Rows; ++i) {
			gtk_color_set_transformation_chain(GTK_COLOR(rows[i].primary), chain);
			for (int j = 0; j < VariantWidgets; ++j) {
				gtk_color_set_transformation_chain(GTK_COLOR(rows[i].variants[j]), chain);
			}
		}
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "empty"), "");
	}
	virtual void deactivate() override {
		update(true);
	}
	virtual GtkWidget *getWidget() override {
		return main;
	}
	void addToPalette() {
		color_list_add_color_object(gs.getColorList(), getColor(), true);
	}
	void addToPalette(VariationsColorNameAssigner &nameAssigner, Color &color, GtkWidget *widget) {
		gtk_color_get_color(GTK_COLOR(widget), &color);
		colorObject.setColor(color);
		nameAssigner.assign(colorObject, identifyColorWidget(widget));
		color_list_add_color_object(gs.getColorList(), colorObject, true);
	}
	void addAllToPalette() {
		VariationsColorNameAssigner nameAssigner(gs);
		Color color;
		addToPalette(nameAssigner, color, allColors);
		for (int i = 0; i < Rows; ++i) {
			for (int j = 0; j < VariantWidgets; ++j) {
				if (j == VariantWidgets / 2)
					addToPalette(nameAssigner, color, rows[i].primary);
				addToPalette(nameAssigner, color, rows[i].variants[j]);
			}
		}
	}
	virtual void setColor(const ColorObject &colorObject) override {
		gtk_color_set_color(GTK_COLOR(lastFocusedColor), colorObject.getColor());
		if (lastFocusedColor == allColors) {
			for (int i = 0; i < Rows; ++i) {
				gtk_color_set_color(GTK_COLOR(rows[i].primary), colorObject.getColor());
			}
		}
		update();
	}
	virtual void setNthColor(size_t index, const ColorObject &colorObject) override {
	}
	ColorObject colorObject;
	virtual const ColorObject &getColor() override {
		Color color;
		gtk_color_get_color(GTK_COLOR(lastFocusedColor), &color);
		colorObject.setColor(color);
		VariationsColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(colorObject, identifyColorWidget(lastFocusedColor));
		return colorObject;
	}
	virtual const ColorObject &getNthColor(size_t index) override {
		return colorObject;
	}
	std::string identifyColorWidget(GtkWidget *widget) {
		if (allColors == widget) {
			return _("all colors");
		} else
			for (int i = 0; i < Rows; ++i) {
				if (rows[i].primary == widget) {
					return common::format(_("primary {}"), i + 1);
				}
				for (int j = 0; j < VariantWidgets; ++j) {
					if (rows[i].variants[j] == widget) {
						return common::format(_("result {} line {}"), j + 1, i + 1);
					}
				}
			}
		return "unknown";
	}
	bool isEditable() {
		if (lastFocusedColor == allColors)
			return true;
		for (int i = 0; i < Rows; ++i)
			if (lastFocusedColor == rows[i].primary)
				return true;
		return false;
	}
	bool isPrimary() {
		for (int i = 0; i < Rows; ++i) {
			if (rows[i].primary == lastFocusedColor)
				return true;
		}
		return false;
	}
	static gboolean onFocusEvent(GtkWidget *widget, GdkEventFocus *, VariationsArgs *args) {
		args->setActiveWidget(widget);
		return false;
	}
	void setActiveWidget(GtkWidget *widget) {
		lastFocusedColor = widget;
	}
	int getActiveRow() {
		for (int i = 0; i < Rows; ++i) {
			if (rows[i].primary == lastFocusedColor)
				return i;
			for (int j = 0; j < VariantWidgets; ++j) {
				if (rows[i].variants[j] == lastFocusedColor)
					return i;
			}
		}
		return 0;
	}
	static void onModeChange(GtkWidget *widget, VariationsArgs *args) {
		if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
			return;
		auto type = static_cast<const Type *>(g_object_get_data(G_OBJECT(widget), "variation_type"));
		args->rows[args->getActiveRow()].type = type;
		gtk_color_set_text(GTK_COLOR(args->rows[args->getActiveRow()].primary), type->symbol);
		args->update();
	}
	static void onColorActivate(GtkWidget *, VariationsArgs *args) {
		args->addToPalette();
	}
	static void onChange(GtkWidget *, VariationsArgs *args) {
		args->update();
	}
	void update(bool saveSettings = false) {
		float strength = static_cast<float>(gtk_range_get_value(GTK_RANGE(strengthRange)));
		if (saveSettings) {
			options->set("strength", strength);
		}
		Color color, rgb, hsl, lab, r, rgbModified, hslModified, labModified;
		for (int i = 0; i < Rows; ++i) {
			gtk_color_get_color(GTK_COLOR(rows[i].primary), &color);
			switch (rows[i].type->component) {
			case Component::rgbRed:
			case Component::rgbGreen:
			case Component::rgbBlue:
				rgb = color.linearRgb();
				break;
			case Component::hslHue:
			case Component::hslSaturation:
			case Component::hslLightness:
				hsl = color.rgbToHsl();
				break;
			case Component::labLightness:
				lab = color.rgbToLabD50();
				break;
			}
			for (int j = 0; j < VariantWidgets; ++j) {
				float position = rows[i].type->strengthMultiplier * strength * (j / static_cast<float>(VariantWidgets - 1) - 0.5f);
				switch (rows[i].type->component) {
				case Component::rgbRed:
					rgbModified = rgb;
					rgbModified.rgb.red = math::clamp(rgbModified.rgb.red + position / 100.0f, 0.0f, 1.0f);
					r = rgbModified.nonLinearRgb();
					break;
				case Component::rgbGreen:
					rgbModified = rgb;
					rgbModified.rgb.green = math::clamp(rgbModified.rgb.green + position / 100.0f, 0.0f, 1.0f);
					r = rgbModified.nonLinearRgb();
					break;
				case Component::rgbBlue:
					rgbModified = rgb;
					rgbModified.rgb.blue = math::clamp(rgbModified.rgb.blue + position / 100.0f, 0.0f, 1.0f);
					r = rgbModified.nonLinearRgb();
					break;
				case Component::hslHue:
					hslModified = hsl;
					hslModified.hsl.hue = math::wrap(hsl.hsl.hue + position / 100.0f);
					r = hslModified.hslToRgb();
					break;
				case Component::hslSaturation:
					hslModified = hsl;
					hslModified.hsl.saturation = math::clamp(hsl.hsl.saturation + position / 100.0f, 0.0f, 1.0f);
					r = hslModified.hslToRgb();
					break;
				case Component::hslLightness:
					hslModified = hsl;
					hslModified.hsl.lightness = math::clamp(hsl.hsl.lightness + position / 100.0f, 0.0f, 1.0f);
					r = hslModified.hslToRgb();
					break;
				case Component::labLightness:
					labModified = lab;
					labModified.lab.L = math::clamp(lab.lab.L + position, 0.0f, 100.0f);
					r = labModified.labToRgbD50().normalizeRgbInplace();
					break;
				}
				gtk_color_set_color(GTK_COLOR(rows[i].variants[j]), r);
			}
		}
	}
	struct Editable: IEditableColorsUI, IMenuExtension {
		Editable(VariationsArgs &args, GtkWidget *widget):
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
			if (position != Position::end || !isEditable() || !args.isPrimary())
				return;
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			GSList *group = nullptr;
			int row = args.getActiveRow();
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			for (size_t i = 0; i < sizeof(types) / sizeof(Type); i++) {
				auto item = gtk_radio_menu_item_new_with_label(group, _(types[i].name));
				group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
				if (args.rows[row].type == &types[i])
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
				g_object_set_data(G_OBJECT(item), "variation_type", const_cast<Type *>(&types[i]));
				g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(VariationsArgs::onModeChange), &args);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			}
		}
	private:
		VariationsArgs &args;
		GtkWidget *widget;
	};
	std::optional<Editable> editable[1 + Rows * (1 + VariantWidgets)];
};
static std::unique_ptr<IColorSource> build(GlobalState &gs, const dynv::Ref &options) {
	auto args = std::make_unique<VariationsArgs>(gs, options);
	GtkWidget *table, *vbox, *hbox, *widget, *hbox2;
	hbox = gtk_hbox_new(false, 0);
	vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 5);
	args->colorPreviews = gtk_table_new(Rows, VariantWidgets + 1, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->colorPreviews, true, true, 0);
	widget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, VariantWidgets / 2, VariantWidgets / 2 + 1, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 0, 0);
	args->allColors = widget;
	gtk_widget_set_size_request(widget, 60, 30);
	g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(VariationsArgs::onColorActivate), args.get());
	g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(VariationsArgs::onFocusEvent), args.get());
	args->editable[0].emplace(*args, widget);
	StandardEventHandler::forWidget(widget, &args->gs, &*args->editable[0]);
	StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[0]);
	for (int i = 0; i < Rows; ++i) {
		args->rows[i].type = &types[i];
		args->rows[i].primary = widget = gtk_color_new();
		gtk_color_set_rounded(GTK_COLOR(widget), true);
		gtk_color_set_hcenter(GTK_COLOR(widget), true);
		gtk_color_set_roundness(GTK_COLOR(widget), 5);
		gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, VariantWidgets / 2, VariantWidgets / 2 + 1, i + 1, i + 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
		gtk_widget_set_size_request(widget, 60, 30);
		g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(VariationsArgs::onColorActivate), args.get());
		g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(VariationsArgs::onFocusEvent), args.get());
		args->editable[1 + i].emplace(*args, widget);
		StandardEventHandler::forWidget(widget, &args->gs, &*args->editable[1 + i]);
		StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[1 + i]);
		for (int j = 0; j < VariantWidgets; ++j) {
			int x = j + (j >= VariantWidgets / 2 ? 1 : 0);
			args->rows[i].variants[j] = widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_color_set_roundness(GTK_COLOR(widget), 5);
			gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, x, x + 1, i + 1, i + 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
			gtk_widget_set_size_request(widget, 30, 30);
			g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(VariationsArgs::onColorActivate), args.get());
			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(VariationsArgs::onFocusEvent), args.get());
			size_t index = 1 + Rows + j + i * VariantWidgets;
			args->editable[index].emplace(*args, widget);
			StandardEventHandler::forWidget(widget, &args->gs, &*args->editable[index]);
			StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[index]);
		}
	}
	char tmp[32];
	for (int i = 0; i < Rows; ++i) {
		sprintf(tmp, "type%d", i);
		auto typeName = options->getString(tmp, "lab_lightness");
		for (uint32_t j = 0; j < sizeof(types) / sizeof(Type); j++) {
			if (types[j].id == typeName) {
				args->rows[i].type = &types[j];
				break;
			}
		}
		sprintf(tmp, "color%d", i);
		gtk_color_set_color(GTK_COLOR(args->rows[i].primary), options->getColor(tmp, Color(0.5f)), args->rows[i].type->symbol);
	}
	gtk_color_set_color(GTK_COLOR(args->allColors), options->getColor("all_colors", Color(0.5f)));
	hbox2 = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 0);
	gint table_y;
	table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(hbox2), table, true, true, 0);
	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Strength:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	args->strengthRange = widget = gtk_hscale_new_with_range(1, 100, 1);
	gtk_range_set_value(GTK_RANGE(widget), options->getFloat("strength", 30));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(VariationsArgs::onChange), args.get());
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	table_y++;
	gtk_widget_show_all(hbox);
	args->update();
	args->main = hbox;
	return args;
}
void registerVariations(ColorSourceManager &csm) {
	csm.add("variations", _("Variations"), RegistrationFlags::none, GDK_KEY_v, build);
}
