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

#include "ColorWidget.h"
#include "../Color.h"
#include "../MathUtil.h"
#include <math.h>

#define GTK_COLOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COLOR, GtkColorPrivate))

G_DEFINE_TYPE (GtkColor, gtk_color, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_color_expose(GtkWidget *widget, GdkEventExpose *event);

enum {
	LAST_SIGNAL,
};

static guint gtk_color_signals[LAST_SIGNAL] = { };

typedef struct GtkColorPrivate GtkColorPrivate;

typedef struct GtkColorPrivate {
	Color color;
	Color text_color;
	gchar* text;
} GtkColorPrivate;

static void gtk_color_class_init(GtkColorClass *color_class) {
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS(color_class);
	widget_class = GTK_WIDGET_CLASS(color_class);

	widget_class->expose_event = gtk_color_expose;
	g_type_class_add_private(obj_class, sizeof(GtkColorPrivate));
}

static void gtk_color_init(GtkColor *swatch) {
	//gtk_widget_add_events(GTK_WIDGET(swatch), GDK_FOCUS_CHANGE_MASK);
}

GtkWidget* gtk_color_new(void) {
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_COLOR, NULL);
	GtkColorPrivate *ns = GTK_COLOR_GET_PRIVATE(widget);

	gtk_widget_set_size_request(GTK_WIDGET(widget), 16+widget->style->xthickness*2, 16+widget->style->ythickness*2);

	color_set(&ns->color, 0);
	ns->text = 0;

	//GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);

	return widget;
}


void gtk_color_set_color(GtkColor* widget, Color* color, gchar* text) {
	GtkColorPrivate *ns = GTK_COLOR_GET_PRIVATE(widget);
	color_copy(color, &ns->color);
	color_get_contrasting(&ns->color, &ns->text_color);
	
	if (ns->text)
		g_free(ns->text);
	ns->text = 0;
	if (text)
		ns->text = g_strdup(text);
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}

static gboolean gtk_color_expose(GtkWidget *widget, GdkEventExpose *event) {
	
	GtkStateType state;
	
	/*if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_ACTIVE;*/

	
	cairo_t *cr;

	GtkColorPrivate *ns = GTK_COLOR_GET_PRIVATE(widget);
	
	//gtk_paint_shadow(widget->style, widget->window, state, GTK_SHADOW_IN, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);

	cr = gdk_cairo_create(widget->window);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_set_source_rgb(cr, ns->color.rgb.red, ns->color.rgb.green, ns->color.rgb.blue);
	cairo_fill(cr);
	
	if (ns->text){
		cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, 14);
	
		cairo_text_extents_t extends;
		cairo_text_extents(cr, ns->text, &extends);
	
		cairo_set_source_rgb(cr, ns->text_color.rgb.red, ns->text_color.rgb.green, ns->text_color.rgb.blue);
		cairo_move_to(cr, widget->style->xthickness, widget->allocation.height/2 + extends.height/2);
		cairo_show_text(cr, ns->text);
	}
	
	cairo_destroy(cr);
	
	/*if (GTK_WIDGET_HAS_FOCUS(widget)){
		gtk_paint_focus(widget->style, widget->window, state, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);
	}*/

	return FALSE;
}

