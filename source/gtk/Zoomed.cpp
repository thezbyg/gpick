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

#include "Zoomed.h"

#include "../Color.h"
#include "../MathUtil.h"

#include <algorithm>
using namespace std;

#define GTK_ZOOMED_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_ZOOMED, GtkZoomedPrivate))

G_DEFINE_TYPE (GtkZoomed, gtk_zoomed, GTK_TYPE_DRAWING_AREA);

static gboolean
gtk_zoomed_expose (GtkWidget *widget, GdkEventExpose *event);

static void gtk_zoomed_finalize(GObject *zoomed_obj);

static GtkWindowClass *parent_class = NULL;

/*
static gboolean
gtk_zoomed_button_release (GtkWidget *widget, GdkEventButton *event);

static gboolean
gtk_zoomed_button_press (GtkWidget *node_system, GdkEventButton *event);

static gboolean
gtk_zoomed_motion_notify (GtkWidget *node_system, GdkEventMotion *event);
*/

enum
{
  COLOR_CHANGED,
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
	/*widget_class->button_release_event = gtk_zoomed_button_release;
	widget_class->button_press_event = gtk_zoomed_button_press;
	widget_class->motion_notify_event = gtk_zoomed_motion_notify;*/

	obj_class->finalize = gtk_zoomed_finalize;

	g_type_class_add_private (obj_class, sizeof (GtkZoomedPrivate));


	gtk_zoomed_signals[COLOR_CHANGED] = g_signal_new (
	     "color-changed",
	     G_OBJECT_CLASS_TYPE (obj_class),
	     G_SIGNAL_RUN_FIRST,
	     G_STRUCT_OFFSET (GtkZoomedClass, color_changed),
	     NULL, NULL,
	     g_cclosure_marshal_VOID__POINTER,
	     G_TYPE_NONE, 1,
	     G_TYPE_POINTER);
}

static void
gtk_zoomed_init (GtkZoomed *zoomed)
{
	gtk_widget_add_events (GTK_WIDGET (zoomed),
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
}





GtkWidget *
gtk_zoomed_new () {
	GtkWidget* widget=(GtkWidget*)g_object_new (GTK_TYPE_ZOOMED, NULL);
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(widget);
	gtk_widget_set_size_request(GTK_WIDGET(widget),150+widget->style->xthickness*2,150+widget->style->ythickness*2);

	ns->zoom=2;
	ns->pixbuf=0;
	ns->point.x=0;
	ns->point.y=0;

	return widget;
}

static void gtk_zoomed_finalize(GObject *zoomed_obj){
	GtkZoomedPrivate *ns = GTK_ZOOMED_GET_PRIVATE(zoomed_obj);

	if (ns->pixbuf){
		g_object_unref (ns->pixbuf);
		ns->pixbuf = 0;
	}
	
	G_OBJECT_CLASS(parent_class)->finalize (zoomed_obj);
}

void gtk_zoomed_update (GtkZoomed* zoomed) {
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);

	//GdkImage* image;
	GdkWindow* window;
	gint32 x, y, width, height;

	window	= gdk_get_default_root_window ();

	gdk_window_get_pointer (window, &x, &y, NULL);
	gdk_window_get_geometry (window, NULL, NULL, &width, &height, NULL);

	gint32 left, right, top, bottom;

	gint32 area_width = gint32(150 / ns->zoom);

	left	= x - area_width/2;
	top		= y - area_width/2;
	right	= x + (area_width-area_width/2);
	bottom	= y + (area_width-area_width/2);

	if (left<0){
		right+=-left;
		left=0;
	}
	if (right>width){
		left-=right-width;
		right=width;
	}
	if (top<0){
		bottom+=-top;
		top=0;
	}
	if (bottom>height){
		top-=bottom-height;
		bottom=height;
	}

	ns->point.x = (x+0.5 - left) * ns->zoom;
	ns->point.y = (y+0.5 - top) * ns->zoom;

	width	= right - left;
	height	= bottom - top;

	/*if (ns->image) g_object_unref (ns->image);
	ns->image = gdk_drawable_get_image (window, left, top, width, height);*/

	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_get_from_drawable (0, window, 0, left, top, 0, 0, width, height);

	if (ns->pixbuf) g_object_unref (ns->pixbuf);
	ns->pixbuf = gdk_pixbuf_scale_simple (pixbuf, 150, 150, GDK_INTERP_NEAREST);
	g_object_unref (pixbuf);

	gtk_widget_queue_draw(GTK_WIDGET(zoomed));

}
void gtk_zoomed_set_zoom (GtkZoomed* zoomed, gfloat zoom) {
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);
	if (zoom<2){
		ns->zoom=2;
	}else if (zoom>10){
		ns->zoom=10;
	}else{
		ns->zoom=zoom;
	}
	gtk_widget_queue_draw(GTK_WIDGET(zoomed));
}

gfloat gtk_zoomed_get_zoom (GtkZoomed* zoomed){
	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(zoomed);
	return ns->zoom;
}

static gboolean
gtk_zoomed_expose (GtkWidget *widget, GdkEventExpose *event)
{

	GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(widget);
	if (ns->pixbuf){
		
		gint pixbuf_x = max(event->area.x-widget->style->xthickness, 0);
		gint pixbuf_y = max(event->area.y-widget->style->ythickness, 0);

		gint pixbuf_width = min(150-pixbuf_x, 150);
		gint pixbuf_height = min(150-pixbuf_y, 150);
		
		if (pixbuf_width>0 && pixbuf_height>0)
			gdk_draw_pixbuf(widget->window,
					  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
					  ns->pixbuf,
					  pixbuf_x, pixbuf_y,
					  pixbuf_x+widget->style->xthickness, pixbuf_y+widget->style->ythickness,
					  pixbuf_width, pixbuf_height,
					  GDK_RGB_DITHER_NONE, 0, 0);
	}

	cairo_t *cr;
	cr = gdk_cairo_create (widget->window);
	
	cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);
	
	//cairo_set_source_rgb(cr, 1,1,1);
	/*cairo_move_to(cr, ns->point.x, ns->point.y);
	cairo_line_to(cr, ns->point.x+5, ns->point.y+5);
	cairo_stroke(cr);*/

	cairo_set_source_rgba(cr, 0,0,0,0.75);
	cairo_arc(cr, ns->point.x, ns->point.y, 5.5, -PI, PI);
	cairo_stroke(cr);

	cairo_set_source_rgba(cr, 1,1,1,0.75);
	cairo_arc(cr, ns->point.x, ns->point.y, 5, -PI, PI);
	cairo_stroke(cr);


	cairo_destroy (cr);

	gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);
	
	
	/*GtkZoomedPrivate *ns=GTK_ZOOMED_GET_PRIVATE(widget);

	GdkGC *dc = gdk_gc_new(widget);

	if (ns->image){
		gdk_draw_image (widget, dc, ns->image, 0, 0, 0, 0, 150/ns->zoom, 150/ns->zoom);
	}*/


	/*
	cr = gdk_cairo_create (widget->window);

	cairo_rectangle (cr,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	cairo_clip (cr);

	cairo_surface_t *image;


	if (ns->pixbuf){

		cairo_set_source(cr, ns->pixbuf);
		cairo_rectangle (cr, 0, 0, 150, 150;


	}

	cairo_destroy (cr);*/

	return FALSE;
}

