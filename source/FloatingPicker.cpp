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

#include "FloatingPicker.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "gtk/Zoomed.h"
#include "gtk/ColorWidget.h"
#include "uiUtilities.h"
#include "Clipboard.h"
#include "CopyMenu.h"
#include "GlobalState.h"
#include "ColorPicker.h"
#include "Converter.h"
#include "DynvHelpers.h"
#include "ToolColorNaming.h"
#include "ScreenReader.h"
#include "Sampler.h"
#include "color_names/ColorNames.h"
#include <gdk/gdkkeysyms.h>
#include <string>
#include <sstream>
using namespace math;
using namespace std;

typedef struct FloatingPickerArgs
{
	GtkWidget* window;
	GtkWidget* zoomed;
	GtkWidget* color_widget;
	guint timeout_source_id;
	ColorSource *color_source;
	Converter *converter;
	GlobalState* gs;
	bool release_mode;
	bool single_pick_mode;
	bool click_mode;
	bool perform_custom_pick_action;
	bool menu_button_pressed;
	function<void(FloatingPicker, const Color&)> custom_pick_action;
	function<void(FloatingPicker)> custom_done_action;
}FloatingPickerArgs;

class PickerColorNameAssigner: public ToolColorNameAssigner
{
	protected:
		stringstream m_stream;
	public:
		PickerColorNameAssigner(GlobalState *gs):
			ToolColorNameAssigner(gs)
		{
		}
		void assign(ColorObject *color_object, const Color *color)
		{
			ToolColorNameAssigner::assign(color_object, color);
		}
		virtual std::string getToolSpecificName(ColorObject *color_object, const Color *color)
		{
			m_stream.str("");
			m_stream << color_names_get(m_gs->getColorNames(), color, false);
			return m_stream.str();
		}
};
static void get_color_sample(FloatingPickerArgs *args, bool update_widgets, Color* c)
{
	GdkScreen *screen;
	GdkModifierType state;
	int x, y;
	gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, &state);
	int monitor = gdk_screen_get_monitor_at_point(screen, x, y);
	GdkRectangle monitor_geometry;
	gdk_screen_get_monitor_geometry(screen, monitor, &monitor_geometry);
	Vec2<int> pointer(x,y);
	Rect2<int> screen_rect(monitor_geometry.x, monitor_geometry.y, monitor_geometry.x + monitor_geometry.width, monitor_geometry.y + monitor_geometry.height);
	auto screen_reader = args->gs->getScreenReader();
	screen_reader_reset_rect(screen_reader);
	Rect2<int> sampler_rect, zoomed_rect, final_rect;
	sampler_get_screen_rect(args->gs->getSampler(), pointer, screen_rect, &sampler_rect);
	screen_reader_add_rect(screen_reader, screen, sampler_rect);
	if (update_widgets){
		gtk_zoomed_get_screen_rect(GTK_ZOOMED(args->zoomed), pointer, screen_rect, &zoomed_rect);
		screen_reader_add_rect(screen_reader, screen, zoomed_rect);
	}
	screen_reader_update_surface(screen_reader, &final_rect);
	Vec2<int> offset;
	offset = Vec2<int>(sampler_rect.getX() - final_rect.getX(), sampler_rect.getY() - final_rect.getY());
	sampler_get_color_sample(args->gs->getSampler(), pointer, screen_rect, offset, c);
	if (update_widgets){
		offset = Vec2<int>(zoomed_rect.getX() - final_rect.getX(), zoomed_rect.getY() - final_rect.getY());
		gtk_zoomed_update(GTK_ZOOMED(args->zoomed), pointer, screen_rect, offset, screen_reader_get_surface(screen_reader));
	}
}
static gboolean update_display(FloatingPickerArgs *args)
{
	GdkScreen *screen;
	GdkModifierType state;
	int x, y;
	int width, height;
	gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, &state);
	width = gdk_screen_get_width(screen);
	height = gdk_screen_get_height(screen);
	gint sx, sy;
	gtk_window_get_size(GTK_WINDOW(args->window), &sx, &sy);
	if (x + sx + sx / 2 > width){
		x -= sx + sx / 2;
	}else{
		x += sx / 2;
	}
	if (y + sy + sy / 2 > height){
		y -= sy + sy / 2;
	}else{
		y += sy / 2;
	}
	if (gtk_window_get_screen(GTK_WINDOW(args->window)) != screen){
		gtk_window_set_screen(GTK_WINDOW(args->window), screen);
	}
	gtk_window_move(GTK_WINDOW(args->window), x, y);
	Color c;
	get_color_sample(args, true, &c);
	string text;
	if (args->converter != nullptr){
		auto color_object = color_list_new_color_object(args->gs->getColorList(), &c);
		converter_get_text(color_object, args->converter, args->gs, text);
		color_object->release();
	}else{
		converter_get_text(c, ConverterArrayType::display, args->gs, text);
	}
	gtk_color_set_color(GTK_COLOR(args->color_widget), &c, text.c_str());
	return true;
}
void floating_picker_activate(FloatingPickerArgs *args, bool hide_on_mouse_release, bool single_pick_mode, const char *converter_name)
{
#ifndef WIN32 //Pointer grabbing in Windows is broken, disabling floating picker for now
	if (converter_name != nullptr){
		args->converter = converters_get(args->gs->getConverters(), converter_name);
	}else{
		args->converter = nullptr;
	}
	args->release_mode = hide_on_mouse_release && !single_pick_mode;
	args->single_pick_mode = single_pick_mode;
	args->click_mode = true;
	GdkCursor* cursor;
	cursor = gdk_cursor_new(GDK_TCROSS);
	gtk_zoomed_set_zoom(GTK_ZOOMED(args->zoomed), dynv_get_float_wd(args->gs->getSettings(), "gpick.picker.zoom", 2));
	update_display(args);
	gtk_widget_show(args->window);
	gdk_pointer_grab(args->window->window, false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK), nullptr, cursor, GDK_CURRENT_TIME);
	gdk_keyboard_grab(args->window->window, false, GDK_CURRENT_TIME);
	float refresh_rate = dynv_get_float_wd(args->gs->getSettings(), "gpick.picker.refresh_rate", 30);
	args->timeout_source_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 1000 / refresh_rate, (GSourceFunc)update_display, args, (GDestroyNotify)nullptr);
	gdk_cursor_destroy(cursor);
