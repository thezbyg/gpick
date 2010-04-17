/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#include "ColorWheel.h"
#include "../Color.h"
#include "../ColorWheelType.h"
#include "../MathUtil.h"
#include <math.h>
#include <stdlib.h>

#include <list>
using namespace std;

#define GTK_COLOR_WHEEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COLOR_WHEEL, GtkColorWheelPrivate))

G_DEFINE_TYPE (GtkColorWheel, gtk_color_wheel, GTK_TYPE_DRAWING_AREA);
GtkWindowClass *parent_class = NULL;

static gboolean gtk_color_wheel_expose(GtkWidget *color_wheel, GdkEventExpose *event);
static gboolean gtk_color_wheel_button_release(GtkWidget *color_wheel, GdkEventButton *event);
static gboolean gtk_color_wheel_button_press(GtkWidget *color_wheel, GdkEventButton *event);
static gboolean gtk_color_wheel_motion_notify(GtkWidget *widget, GdkEventMotion *event);

enum {
	HUE_CHANGED, SATURATION_VALUE_CHANGED, LAST_SIGNAL
};

static guint gtk_color_wheel_signals[LAST_SIGNAL] = { 0 };

typedef struct ColorPoint{
	double hue;
	double lightness;
	double saturation;

}ColorPoint;

typedef struct GtkColorWheelPrivate{
	ColorPoint cpoint[10];
	uint32_t n_cpoint;

	ColorPoint *grab_active;
    bool grab_block;

	ColorPoint *selected;

	int active_color;

	double radius;
	double circle_width;
	double block_size;

	bool block_editable;

	const ColorWheelType *color_wheel_type;

	cairo_surface_t *cache_color_wheel;
}GtkColorWheelPrivate;

static uint32_t get_color_index(GtkColorWheelPrivate *ns, ColorPoint *cp);

static void gtk_color_wheel_finalize(GObject *color_wheel_obj){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel_obj);

	if (ns->cache_color_wheel){
		cairo_surface_destroy(ns->cache_color_wheel);
		ns->cache_color_wheel = 0;
	}

	G_OBJECT_CLASS(parent_class)->finalize(color_wheel_obj);
}

static void gtk_color_wheel_class_init(GtkColorWheelClass *color_wheel_class) {
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS(color_wheel_class);
	widget_class = GTK_WIDGET_CLASS(color_wheel_class);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_color_wheel_expose;
	widget_class->button_release_event = gtk_color_wheel_button_release;
	widget_class->button_press_event = gtk_color_wheel_button_press;

	widget_class->motion_notify_event = gtk_color_wheel_motion_notify;

	parent_class = (GtkWindowClass*)g_type_class_peek_parent(G_OBJECT_CLASS(color_wheel_class));
	obj_class->finalize = gtk_color_wheel_finalize;

	g_type_class_add_private(obj_class, sizeof(GtkColorWheelPrivate));

	gtk_color_wheel_signals[HUE_CHANGED] = g_signal_new("hue_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkColorWheelClass, hue_changed), NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
	gtk_color_wheel_signals[SATURATION_VALUE_CHANGED] = g_signal_new("saturation_value_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkColorWheelClass, saturation_value_changed), NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

}

static void gtk_color_wheel_init(GtkColorWheel *color_wheel){
	gtk_widget_add_events(GTK_WIDGET(color_wheel), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}

GtkWidget* gtk_color_wheel_new(){
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_COLOR_WHEEL, NULL);
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);


	ns->active_color = 1;
	ns->radius = 80;
	ns->circle_width = 14;
	ns->block_size = 2 * (ns->radius - ns->circle_width) * sin(M_PI / 4) - 8;

	gtk_widget_set_size_request(GTK_WIDGET(widget), ns->radius * 2 + widget->style->xthickness*2, ns->radius * 2 + widget->style->ythickness*2);
	ns->n_cpoint = 0;
	ns->grab_active = 0;
	ns->grab_block = false;
    ns->selected = &ns->cpoint[0];
	ns->block_editable = true;

	ns->color_wheel_type = &color_wheel_types_get()[0];
	ns->cache_color_wheel = 0;

	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	return widget;
}


void gtk_color_wheel_set_color_wheel_type(GtkColorWheel *color_wheel, const ColorWheelType *color_wheel_type){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (ns->color_wheel_type != color_wheel_type){
		ns->color_wheel_type = color_wheel_type;
		if (ns->cache_color_wheel){
			cairo_surface_destroy(ns->cache_color_wheel);
			ns->cache_color_wheel = 0;
		}
		gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
	}
}


void gtk_color_wheel_set_block_editable(GtkColorWheel* color_wheel, bool editable){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
    ns->block_editable = editable;
}

bool gtk_color_wheel_get_block_editable(GtkColorWheel* color_wheel){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
    return ns->block_editable;
}

void gtk_color_wheel_set_selected(GtkColorWheel* color_wheel, guint32 index){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		ns->selected = &ns->cpoint[index];
		gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
	}
}

