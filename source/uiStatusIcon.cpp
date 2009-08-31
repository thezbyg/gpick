/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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
#include "uiUtilities.h"
#include "gtk/Zoomed.h"
#include "main.h"
#include "uiConverter.h"

struct uiStatusIcon{
	GtkWidget* parent;
	GtkWidget* fake_window;
	GtkWidget* window;
	GtkWidget* zoomed;
	
	GtkStatusIcon* status_icon;
	
	GlobalState* gs;
};

static void status_icon_popup_detach(GtkWidget *attach_widget, GtkMenu *menu){
	gtk_widget_destroy(GTK_WIDGET(menu));	
}

static void status_icon_destroy_parent(GtkWidget *widget, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	gtk_widget_destroy(GTK_WIDGET(si->parent));	
}

static void status_icon_show_parent(GtkWidget *widget, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	
	gtk_widget_hide(si->fake_window);
	gtk_widget_hide(si->window);
	status_icon_set_visible(si, false);
	
	main_show_window(si->parent, si->gs->settings);
}


static void status_icon_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;

	GtkMenu* menu;
	GtkWidget* item;
	GtkWidget* file_item ;

	menu = GTK_MENU(gtk_menu_new ());

    item = gtk_menu_item_new_with_image ("_Show Main Window", gtk_image_new_from_icon_name("gpick", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (status_icon_show_parent), si);


    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new());

    item = gtk_menu_item_new_with_image ("_Quit", gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (status_icon_destroy_parent), si);

    gtk_widget_show_all (GTK_WIDGET(menu));

	gtk_menu_attach_to_widget(GTK_MENU (menu), GTK_WIDGET(si->parent), status_icon_popup_detach );


	//g_signal_connect (G_OBJECT (menu), "leave-notify-event", G_CALLBACK (tray_popup_leave), menu);


	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 0, 0, button, activate_time);
}

static gboolean status_icon_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	
	gint x = event->x_root;
	gint y = event->y_root;
	
	gint sx, sy;
	
	gtk_window_get_size(GTK_WINDOW(si->window), &sx, &sy);
	
	if (x+sx+sx/2 > gdk_screen_width()){
		x -= sx + sx/2;
	}else{
		x += sx/2;
	}
	
	if (y+sy+sy/2 > gdk_screen_height()){
		y -= sy + sy/2;
	}else{
		y += sy/2;
	}
	
	gtk_window_move(GTK_WINDOW(si->window), x, y );
	gtk_zoomed_update(GTK_ZOOMED(si->zoomed));
	
	return TRUE;
}

static gboolean status_icon_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	double zoom = gtk_zoomed_get_zoom(GTK_ZOOMED(si->zoomed));
	
	if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_RIGHT)){
		zoom += 1;
	}else{
		zoom -= 1;
	}
	
	gtk_zoomed_set_zoom(GTK_ZOOMED(si->zoomed), zoom);

	return TRUE;
}
                                                        
                                                        

static void status_icon_activate(GtkWidget *widget, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	
	gtk_zoomed_set_zoom(GTK_ZOOMED(si->zoomed), g_key_file_get_double_with_default(si->gs->settings, "Zoom", "Zoom", 2));
	
	gtk_widget_show(si->fake_window);
	gdk_pointer_grab(si->fake_window->window, FALSE, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK	), NULL, NULL, GDK_CURRENT_TIME);
	
	GdkEventMotion event;
	

	GdkWindow* root_window = gdk_get_default_root_window();
	GdkModifierType state;
	gint x, y;
	gdk_window_get_pointer(root_window, &x, &y, &state);
	
	event.x_root = x;
	event.y_root = y;
	status_icon_motion_notify(si->window, &event, si);
	gtk_widget_show(si->window);
	
	GdkCursor* cursor;
	cursor = gdk_cursor_new(GDK_TCROSS);
	gdk_window_set_cursor(si->fake_window->window, cursor);
	gdk_cursor_destroy(cursor);

}



static gboolean status_icon_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	
	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 1)) {
	
		Color c;
		sampler_get_color_sample(si->gs->sampler, &c);
		
		struct ColorObject* color_object;
		color_object = color_list_new_color_object(si->gs->colors, &c);
		
		gchar** source_array;
		gsize source_array_size;
		if ((source_array = g_key_file_get_string_list(si->gs->settings, "Converter", "Names", &source_array_size, 0))){
			if (source_array_size>0){	
				converter_get_clipboard(source_array[0], color_object, 0, si->gs->lua);
			}					
			g_strfreev(source_array);
		}
		color_object_release(color_object);
	
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		
		gtk_widget_hide(si->fake_window);
		gtk_widget_hide(si->window);
		
		g_key_file_set_double(si->gs->settings, "Zoom", "Zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(si->zoomed)));
	
	}
	
}


void status_icon_set_visible(struct uiStatusIcon* si, bool visible){
	if (visible==false){
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gtk_widget_hide(si->fake_window);
		gtk_widget_hide(si->window);
	}
	gtk_status_icon_set_visible (si->status_icon, visible);
}

struct uiStatusIcon* status_icon_new(GtkWidget* parent, GlobalState* gs){
	struct uiStatusIcon *si = new struct uiStatusIcon;
	
	si->gs = gs;
	si->parent = parent;
	si->fake_window = gtk_window_new(GTK_WINDOW_POPUP);
	
	gtk_window_set_skip_pager_hint(GTK_WINDOW(si->fake_window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(si->fake_window), TRUE);
	gtk_window_set_decorated(GTK_WINDOW(si->fake_window), FALSE);
	gtk_widget_set_size_request(si->fake_window, 0, 0);
	
	si->window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(si->window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(si->window), TRUE);
	gtk_window_set_decorated(GTK_WINDOW(si->window), FALSE);
	gtk_widget_set_size_request(si->window, -1, -1);
	
	si->zoomed = gtk_zoomed_new();
	gtk_container_add (GTK_CONTAINER(si->window), si->zoomed);
	gtk_widget_show(si->zoomed);
	
	
	GtkStatusIcon *status_icon = gtk_status_icon_new();
	gtk_status_icon_set_visible (status_icon, FALSE);
	gtk_status_icon_set_from_icon_name(status_icon, "gpick");
	g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(status_icon_popup), si);
	g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(status_icon_activate), si);
	
	g_signal_connect(G_OBJECT(si->fake_window), "motion-notify-event", G_CALLBACK(status_icon_motion_notify), si);
	g_signal_connect(G_OBJECT(si->fake_window), "scroll_event", G_CALLBACK(status_icon_scroll_event), si);
	g_signal_connect(G_OBJECT(si->fake_window), "button-release-event", G_CALLBACK(status_icon_button_release), si);

	
	si->status_icon = status_icon;
	
	//gtk_status_icon_set_visible (statusIcon, TRUE);
	return si;
}

void status_icon_destroy(struct uiStatusIcon* si){
	
	gtk_widget_destroy(si->window);
	gtk_widget_destroy(si->fake_window);
	
	delete si;

}


