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

#include "uiDialogMix.h"
#include "uiDialogBase.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "I18N.h"
#include <sstream>
namespace {
struct MixColorNameAssigner: public ToolColorNameAssigner {
	MixColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
		m_isNode = false;
	}
	void setStartName(std::string_view name) {
		m_startName = name;
	}
	void setEndName(std::string_view name) {
		m_endName = name;
	}
	void setStepsAndStage(int steps, int stage) {
		m_steps = steps;
		m_stage = stage;
	}
	void assign(ColorObject &colorObject, int step) {
		m_startPercent = step * 100 / (m_steps - 1);
		m_endPercent = 100 - (step * 100 / (m_steps - 1));
		m_isNode = (((step == 0 || step == m_steps - 1) && m_stage == 0) || (m_stage == 1 && step == m_steps - 1));
		ToolColorNameAssigner::assign(colorObject);
	}
	void assign(ColorObject &colorObject, std::string_view name) {
		m_startName = name;
		m_isNode = false;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		if (m_isNode) {
			if (m_endPercent == 100) {
				m_stream << m_endName << " " << _("mix node");
			} else {
				m_stream << m_startName << " " << _("mix node");
			}
		} else {
			m_stream << m_startName << " " << m_startPercent << " " << _("mix") << " " << m_endPercent << " " << m_endName;
		}
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_startName, m_endName;
	int m_startPercent, m_endPercent, m_steps, m_stage;
	bool m_isNode;
};
struct MixDialog: public DialogBase {
	ColorList &selectedColorList;
	GtkWidget *mixTypeCombo, *mixStepsSpin, *endpointsToggle, *previewExpander;
	MixDialog(ColorList &selectedColorList, GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.mix", _("Mix colors"), parent),
		selectedColorList(selectedColorList) {
		Grid grid(2, 4);
		grid.addLabel(_("Type:"));
		grid.add(mixTypeCombo = gtk_combo_box_text_new(), true);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mixTypeCombo), _("RGB"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mixTypeCombo), _("HSV"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mixTypeCombo), _("LAB"));
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mixTypeCombo), _("LCH"));
		gtk_combo_box_set_active(GTK_COMBO_BOX(mixTypeCombo), options->getInt32("type", 0));
		g_signal_connect(G_OBJECT(mixTypeCombo), "changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Steps:"));
		grid.add(mixStepsSpin = gtk_spin_button_new_with_range(3, 255, 1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(mixStepsSpin), options->getInt32("steps", 3));
		g_signal_connect(G_OBJECT(mixStepsSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.nextColumn().add(endpointsToggle = gtk_check_button_new_with_mnemonic(_("_Include Endpoints")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(endpointsToggle), options->getBool("includeendpoints", true));
		g_signal_connect(G_OBJECT(endpointsToggle), "toggled", G_CALLBACK(onUpdate), this);
		grid.add(previewExpander = palette_list_preview_new(gs, true, options->getBool("show_preview", true), previewColorList), true, 2, true);
		apply(true);
		setContent(grid);
	}
	virtual ~MixDialog() {
		options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(previewExpander)));
	}
	virtual void apply(bool preview) override {
		if (preview)
			previewColorList->removeAll();
		int steps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(mixStepsSpin));
		int type = gtk_combo_box_get_active(GTK_COMBO_BOX(mixTypeCombo));
		bool withEndpoints = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endpointsToggle));
		int startStep = 0, maxStep = steps;
		MixColorNameAssigner nameAssigner(gs);
		if (!preview) {
			options->set("type", type);
			options->set("steps", steps);
			options->set("includeendpoints", withEndpoints);
		}
		if (withEndpoints == false) {
			startStep = 1;
			maxStep = steps - 1;
		}
		Color r;
		std::stringstream s;
		s.precision(0);
		s.setf(std::ios::fixed, std::ios::floatfield);
		Color a, b;
		ColorList &colorList = preview ? *previewColorList : gs.colorList();
		common::Guard colorListGuard = colorList.changeGuard();
		ColorList::iterator j;
		int count = 0;
		for (auto i = selectedColorList.begin(); i != selectedColorList.end(); ++i) {
			a = (*i)->getColor();
			if (type == 0)
				a.linearRgbInplace();
			nameAssigner.setStartName((*i)->getName());
			j = i;
			++j;
			for (; j != selectedColorList.end(); ++j) {
				b = (*j)->getColor();
				if (type == 0)
					b.linearRgbInplace();
				nameAssigner.setEndName((*j)->getName());
				nameAssigner.setStepsAndStage(steps, 0);

				switch (type) {
				case 0:
					for (int stepIndex = startStep; stepIndex < maxStep; ++stepIndex) {
						r = math::mix(a, b, stepIndex / (float)(steps - 1));
						r.nonLinearRgbInplace();
						addColor(colorList, r, stepIndex, nameAssigner);
						if (preview) {
							++count;
							if (count >= 100)
								return;
						}
					}
					break;

				case 1: {
					Color a_hsv, b_hsv, r_hsv;
					a_hsv = a.rgbToHsv();
					b_hsv = b.rgbToHsv();
					if (a_hsv.hsv.hue > b_hsv.hsv.hue) {
						if (a_hsv.hsv.hue - b_hsv.hsv.hue > 0.5f)
							a_hsv.hsv.hue -= 1;
					} else {
						if (b_hsv.hsv.hue - a_hsv.hsv.hue > 0.5f)
							b_hsv.hsv.hue -= 1;
					}
					for (int stepIndex = startStep; stepIndex < maxStep; ++stepIndex) {
						r_hsv = math::mix(a_hsv, b_hsv, stepIndex / (float)(steps - 1));
						if (r_hsv.hsv.hue < 0) r_hsv.hsv.hue += 1;
						r = r_hsv.hsvToRgb();
						addColor(colorList, r, stepIndex, nameAssigner);
						if (preview) {
							++count;
							if (count >= 100)
								return;
						}
					}
				} break;

				case 2: {
					Color a_lab, b_lab, r_lab;
					a_lab = a.rgbToLabD50();
					b_lab = b.rgbToLabD50();
					for (int stepIndex = startStep; stepIndex < maxStep; ++stepIndex) {
						r_lab = math::mix(a_lab, b_lab, stepIndex / (float)(steps - 1));
						r = r_lab.labToRgbD50().normalizeRgbInplace();
						addColor(colorList, r, stepIndex, nameAssigner);
						if (preview) {
							++count;
							if (count >= 100)
								return;
						}
					}
				} break;

				case 3: {
					Color a_lch, b_lch, r_lch;
					a_lch = a.rgbToLchD50();
					b_lch = b.rgbToLchD50();
					if (a_lch.lch.h > b_lch.lch.h) {
						if (a_lch.lch.h - b_lch.lch.h > 180)
							a_lch.lch.h -= 360;
					} else {
						if (b_lch.lch.h - a_lch.lch.h > 180)
							b_lch.lch.h -= 360;
					}
					for (int stepIndex = startStep; stepIndex < maxStep; ++stepIndex) {
						r_lch = math::mix(a_lch, b_lch, stepIndex / (float)(steps - 1));
						if (r_lch.lch.h < 0) r_lch.lch.h += 360;
						r = r_lch.lchToRgbD50().normalizeRgbInplace();
						addColor(colorList, r, stepIndex, nameAssigner);
						if (preview) {
							++count;
							if (count >= 100)
								return;
						}
					}
				} break;
				}
			}
		}
	}
	static void addColor(ColorList &colorList, const Color &color, int step, MixColorNameAssigner &nameAssigner) {
		ColorObject colorObject(color);
		nameAssigner.assign(colorObject, step);
		colorList.add(colorObject);
	}
};
}
void dialog_mix_show(GtkWindow *parent, GtkWidget *paletteWidget, GlobalState &gs) {
	ColorList colorList;
	palette_list_get_selected(paletteWidget, colorList);
	MixDialog(colorList, gs, parent).run();
}