void gtk_color_wheel_set_n_colors(GtkColorWheel* color_wheel, guint32 number_of_colors){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (number_of_colors <= 10){
        if (ns->n_cpoint != number_of_colors){
			ns->n_cpoint = number_of_colors;

			if (ns->selected){
				uint32_t index = get_color_index(ns, ns->selected);
				if (index >= number_of_colors){
					ns->selected = &ns->cpoint[0];
				}
			}
			gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
		}
	}
}

void gtk_color_wheel_set_hue(GtkColorWheel* color_wheel, guint32 index, double hue){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		ns->cpoint[index].hue = hue;
		gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
	}
}

void gtk_color_wheel_set_saturation(GtkColorWheel* color_wheel, guint32 index, double saturation){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		ns->cpoint[index].saturation = saturation;
		if (&ns->cpoint[index] == ns->selected)
			gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
	}
}

void gtk_color_wheel_set_value(GtkColorWheel* color_wheel, guint32 index, double value){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		ns->cpoint[index].lightness = value;
		if (&ns->cpoint[index] == ns->selected)
			gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
	}
}

double gtk_color_wheel_get_hue(GtkColorWheel* color_wheel, guint32 index){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		return ns->cpoint[index].hue;
	}
	return 0;
}

double gtk_color_wheel_get_saturation(GtkColorWheel* color_wheel, guint32 index){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		return ns->cpoint[index].saturation;
	}
	return 0;
}

double gtk_color_wheel_get_value(GtkColorWheel* color_wheel, guint32 index){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	if (index < 10){
		return ns->cpoint[index].lightness;
	}
	return 0;
}


static void draw_dot(cairo_t *cr, double x, double y, double size){
	cairo_arc(cr, x, y, size - 1, 0, 2 * M_PI);
	cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
	cairo_set_line_width(cr, 2);
	cairo_stroke(cr);

	cairo_arc(cr, x, y, size, 0, 2 * M_PI);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);

}

static void draw_sat_val_block(cairo_t *cr, double pos_x, double pos_y, double size, double hue){
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(size), ceil(size));
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	int surface_width = cairo_image_surface_get_width(surface);
	int surface_height = cairo_image_surface_get_height(surface);

	Color c;

	unsigned char *line_data;
	for (int y = 0; y < surface_height; ++y){
		line_data = data + stride * y;
		for (int x = 0; x < surface_width; ++x){

			c.hsv.hue = hue;
			c.hsv.saturation = x / size;
			c.hsv.value = y / size;

			color_hsv_to_rgb(&c, &c);

			line_data[2] = c.rgb.red * 255;
			line_data[1] = c.rgb.green * 255;
			line_data[0] = c.rgb.blue * 255;
			line_data[3] = 0xFF;

			line_data += 4;
		}
	}

	cairo_save(cr);

	cairo_set_source_surface(cr, surface, pos_x - size / 2, pos_y - size / 2);
	cairo_surface_destroy(surface);

	cairo_rectangle(cr, pos_x - size / 2, pos_y - size / 2, size, size);
	cairo_fill(cr);

	cairo_restore(cr);

}