#endif
}
void floating_picker_deactivate(FloatingPickerArgs *args)
{
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	if (args->timeout_source_id > 0){
		g_source_remove(args->timeout_source_id);
		args->timeout_source_id = 0;
	}
	gtk_widget_hide(args->window);
}
static gboolean scroll_event_cb(GtkWidget *widget, GdkEventScroll *event, FloatingPickerArgs *args)
{
	double zoom = gtk_zoomed_get_zoom(GTK_ZOOMED(args->zoomed));
	if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_RIGHT)){
		zoom += 1;
	}else{
		zoom -= 1;
	}
	gtk_zoomed_set_zoom(GTK_ZOOMED(args->zoomed), zoom);
	return TRUE;
}
static void finish_picking(FloatingPickerArgs *args)
{
	floating_picker_deactivate(args);
	dynv_set_float(args->gs->getSettings(), "gpick.picker.zoom", gtk_zoomed_get_zoom(GTK_ZOOMED(args->zoomed)));
	if (args->custom_done_action)
		args->custom_done_action(args);
}
static void complete_picking(FloatingPickerArgs *args)
{
	if (args->release_mode || args->click_mode){
		Color c;
		get_color_sample(args, false, &c);
		if (args->perform_custom_pick_action){
			if (args->custom_pick_action)
				args->custom_pick_action(args, c);
		}else{
			ColorObject* color_object;
			color_object = color_list_new_color_object(args->gs->getColorList(), &c);
			if (args->single_pick_mode){
				Clipboard::set(color_object, args->gs, args->converter);
			}else{
				if (dynv_get_bool_wd(args->gs->getSettings(), "gpick.picker.sampler.copy_on_release", false)){
					Clipboard::set(color_object, args->gs, args->converter);
				}
				if (dynv_get_bool_wd(args->gs->getSettings(), "gpick.picker.sampler.add_on_release", false)){
					PickerColorNameAssigner name_assigner(args->gs);
					name_assigner.assign(color_object, &c);
					color_list_add_color_object(args->gs->getColorList(), color_object, 1);
				}
			}
			color_object->release();
		}
	}
}
static void show_copy_menu(int button, int event_time, FloatingPickerArgs *args)
{
	Color c;
	get_color_sample(args, false, &c);
	GtkWidget *menu;
	ColorList *color_list = color_list_new_with_one_color(args->gs->getColorList(), &c);
	menu = CopyMenu::newMenu(*color_list->colors.begin(), args->gs);
	gtk_widget_show_all(GTK_WIDGET(menu));
	gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, nullptr, nullptr, button, event_time);
	g_object_ref_sink(menu);
	g_object_unref(menu);
	color_list_destroy(color_list);
}
static gboolean button_release_cb(GtkWidget *widget, GdkEventButton *event, FloatingPickerArgs *args)
{
	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 1)) {
		complete_picking(args);
		finish_picking(args);
	}else if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 3) && args->menu_button_pressed) {
		args->menu_button_pressed = false;
		show_copy_menu(event->button, event->time, args);
		finish_picking(args);
	}
	return false;
}
static gboolean button_press_cb(GtkWidget *widget, GdkEventButton *event, FloatingPickerArgs *args)
{
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		args->menu_button_pressed = true;
	}
	return false;
}
static gboolean key_up_cb(GtkWidget *widget, GdkEventKey *event, FloatingPickerArgs *args)
{
	guint modifiers = gtk_accelerator_get_default_mod_mask();
	gint add_x = 0, add_y = 0;
	switch (event->keyval){
		case GDK_KEY_Return:
			complete_picking(args);
			finish_picking(args);
			return TRUE;
			break;
		case GDK_KEY_Escape:
			finish_picking(args);
			return TRUE;
			break;
		case GDK_KEY_m:
			{
				int x, y;
				gdk_display_get_pointer(gdk_display_get_default(), nullptr, &x, &y, nullptr);
				math::Vec2<int> position(x, y);
				if ((event->state & modifiers) == GDK_CONTROL_MASK){
					gtk_zoomed_set_mark(GTK_ZOOMED(args->zoomed), 1, position);
				}else{
					gtk_zoomed_set_mark(GTK_ZOOMED(args->zoomed), 0, position);
				}
			}
			break;
		case GDK_KEY_Left:
			if ((event->state & modifiers) == GDK_SHIFT_MASK)
				add_x -= 10;
			else
				add_x--;
			break;
		case GDK_KEY_Right:
			if ((event->state & modifiers) == GDK_SHIFT_MASK)
				add_x += 10;
			else
				add_x++;
			break;
		case GDK_KEY_Up:
			if ((event->state & modifiers) == GDK_SHIFT_MASK)
				add_y -= 10;
			else
				add_y--;
			break;
		case GDK_KEY_Down:
			if ((event->state & modifiers) == GDK_SHIFT_MASK)
				add_y += 10;
			else
				add_y++;
			break;
		default:
			if (args->color_source && color_picker_key_up(args->color_source, event)){
				args->release_mode = false; //key pressed and color picked, disable copy on mouse button release
				return TRUE;
			}
	}
	if (add_x || add_y){
		gint x, y;
		GdkDisplay *display = nullptr;
		GdkScreen *screen = nullptr;
		display = gdk_display_get_default();
		screen = gdk_display_get_default_screen(display);
		gdk_display_get_pointer(display, nullptr, &x, &y, nullptr);
		x += add_x;
		y += add_y;
		gdk_display_warp_pointer(display, screen, x, y);
	}
	return FALSE;
}
static void destroy_cb(GtkWidget *widget, FloatingPickerArgs *args)
{
	delete args;
}
FloatingPickerArgs* floating_picker_new(GlobalState *gs)
{
	FloatingPickerArgs *args = new FloatingPickerArgs;
	args->gs = gs;
	args->window = gtk_window_new(GTK_WINDOW_POPUP);
	args->color_source = nullptr;
	args->perform_custom_pick_action = false;
	args->menu_button_pressed = false;
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
	g_signal_connect(G_OBJECT(args->window), "button-press-event", G_CALLBACK(button_press_cb), args);
	g_signal_connect(G_OBJECT(args->window), "button-release-event", G_CALLBACK(button_release_cb), args);
	g_signal_connect(G_OBJECT(args->window), "key_press_event", G_CALLBACK(key_up_cb), args);
	g_signal_connect(G_OBJECT(args->window), "destroy", G_CALLBACK(destroy_cb), args);
	return args;
}
void floating_picker_set_picker_source(FloatingPickerArgs *args, ColorSource* color_source)
{
	args->color_source = color_source;
}
void floating_picker_free(FloatingPickerArgs *args)
{
	gtk_widget_destroy(args->window);
}
void floating_picker_enable_custom_pick_action(FloatingPickerArgs *args)
{
	args->perform_custom_pick_action = true;
}
void floating_picker_set_custom_pick_action(FloatingPickerArgs *args, std::function<void(FloatingPicker, const Color&)> action)
{
	args->custom_pick_action = action;
}
void floating_picker_set_custom_done_action(FloatingPickerArgs *args, std::function<void(FloatingPicker)> action)
{
	args->custom_done_action = action;
}
