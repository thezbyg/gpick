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

#include "Zoomed.h"

#include "../Color.h"
#include "../MathUtil.h"
#include <math.h>

#include <algorithm>
using namespace std;

#define GTK_ZOOMED_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_ZOOMED, GtkZoomedPrivate))

G_DEFINE_TYPE (GtkZoomed, gtk_zoomed, GTK_TYPE_DRAWING_AREA);

static gboolean
gtk_zoomed_expose (GtkWidget *widget, GdkEventExpose *event);

static void gtk_zoomed_finalize(GObject *zoomed_obj);

static GtkWindowClass *parent_class = NULL;

static gboolean
gtk_zoomed_button_press (GtkWidget *node_system, GdkEventButton *event);

/*
static gboolean
gtk_zoomed_button_release (GtkWidget *widget, GdkEventButton *event);


static gboolean
gtk_zoomed_motion_notify (GtkWidget *node_system, GdkEventMotion *event);
*/

enum
{
  COLOR_CHANGED,
	ACTIVATED,
  LAST_SIGNAL
};

static guint gtk_zoomed_signals[LAST_SIGNAL] = { 0 };


typedef struct GtkZoomedPrivate GtkZoomedPrivate;

typedef struct GtkZoomedPrivate
{
	Color color;
	gfloat zoom;

	GdkPixbuf *pixbuf;

	vector2 point;
	vector2 point_size;
	int32_t width_height;

	bool fade;

}GtkZoomedPrivate;

static void
gtk_zoomed_class_init (GtkZoomedClass *zoomed_class)
{
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS (zoomed_class);
	widget_class = GTK_WIDGET_CLASS (zoomed_class);

	parent_class = (GtkWindowClass*)g_type_class_peek_parent(G_OBJECT_CLASS(zoomed_class));

	/* GtkWidget signals */

	widget_class->expose_event = gtk_zoomed_expose;
	widget_class->button_press_event = gtk_zoomed_button_press;
	/*widget_class->button_release_event = gtk_zoomed_button_release;
	widget_class->motion_notify_event = gtk_zoomed_motion_notify;*/

	obj_class->finalize = gtk_zoomed_finalize;

	g_type_class_add_private (obj_class, sizeof (GtkZoomedPrivate));

	gtk_zoomed_signals[ACTIVATED] = g_signal_new(
			"activated",
			G_OBJECT_CLASS_TYPE(obj_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkZoomedClass, activated),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);

	gtk_zoomed_signals[COLOR_CHANGED] = g_signal_new (
	     "color-changed",
	     G_OBJECT_CLASS_TYPE(obj_class),
	     G_SIGNAL_RUN_FIRST,
	     G_STRUCT_OFFSET(GtkZoomedClass, color_changed),
	     NULL, NULL,
	     g_cclosure_marshal_VOID__POINTER,
	     G_TYPE_NONE, 1,
	     G_TYPE_POINTER);
}

static void
gtk_zoomed_init (GtkZoomed *zoomed)
{
	gtk_widget_add_events (GTK_WIDGET (zoomed),
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK | GDK_2BUTTON_PRESS);
}





GtkWidget *
gtk_zoomed_new () {
	GtkWidget* widget=(GtkWidget*)g_object_new (GTK_TYPE_ZOOMED, NULL);
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(widget);

	ns->fade = false;
	ns->zoom = 20;
	ns->point.x = 0;
	ns->point.y = 0;
	ns->width_height = 150;
	ns->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, ns->width_height, ns->width_height);

  int rowstride;
  rowstride = gdk_pixbuf_get_rowstride(ns->pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels(ns->pixbuf);
  for (int y = 0; y < ns->width_height; y++){
		guchar *p = pixels + y * rowstride;
		for (int x = 0; x < ns->width_height * 3; x++){
			p[x] = 0x80;
		}
	}

	gtk_widget_set_size_request(GTK_WIDGET(widget), ns->width_height + widget->style->xthickness*2, ns->width_height + widget->style->ythickness*2);

	return widget;
}

int32_t gtk_zoomed_get_size(GtkZoomed *zoomed){
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);
	return ns->width_height;
}

void gtk_zoomed_set_size(GtkZoomed *zoomed, int32_t width_height){
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);
	if (ns->width_height != width_height){
		if (ns->pixbuf){
			g_object_unref (ns->pixbuf);
			ns->pixbuf = 0;
		}

		ns->width_height = width_height;
		ns->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, ns->width_height, ns->width_height);

		int rowstride;
		rowstride = gdk_pixbuf_get_rowstride(ns->pixbuf);
		guchar *pixels = gdk_pixbuf_get_pixels(ns->pixbuf);
		for (int y = 0; y < ns->width_height; y++){
			guchar *p = pixels + y * rowstride;
			for (int x = 0; x < ns->width_height * 3; x++){
				p[x] = 0x80;
			}
		}

		gtk_widget_set_size_request(GTK_WIDGET(zoomed), ns->width_height + GTK_WIDGET(zoomed)->style->xthickness*2, ns->width_height + GTK_WIDGET(zoomed)->style->ythickness*2);
	}
}

void gtk_zoomed_set_fade(GtkZoomed* zoomed, bool fade){
	GtkZoomedPrivate *ns = GTK_ZOOMED_GET_PRIVATE(zoomed);
	ns->fade = fade;
	gtk_widget_queue_draw(GTK_WIDGET(zoomed));
}