static void draw_wheel(GtkColorWheelPrivate *ns, cairo_t *cr, double radius, double width, const ColorWheelType *wheel){
	cairo_surface_t *surface;
	double inner_radius = radius - width;

	if (ns->cache_color_wheel){
        surface = ns->cache_color_wheel;
	}else{
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(radius * 2), ceil(radius * 2));
		unsigned char *data = cairo_image_surface_get_data(surface);
		int stride = cairo_image_surface_get_stride(surface);
		int surface_width = cairo_image_surface_get_width(surface);
		int surface_height = cairo_image_surface_get_height(surface);


		double radius_sq = radius * radius + 2 * radius + 1;
		double inner_radius_sq = inner_radius * inner_radius - 2 * inner_radius + 1;

		Color c;

		unsigned char *line_data;
		for (int y = 0; y < surface_height; ++y){
			line_data = data + stride * y;
			for (int x = 0; x < surface_width; ++x){

				int dx = -(x - surface_width / 2);
				int dy = y - surface_height / 2;
				int dist = dx * dx + dy * dy;

				if ((dist >= inner_radius_sq) && (dist <= radius_sq)){

					double angle = atan2(dx, dy) + M_PI;

					wheel->hue_to_hsl(angle / (M_PI * 2), &c);
					color_hsl_to_rgb(&c, &c);

					line_data[2] = c.rgb.red * 255;
					line_data[1] = c.rgb.green * 255;
					line_data[0] = c.rgb.blue * 255;
					line_data[3] = 0xFF;
				}

				line_data += 4;
			}

		}

		ns->cache_color_wheel = surface;
	}

	cairo_save(cr);

	cairo_set_source_surface(cr, surface, 0, 0);
	//cairo_surface_destroy(surface);

	cairo_set_line_width(cr, width);
	cairo_new_path(cr);
	cairo_arc(cr, radius, radius, (inner_radius + radius) / 2, 0, M_PI * 2);

	cairo_stroke(cr);

	cairo_restore(cr);
}

static bool is_inside_block(GtkColorWheelPrivate *ns, gint x, gint y){
	double size = ns->block_size;

	if ((x >= ns->radius - size / 2) && (x <= ns->radius + size / 2)){
		if ((y >= ns->radius - size / 2) && (y <= ns->radius + size / 2)){
		return true;
		}
	}
	return false;
}

static ColorPoint* get_cpoint_at(GtkColorWheelPrivate *ns, gint x, gint y){

	double dx, dy;
	for (uint32_t i = 0; i != ns->n_cpoint; i++){
		dx = ns->radius + (ns->radius - ns->circle_width / 2) * sin(ns->cpoint[i].hue * M_PI * 2) - x;
		dy = ns->radius - (ns->radius - ns->circle_width / 2) * cos(ns->cpoint[i].hue * M_PI * 2) - y;

		if (sqrt(dx * dx + dy * dy) < 16){
			return &ns->cpoint[i];
		}
	}

	return 0;
}

static uint32_t get_color_index(GtkColorWheelPrivate *ns, ColorPoint *cp){
	return (uint64_t(cp) - uint64_t(ns)) / sizeof(ColorPoint);
}

int gtk_color_wheel_get_at(GtkColorWheel *color_wheel, int x, int y){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);

	ColorPoint *cp = get_cpoint_at(ns, x, y);
	if (cp){
		return get_color_index(ns, cp);
	}

	if (is_inside_block(ns, x, y)) return -1;

	return -2;
}

