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

#include "Range2D.h"
#include "../Color.h"
#include "../MathUtil.h"
#include <math.h>
#include <stdlib.h>

#include <string>
#include <list>
using namespace std;

#define GTK_RANGE_2D_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_RANGE_2D, GtkRange2DPrivate))

G_DEFINE_TYPE (GtkRange2D, gtk_range_2d, GTK_TYPE_DRAWING_AREA);
static GtkWindowClass *parent_class = NULL;

static gboolean gtk_range_2d_expose(GtkWidget *range_2d, GdkEventExpose *event);
static gboolean gtk_range_2d_button_release(GtkWidget *range_2d, GdkEventButton *event);
static gboolean gtk_range_2d_button_press(GtkWidget *range_2d, GdkEventButton *event);
static gboolean gtk_range_2d_motion_notify(GtkWidget *widget, GdkEventMotion *event);

enum {
	VALUES_CHANGED, LAST_SIGNAL
};

static guint gtk_range_2d_signals[LAST_SIGNAL] = { 0 };

typedef struct GtkRange2DPrivate{
	double x;
	double y;
	char *xname;
	char *yname;

	double block_size;

	bool grab_block;

	cairo_surface_t *cache_range_2d;
}GtkRange2DPrivate;

static void gtk_range_2d_finalize(GObject *range_2d_obj){
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(range_2d_obj);

	if (ns->xname) g_free(ns->xname);
	if (ns->yname) g_free(ns->yname);
	if (ns->cache_range_2d){
		cairo_surface_destroy(ns->cache_range_2d);
		ns->cache_range_2d = 0;
	}

	G_OBJECT_CLASS(parent_class)->finalize(range_2d_obj);
}