static void gtk_zoomed_finalize(GObject *zoomed_obj){
	GtkZoomedPrivate *ns = GTK_ZOOMED_GET_PRIVATE(zoomed_obj);

	if (ns->pixbuf){
		g_object_unref (ns->pixbuf);
		ns->pixbuf = 0;
	}

	G_OBJECT_CLASS(parent_class)->finalize (zoomed_obj);
}

static double zoom_transformation(double value)
{
	return (1 - log(1 + value * 0.01 * 3) / log(1 + 3)) / 2;
}

void gtk_zoomed_get_screen_rect(GtkZoomed* zoomed, math::Vec2<int>& pointer, math::Vec2<int>& screen_size, math::Rect2<int> *rect){
	GtkZoomedPrivate *ns = GTK_ZOOMED_GET_PRIVATE(zoomed);

	gint32 x = pointer.x, y = pointer.y;
	gint32 width = screen_size.x, height = screen_size.y;

	gint32 left, right, top, bottom;

	gint32 area_width = uint32_t(ns->width_height * zoom_transformation(ns->zoom));
	if (!area_width) area_width = 1;

	left	= x - area_width / 2;
	top		= y - area_width / 2;
	right	= x + (area_width - area_width / 2);
	bottom	= y + (area_width - area_width / 2);

	if (left < 0){
		right += -left;
		left = 0;
	}
	if (right > width){
		left -= right - width;
		right = width;
	}
	if (top < 0){
		bottom += -top;
		top = 0;
	}
	if (bottom > height){
		top -= bottom - height;
		bottom = height;
	}

	width	= right - left;
	height	= bottom - top;

	*rect = math::Rect2<int>(left, top, right, bottom);
}

void gtk_zoomed_update(GtkZoomed* zoomed, math::Vec2<int>& pointer, math::Vec2<int>& screen_size, math::Vec2<int>& offset, GdkPixbuf* pixbuf){
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);

	gint32 x=pointer.x, y=pointer.y;
	gint32 width=screen_size.x, height=screen_size.y;

	gint32 left, right, top, bottom;

	gint32 area_width = uint32_t(ns->width_height * zoom_transformation(ns->zoom));
	if (!area_width) area_width = 1;

	left	= x - area_width / 2;
	top		= y - area_width / 2;
	right	= x + (area_width - area_width / 2);
	bottom	= y + (area_width - area_width / 2);

	if (left < 0){
		right += -left;
		left=0;
	}
	if (right > width){
		left -= right - width;
		right = width;
	}
	if (top < 0){
		bottom += -top;
		top = 0;
	}
	if (bottom > height){
		top -= bottom - height;
		bottom = height;
	}

	gint32 xl = ((x - left) * ns->width_height) / area_width;
	gint32 xh = (((x + 1) - left) * ns->width_height) / area_width;
	gint32 yl = ((y - top) * ns->width_height) / area_width;
	gint32 yh = (((y + 1) - top) * ns->width_height) / area_width;

	ns->point.x = (xl + xh) / 2.0;
	ns->point.y = (yl + yh) / 2.0;
	ns->point_size.x = xh - xl;
	ns->point_size.y = yh - yl;

	width	= right - left;
	height	= bottom - top;

	gdk_pixbuf_scale(pixbuf, ns->pixbuf, 0, 0, ns->width_height, ns->width_height, -offset.x * ns->width_height / (double)width, -offset.y * ns->width_height / (double)height, ns->width_height / (double)width, ns->width_height / (double)height, GDK_INTERP_NEAREST);

	gtk_widget_queue_draw(GTK_WIDGET(zoomed));

}
void gtk_zoomed_set_zoom (GtkZoomed* zoomed, gfloat zoom) {
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);
	if (zoom < 0){
		ns->zoom = 0;
	}else if (zoom > 100){
		ns->zoom = 100;
	}else{
		ns->zoom = zoom;
	}
	gtk_widget_queue_draw(GTK_WIDGET(zoomed));
}

gfloat gtk_zoomed_get_zoom (GtkZoomed* zoomed){
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);
	return ns->zoom;
}

static gboolean gtk_zoomed_expose (GtkWidget *widget, GdkEventExpose *event){
	GtkZoomedPrivate *ns = GTK_ZOOMED_GET_PRIVATE(widget);

	cairo_t *cr;
	cr = gdk_cairo_create(widget->window);
	cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);

	if (ns->pixbuf){
		gdk_cairo_set_source_pixbuf(cr, ns->pixbuf, 0, 0);
		if (ns->fade){
			cairo_paint_with_alpha(cr, 0.2);
		}else{
			cairo_paint(cr);
		}
	}

	if (!ns->fade){
		float radius;
		float size = vector2_length(&ns->point_size);
    if (size < 5){
			radius = 5;
		}else if (size < 25){
			radius = 7;
		}else if (size < 50){
			radius = 10;
		}else{
			radius = 15;
		}

		cairo_set_source_rgba(cr, 0,0,0,0.75);
		cairo_arc(cr, ns->point.x, ns->point.y, radius + 0.5, -PI, PI);
		cairo_stroke(cr);

		cairo_set_source_rgba(cr, 1,1,1,0.75);
		cairo_arc(cr, ns->point.x, ns->point.y, radius, -PI, PI);
		cairo_stroke(cr);
	}

	cairo_destroy (cr);

	gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, ns->width_height, ns->width_height);

	return true;
}

static gboolean gtk_zoomed_button_press(GtkWidget *widget, GdkEventButton *event){
	gtk_widget_grab_focus(widget);

	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
		g_signal_emit(widget, gtk_zoomed_signals[ACTIVATED], 0);
	}

	return FALSE;
}
