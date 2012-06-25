/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#include "uiStatusIcon.h"
#include "gtk/Zoomed.h"
#include "gtk/ColorWidget.h"
#include "uiUtilities.h"
#include "uiApp.h"
#include "GlobalStateStruct.h"
#include "ColorPicker.h"
#include "FloatingPicker.h"
#include "Converter.h"
#include "DynvHelpers.h"
#include "Internationalisation.h"
#include <gdk/gdkkeysyms.h>

using namespace math;

struct uiStatusIcon{
	GtkWidget* parent;
	GtkStatusIcon* status_icon;
	FloatingPicker floating_picker;
	GlobalState* gs;
};

static void status_icon_destroy_parent(GtkWidget *widget, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	gtk_widget_destroy(GTK_WIDGET(si->parent));
}

static void status_icon_show_parent(GtkWidget *widget, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;

	floating_picker_deactivate(si->floating_picker);

	status_icon_set_visible(si, false);

	struct dynvSystem *params = dynv_get_dynv(si->gs->params, "gpick.main");
	main_show_window(si->parent, params);
	dynv_system_release(params);
}


static void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;

	GtkMenu* menu;
	GtkWidget* item;

	menu = GTK_MENU(gtk_menu_new ());

    item = gtk_menu_item_new_with_image (_("_Show Main Window"), gtk_image_new_from_icon_name("gpick", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (status_icon_show_parent), si);


    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());

    item = gtk_menu_item_new_with_image (_("_Quit"), gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (status_icon_destroy_parent), si);

    gtk_widget_show_all (GTK_WIDGET(menu));

	//g_signal_connect (G_OBJECT (menu), "leave-notify-event", G_CALLBACK (tray_popup_leave), menu);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 0, 0, button, activate_time);

	g_object_ref_sink(menu);
	g_object_unref(menu);
}


static void status_icon_activate(GtkWidget *widget, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;

	floating_picker_activate(si->floating_picker, false);

}

void status_icon_set_visible(struct uiStatusIcon* si, bool visible){
	if (visible==false){
		floating_picker_deactivate(si->floating_picker);
	}
	gtk_status_icon_set_visible(si->status_icon, visible);
}

struct uiStatusIcon* status_icon_new(GtkWidget* parent, GlobalState* gs, FloatingPicker floating_picker){
	struct uiStatusIcon *si = new struct uiStatusIcon;

	si->gs = gs;
	si->parent = gtk_widget_get_toplevel(parent);

	GtkStatusIcon *status_icon = gtk_status_icon_new();
	gtk_status_icon_set_visible (status_icon, FALSE);
	gtk_status_icon_set_from_icon_name(status_icon, "gpick");
	g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(status_icon_popup), si);

#ifndef WIN32
	g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(status_icon_activate), si);
#endif

	si->floating_picker = floating_picker;

	si->status_icon = status_icon;
	//si->color_source = color_source;

	return si;
}

void status_icon_destroy(struct uiStatusIcon* si){
	//floating_picker_free(si->floating_picker);
	delete si;
}

