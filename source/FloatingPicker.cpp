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

#include "FloatingPicker.h"

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

struct Arguments{
	GtkWidget* parent;

	GtkWidget* window;
	GtkWidget* zoomed;
	GtkWidget* color_widget;
	
	guint timeout_source_id;
	
	ColorSource *color_source;
	
	GlobalState* gs;
	
	bool release_mode;
};

static void get_color_sample(struct Arguments *args, bool updateWidgets, Color* c){
	
	GdkScreen *screen;
	GdkModifierType state;
	
	int x, y;
	int width, height;

	gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, &state);
	width = gdk_screen_get_width(screen);
	height = gdk_screen_get_height(screen);
	
	Vec2<int> pointer(x,y);
	Vec2<int> window_size(width, height);
	
	screen_reader_reset_rect(args->gs->screen_reader);
	Rect2<int> sampler_rect, zoomed_rect, final_rect;

	sampler_get_screen_rect(args->gs->sampler, pointer, window_size, &sampler_rect);
	screen_reader_add_rect(args->gs->screen_reader, screen, sampler_rect);

	if (updateWidgets){
		gtk_zoomed_get_screen_rect(GTK_ZOOMED(args->zoomed), pointer, window_size, &zoomed_rect);
		screen_reader_add_rect(args->gs->screen_reader, screen, zoomed_rect);
	}
		
	screen_reader_update_pixbuf(args->gs->screen_reader, &final_rect);
	
	Vec2<int> offset;
	
	offset = Vec2<int>(sampler_rect.getX()-final_rect.getX(), sampler_rect.getY()-final_rect.getY());
	sampler_get_color_sample(args->gs->sampler, pointer, window_size, offset, c);
	
	if (updateWidgets){
		offset = Vec2<int>(zoomed_rect.getX()-final_rect.getX(), zoomed_rect.getY()-final_rect.getY());
		gtk_zoomed_update(GTK_ZOOMED(args->zoomed), pointer, window_size, offset, screen_reader_get_pixbuf(args->gs->screen_reader));
	}
}



static gboolean update_display(struct Arguments *args){
	
	GdkScreen *screen;
	GdkModifierType state;
	
	int x, y;
	int width, height;
	
	gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, &state);
	width = gdk_screen_get_width(screen);
	height = gdk_screen_get_height(screen);
	
	gint sx, sy;
	gtk_window_get_size(GTK_WINDOW(args->window), &sx, &sy);
	
	if (x+sx+sx/2 > width){
		x -= sx + sx/2;
	}else{
		x += sx/2;
	}
	
	if (y+sy+sy/2 > height){
		y -= sy + sy/2;
	}else{
		y += sy/2;
	}
	
	if (gtk_window_get_screen(GTK_WINDOW(args->window)) != screen){
		gtk_window_set_screen(GTK_WINDOW(args->window), screen);
	}	
	gtk_window_move(GTK_WINDOW(args->window), x, y );

	
	Color c;
	get_color_sample(args, true, &c);
	
	struct ColorObject* color_object;
	color_object = color_list_new_color_object(args->gs->colors, &c);
		
	gchar* text = 0;
	
	Converters *converters = (Converters*)dynv_system_get(args->gs->params, "ptr", "Converters");
	Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_DISPLAY);
	if (converter){
		converter_get_text(converter->function_name, color_object, 0, args->gs->params, &text);
	}
	
	color_object_release(color_object);
	
	gtk_color_set_color(GTK_COLOR(args->color_widget), &c, text);
	
	if (text) g_free(text);

	return true;
}

void floating_picker_activate(struct Arguments *args, bool hide_on_mouse_release){
	
#ifndef WIN32			//Pointer grabbing in Windows is broken, disabling floating picker for now

	args->release_mode = hide_on_mouse_release;
	
	GdkCursor* cursor;
	cursor = gdk_cursor_new(GDK_TCROSS);
	
	gtk_zoomed_set_zoom(GTK_ZOOMED(args->zoomed), dynv_get_float_wd(args->gs->params, "gpick.picker.zoom", 2));
	
	update_display(args);
	
	gtk_widget_show(args->window);
	

	//gdk_window_set_cursor(si->fake_window->window, cursor);
	
	gdk_pointer_grab(args->window->window, false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK	), NULL, cursor, GDK_CURRENT_TIME);
	gdk_keyboard_grab(args->window->window, false, GDK_CURRENT_TIME);
	
	float refresh_rate = dynv_get_float_wd(args->gs->params, "gpick.picker.refresh_rate", 30);
	args->timeout_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000/refresh_rate, (GSourceFunc)update_display, args, (GDestroyNotify)NULL);

	gdk_cursor_destroy(cursor);
	
