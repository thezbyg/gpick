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

#include "uiTemporaryPalette.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "IPalette.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "I18N.h"
namespace {
struct TemporaryPalette: public IPalette {
	GlobalState &gs;
	dynv::Ref options;
	ColorList colorList;
	GtkWindow *window;
	GtkWidget *palette;
	TemporaryPalette(GlobalState &gs, GtkWindow *parent):
		gs(gs),
		colorList(*this) {
		options = gs.settings().getOrCreateMap("gpick.temporary_palette");
		window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
		gtk_window_set_title(window, _("Temporary palette"));
		gtk_window_set_transient_for(window, parent);
		gtk_window_set_destroy_with_parent(window, true);
		gtk_window_set_skip_taskbar_hint(window, true);
		gtk_window_set_skip_pager_hint(window, true);
		gtk_window_set_default_size(window, options->getInt32("window.width", -1), options->getInt32("window.height", -1));
		GtkWidget *informationLabel = gtk_label_new("");
#if GTK_MAJOR_VERSION >= 3
		gtk_widget_set_halign(informationLabel, GTK_ALIGN_END);
		gtk_widget_set_margin_end(informationLabel, 5);
#else
		gtk_misc_set_alignment(GTK_MISC(informationLabel), 1.0f, 0.5f);
#endif
		GtkWidget* vbox = gtk_vbox_new(false, 0);
		palette = palette_list_temporary_new(gs, informationLabel, colorList);
		auto columnWidths = options->getInt32s("column_widths");
		for (size_t i = 0; ; ++i) {
			GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(palette), i);
			if (!column)
				break;
			if (i < columnWidths.size())
				gtk_tree_view_column_set_fixed_width(column, columnWidths[i]);
		}
		GtkWidget *scrolledWindow = gtk_scrolled_window_new(0, 0);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_IN);
		gtk_container_add(GTK_CONTAINER(scrolledWindow), palette);
		gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, true, true, 0);
		gtk_box_pack_start(GTK_BOX(vbox), informationLabel, false, false, 5);
		gtk_container_add(GTK_CONTAINER(window), vbox);
		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(TemporaryPalette::onDestroy), this);
		g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(TemporaryPalette::onConfigureEvent), this);
		gtk_widget_show_all(GTK_WIDGET(window));
	}
	virtual ~TemporaryPalette() {
		std::vector<int> columnWidths;
		for (size_t i = 0; ; ++i) {
			GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(palette), i);
			if (!column)
				break;
			columnWidths.push_back(gtk_tree_view_column_get_width(column));
		}
		options->set("column_widths", columnWidths);
	}
	void updateWindowSize() {
		if (gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(window))) & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_ICONIFIED))
			return;
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(window), &width, &height);
		options->set("window.width", width);
		options->set("window.height", height);
	}
	static void onDestroy(GtkWidget *, TemporaryPalette *temporaryPalette) {
		delete temporaryPalette;
	}
	static gboolean onConfigureEvent(GtkWidget *, GdkEventConfigure *, TemporaryPalette *temporaryPalette) {
		temporaryPalette->updateWindowSize();
		return false;
	}
	virtual void add(ColorList &colorList, ColorObject *colorObject) override {
		palette_list_add_entry(palette, colorObject, !colorList.blocked());
	}
	virtual void remove(ColorList &colorList, ColorObject *colorObject) override {
		palette_list_remove_entry(palette, colorObject, !colorList.blocked());
	}
	virtual void removeSelected(ColorList &colorList) override {
		palette_list_remove_selected_entries(palette, !colorList.blocked());
	}
	virtual void clear(ColorList &colorList) override {
		palette_list_remove_all_entries(palette, !colorList.blocked());
	}
	virtual void update(ColorList &) override {
		palette_list_after_update(palette);
	}
};
}
void temporary_palette_show(GtkWindow *parent, GlobalState &gs) {
	new TemporaryPalette(gs, parent);
}
