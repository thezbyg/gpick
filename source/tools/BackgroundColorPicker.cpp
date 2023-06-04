/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#include "BackgroundColorPicker.h"
#include "uiUtilities.h"
#include "ColorObject.h"
#include "GlobalState.h"
#include "ColorSpaces.h"
#include "Converters.h"
#include "I18N.h"
#include "EventBus.h"
#include "ColorList.h"
#include "uiColorInput.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "ToolColorNaming.h"
#include "common/Match.h"
#include "common/Bitmask.h"
#include "dynv/Map.h"
#include "layout/Box.h"
#include "layout/Style.h"
#include "layout/System.h"
#include "gtk/LayoutPreview.h"
#include "gtk/ColorComponent.h"
#include "gtk/ColorWidget.h"
#include <gdk/gdkkeysyms.h>
#include <string>
#include <vector>
namespace {
enum struct Mask: uint32_t {
	none = 0,
	colorWidget = 1,
	colorComponent = 2,
	all = 3,
};
}
ENABLE_BITMASK_OPERATORS(Mask);
namespace {
struct BackgroundColorPickerNameAssigner: public ToolColorNameAssigner {
	BackgroundColorPickerNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view label) {
		m_label = label;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		return std::string(m_label);
	}
protected:
	std::string_view m_label;
};
struct BackgroundColorPicker: public IEventHandler {
	GlobalState &gs;
	dynv::Ref options;
	common::Ref<layout::System> layoutSystem;
	common::Ref<layout::Box> backgroundBox;
	common::Ref<layout::Text> textBox, shadowBox;
	common::Ref<layout::Style> backgroundStyle, textStyle, shadowStyle;
	struct AdjustableColor: public IEditableColorUI {
		std::string name, label;
		const ColorSpaceDescription *colorSpace;
		dynv::Ref options;
		GtkWidget *colorSpaceComboBox, *colorComponent, *colorWidget;
		AdjustableColor(BackgroundColorPicker &backgroundColorPicker, GlobalState &gs):
			backgroundColorPicker(backgroundColorPicker),
			gs(gs) {
		}
		virtual ~AdjustableColor() {
			Color color;
			gtk_color_get_color(GTK_COLOR(colorWidget), &color);
			options->set("color", color);
			options->set("color_space", colorSpace->id);
		}
		virtual void addToPalette(const ColorObject &) override {
			Color color;
			gtk_color_get_color(GTK_COLOR(colorWidget), &color);
			ColorObject colorObject;
			BackgroundColorPickerNameAssigner nameAssigner(gs);
			nameAssigner.assign(colorObject, label);
			gs.colorList().add(colorObject);
		}
		virtual const ColorObject &getColor() override {
			Color color;
			gtk_color_get_color(GTK_COLOR(colorWidget), &color);
			colorObject.setColor(color);
			BackgroundColorPickerNameAssigner nameAssigner(gs);
			nameAssigner.assign(colorObject, label);
			return colorObject;
		}
		virtual void setColor(const ColorObject &colorObject) override {
			update(colorObject.getColor(), Mask::all);
		}
		virtual bool hasSelectedColor() override {
			return true;
		}
		virtual bool isEditable() override {
			return true;
		}
		void update() {
			Color color;
			gtk_color_get_color(GTK_COLOR(colorWidget), &color);
			update(color, Mask::all);
		}
		void update(const Color &color, Mask updateMask) {
			std::string text = gs.converters().serialize(color, Converters::Type::display);
			if ((updateMask & Mask::colorWidget) == Mask::colorWidget)
				gtk_color_set_color(GTK_COLOR(colorWidget), color, text);
			if ((updateMask & Mask::colorComponent) == Mask::colorComponent)
				gtk_color_component_set_color(GTK_COLOR_COMPONENT(colorComponent), color);
			Color transformedColor;
			gtk_color_component_get_transformed_color(GTK_COLOR_COMPONENT(colorComponent), transformedColor);
			float alpha = gtk_color_component_get_alpha(GTK_COLOR_COMPONENT(colorComponent));
			std::vector<std::string> values = toTexts(colorSpace->type, transformedColor, alpha, gs);
			const char *texts[5] = { 0 };
			int j = 0;
			for (auto &value: values) {
				texts[j++] = value.c_str();
				if (j > 4)
					break;
			}
			gtk_color_component_set_texts(GTK_COLOR_COMPONENT(colorComponent), texts);
			gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(backgroundColorPicker.layoutView), color, name.c_str());
		}
	private:
		BackgroundColorPicker &backgroundColorPicker;
		GlobalState &gs;
		ColorObject colorObject;
	};
	static constexpr size_t adjustableColorCount = 3;
	std::vector<AdjustableColor> adjustableColors;
	GtkWidget *layoutView, *colorSpaceComboBox;
	GtkWindow *window, *parent;
	BackgroundColorPicker(GlobalState &gs, GtkWindow *parent):
		gs(gs),
		parent(parent) {
		options = gs.settings().getOrCreateMap("gpick.tools.background_color_picker");
		window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
		gtk_window_set_title(window, _("Background color picker"));
		gtk_window_set_transient_for(window, parent);
		gtk_window_set_destroy_with_parent(window, true);
		gtk_window_set_skip_taskbar_hint(window, true);
		gtk_window_set_skip_pager_hint(window, true);
		gtk_window_set_default_size(window, options->getInt32("window.width", 600), options->getInt32("window.height", 600));
		GtkWidget *vbox = gtk_vbox_new(false, 0);
		layoutView = gtk_layout_preview_new();
		gtk_layout_preview_set_fill(GTK_LAYOUT_PREVIEW(layoutView), true);
		layoutSystem = common::Ref<layout::System>(new layout::System());
		backgroundStyle = common::Ref<layout::Style>(new layout::Style("background", Color(0.5f, 0.1f, 0.0f), 0));
		textStyle = common::Ref<layout::Style>(new layout::Style("text", Color(1.0f, 1.0f, 1.0f), 0.2f));
		shadowStyle = common::Ref<layout::Style>(new layout::Style("shadow", Color(0.0f, 0.0f, 0.0f), 0.2f));
		shadowStyle->setTextOffset(math::Vector2f(0.01f, 0.01f));
		backgroundBox = common::Ref<layout::Box>(new layout::Fill("background", 0.0f, 0.0f, 1.0f, 1.0f));
		backgroundBox->setStyle(backgroundStyle);
		textBox = common::Ref<layout::Text>(new layout::Text("text", 0.05f, 0.05f, 0.9f, 0.45f));
		textBox->setStyle(textStyle).setText(_("The quick brown fox jumps over the lazy dog"));
		shadowBox = common::Ref<layout::Text>(new layout::Text("shadow", 0.05f, 0.05f, 0.9f, 0.45f));
		shadowBox->setStyle(shadowStyle).setText(_("The quick brown fox jumps over the lazy dog"));
		backgroundBox->addChild(shadowBox);
		backgroundBox->addChild(textBox);
		layoutSystem->setSelectable(false);
		layoutSystem->setBox(backgroundBox);
		layoutSystem->addStyle(backgroundStyle);
		layoutSystem->addStyle(textStyle);
		layoutSystem->addStyle(shadowStyle);
		g_signal_connect(G_OBJECT(window), "size-allocate", G_CALLBACK(BackgroundColorPicker::onLayoutResize), this);
		gtk_layout_preview_set_system(GTK_LAYOUT_PREVIEW(layoutView), layoutSystem);
		gtk_box_pack_start(GTK_BOX(vbox), layoutView, true, true, 0);
		Grid grid(adjustableColorCount * 2, 3);
		static const char *names[adjustableColorCount] = { "background", "text", "shadow" };
		static const char *labels[adjustableColorCount] = { N_("Background"), N_("Text"), N_("Shadow") };
		adjustableColors.reserve(adjustableColorCount);
		for (size_t i = 0; i < adjustableColorCount; ++i) {
			auto &adjustableColor = adjustableColors.emplace_back(*this, gs);
			adjustableColor.name = names[i];
			adjustableColor.label = _(labels[i]);
			adjustableColor.options = options->getOrCreateMap(adjustableColor.name);
			adjustableColor.colorSpace = &common::matchById(colorSpaces(), adjustableColor.options->getString("color_space", "rgb"));
			grid.setColumnAndRow(i* 2, 0);
			auto label = std::string(_(labels[i])) + ':';
			grid.addLabel(label.c_str());
			grid.add(adjustableColor.colorWidget = gtk_color_new(), true);
			gtk_color_set_rounded(GTK_COLOR(adjustableColor.colorWidget), true);
			gtk_color_set_hcenter(GTK_COLOR(adjustableColor.colorWidget), true);
			gtk_color_set_roundness(GTK_COLOR(adjustableColor.colorWidget), 5);
			gtk_widget_set_size_request(adjustableColor.colorWidget, 30, 30);
			StandardEventHandler::forWidget(adjustableColor.colorWidget, &gs, &adjustableColor);
			StandardDragDropHandler::forWidget(adjustableColor.colorWidget, &gs, &adjustableColor);
			grid.setColumnAndRow(i * 2, 1);
			grid.addLabel(_("Color space:"));
			grid.add(adjustableColor.colorSpaceComboBox = gtk_combo_box_text_new(), true);
			for (const auto &i: colorSpaces()) {
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(adjustableColor.colorSpaceComboBox), _(i.name));
				if (&i == adjustableColor.colorSpace)
					gtk_combo_box_set_active(GTK_COMBO_BOX(adjustableColor.colorSpaceComboBox), &i - colorSpaces().data());
			}
			g_object_set_data(G_OBJECT(adjustableColor.colorSpaceComboBox), "adjustableColor", &adjustableColor);
			g_signal_connect(G_OBJECT(adjustableColor.colorSpaceComboBox), "changed", G_CALLBACK(BackgroundColorPicker::onColorSpaceChange), this);
			grid.setColumnAndRow(i * 2, 2);
			grid.add(adjustableColor.colorComponent = gtk_color_component_new(ColorSpace::rgb), true, 2, false);
			updateColorSpace(adjustableColor);
			g_object_set_data(G_OBJECT(adjustableColor.colorComponent), "adjustableColor", &adjustableColor);
			g_signal_connect(G_OBJECT(adjustableColor.colorComponent), "color-changed", G_CALLBACK(BackgroundColorPicker::onComponentChangeValue), this);
			g_signal_connect(G_OBJECT(adjustableColor.colorComponent), "input-clicked", G_CALLBACK(BackgroundColorPicker::onComponentInputClicked), this);
			adjustableColor.update(adjustableColor.options->getColor("color", Color(1.0f - 0.5f * i, 0.8f - 0.4f * i, 0.1f, 1.0f - 0.8f * (i / 2))), Mask::all);
		}
		gtk_box_pack_start(GTK_BOX(vbox), grid, false, false, 5);
		gtk_container_add(GTK_CONTAINER(window), vbox);
		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(BackgroundColorPicker::onDestroy), this);
		g_signal_connect(G_OBJECT(window), "window-state-event", G_CALLBACK(BackgroundColorPicker::onWindowStateEvent), this);
		g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(BackgroundColorPicker::onConfigureEvent), this);
		g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(BackgroundColorPicker::onKeyPress), this);
		gtk_widget_realize(GTK_WIDGET(window));
		if (options->getBool("window.fullscreen", false)) {
			gtk_window_set_transient_for(window, nullptr);
			gtk_window_fullscreen(window);
		} else if (options->getBool("window.maximized", false)) {
			gtk_window_maximize(window);
		}
		gtk_widget_show_all(GTK_WIDGET(window));
		gs.eventBus().subscribe(EventType::optionsUpdate, *this);
		gs.eventBus().subscribe(EventType::convertersUpdate, *this);
		gs.eventBus().subscribe(EventType::displayFiltersUpdate, *this);
	}
	virtual ~BackgroundColorPicker() {
		gs.eventBus().unsubscribe(*this);
	}
	void setTransformationChain() {
		auto chain = gs.getTransformationChain();
		gtk_layout_preview_set_transformation_chain(GTK_LAYOUT_PREVIEW(layoutView), chain);
		for (auto &adjustableColor: adjustableColors) {
			gtk_color_set_transformation_chain(GTK_COLOR(adjustableColor.colorWidget), chain);
		}
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
			for (auto &adjustableColor: adjustableColors) {
				adjustableColor.update();
			}
			break;
		case EventType::displayFiltersUpdate:
			setTransformationChain();
			break;
		case EventType::colorDictionaryUpdate:
		case EventType::paletteChanged:
			break;
		}
	}
	void updateWindowSize() {
		auto state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window)));
		if (state & GDK_WINDOW_STATE_MAXIMIZED) {
			options->set("window.maximized", true);
			return;
		}
		if (state & GDK_WINDOW_STATE_FULLSCREEN) {
			options->set("window.fullscreen", true);
			return;
		}
		if (state & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_ICONIFIED))
			return;
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(window), &width, &height);
		options->set("window.width", width);
		options->set("window.height", height);
		options->set("window.maximized", false);
		options->set("window.fullscreen", false);
	}
	void updateColorSpace(AdjustableColor &adjustableColor) {
		gtk_color_component_set_color_space(GTK_COLOR_COMPONENT(adjustableColor.colorComponent), adjustableColor.colorSpace->type);
		const char *labels[ColorSpaceDescription::maxChannels * 2] = { 0 };
		const auto &type = colorSpace(adjustableColor.colorSpace->type);
		for (int i = 0; i < type.channelCount; ++i) {
			labels[i * 2 + 0] = type.channels[i].shortName;
			labels[i * 2 + 1] = _(type.channels[i].name);
		}
		gtk_color_component_set_labels(GTK_COLOR_COMPONENT(adjustableColor.colorComponent), labels);
	}
	void updateShadowOffset(int width, int height) {
		math::Rectanglef shadowRectangle;
		backgroundBox->visit(math::Rectanglef(0, 0, width, height), [&](const math::Rectanglef &rectangle, const layout::Box &box) {
			if (&*shadowBox == &box) {
				shadowRectangle = rectangle;
			}
		});
		if (shadowRectangle.getWidth() <= 0 || shadowRectangle.getHeight() <= 0) {
			shadowStyle->setTextOffset(math::Vector2f(0, 0));
			return;
		}
		float offset = shadowRectangle.getHeight() / 100.0f;
		shadowStyle->setTextOffset(math::Vector2f(offset / shadowRectangle.getWidth(), offset / shadowRectangle.getHeight()));
	}
	static void onDestroy(GtkWidget *, BackgroundColorPicker *backgroundColorPicker) {
		delete backgroundColorPicker;
	}
	static gboolean onSizeUpdateTimeout(BackgroundColorPicker *backgroundColorPicker) {
		backgroundColorPicker->updateWindowSize();
		return false;
	}
	static gboolean onConfigureEvent(GtkWidget *, GdkEventConfigure *, BackgroundColorPicker *backgroundColorPicker) {
		g_timeout_add(10, G_SOURCE_FUNC(onSizeUpdateTimeout), backgroundColorPicker);
		return false;
	}
	static gboolean onWindowStateEvent(GtkWidget *, GdkEventWindowState *event, BackgroundColorPicker *backgroundColorPicker) {
		backgroundColorPicker->updateWindowSize();
		return false;
	}
	static void onLayoutResize(GtkWidget *, GtkAllocation *allocation, BackgroundColorPicker *backgroundColorPicker) {
		backgroundColorPicker->updateShadowOffset(allocation->width, allocation->height);
	}
	static gboolean onKeyPress(GtkWidget *widget, GdkEventKey *event, BackgroundColorPicker *backgroundColorPicker) {
		guint state = event->state & gtk_accelerator_get_default_mod_mask();
		if (event->keyval == GDK_KEY_F11 && state == 0) {
			auto *window = backgroundColorPicker->window;
			auto state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window)));
			if (state & GDK_WINDOW_STATE_FULLSCREEN) {
				gtk_window_unfullscreen(window);
				gtk_window_set_transient_for(window, backgroundColorPicker->parent);
			} else {
				gtk_window_set_transient_for(window, nullptr);
				gtk_window_fullscreen(window);
			}
			return true;
		}
		return false;
	}
	static void onColorSpaceChange(GtkWidget *widget, BackgroundColorPicker *backgroundColorPicker) {
		auto &adjustableColor = *reinterpret_cast<AdjustableColor *>(g_object_get_data(G_OBJECT(widget), "adjustableColor"));
		adjustableColor.colorSpace = &colorSpaces()[gtk_combo_box_get_active(GTK_COMBO_BOX(widget))];
		backgroundColorPicker->updateColorSpace(adjustableColor);
	}
	static void onComponentChangeValue(GtkWidget *widget, const Color *color, BackgroundColorPicker *) {
		auto &adjustableColor = *reinterpret_cast<AdjustableColor *>(g_object_get_data(G_OBJECT(widget), "adjustableColor"));
		adjustableColor.update(*color, Mask::all & ~Mask::colorComponent);
	}
	static void onComponentInputClicked(GtkWidget *widget, int channel, BackgroundColorPicker *backgroundColorPicker) {
		dialog_color_component_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_COLOR_COMPONENT(widget), channel, backgroundColorPicker->options->getOrCreateMap("component_edit"));
	}
};
}
void tools_background_color_picker(GtkWindow *parent, GlobalState &gs) {
	new BackgroundColorPicker(gs, parent);
}
