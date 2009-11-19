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
#include "gtk/Zoomed.h"
#include "gtk/ColorWidget.h"
#include "uiUtilities.h"
#include "uiApp.h"
#include "GlobalStateStruct.h"
#include "ColorPicker.h"
#include "Converter.h"
#include "DynvHelpers.h"
#include <gdk/gdkkeysyms.h>

using namespace math;

struct uiStatusIcon{
	GtkWidget* parent;
	GtkWidget* fake_window;
	GtkWidget* window;
	GtkWidget* zoomed;
	//GtkWidget* color_code;
	GtkWidget* color_widget;
	
	GtkStatusIcon* status_icon;
	
	ColorSource *color_source;
	
	GlobalState* gs;
	
	bool release_mode;
};

static void status_icon_get_color_sample(struct uiStatusIcon *status_icon, bool updateWidgets, Color* c){

	GdkWindow* root_window;
	GdkModifierType state;
	
	root_window = gdk_get_default_root_window();
	int x, y;
	int width, height;
	gdk_window_get_pointer(root_window, &x, &y, &state);
	gdk_window_get_geometry(root_window, NULL, NULL, &width, &height, NULL);
	
	Vec2<int> pointer(x,y);
	Vec2<int> window_size(width, height);
	
	screen_reader_reset_rect(status_icon->gs->screen_reader);
	Rect2<int> sampler_rect, zoomed_rect, final_rect;

	sampler_get_screen_rect(status_icon->gs->sampler, pointer, window_size, &sampler_rect);
	screen_reader_add_rect(status_icon->gs->screen_reader, sampler_rect);

	if (updateWidgets){
		gtk_zoomed_get_screen_rect(GTK_ZOOMED(status_icon->zoomed), pointer, window_size, &zoomed_rect);
		screen_reader_add_rect(status_icon->gs->screen_reader, zoomed_rect);
	}
		
	screen_reader_update_pixbuf(status_icon->gs->screen_reader, &final_rect);
	
	Vec2<int> offset;
	
	offset = Vec2<int>(sampler_rect.getX()-final_rect.getX(), sampler_rect.getY()-final_rect.getY());
	sampler_get_color_sample(status_icon->gs->sampler, pointer, window_size, offset, c);
	
	if (updateWidgets){
		offset = Vec2<int>(zoomed_rect.getX()-final_rect.getX(), zoomed_rect.getY()-final_rect.getY());
		gtk_zoomed_update(GTK_ZOOMED(status_icon->zoomed), pointer, window_size, offset, screen_reader_get_pixbuf(status_icon->gs->screen_reader));
	}
}

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
	
	struct dynvSystem *params = dynv_get_dynv(si->gs->params, "gpick.main");
	main_show_window(si->parent, params);
	dynv_system_release(params);
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
	//TODO: gtk_zoomed_update(GTK_ZOOMED(si->zoomed));
	
	
	
	Color c;
	//sampler_get_color_sample(si->gs->sampler, &c);
	status_icon_get_color_sample(si, true, &c);
	
		
	struct ColorObject* color_object;
	color_object = color_list_new_color_object(si->gs->colors, &c);
		
	gchar* text = 0;
	
	Converters *converters = (Converters*)dynv_system_get(si->gs->params, "ptr", "Converters");
	Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_DISPLAY);
	if (converter){
		converter_get_text(converter->function_name, color_object, 0, si->gs->params, &text);
	}
	
	color_object_release(color_object);
	
	gtk_color_set_color(GTK_COLOR(si->color_widget), &c, text);
	
	if (text){
		//gtk_label_set_text(GTK_LABEL(si->color_code), text);
		g_free(text);
	}else{
		//gtk_label_set_text(GTK_LABEL(si->color_code), "");
	}
	
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
	
	si->release_mode = true;
	
	GdkCursor* cursor;
	cursor = gdk_cursor_new(GDK_TCROSS);
	
	gtk_zoomed_set_zoom(GTK_ZOOMED(si->zoomed), dynv_get_float_wd(si->gs->params, "gpick.picker.zoom", 2));
	
	gtk_widget_show(si->fake_window);
	GdkEventMotion event;
	

	GdkWindow* root_window = gdk_get_default_root_window();
	GdkModifierType state;
	gint x, y;
	gdk_window_get_pointer(root_window, &x, &y, &state);
	
	event.x_root = x;
	event.y_root = y;
	status_icon_motion_notify(si->window, &event, si);
	gtk_widget_show(si->window);
	

	//gdk_window_set_cursor(si->fake_window->window, cursor);
	
	
	gdk_pointer_grab(si->fake_window->window, FALSE, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK	), NULL, cursor, GDK_CURRENT_TIME);
	gdk_keyboard_grab(si->fake_window->window, FALSE, GDK_CURRENT_TIME);
	
	gdk_cursor_destroy(cursor);
}


