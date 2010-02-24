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
#include "../ColorRYB.h"
#include "../MathUtil.h"
#include <math.h>
#include <stdlib.h>

#include <list>
using namespace std;

#define GTK_COLOR_WHEEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COLOR_WHEEL, GtkColorWheelPrivate))

G_DEFINE_TYPE (GtkColorWheel, gtk_color_wheel, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_color_wheel_expose(GtkWidget *color_wheel, GdkEventExpose *event);
static gboolean gtk_color_wheel_button_release(GtkWidget *color_wheel, GdkEventButton *event);
static gboolean gtk_color_wheel_button_press(GtkWidget *color_wheel, GdkEventButton *event);

static int color_wheel_get_color_by_position(gint x, gint y);

enum {
	ACTIVE_COLOR_CHANGED, COLOR_CHANGED, COLOR_ACTIVATED, CENTER_ACTIVATED, LAST_SIGNAL
};

static guint gtk_color_wheel_signals[LAST_SIGNAL] = { 0 };

typedef struct ColorPoint{
	double hue;
	double lightness;
	double saturation;

}ColorPoint;

typedef struct GtkColorWheelPrivate{
	list<ColorPoint*> cpoint;
	int active_color;
}GtkColorWheelPrivate;

static void gtk_color_wheel_class_init(GtkColorWheelClass *color_wheel_class) {
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS(color_wheel_class);
	widget_class = GTK_WIDGET_CLASS(color_wheel_class);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_color_wheel_expose;
	widget_class->button_release_event = gtk_color_wheel_button_release;
	widget_class->button_press_event = gtk_color_wheel_button_press;

	/*widget_class->button_press_event = gtk_node_view_button_press;
	 widget_class->button_release_event = gtk_node_view_button_release;
	 widget_class->motion_notify_event = gtk_node_view_motion_notify;*/

	g_type_class_add_private(obj_class, sizeof(GtkColorWheelPrivate));

	gtk_color_wheel_signals[ACTIVE_COLOR_CHANGED] = g_signal_new("active_color_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorWheelClass, active_color_changed), NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

	gtk_color_wheel_signals[COLOR_CHANGED] = g_signal_new("color_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorWheelClass, color_changed), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	gtk_color_wheel_signals[COLOR_ACTIVATED] = g_signal_new("color_activated", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorWheelClass, color_activated), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	gtk_color_wheel_signals[CENTER_ACTIVATED] = g_signal_new("center_activated", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorWheelClass, center_activated), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

}

static void gtk_color_wheel_init(GtkColorWheel *color_wheel) {
	gtk_widget_add_events(GTK_WIDGET(color_wheel), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}

GtkWidget* gtk_color_wheel_new(){
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_COLOR_WHEEL, NULL);
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

	gtk_widget_set_size_request(GTK_WIDGET(widget), 200 + widget->style->xthickness*2, 200 + widget->style->ythickness*2);

	ns->active_color = 1;

	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	return widget;
}

void gtk_color_wheel_set_active_index(GtkColorWheel* color_wheel, guint32 index) {
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(color_wheel);
	ns->active_color = index;
	gtk_widget_queue_draw(GTK_WIDGET(color_wheel));
}


typedef struct ColorWheelType{
	const char *name;
	void (*hue_to_hsl)(double hue, Color* hsl);
	void (*rgbhue_to_hue)(double rgbhue, double *hue);
}ColorWheelType;

static void rgb_hue2hue(double hue, Color* hsl){
	hsl->hsl.hue = hue;
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5;
}

static void rgb_rgbhue2hue(double rgbhue, double *hue){
	*hue = rgbhue;
}

static void ryb1_hue2hue(double hue, Color* hsl){
	Color c;
	color_rybhue_to_rgb(hue, &c);
	color_rgb_to_hsl(&c, hsl);
}

static void ryb1_rgbhue2hue(double rgbhue, double *hue){
	color_rgbhue_to_rybhue(rgbhue, hue);
}

static void ryb2_hue2hue(double hue, Color* hsl){
	hsl->hsl.hue = color_rybhue_to_rgbhue_f(hue);
	hsl->hsl.saturation = 1;
	hsl->hsl.lightness = 0.5;
}

static void ryb2_rgbhue2hue(double rgbhue, double *hue){
	color_rgbhue_to_rybhue_f(rgbhue, hue);
}

const ColorWheelType color_wheel_types[]={
	{"RGB", rgb_hue2hue, rgb_rgbhue2hue},
	{"RYB v1", ryb1_hue2hue, ryb1_rgbhue2hue},
	{"RYB v2", ryb2_hue2hue, ryb2_rgbhue2hue},
};


static void draw_wheel(cairo_t *cr, double radius, double width, const ColorWheelType *wheel){

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(radius * 2), ceil(radius * 2));
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	int surface_width = cairo_image_surface_get_width(surface);
	int surface_height = cairo_image_surface_get_height(surface);

	double inner_radius = radius - width;
	double offset = radius;

	double radius_sq = radius * radius + 2 * radius + 1;
	double inner_radius_sq = inner_radius * inner_radius - 2 * inner_radius + 1;

	double block_width = (2 * inner_radius * sin(M_PI / 4)) - 2;

	Color c;

	int bl_start_x = (surface_width - block_width) / 2;
	int bl_end_x = bl_start_x + block_width;
	int bl_start_y = (surface_height - block_width) / 2;
	int bl_end_y = bl_start_y + block_width;

	unsigned char *line_data;
	for (int y = 0; y < surface_height; ++y){
		line_data = data + stride * y;
		for (int x = 0; x < surface_width; ++x){

			int dx = -(x - surface_width / 2);
			int dy = y - surface_height / 2;
			int dist = dx * dx + dy * dy;

			if (dist < inner_radius_sq){

				if ((x >= bl_start_x) && (x <= bl_end_x) && (y >= bl_start_y) && (y <= bl_end_y)){

					c.hsl.hue = 0;
					c.hsl.saturation = (x - bl_start_x) / block_width;
					c.hsl.lightness = (y - bl_start_y) / block_width;

					color_hsv_to_rgb(&c, &c);

					line_data[2] = c.rgb.red * 255;
					line_data[1] = c.rgb.green * 255;
					line_data[0] = c.rgb.blue * 255;
					line_data[3] = 0xFF;
				}

			}else if (dist <= radius_sq){

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

	//cairo_t *source_cr = cairo_create(surface);

	cairo_save(cr);

	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_surface_destroy(surface);

	cairo_set_line_width(cr, width);
	cairo_new_path(cr);
	cairo_arc(cr, radius, radius, (inner_radius + radius) / 2, 0, M_PI * 2);

	cairo_stroke(cr);

	cairo_rectangle(cr, radius - block_width / 2, radius - block_width / 2, block_width, block_width);
	cairo_fill(cr);

	cairo_restore(cr);


	/*const double step = 1 / 360.0;
	for (double i = 0; i < 1; i += step){
        cairo_new_path(cr);

		cairo_move_to(cr, offset + sin(i * PI * 2) * inner_radius, offset - cos(i * PI * 2) * inner_radius);
		cairo_line_to(cr, offset + sin((i + step) * PI * 2) * inner_radius, offset - cos((i + step) * PI * 2) * inner_radius);
		cairo_line_to(cr, offset + sin((i + step) * PI * 2) * radius, offset - cos((i + step) * PI * 2) * radius);
		cairo_line_to(cr, offset + sin(i * PI * 2) * radius, offset - cos(i * PI * 2) * radius);

		cairo_close_path(cr);

		wheel->hue_to_hsl(i, &c);
		color_hsl_to_rgb(&c, &c);

		cairo_set_source_rgb(cr, c.rgb.red, c.rgb.green, c.rgb.blue);

		cairo_stroke_preserve(cr);
		cairo_fill(cr);

	}*/
}

static gboolean gtk_color_wheel_expose(GtkWidget *widget, GdkEventExpose *event) {

	GtkStateType state;

	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_ACTIVE;


	cairo_t *cr;

	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

//	gtk_paint_shadow(widget->style, widget->window, state, GTK_SHADOW_IN, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);

/*	if (GTK_WIDGET_HAS_FOCUS(widget)){
		gtk_paint_focus(widget->style, widget->window, state, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);
	}*/

	cr = gdk_cairo_create(widget->window);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	cairo_translate(cr, widget->style->xthickness + 0.5, widget->style->ythickness + 0.5);

	draw_wheel(cr, 100, 20, &color_wheel_types[0]);

	/*
	cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12);

	cairo_matrix_t matrix;
	cairo_get_matrix(cr, &matrix);
	cairo_translate(cr, 75+widget->style->xthickness, 75+widget->style->ythickness);

	int edges = 6;

	cairo_set_source_rgb(cr, 0, 0, 0);

	float radius_multi = 50 * cos((180 / edges) / (180 / PI));
	float rotation = -(PI/6 * 4);

	//Draw stroke
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_width(cr, 3);
	for (int i = 1; i < 7; ++i) {
		if (i == ns->current_color)
			continue;
		gtk_color_wheel_draw_hexagon(cr, radius_multi * cos(rotation + i * (2 * PI) / edges), radius_multi * sin(rotation + i * (2 * PI) / edges), 27);
	}

	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 1, 1, 1);
	gtk_color_wheel_draw_hexagon(cr, radius_multi * cos(rotation + (ns->current_color) * (2 * PI) / edges), radius_multi * sin(rotation + (ns->current_color) * (2
			* PI) / edges), 27);
	cairo_stroke(cr);

	//Draw fill
	for (int i = 1; i < 7; ++i) {
		if (i == ns->current_color)
			continue;
		cairo_set_source_rgb(cr, ns->color[i].rgb.red, ns->color[i].rgb.green, ns->color[i].rgb.blue);
		gtk_color_wheel_draw_hexagon(cr, radius_multi * cos(rotation + i * (2 * PI) / edges), radius_multi * sin(rotation + i * (2 * PI) / edges), 25.5);
		cairo_fill(cr);
	}

	cairo_set_source_rgb(cr, ns->color[ns->current_color].rgb.red, ns->color[ns->current_color].rgb.green, ns->color[ns->current_color].rgb.blue);
	gtk_color_wheel_draw_hexagon(cr, radius_multi * cos(rotation + (ns->current_color) * (2 * PI) / edges), radius_multi * sin(rotation + (ns->current_color) * (2
			* PI) / edges), 25.5);
	cairo_fill(cr);

	//Draw center
	cairo_set_source_rgb(cr, ns->color[0].rgb.red, ns->color[0].rgb.green, ns->color[0].rgb.blue);
	gtk_color_wheel_draw_hexagon(cr, 0, 0, 25.5);
	cairo_fill(cr);

	//Draw numbers
	char numb[2] = " ";
	for (int i = 1; i < 7; ++i) {
		Color c;
		color_get_contrasting(&ns->color[i], &c);

		cairo_text_extents_t extends;
		numb[0] = '0' + i;
		cairo_text_extents(cr, numb, &extends);
		cairo_set_source_rgb(cr, c.rgb.red, c.rgb.green, c.rgb.blue);
		cairo_move_to(cr, radius_multi * cos(rotation + i * (2 * PI) / edges) - extends.width / 2, radius_multi * sin(rotation + i * (2 * PI) / edges)
				+ extends.height / 2);
		cairo_show_text(cr, numb);
	}

	cairo_set_matrix(cr, &matrix);
    */

	cairo_destroy(cr);



	return FALSE;
}


static gboolean gtk_color_wheel_button_press(GtkWidget *widget, GdkEventButton *event) {
	GtkColorWheelPrivate *ns = GTK_COLOR_WHEEL_GET_PRIVATE(widget);

	/*int new_color = color_wheel_get_color_by_position(event->x - widget->style->xthickness, event->y - widget->style->ythickness);

	gtk_widget_grab_focus(widget);

	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
		if (new_color>0){
			g_signal_emit(widget, gtk_color_wheel_signals[COLOR_ACTIVATED], 0);
		}
	}else if ((event->type == GDK_BUTTON_PRESS) && ((event->button == 1) || (event->button == 3))) {
		if (new_color==0){
			g_signal_emit(widget, gtk_color_wheel_signals[CENTER_ACTIVATED], 0);
		}else if (new_color<0){
			g_signal_emit(widget, gtk_color_wheel_signals[ACTIVE_COLOR_CHANGED], 0, ns->current_color);
		}else{
			if (new_color != ns->current_color){
				ns->current_color = new_color;

				g_signal_emit(widget, gtk_color_wheel_signals[ACTIVE_COLOR_CHANGED], 0, ns->current_color);

				gtk_widget_queue_draw(GTK_WIDGET(widget));
			}
		}
	}*/
	return FALSE;
}

static gboolean gtk_color_wheel_button_release(GtkWidget *widget, GdkEventButton *event) {
	return FALSE;
}