static gboolean gtk_color_wheel_motion_notify(GtkWidget *widget, GdkEventMotion *event){
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

    if (ns->grab_active){
		double dx = -((event->x - widget->style->xthickness) - ns->radius);
		double dy = (event->y - widget->style->ythickness) - ns->radius;

		double angle = atan2(dx, dy) + M_PI;

		ns->grab_active->hue = angle / (M_PI * 2);

		g_signal_emit(widget, gtk_color_wheel_signals[HUE_CHANGED], 0, get_color_index(ns, ns->grab_active));

		gtk_widget_queue_draw(widget);

		return true;
	}else if (ns->grab_block){

		double dx = (event->x - widget->style->xthickness) - ns->radius + ns->block_size / 2;
		double dy = (event->y - widget->style->ythickness) - ns->radius + ns->block_size / 2;

		ns->selected->saturation = clamp_float(dx / ns->block_size, 0, 1);
		ns->selected->lightness = clamp_float(dy / ns->block_size, 0, 1);

		g_signal_emit(widget, gtk_color_wheel_signals[SATURATION_VALUE_CHANGED], 0, get_color_index(ns, ns->selected));

		gtk_widget_queue_draw(widget);
		return true;
	}

/*	ColorPoint *p = get_cpoint_at(ns, event->x, event->y);
	if (p){
		GdkCursor *grab = gdk_cursor_new(GDK_HAND2);
		gdk_window_set_cursor(gtk_widget_get_window(widget), grab);
		gdk_cursor_destroy(grab);

	}
*/
	return false;
}

static gboolean gtk_color_wheel_expose(GtkWidget *widget, GdkEventExpose *event){

	GtkStateType state;

	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_ACTIVE;

	cairo_t *cr;

	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

/*	if (GTK_WIDGET_HAS_FOCUS(widget)){
		gtk_paint_focus(widget->style, widget->window, state, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);
	}*/

	cr = gdk_cairo_create(widget->window);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	cairo_translate(cr, widget->style->xthickness + 0.5, widget->style->ythickness + 0.5);

	draw_wheel(ns, cr, ns->radius, ns->circle_width, ns->color_wheel_type);

	if (ns->selected){
		double block_size = 2 * (ns->radius - ns->circle_width) * sin(M_PI / 4) - 6;

		Color hsl;
		ns->color_wheel_type->hue_to_hsl(ns->selected->hue, &hsl);

		draw_sat_val_block(cr, ns->radius, ns->radius, block_size, hsl.hsl.hue);

		draw_dot(cr, ns->radius - block_size / 2 + block_size * ns->selected->saturation, ns->radius - block_size / 2 + block_size * ns->selected->lightness, 4);

	}

	for (uint32_t i = 0; i != ns->n_cpoint; i++){
		draw_dot(cr, ns->radius + (ns->radius - ns->circle_width / 2) * sin(ns->cpoint[i].hue * M_PI * 2), ns->radius - (ns->radius - ns->circle_width / 2) * cos(ns->cpoint[i].hue * M_PI * 2), (&ns->cpoint[i] == ns->selected) ? 7 : 4);
	}

	cairo_destroy(cr);

	return FALSE;
}


static gboolean gtk_color_wheel_button_press(GtkWidget *widget, GdkEventButton *event) {
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

	gint x = event->x - widget->style->xthickness;
	gint y = event->y - widget->style->ythickness;

	gtk_widget_grab_focus(widget);

    ColorPoint *p;
	if (is_inside_block(ns, x, y)){
		if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){
			if (ns->block_editable && ns->selected){

				ns->grab_block = true;

				GdkCursor *cursor = gdk_cursor_new(GDK_CROSS);
				gdk_pointer_grab(gtk_widget_get_window(widget), false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK), NULL, cursor, GDK_CURRENT_TIME);
				gdk_cursor_destroy(cursor);
				return true;
			}
		}
	}else if ((p = get_cpoint_at(ns, x, y))){
		if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){

			ns->grab_active = p;
			ns->selected = p;

			GdkCursor *cursor = gdk_cursor_new(GDK_CROSS);
			gdk_pointer_grab(gtk_widget_get_window(widget), false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK), NULL, cursor, GDK_CURRENT_TIME);
			gdk_cursor_destroy(cursor);
			return true;
		}
	}
	return false;
}

static gboolean gtk_color_wheel_button_release(GtkWidget *widget, GdkEventButton *event) {
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	ns->grab_active = 0;
	ns->grab_block = false;

	return false;
}