static gboolean status_icon_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	
	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 1)) {
		if (si->release_mode){
			Color c;
			//sampler_get_color_sample(si->gs->sampler, &c);
			status_icon_get_color_sample(si, false, &c);
		
			struct ColorObject* color_object;
			color_object = color_list_new_color_object(si->gs->colors, &c);
		
			Converters *converters = (Converters*)dynv_system_get(si->gs->params, "ptr", "Converters");
			Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
			if (converter){
				converter_get_clipboard(converter->function_name, color_object, 0, si->gs->params);
			}

			color_object_release(color_object);
		}
		
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	
		gtk_widget_hide(si->fake_window);
		gtk_widget_hide(si->window);
	
		dynv_set_float(si->gs->params, "gpick.picker.zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(si->zoomed)));
	}
}



static gboolean status_icon_key_up (GtkWidget *widget, GdkEventKey *event, gpointer user_data){
	struct uiStatusIcon* si = (struct uiStatusIcon*)user_data;
	
	GdkEventButton event2;
	
	switch(event->keyval){
	case GDK_Escape:
		event2.type = GDK_BUTTON_RELEASE;
		event2.button = 1;
		status_icon_button_release(widget, &event2, user_data);
		return TRUE;
		break;
	default:
		if (color_picker_key_up(si->color_source, event)){
			si->release_mode = false;	//key pressed and color picked, disable copy on mouse button release
			return TRUE;
		}
	}
	return FALSE;
}

void status_icon_set_visible(struct uiStatusIcon* si, bool visible){
	if (visible==false){
		gdk_pointer_ungrab(GDK_CURRENT_TIME);
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
		
		gtk_widget_hide(si->fake_window);
		gtk_widget_hide(si->window);
	}
	gtk_status_icon_set_visible (si->status_icon, visible);
}

struct uiStatusIcon* status_icon_new(GtkWidget* parent, GlobalState* gs, ColorSource* color_source){
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
	
	GtkWidget* vbox = gtk_vbox_new(0, 0);
	gtk_container_add (GTK_CONTAINER(si->window), vbox);
	gtk_widget_show(vbox);
	
	si->zoomed = gtk_zoomed_new();
	gtk_widget_show(si->zoomed);
	gtk_box_pack_start(GTK_BOX(vbox), si->zoomed, FALSE, FALSE, 0);
	
	si->color_widget = gtk_color_new();
	gtk_widget_show(si->color_widget);
	gtk_box_pack_start(GTK_BOX(vbox), si->color_widget, TRUE, TRUE, 0);
	
	
	GtkStatusIcon *status_icon = gtk_status_icon_new();
	gtk_status_icon_set_visible (status_icon, FALSE);
	gtk_status_icon_set_from_icon_name(status_icon, "gpick");
	g_signal_connect(G_OBJECT(status_icon), "popup-menu", G_CALLBACK(status_icon_popup), si);
	
#ifndef WIN32
	g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(status_icon_activate), si);
#endif

	g_signal_connect(G_OBJECT(si->fake_window), "motion-notify-event", G_CALLBACK(status_icon_motion_notify), si);
	g_signal_connect(G_OBJECT(si->fake_window), "scroll_event", G_CALLBACK(status_icon_scroll_event), si);
	g_signal_connect(G_OBJECT(si->fake_window), "button-release-event", G_CALLBACK(status_icon_button_release), si);
	g_signal_connect(G_OBJECT(si->fake_window), "key_press_event", G_CALLBACK (status_icon_key_up), si);

	si->status_icon = status_icon;
	si->color_source = color_source;
	
	return si;
}

void status_icon_destroy(struct uiStatusIcon* si){
	
	gtk_widget_destroy(si->window);
	gtk_widget_destroy(si->fake_window);
	
	delete si;

}