#endif

}

void floating_picker_deactivate(struct Arguments *args){
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	
	if (args->timeout_source_id > 0){
        g_source_remove(args->timeout_source_id);
	    args->timeout_source_id = 0;
    }
    gtk_widget_hide(args->window);
}

static gboolean scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, struct Arguments *args){

	double zoom = gtk_zoomed_get_zoom(GTK_ZOOMED(args->zoomed));
	
	if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_RIGHT)){
		zoom += 1;
	}else{
		zoom -= 1;
	}
	
	gtk_zoomed_set_zoom(GTK_ZOOMED(args->zoomed), zoom);

	return TRUE;
}



static gboolean button_release_cb(GtkWidget *widget, GdkEventButton *event, struct Arguments *args){
	
	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 1)) {
		if (args->release_mode){
			Color c;
			//sampler_get_color_sample(si->gs->sampler, &c);
			get_color_sample(args, false, &c);
		
			struct ColorObject* color_object;
			color_object = color_list_new_color_object(args->gs->colors, &c);
		
			Converters *converters = (Converters*)dynv_system_get(args->gs->params, "ptr", "Converters");
			Converter *converter = converters_get_first(converters, CONVERTERS_ARRAY_TYPE_COPY);
			if (converter){
				converter_get_clipboard(converter->function_name, color_object, 0, args->gs->params);
			}

			color_object_release(color_object);
		}
		
		floating_picker_deactivate(args);
		
		dynv_set_float(args->gs->params, "gpick.picker.zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(args->zoomed)));
	}
}



static gboolean key_up_cb (GtkWidget *widget, GdkEventKey *event, struct Arguments *args){

	GdkEventButton event2;
	
	switch(event->keyval){
	case GDK_Escape:
		event2.type = GDK_BUTTON_RELEASE;
		event2.button = 1;
		button_release_cb(widget, &event2, args);
		return TRUE;
		break;
	default:
		if (color_picker_key_up(args->color_source, event)){
			args->release_mode = false;	//key pressed and color picked, disable copy on mouse button release
			return TRUE;
		}
	}
	return FALSE;
}



struct Arguments* floating_picker_new(GtkWidget *parent, GlobalState *gs, ColorSource* color_source){
	struct Arguments *args = new struct Arguments;
	
	args->gs = gs;
	args->parent = gtk_widget_get_toplevel(parent);


	args->window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(args->window), true);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(args->window), true);
	gtk_window_set_decorated(GTK_WINDOW(args->window), false);
	gtk_widget_set_size_request(args->window, -1, -1);
	
	GtkWidget* vbox = gtk_vbox_new(0, 0);
	gtk_container_add (GTK_CONTAINER(args->window), vbox);
	gtk_widget_show(vbox);
	
	args->zoomed = gtk_zoomed_new();
	gtk_widget_show(args->zoomed);
	gtk_box_pack_start(GTK_BOX(vbox), args->zoomed, false, false, 0);
	
	args->color_widget = gtk_color_new();
	gtk_widget_show(args->color_widget);
	gtk_box_pack_start(GTK_BOX(vbox), args->color_widget, true, true, 0);
	
	g_signal_connect(G_OBJECT(args->window), "scroll_event", G_CALLBACK(scroll_event_cb), args);
	g_signal_connect(G_OBJECT(args->window), "button-release-event", G_CALLBACK(button_release_cb), args);
	g_signal_connect(G_OBJECT(args->window), "key_press_event", G_CALLBACK(key_up_cb), args);
	
	args->color_source = color_source;
	
	return args;
}

void floating_picker_free(struct Arguments *args){
	gtk_widget_destroy(args->window);
}
