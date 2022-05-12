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

#include "GenerateScheme.h"
#include "ColorObject.h"
#include "ColorSourceManager.h"
#include "IColorSource.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorRYB.h"
#include "ColorList.h"
#include "gtk/ColorWidget.h"
#include "gtk/ColorWheel.h"
#include "ColorWheelType.h"
#include "Converters.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "Random.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IDroppableColorUI.h"
#include "IMenuExtension.h"
#include "EventBus.h"
#include "color_names/ColorNames.h"
#include "common/Guard.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>

const int MaxColors = 6;
const SchemeType types[] = {
	{ N_("Complementary"), 1, 1, { 180 } },
	{ N_("Analogous"), 5, 1, { 30 } },
	{ N_("Triadic"), 2, 1, { 120 } },
	{ N_("Split-Complementary"), 2, 2, { 150, 60 } },
	{ N_("Rectangle (tetradic)"), 3, 2, { 60, 120 } },
	{ N_("Square"), 3, 1, { 90 } },
	{ N_("Neutral"), 5, 1, { 15 } },
	{ N_("Clash"), 2, 2, { 90, 180 } },
	{ N_("Five-Tone"), 4, 4, { 115, 40, 50, 40 } },
	{ N_("Six-Tone"), 5, 2, { 30, 90 } },
};
struct GenerateSchemeColorNameAssigner: public ToolColorNameAssigner {
	GenerateSchemeColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, int ident, int schemeType) {
		m_ident = ident;
		m_schemeType = schemeType;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << _("scheme") << " " << _(generate_scheme_get_scheme_type(m_schemeType)->name) << " #" << m_ident << "[" << color_names_get(m_gs.getColorNames(), &colorObject.getColor(), false) << "]";
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	int m_ident;
	int m_schemeType;
};
struct GenerateSchemeArgs: public IColorSource, public IEventHandler {
	GtkWidget *main, *statusBar, *generationType, *wheelTypeCombo, *colorWheel, *hueRange, *saturationRange, *lightnessRange, *colorPreviews, *lastFocusedColor;
	struct {
		GtkWidget *widget;
		float colorHue, hueShift, saturationShift, valueShift, originalHue, originalSaturation, originalValue;
	} items[MaxColors];
	bool wheelLocked;
	int colorsVisible;
	dynv::Ref options;
	GlobalState &gs;
	GenerateSchemeArgs(GlobalState &gs, const dynv::Ref &options):
		options(options),
		gs(gs),
		colorWheelEditable(*this) {
		statusBar = gs.getStatusBar();
		gs.eventBus().subscribe(EventType::optionsUpdate, *this);
		gs.eventBus().subscribe(EventType::convertersUpdate, *this);
		gs.eventBus().subscribe(EventType::displayFiltersUpdate, *this);
	}
	virtual ~GenerateSchemeArgs() {
		gtk_widget_destroy(main);
		gs.eventBus().unsubscribe(*this);
	}
	virtual std::string_view name() const {
		return "generate_scheme";
	}
	virtual void activate() override {
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "empty"), "");
	}
	virtual void deactivate() override {
		update(true);
		options->set("wheel_locked", wheelLocked);
		float hsvShifts[MaxColors * 3];
		for (uint32_t i = 0; i < MaxColors; ++i) {
			hsvShifts[i * 3 + 0] = items[i].hueShift;
			hsvShifts[i * 3 + 1] = items[i].saturationShift;
			hsvShifts[i * 3 + 2] = items[i].valueShift;
		}
		options->set("hsv_shift", common::Span<float>(hsvShifts, MaxColors * 3));
	}
	virtual GtkWidget *getWidget() override {
		return main;
	}
	void setTransformationChain() {
		auto chain = gs.getTransformationChain();
		for (int i = 0; i < MaxColors; ++i) {
			gtk_color_set_transformation_chain(GTK_COLOR(items[i].widget), chain);
		}
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
			update();
			break;
		case EventType::displayFiltersUpdate:
			setTransformationChain();
			break;
		case EventType::colorDictionaryUpdate:
		case EventType::paletteChanged:
			break;
		}
	}
	void addToPalette() {
		color_list_add_color_object(gs.getColorList(), getColor(), true);
	}
	void addToPalette(GenerateSchemeColorNameAssigner &nameAssigner, Color &color, GtkWidget *widget) {
		gtk_color_get_color(GTK_COLOR(widget), &color);
		colorObject.setColor(color);
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(generationType));
		nameAssigner.assign(colorObject, identifyColorWidget(widget), type);
		color_list_add_color_object(gs.getColorList(), colorObject, true);
	}
	void addAllToPalette() {
		common::Guard colorListGuard(color_list_start_changes(gs.getColorList()), color_list_end_changes, gs.getColorList());
		GenerateSchemeColorNameAssigner nameAssigner(gs);
		Color color;
		for (int i = 0; i < colorsVisible; ++i)
			addToPalette(nameAssigner, color, items[i].widget);
	}
	virtual void setColor(const ColorObject &colorObject) override {
		int index = 0;
		for (int i = 0; i < colorsVisible; ++i) {
			if (items[i].widget == lastFocusedColor) {
				index = i;
				break;
			}
		}
		Color color = colorObject.getColor();
		float hue, saturation, lightness, shiftedHue;
		Color hsl, hsv, hslResult;
		hsv = color.rgbToHsv();
		hsl = hsv.hsvToHsl();
		int wheelType = gtk_combo_box_get_active(GTK_COMBO_BOX(wheelTypeCombo));
		auto &wheel = color_wheel_types_get()[wheelType];
		double tmp;
		wheel.rgbhue_to_hue(hsl.hsl.hue, &tmp);
		hue = static_cast<float>(tmp);
		shiftedHue = math::wrap(hue - items[index].colorHue - items[index].hueShift);
		wheel.hue_to_hsl(hue, &hslResult);
		saturation = hsl.hsl.saturation * 1 / hslResult.hsl.saturation;
		lightness = hsl.hsl.lightness - hslResult.hsl.lightness;
		shiftedHue *= 360.0f;
		saturation *= 100.0f;
		lightness *= 100.0f;
		gtk_range_set_value(GTK_RANGE(hueRange), shiftedHue);
		gtk_range_set_value(GTK_RANGE(saturationRange), saturation);
		gtk_range_set_value(GTK_RANGE(lightnessRange), lightness);
		update();
	}
	virtual void setNthColor(size_t index, const ColorObject &colorObject) override {
		if (index < 0 || index >= MaxColors)
			return;
		setActiveWidget(items[index].widget);
		setColor(colorObject);
	}
	ColorObject colorObject;
	virtual const ColorObject &getColor() override {
		Color color;
		gtk_color_get_color(GTK_COLOR(lastFocusedColor), &color);
		colorObject.setColor(color);
		GenerateSchemeColorNameAssigner nameAssigner(gs);
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(generationType));
		nameAssigner.assign(colorObject, identifyColorWidget(lastFocusedColor), type);
		return colorObject;
	}
	virtual const ColorObject &getNthColor(size_t index) override {
		if (index < 0 || index >= MaxColors)
			return colorObject;
		Color color;
		gtk_color_get_color(GTK_COLOR(items[index].widget), &color);
		colorObject.setColor(color);
		GenerateSchemeColorNameAssigner nameAssigner(gs);
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(generationType));
		nameAssigner.assign(colorObject, index + 1, type);
		return colorObject;
	}
	int identifyColorWidget(GtkWidget *widget) {
		for (int i = 0; i < MaxColors; ++i) {
			if (items[i].widget == widget) {
				return i + 1;
			}
		}
		return 0;
	}
	static gboolean onFocusEvent(GtkWidget *widget, GdkEventFocus *, GenerateSchemeArgs *args) {
		args->setActiveWidget(widget);
		return false;
	}
	static gchar *onSaturationFormat(GtkScale *scale, gdouble value) {
		return g_strdup_printf("%d%%", int(value));
	}
	static gchar *onLightnessFormat(GtkScale *scale, gdouble value) {
		if (value >= 0)
			return g_strdup_printf("+%d%%", int(value));
		else
			return g_strdup_printf("-%d%%", -int(value));
	}
	static void onLockToggle(GtkWidget *widget, GenerateSchemeArgs *args) {
		args->wheelLocked = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
		gtk_color_wheel_set_block_editable(GTK_COLOR_WHEEL(args->colorWheel), !args->wheelLocked);
	}
	void setActiveWidget(GtkWidget *widget) {
		lastFocusedColor = widget;
		for (int i = 0; i < MaxColors; ++i) {
			if (widget == items[i].widget) {
				gtk_color_wheel_set_selected(GTK_COLOR_WHEEL(colorWheel), i);
				break;
			}
		}
	}
	static void onReset(GtkWidget *, GenerateSchemeArgs *args) {
		for (int i = 0; i < args->colorsVisible; ++i) {
			if (args->items[i].widget == args->lastFocusedColor) {
				args->items[i].hueShift = 0;
				args->items[i].saturationShift = 0;
				args->items[i].valueShift = 0;
				args->update();
				break;
			}
		}
	}
	static void onResetAll(GtkWidget *, GenerateSchemeArgs *args) {
		for (int i = 0; i < MaxColors; ++i) {
			args->items[i].hueShift = 0;
			args->items[i].saturationShift = 0;
			args->items[i].valueShift = 0;
		}
		args->update();
	}
	static void onColorActivate(GtkWidget *, GenerateSchemeArgs *args) {
		args->addToPalette();
	}
	static void onHueChange(GtkWidget *widget, gint colorId, GenerateSchemeArgs *args) {
		if (args->wheelLocked) {
			float hue = static_cast<float>(gtk_range_get_value(GTK_RANGE(args->hueRange)) / 360.0f);
			hue = math::wrap(hue - args->items[colorId].hueShift + static_cast<float>(gtk_color_wheel_get_hue(GTK_COLOR_WHEEL(widget), colorId)) - args->items[colorId].originalHue);
			gtk_range_set_value(GTK_RANGE(args->hueRange), hue * 360.0f);
		} else {
			args->items[colorId].hueShift = static_cast<float>(gtk_color_wheel_get_hue(GTK_COLOR_WHEEL(widget), colorId)) - args->items[colorId].originalHue;
			onChange(widget, args);
		}
	}
	static void onSaturationChange(GtkWidget *widget, gint colorId, GenerateSchemeArgs *args) {
		if (args->wheelLocked) {
			double saturation = static_cast<float>(gtk_range_get_value(GTK_RANGE(args->saturationRange)) / 100.0f);
			double lightness = static_cast<float>(gtk_range_get_value(GTK_RANGE(args->lightnessRange)) / 100.0f);
			gtk_range_set_value(GTK_RANGE(args->saturationRange), saturation * 100.0f);
			gtk_range_set_value(GTK_RANGE(args->lightnessRange), lightness * 100.0f);
		} else {
			args->items[colorId].saturationShift = static_cast<float>(gtk_color_wheel_get_saturation(GTK_COLOR_WHEEL(widget), colorId)) - args->items[colorId].originalSaturation;
			args->items[colorId].valueShift = static_cast<float>(gtk_color_wheel_get_value(GTK_COLOR_WHEEL(widget), colorId)) - args->items[colorId].originalValue;
			onChange(widget, args);
		}
	}
	static void onChange(GtkWidget *, GenerateSchemeArgs *args) {
		args->update();
	}
	void update(bool saveSettings = false) {
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(generationType));
		int colorCount = generate_scheme_get_scheme_type(type)->colors + 1;
		int wheelType = gtk_combo_box_get_active(GTK_COMBO_BOX(wheelTypeCombo));
		gtk_color_wheel_set_color_wheel_type(GTK_COLOR_WHEEL(colorWheel), &color_wheel_types_get()[wheelType]);
		gtk_color_wheel_set_n_colors(GTK_COLOR_WHEEL(colorWheel), colorCount);
		float hue = static_cast<float>(gtk_range_get_value(GTK_RANGE(hueRange)));
		float saturation = static_cast<float>(gtk_range_get_value(GTK_RANGE(saturationRange)));
		float lightness = static_cast<float>(gtk_range_get_value(GTK_RANGE(lightnessRange)));
		if (saveSettings) {
			options->set("type", type);
			options->set("wheel_type", wheelType);
			options->set("hue", hue);
			options->set("saturation", saturation);
			options->set("lightness", lightness);
			return;
		}
		for (int i = colorsVisible; i > colorCount; --i)
			gtk_widget_hide(items[i - 1].widget);
		for (int i = colorsVisible; i < colorCount; ++i)
			gtk_widget_show(items[i].widget);
		colorsVisible = colorCount;
		hue /= 360.0f;
		saturation /= 100.0f;
		lightness /= 100.0f;
		Color hsl, r;
		auto &wheel = color_wheel_types_get()[wheelType];
		float chaos = 0;
		float hueOffset = 0;
		float hueStep;
		Color hsv;
		for (int i = 0; i < colorCount; ++i) {
			wheel.hue_to_hsl(math::wrap(hue + items[i].hueShift), &hsl);
			hsl.hsl.lightness = math::clamp(hsl.hsl.lightness + lightness, 0.0f, 1.0f);
			hsl.hsl.saturation = math::clamp(hsl.hsl.saturation * saturation, 0.0f, 1.0f);
			hsv = hsl.hslToHsv();
			items[i].originalHue = hue;
			items[i].originalSaturation = hsv.hsv.saturation;
			items[i].originalValue = hsv.hsv.value;
			hsv.hsv.saturation = math::clamp(hsv.hsv.saturation + items[i].saturationShift, 0.0f, 1.0f);
			hsv.hsv.value = math::clamp(hsv.hsv.value + items[i].valueShift, 0.0f, 1.0f);
			r = hsv.hsvToRgb();
			r.alpha = 1;
			auto text = gs.converters().serialize(r, Converters::Type::display);
			gtk_color_set_color(GTK_COLOR(items[i].widget), r, text);
			items[i].colorHue = hueOffset;
			gtk_color_wheel_set_hue(GTK_COLOR_WHEEL(colorWheel), i, math::wrap(hue + items[i].hueShift));
			gtk_color_wheel_set_saturation(GTK_COLOR_WHEEL(colorWheel), i, hsv.hsv.saturation);
			gtk_color_wheel_set_value(GTK_COLOR_WHEEL(colorWheel), i, hsv.hsv.value);
			hueStep = (generate_scheme_get_scheme_type(type)->turn[i % generate_scheme_get_scheme_type(type)->turn_types]) / (360.0f) + chaos * static_cast<float>(random_get_double(gs.getRandom()) - 0.5);
			hue = math::wrap(hue + hueStep);
			hueOffset = math::wrap(hueOffset + hueStep);
		}
	}
	struct Editable: IEditableColorsUI, IMenuExtension {
		Editable(GenerateSchemeArgs &args, GtkWidget *widget):
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
			return true;
		}
		virtual bool hasColor() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return true;
		}
		virtual void extendMenu(GtkWidget *menu, Position position) override {
			if (position != Position::end)
				return;
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			auto item = newMenuItem(_("_Reset"), GTK_STOCK_CANCEL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(GenerateSchemeArgs::onReset), &args);
			item = newMenuItem(_("_Reset scheme"), GTK_STOCK_CANCEL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(GenerateSchemeArgs::onResetAll), &args);
		}
	private:
		GenerateSchemeArgs &args;
		GtkWidget *widget;
	};
	std::optional<Editable> editable[MaxColors];
	struct ColorWheelEditable: IEditableColorUI, IDroppableColorUI {
		ColorWheelEditable(GenerateSchemeArgs &args):
			args(args) {
		}
		virtual ~ColorWheelEditable() = default;
		virtual void addToPalette(const ColorObject &) override {
		}
		virtual void setColor(const ColorObject &colorObject) override {
		}
		virtual void setColorAt(const ColorObject &colorObject, int x, int y) override {
			int index = gtk_color_wheel_get_at(GTK_COLOR_WHEEL(args.colorWheel), x, y);
			if (!(index >= 0 && index < MaxColors))
				return;
			Color color, hsl;
			float hue;
			color = colorObject.getColor();
			hsl = color.rgbToHsl();
			int wheelType = gtk_combo_box_get_active(GTK_COMBO_BOX(args.wheelTypeCombo));
			auto &wheel = color_wheel_types_get()[wheelType];
			double tmp;
			wheel.rgbhue_to_hue(hsl.hsl.hue, &tmp);
			hue = static_cast<float>(tmp);
			if (args.wheelLocked) {
				float hueShift = (hue - args.items[index].originalHue) - args.items[index].hueShift;
				hue = math::wrap(static_cast<float>(gtk_range_get_value(GTK_RANGE(args.hueRange))) / 360.0f + hueShift);
				gtk_range_set_value(GTK_RANGE(args.hueRange), hue * 360.0f);
			} else {
				args.items[index].hueShift = hue - args.items[index].originalHue;
				args.update();
			}
		}
		virtual const ColorObject &getColor() override {
			return colorObject;
		}
		virtual bool isEditable() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return true;
		}
		virtual bool testDropAt(int x, int y) override {
			int index = gtk_color_wheel_get_at(GTK_COLOR_WHEEL(args.colorWheel), x, y);
			return index >= 0 && index < MaxColors;
		}
		virtual void dropEnd(bool move) override {
		}
	private:
		GenerateSchemeArgs &args;
		ColorObject colorObject;
	};
	std::optional<ColorWheelEditable> colorWheelEditable;
};
static void showMenu(GtkWidget *widget, GenerateSchemeArgs *args, GdkEventButton *event) {
	auto menu = gtk_menu_new();
	auto item = gtk_check_menu_item_new_with_mnemonic(_("_Linked"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), args->wheelLocked);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(GenerateSchemeArgs::onLockToggle), args);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	item = newMenuItem(_("_Reset scheme"), GTK_STOCK_CANCEL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(GenerateSchemeArgs::onResetAll), args);
	showContextMenu(menu, event);
}
static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, GenerateSchemeArgs *args) {
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		showMenu(widget, args, event);
	}
	return false;
}
static void onPopupMenu(GtkWidget *widget, GenerateSchemeArgs *args) {
	showMenu(widget, args, nullptr);
}
static std::unique_ptr<IColorSource> build(GlobalState &gs, const dynv::Ref &options) {
	auto args = std::make_unique<GenerateSchemeArgs>(gs, options);
	auto hsvShifts = args->options->getFloats("hsv_shift");
	auto hsvShiftCount = hsvShifts.size() / 3;
	for (uint32_t i = 0; i < MaxColors; ++i) {
		if (i < hsvShiftCount) {
			args->items[i].hueShift = hsvShifts[i * 3 + 0];
			args->items[i].saturationShift = hsvShifts[i * 3 + 1];
			args->items[i].valueShift = hsvShifts[i * 3 + 2];
		} else {
			args->items[i].hueShift = 0;
			args->items[i].saturationShift = 0;
			args->items[i].valueShift = 0;
		}
	}
	GtkWidget *table, *vbox, *hbox, *widget, *hbox2;
	hbox = gtk_hbox_new(false, 5);
	vbox = gtk_vbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 5);
	args->colorPreviews = gtk_table_new(3, 2, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->colorPreviews, true, true, 0);
	for (intptr_t i = 0; i < MaxColors; ++i) {
		widget = gtk_color_new();
		gtk_color_set_rounded(GTK_COLOR(widget), true);
		gtk_color_set_roundness(GTK_COLOR(widget), 5);
		gtk_color_set_hcenter(GTK_COLOR(widget), true);
		gtk_table_attach(GTK_TABLE(args->colorPreviews), widget, i % 2, (i % 2) + 1, i / 2, i / 2 + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
		args->items[i].widget = widget;
		g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(GenerateSchemeArgs::onColorActivate), args.get());
		g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(GenerateSchemeArgs::onFocusEvent), args.get());
		args->editable[i].emplace(*args, widget);
		StandardEventHandler::forWidget(widget, &args->gs, &*args->editable[i]);
		StandardDragDropHandler::forWidget(widget, &args->gs, &*args->editable[i]);
	}
	hbox2 = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 0);
	args->colorWheel = widget = gtk_color_wheel_new();
	gtk_box_pack_start(GTK_BOX(hbox2), args->colorWheel, false, false, 0);
	g_signal_connect(G_OBJECT(args->colorWheel), "hue_changed", G_CALLBACK(GenerateSchemeArgs::onHueChange), args.get());
	g_signal_connect(G_OBJECT(args->colorWheel), "saturation_value_changed", G_CALLBACK(GenerateSchemeArgs::onSaturationChange), args.get());
	g_signal_connect(G_OBJECT(args->colorWheel), "popup-menu", G_CALLBACK(onPopupMenu), args.get());
	g_signal_connect(G_OBJECT(args->colorWheel), "button-press-event", G_CALLBACK(onButtonPress), args.get());
	args->wheelLocked = options->getBool("wheel_locked", true);
	gtk_color_wheel_set_block_editable(GTK_COLOR_WHEEL(args->colorWheel), !args->wheelLocked);
	StandardDragDropHandler::forWidget(widget, &args->gs, &*args->colorWheelEditable, StandardDragDropHandler::Options().allowDrag(false));

	gint table_y;
	table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(hbox2), table, true, true, 0);
	table_y = 0;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Hue:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->hueRange = widget = gtk_hscale_new_with_range(0, 360, 1);
	gtk_range_set_value(GTK_RANGE(widget), options->getFloat("hue", 180));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(GenerateSchemeArgs::onChange), args.get());
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Saturation:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->saturationRange = widget = gtk_hscale_new_with_range(0, 120, 1);
	gtk_range_set_value(GTK_RANGE(widget), options->getFloat("saturation", 100));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(GenerateSchemeArgs::onChange), args.get());
	g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(GenerateSchemeArgs::onSaturationFormat), args.get());
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Lightness:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->lightnessRange = widget = gtk_hscale_new_with_range(-50, 80, 1);
	gtk_range_set_value(GTK_RANGE(widget), options->getFloat("lightness", 0));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(GenerateSchemeArgs::onChange), args.get());
	g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(GenerateSchemeArgs::onLightnessFormat), args.get());
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Type:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_SHRINK, 5, 5);
	args->generationType = widget = gtk_combo_box_text_new();
	for (size_t i = 0; i < generate_scheme_get_n_scheme_types(); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _(generate_scheme_get_scheme_type(i)->name));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), options->getInt32("type", 0));
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(GenerateSchemeArgs::onChange), args.get());
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GTK_FILL, GTK_SHRINK, 5, 0);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Color wheel:"), 0, 0.5, 0, 0), 0, 1, table_y, table_y + 1, GTK_FILL, GTK_SHRINK, 5, 5);
	args->wheelTypeCombo = widget = gtk_combo_box_text_new();
	for (size_t i = 0; i < color_wheel_types_get_n(); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _(color_wheel_types_get()[i].name));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), options->getInt32("wheel_type", 0));
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(GenerateSchemeArgs::onChange), args.get());
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GTK_FILL, GTK_SHRINK, 5, 0);
	table_y++;
	args->colorsVisible = MaxColors;
	gtk_widget_show_all(hbox);
	args->update();
	args->main = hbox;
	args->setTransformationChain();
	return args;
}
void registerGenerateScheme(ColorSourceManager &csm) {
	csm.add("generate_scheme", _("Scheme generation"), RegistrationFlags::needsViewport, GDK_KEY_g, build);
}
const SchemeType *generate_scheme_get_scheme_type(size_t index) {
	if (index >= 0 && index < generate_scheme_get_n_scheme_types())
		return &types[index];
	else
		return 0;
}
size_t generate_scheme_get_n_scheme_types() {
	return sizeof(types) / sizeof(SchemeType);
}
