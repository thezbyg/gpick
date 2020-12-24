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

#include "BlendColors.h"
#include "ColorObject.h"
#include "ColorSource.h"
#include "ColorSourceManager.h"
#include "ColorUtils.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "ColorList.h"
#include "color_names/ColorNames.h"
#include "gtk/ColorWidget.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IMenuExtension.h"
#include "I18N.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>

struct BlendColorNameAssigner: ToolColorNameAssigner {
	BlendColorNameAssigner(GlobalState *gs):
		ToolColorNameAssigner(gs) {
		m_isColorItem = false;
	}
	void setNames(const std::string &startColorName, const std::string &endColorName) {
		m_colorStart = startColorName;
		m_colorEnd = endColorName;
	}
	void setStepsAndStage(int steps, int stage) {
		m_steps = steps;
		m_stage = stage;
	}
	void assign(ColorObject *colorObject, const Color *color, int step) {
		m_startPercent = step * 100 / (m_steps - 1);
		m_endPercent = 100 - (step * 100 / (m_steps - 1));
		m_isColorItem = (((step == 0 || step == m_steps - 1) && m_stage == 0) || (m_stage == 1 && step == m_steps - 1));
		ToolColorNameAssigner::assign(colorObject, color);
	}
	void assign(ColorObject *colorObject, const Color *color) {
		m_isColorItem = true;
		ToolColorNameAssigner::assign(colorObject, color);
	}
	virtual std::string getToolSpecificName(ColorObject *colorObject, const Color *color) {
		m_stream.str("");
		if (m_isColorItem) {
			if (m_endPercent == 100) {
				m_stream << m_colorEnd << " " << _("blend node");
			} else {
				m_stream << m_colorStart << " " << _("blend node");
			}
		} else {
			m_stream << m_colorStart << " " << m_startPercent << " " << _("blend") << " " << m_endPercent << " " << m_colorEnd;
		}
		return m_stream.str();
	}
private:
	std::stringstream m_stream;
	std::string m_colorStart, m_colorEnd;
	int m_startPercent, m_endPercent, m_steps, m_stage;
	bool m_isColorItem;
};
struct BlendColorsArgs {
	ColorSource source;
	GtkWidget *main, *mixType, *stepsSpinButton1, *stepsSpinButton2, *startColor, *middleColor, *endColor, *lastFocusedColor;
	ColorList *previewColorList;
	dynv::Ref options;
	GlobalState *gs;
	ColorObject colorObject;
	void add(const Color &color, int step, BlendColorNameAssigner &nameAssigner) {
		colorObject.setColor(color);
		nameAssigner.assign(&colorObject, &color, step);
		color_list_add_color_object(previewColorList, colorObject, true);
	}
	void addToPalette() {
		colorObject = getColor();
		color_list_add_color_object(gs->getColorList(), colorObject, true);
	}
	const ColorObject &getColor() {
		Color color;
		gtk_color_get_color(GTK_COLOR(lastFocusedColor), &color);
		colorObject.setColor(color);
		BlendColorNameAssigner nameAssigner(gs);
		auto name = color_names_get(gs->getColorNames(), &color, false);
		nameAssigner.setNames(name, name);
		nameAssigner.assign(&colorObject, &color);
		return colorObject;
	}
	void setColor(const ColorObject &colorObject) {
		gtk_color_set_color(GTK_COLOR(lastFocusedColor), colorObject.getColor());
		update();
	}
	void setActiveWidget(int index) {
		switch (index) {
		case 0: lastFocusedColor = startColor; break;
		case 1: lastFocusedColor = middleColor; break;
		case 2: lastFocusedColor = endColor; break;
		}
	}
	void setActiveWidget(GtkWidget *widget) {
		lastFocusedColor = widget;
	}
	static void onResetMiddleColor(GtkWidget *, BlendColorsArgs *args) {
		Color a, b;
		gtk_color_get_color(GTK_COLOR(args->startColor), &a);
		gtk_color_get_color(GTK_COLOR(args->endColor), &b);
		color_multiply(&a, 0.5f);
		color_multiply(&b, 0.5f);
		color_add(&a, &b);
		gtk_color_set_color(GTK_COLOR(args->middleColor), a);
		args->update();
	}
	static void onChange(GtkWidget *, BlendColorsArgs *args) {
		args->update();
	}
	void update(int limit = 101) {
		int steps1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(stepsSpinButton1));
		int steps2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(stepsSpinButton2));
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(mixType));
		Color r, a, b;
		BlendColorNameAssigner nameAssigner(gs);
		color_list_remove_all(previewColorList);
		for (int stage = 0; stage < 2; stage++) {
			int steps;
			if (stage == 0) {
				steps = steps1 + 1;
				gtk_color_get_color(GTK_COLOR(startColor), &a);
				gtk_color_get_color(GTK_COLOR(middleColor), &b);
			} else {
				steps = steps2 + 1;
				gtk_color_get_color(GTK_COLOR(middleColor), &a);
				gtk_color_get_color(GTK_COLOR(endColor), &b);
			}
			auto startName = color_names_get(gs->getColorNames(), &a, false);
			auto endName = color_names_get(gs->getColorNames(), &b, false);
			nameAssigner.setNames(startName, endName);
			nameAssigner.setStepsAndStage(steps, stage);
			int i = stage;
			switch (type) {
			case 0:
				color_rgb_get_linear(&a, &a);
				color_rgb_get_linear(&b, &b);
				for (; i < steps; ++i) {
					color_utils::mix(a, b, i / static_cast<float>(steps - 1), r);
					color_linear_get_rgb(&r, &r);
					add(r, i, nameAssigner);
				}
				break;
			case 1: {
				Color a_hsv, b_hsv, r_hsv;
				color_rgb_to_hsv(&a, &a_hsv);
				color_rgb_to_hsv(&b, &b_hsv);
				if (a_hsv.hsv.hue > b_hsv.hsv.hue) {
					if (a_hsv.hsv.hue - b_hsv.hsv.hue > 0.5)
						a_hsv.hsv.hue -= 1;
				} else {
					if (b_hsv.hsv.hue - a_hsv.hsv.hue > 0.5)
						b_hsv.hsv.hue -= 1;
				}
				for (; i < steps; ++i) {
					color_utils::mix(a_hsv, b_hsv, i / static_cast<float>(steps - 1), r_hsv);
					if (r_hsv.hsv.hue < 0) r_hsv.hsv.hue += 1;
					color_hsv_to_rgb(&r_hsv, &r);
					add(r, i, nameAssigner);
				}
			} break;
			case 2: {
				Color a_lab, b_lab, r_lab;
				color_rgb_to_lab_d50(&a, &a_lab);
				color_rgb_to_lab_d50(&b, &b_lab);
				for (; i < steps; ++i) {
					color_utils::mix(a_lab, b_lab, i / static_cast<float>(steps - 1), r_lab);
					color_lab_to_rgb_d50(&r_lab, &r);
					color_rgb_normalize(&r);
					add(r, i, nameAssigner);
				}
			} break;
			case 3: {
				Color a_lch, b_lch, r_lch;
				color_rgb_to_lch_d50(&a, &a_lch);
				color_rgb_to_lch_d50(&b, &b_lch);
				if (a_lch.lch.h > b_lch.lch.h) {
					if (a_lch.lch.h - b_lch.lch.h > 180)
						a_lch.lch.h -= 360;
				} else {
					if (b_lch.lch.h - a_lch.lch.h > 180)
						b_lch.lch.h -= 360;
				}
				for (; i < steps; ++i) {
					color_utils::mix(a_lch, b_lch, i / static_cast<float>(steps - 1), r_lch);
					if (r_lch.lch.h < 0) r_lch.lch.h += 360;
					color_lch_to_rgb_d50(&r_lch, &r);
					color_rgb_normalize(&r);
					add(r, i, nameAssigner);
				}
			} break;
			}
		}
	}
	struct Editable: IEditableColorUI, IMenuExtension {
		Editable(BlendColorsArgs *args, int index):
			args(args),
			index(index) {
		}
		virtual ~Editable() = default;
		virtual void addToPalette(const ColorObject &) override {
			args->setActiveWidget(index);
			args->addToPalette();
		}
		virtual void setColor(const ColorObject &colorObject) override {
			args->setActiveWidget(index);
			args->setColor(colorObject.getColor());
		}
		virtual const ColorObject &getColor() override {
			args->setActiveWidget(index);
			return args->getColor();
		}
		virtual bool isEditable() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return true;
		}
		virtual void extendMenu(GtkWidget *menu, Position position) {
			if (position != Position::end || index != 1)
				return;
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
			auto item = gtk_menu_item_new_with_mnemonic(_("_Reset"));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(onResetMiddleColor), args);
		}
	private:
		BlendColorsArgs *args;
		int index;
	};
	std::vector<Editable> editables;
};
static int getColor(BlendColorsArgs *args, ColorObject **color) {
	return -1;
}
static int setColor(BlendColorsArgs *args, ColorObject *color) {
	return -1;
}
static int activate(BlendColorsArgs *args) {
	return 0;
}
static int deactivate(BlendColorsArgs *args) {
	return 0;
}
static int destroy(BlendColorsArgs *args) {
	int steps1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->stepsSpinButton1));
	int steps2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(args->stepsSpinButton2));
	int type = gtk_combo_box_get_active(GTK_COMBO_BOX(args->mixType));
	args->options->set("type", type);
	args->options->set("steps1", steps1);
	args->options->set("steps2", steps2);
	Color color;
	gtk_color_get_color(GTK_COLOR(args->startColor), &color);
	args->options->set("start_color", color);
	gtk_color_get_color(GTK_COLOR(args->middleColor), &color);
	args->options->set("middle_color", color);
	gtk_color_get_color(GTK_COLOR(args->endColor), &color);
	args->options->set("end_color", color);
	color_list_destroy(args->previewColorList);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}