static void gtk_range_2d_class_init(GtkRange2DClass *range_2d_class) {
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS(range_2d_class);
	widget_class = GTK_WIDGET_CLASS(range_2d_class);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_range_2d_expose;
	widget_class->button_release_event = gtk_range_2d_button_release;
	widget_class->button_press_event = gtk_range_2d_button_press;


	widget_class->motion_notify_event = gtk_range_2d_motion_notify;

	parent_class = (GtkWindowClass*)g_type_class_peek_parent(G_OBJECT_CLASS(range_2d_class));
	obj_class->finalize = gtk_range_2d_finalize;

	g_type_class_add_private(obj_class, sizeof(GtkRange2DPrivate));

	gtk_range_2d_signals[VALUES_CHANGED] = g_signal_new("values_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkRange2DClass, values_changed), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

}

static void gtk_range_2d_init(GtkRange2D *range_2d){
	gtk_widget_add_events(GTK_WIDGET(range_2d), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}

GtkWidget* gtk_range_2d_new(){
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_RANGE_2D, NULL);
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(widget);

	ns->block_size = 128;
	ns->grab_block = false;
	ns->xname = 0;
	ns->yname = 0;

	gtk_widget_set_size_request(GTK_WIDGET(widget), ns->block_size + widget->style->xthickness * 2, ns->block_size + widget->style->ythickness * 2);

	ns->cache_range_2d = 0;

	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	return widget;
}


void gtk_range_2d_set_values(GtkRange2D* range_2d, double x, double y){
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(range_2d);
	ns->x = x;
	ns->y = 1 - y;
	gtk_widget_queue_draw(GTK_WIDGET(range_2d));
}


double gtk_range_2d_get_x(GtkRange2D* range_2d){
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(range_2d);
	return ns->x;
}

double gtk_range_2d_get_y(GtkRange2D* range_2d){
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(range_2d);
	return 1 - ns->y;
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

static void draw_sat_val_block(GtkRange2DPrivate *ns, cairo_t *cr, double pos_x, double pos_y, double size){
	cairo_surface_t *surface;

	if (ns->cache_range_2d){
        surface = ns->cache_range_2d;
	}else{
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(size), ceil(size));
		unsigned char *data = cairo_image_surface_get_data(surface);
		int stride = cairo_image_surface_get_stride(surface);
		int surface_width = cairo_image_surface_get_width(surface);
		int surface_height = cairo_image_surface_get_height(surface);

		Color c;

		double hue = 0;
		float v;

		unsigned char *line_data;
		for (int y = 0; y < surface_height; ++y){
			line_data = data + stride * y;
			for (int x = 0; x < surface_width; ++x){

				if ((x % 16 < 8) ^ (y % 16 < 8) ){
					v = mix_float(0.5, 1.0, pow(x / size, 2));
				}else{
					v = mix_float(0.5, 0.0, pow(1 - (y / size), 2));
				}

				line_data[2] = v * 255;
				line_data[1] = v / 2 * 255;
				line_data[0] = v / 4 * 255;
				line_data[3] = 0xFF;

				line_data += 4;
			}
		}

		ns->cache_range_2d = surface;
	}

	cairo_surface_mark_dirty(surface);
	cairo_save(cr);

	cairo_set_source_surface(cr, surface, pos_x, pos_y);
	//cairo_surface_destroy(surface);

	cairo_rectangle(cr, pos_x, pos_y, size, size);
	cairo_fill(cr);

	cairo_restore(cr);

}


static gboolean gtk_range_2d_motion_notify(GtkWidget *widget, GdkEventMotion *event){
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(widget);

	if (ns->grab_block){

		double dx = (event->x - widget->style->xthickness);
		double dy = (event->y - widget->style->ythickness);

		ns->x = clamp_float(dx / ns->block_size, 0, 1);
		ns->y = clamp_float(dy / ns->block_size, 0, 1);

		g_signal_emit(widget, gtk_range_2d_signals[VALUES_CHANGED], 0);

		gtk_widget_queue_draw(widget);
		return true;
	}

	return false;
}

static gboolean gtk_range_2d_expose(GtkWidget *widget, GdkEventExpose *event){

	GtkStateType state;

	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_ACTIVE;

	cairo_t *cr;

	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(widget);

/*	if (GTK_WIDGET_HAS_FOCUS(widget)){
		gtk_paint_focus(widget->style, widget->window, state, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);
	}*/

	cr = gdk_cairo_create(widget->window);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);


	draw_sat_val_block(ns, cr, 0, 0, ns->block_size);
	draw_dot(cr, ns->block_size * ns->x, ns->block_size * ns->y, 6);


	if (ns->xname){
		PangoLayout *layout;
		PangoFontDescription *font_description;
		font_description = pango_font_description_new();
		layout = pango_cairo_create_layout(cr);

		pango_font_description_set_family(font_description, "monospace");
		pango_font_description_set_weight(font_description, PANGO_WEIGHT_NORMAL);
		pango_font_description_set_absolute_size(font_description, 12 * PANGO_SCALE);
		pango_layout_set_font_description(layout, font_description);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

		cairo_set_source_rgb(cr, 1, 1, 1);

		pango_layout_set_text(layout, ns->xname, -1);
		pango_layout_set_width(layout, (widget->allocation.width - widget->style->xthickness * 2) * PANGO_SCALE);
		pango_layout_set_height(layout, (widget->allocation.height - widget->style->ythickness * 2) * PANGO_SCALE);
		pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		pango_cairo_update_layout(cr, layout);

		int width, height;
		pango_layout_get_pixel_size(layout, &width, &height);
		cairo_move_to(cr, 0, ns->block_size - height - 1);
		pango_cairo_show_layout(cr, layout);

		PangoContext *context = pango_layout_get_context(layout);
		pango_context_set_gravity_hint(context, PANGO_GRAVITY_HINT_STRONG);
		pango_context_set_base_gravity(context, PANGO_GRAVITY_NORTH);
		pango_layout_context_changed(layout);
		pango_layout_set_text(layout, ns->yname, -1);

		pango_layout_get_pixel_size(layout, &width, &height);
		cairo_move_to(cr, height + 1, 0);
		cairo_rotate(cr, 90 / (180.0 / M_PI));

		pango_cairo_update_layout(cr, layout);



		pango_cairo_show_layout(cr, layout);

		g_object_unref (layout);
		pango_font_description_free (font_description);
	}

	cairo_destroy(cr);

	return FALSE;
}


static gboolean gtk_range_2d_button_press(GtkWidget *widget, GdkEventButton *event) {
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(widget);


	gtk_widget_grab_focus(widget);

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){

		ns->grab_block = true;

		GdkCursor *cursor = gdk_cursor_new(GDK_CROSS);
		gdk_pointer_grab(gtk_widget_get_window(widget), false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK), NULL, cursor, GDK_CURRENT_TIME);
		gdk_cursor_destroy(cursor);

		double dx = (event->x - widget->style->xthickness);
		double dy = (event->y - widget->style->ythickness);

		ns->x = clamp_float(dx / ns->block_size, 0, 1);
		ns->y = clamp_float(dy / ns->block_size, 0, 1);

		g_signal_emit(widget, gtk_range_2d_signals[VALUES_CHANGED], 0);
		gtk_widget_queue_draw(widget);

		return true;
	}
	return false;
}

static gboolean gtk_range_2d_button_release(GtkWidget *widget, GdkEventButton *event) {
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(widget);

	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	ns->grab_block = false;

	return false;
}

void gtk_range_2d_set_axis(GtkRange2D *range_2d, const char *x, const char *y){
	GtkRange2DPrivate *ns = GTK_RANGE_2D_GET_PRIVATE(range_2d);
	if (ns->xname) g_free(ns->xname);
	ns->xname = g_strdup(x);
	if (ns->yname) g_free(ns->yname);
	ns->yname = g_strdup(y);
	gtk_widget_queue_draw(GTK_WIDGET(range_2d));
}



