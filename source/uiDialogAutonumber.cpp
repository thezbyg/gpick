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

#include "uiDialogAutonumber.h"
#include "uiDialogBase.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorObject.h"
#include "I18N.h"
#include "dynv/Map.h"
#include <sstream>
#include <iomanip>
namespace {
struct AutonumberDialog: public DialogBase {
	GtkWidget *paletteWidget, *nameEntry, *decimalPlacesSpin, *startIndexSpin, *decreasingToggle, *appendToggle, *sampleEntry;
	int selectedCount;
	AutonumberDialog(GtkWidget *paletteWidget, GlobalState &gs, GtkWindow *parent):
		DialogBase(gs, "gpick.autonumber", _("Autonumber colors"), parent),
		paletteWidget(paletteWidget) {
		selectedCount = palette_list_get_selected_count(paletteWidget);
		Grid grid(2, 6);
		grid.addLabel(_("Name:"));
		grid.add(nameEntry = gtk_entry_new(), true);
		auto name = options->getString("name", "autonum");
		gtk_entry_set_text(GTK_ENTRY(nameEntry), name.c_str());
		g_signal_connect(G_OBJECT(nameEntry), "changed", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Decimal places:"));
		grid.add(decimalPlacesSpin = gtk_spin_button_new_with_range(1, 6, 1), true);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(decimalPlacesSpin), options->getInt32("nplaces", 3));
		g_signal_connect(G_OBJECT(decimalPlacesSpin), "value-changed", G_CALLBACK(onUpdate), this);
		bool decreasing = options->getBool("decreasing", false);
		grid.addLabel(_("Starting number:"));
		grid.add(startIndexSpin = gtk_spin_button_new_with_range(0, 0x7fffffff, 1), true);
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(startIndexSpin), decreasing ? selectedCount - 1 : 0, 0x7fffffff);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(startIndexSpin), options->getInt32("startindex", 1));
		g_signal_connect(G_OBJECT(startIndexSpin), "value-changed", G_CALLBACK(onUpdate), this);
		grid.nextColumn().add(decreasingToggle = gtk_check_button_new_with_mnemonic(_("_Decreasing")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decreasingToggle), decreasing);
		g_signal_connect(G_OBJECT(decreasingToggle), "toggled", G_CALLBACK(onDecreasingChange), this);
		grid.nextColumn().add(appendToggle = gtk_check_button_new_with_mnemonic(_("_Append")), true);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(appendToggle), options->getBool("append", false));
		g_signal_connect(G_OBJECT(appendToggle), "toggled", G_CALLBACK(onUpdate), this);
		grid.addLabel(_("Sample:"));
		grid.add(sampleEntry = gtk_entry_new(), true);
		gtk_editable_set_editable(GTK_EDITABLE(sampleEntry), false);
		gtk_widget_set_sensitive(sampleEntry, false);
		apply(true);
		setContent(grid);
	}
	virtual ~AutonumberDialog() {
	}
	virtual void apply(bool preview) override {
		int decimalPlaces = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(decimalPlacesSpin)));
		int startIndex = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(startIndexSpin)));
		bool append = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(appendToggle));
		bool decreasing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(decreasingToggle));
		std::string name = gtk_entry_get_text(GTK_ENTRY(nameEntry));
		if (preview) {
			std::stringstream ss;
			ss << name << "-";
			ss.fill('0');
			ss.width(decimalPlaces);
			ss << std::right << startIndex;
			auto text = ss.str();
			gtk_entry_set_text(GTK_ENTRY(sampleEntry), text.c_str());
		} else {
			options->set("name", name);
			options->set("append", append);
			options->set("decreasing", decreasing);
			options->set("nplaces", decimalPlaces);
			options->set("startindex", startIndex);
			int index = startIndex;
			palette_list_foreach(paletteWidget, true, [&](ColorObject *colorObject) {
				std::stringstream ss;
				if (append) {
					ss << colorObject->getName() << " ";
				}
				ss << name << "-";
				ss.width(decimalPlaces);
				ss.fill('0');
				if (decreasing) {
					ss << std::right << index;
					--index;
				} else {
					ss << std::right << index;
					++index;
				}
				colorObject->setName(ss.str());
				return Update::name;
			});
		}
	}
	void updateStartIndexRange() {
		int startIndex = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(startIndexSpin)));
		int newIndex;
		gdouble min, max;
		gtk_spin_button_get_range(GTK_SPIN_BUTTON(startIndexSpin), &min, &max);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(decreasingToggle))) {
			if (startIndex == 0) {
				newIndex = selectedCount - 1;
			} else {
				newIndex = selectedCount + startIndex;
			}
			min = selectedCount - 1;
		} else {
			newIndex = startIndex - selectedCount;
			min = 0;
		}
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(startIndexSpin), min, max);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(startIndexSpin), newIndex);
		apply(true);
	}
	static void onDecreasingChange(GtkWidget *, AutonumberDialog *dialog) {
		dialog->updateStartIndexRange();
	}
};
}
void dialog_autonumber_show(GtkWindow *parent, GtkWidget *paletteWidget, GlobalState &gs) {
	AutonumberDialog(paletteWidget, gs, parent).run();
}