static ColorSource *source_implement(ColorSource *source, GlobalState *gs, const dynv::Ref &options) {
	auto *args = new BlendColorsArgs;
	args->editables.emplace_back(args, 0);
	args->editables.emplace_back(args, 1);
	args->editables.emplace_back(args, 2);
	args->options = options;
	args->gs = gs;
	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *))destroy;
	args->source.get_color = (int (*)(ColorSource *, ColorObject **))getColor;
	args->source.set_color = (int (*)(ColorSource *, ColorObject *))setColor;
	args->source.deactivate = (int (*)(ColorSource *))deactivate;
	args->source.activate = (int (*)(ColorSource *))activate;
	GtkWidget *table, *widget;
	int table_y = 0;
	table = gtk_table_new(6, 2, false);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Start:"), 0, 0, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->startColor = widget = gtk_color_new(args->options->getColor("start_color", Color(0.5f)), ColorWidgetConfiguration::standard);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	StandardEventHandler::forWidget(widget, args->gs, &args->editables[0]);
	StandardDragDropHandler::forWidget(widget, args->gs, &args->editables[0]);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Middle:"), 0, 0, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->middleColor = widget = gtk_color_new(args->options->getColor("middle_color", Color(0.5f)), ColorWidgetConfiguration::standard);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	StandardEventHandler::forWidget(widget, args->gs, &args->editables[1]);
	StandardDragDropHandler::forWidget(widget, args->gs, &args->editables[1]);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("End:"), 0, 0, 0, 0), 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->endColor = widget = gtk_color_new(args->options->getColor("end_color", Color(0.5f)), ColorWidgetConfiguration::standard);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 0, 0);
	StandardEventHandler::forWidget(widget, args->gs, &args->editables[2]);
	StandardDragDropHandler::forWidget(widget, args->gs, &args->editables[2]);
	table_y = 0;
	GtkWidget *vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_label_aligned_new(_("Type:"), 0, 0, 0, 0), false, false, 0);
	args->mixType = widget = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("RGB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("HSV"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("LAB"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), _("LCH"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), args->options->getInt32("type", 0));
	gtk_box_pack_start(GTK_BOX(vbox), widget, false, false, 0);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(BlendColorsArgs::onChange), args);
	gtk_table_attach(GTK_TABLE(table), vbox, 4, 5, table_y, table_y + 3, GtkAttachOptions(GTK_FILL), GtkAttachOptions(GTK_FILL), 5, 0);
	table_y = 0;

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Start steps:"), 0, 0, 0, 0), 2, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->stepsSpinButton1 = widget = gtk_spin_button_new_with_range(1, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), args->options->getInt32("steps1", 3));
	gtk_table_attach(GTK_TABLE(table), widget, 3, 4, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(BlendColorsArgs::onChange), args);

	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("End steps:"), 0, 0, 0, 0), 2, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 5, 5);
	args->stepsSpinButton2 = widget = gtk_spin_button_new_with_range(1, 255, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), args->options->getInt32("steps2", 3));
	gtk_table_attach(GTK_TABLE(table), widget, 3, 4, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 0);
	table_y++;
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(BlendColorsArgs::onChange), args);
	table_y = 3;
	ColorList *previewColorList = nullptr;
	gtk_table_attach(GTK_TABLE(table), palette_list_preview_new(gs, false, false, gs->getColorList(), &previewColorList), 0, 5, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	table_y++;
	args->previewColorList = previewColorList;
	args->update();
	gtk_widget_show_all(table);
	args->main = table;
	args->source.widget = table;
	return (ColorSource *)args;
}
int blend_colors_source_register(ColorSourceManager *csm) {
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "blend_colors", _("Blend colors"));
	color_source->implement = source_implement;
	color_source->default_accelerator = GDK_KEY_b;
	color_source_manager_add_source(csm, color_source);
	return 0;
}
