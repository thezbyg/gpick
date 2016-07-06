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

#include "ColorWidget.h"
#include "../Color.h"
#include "../MathUtil.h"
#include <math.h>
#include <boost/math/special_functions/round.hpp>
#include <iostream>
using namespace std;

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_TYPE_COLOR, GtkColorPrivate))
G_DEFINE_TYPE(GtkColor, gtk_color, GTK_TYPE_DRAWING_AREA);
static gboolean button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean button_press(GtkWidget *widget, GdkEventButton *event);
static void finalize(GObject *color_obj);
#if GTK_MAJOR_VERSION >= 3
static gboolean draw(GtkWidget *widget, cairo_t *cr);
#else
static gboolean expose(GtkWidget *widget, GdkEventExpose *event);
static void size_request(GtkWidget *widget, GtkRequisition *requisition);
#endif
enum
{
	ACTIVATED, LAST_SIGNAL,
};
static guint signals[LAST_SIGNAL] = {};
struct GtkColorPrivate
{
	Color color;
	Color text_color;
	gchar* text;
	bool rounded_rectangle;
	bool h_center;
	bool secondary_color;
	double roundness;
	transformation::Chain *transformation_chain;
};
static void gtk_color_class_init(GtkColorClass *color_class)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(color_class);
	obj_class->finalize = finalize;
	g_type_class_add_private(obj_class, sizeof(GtkColorPrivate));
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(color_class);
	widget_class->button_release_event = button_release;
	widget_class->button_press_event = button_press;
#if GTK_MAJOR_VERSION >= 3
	widget_class->draw = draw;
#else
	widget_class->expose_event = expose;
	widget_class->size_request = size_request;
#endif
	signals[ACTIVATED] = g_signal_new("activated", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkColorClass, activated), nullptr, nullptr, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}
static void gtk_color_init(GtkColor *color)
{
	gtk_widget_add_events (GTK_WIDGET (color), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_2BUTTON_PRESS | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}
GtkWidget* gtk_color_new()
{
	GtkWidget* widget = (GtkWidget*)g_object_new(GTK_TYPE_COLOR, nullptr);
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	color_set(&ns->color, 0);
	ns->text = 0;
	ns->rounded_rectangle = false;
	ns->h_center = false;
	ns->secondary_color = false;
	ns->roundness = 20;
	ns->transformation_chain = 0;
	gtk_widget_set_can_focus(widget, true);
	return widget;
}
#if GTK_MAJOR_VERSION < 3
static void size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	gint width = 32 + widget->style->xthickness * 2;
	gint height = 16 + widget->style->ythickness * 2;
	if (ns->rounded_rectangle){
		width += ns->roundness;
		height += ns->roundness;
	}
	requisition->width = width;
	requisition->height = height;
}
#endif
static void finalize(GObject *color_obj)
{
	GtkColorPrivate *ns = GET_PRIVATE(color_obj);
	if (ns->text){
		g_free(ns->text);
		ns->text = 0;
	}
	gpointer parent_class = g_type_class_peek_parent(G_OBJECT_CLASS(GTK_COLOR_GET_CLASS(color_obj)));
	G_OBJECT_CLASS(parent_class)->finalize(color_obj);
}
void gtk_color_get_color(GtkColor* widget, Color* color)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	color_copy(&ns->color, color);
}
void gtk_color_set_text_color(GtkColor* widget, Color* color)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	color_copy(color, &ns->text_color);
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
void gtk_color_set_roundness(GtkColor* widget, double roundness)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	ns->roundness = roundness;
	gint width = 32;
	gint height = 16;
#if GTK_MAJOR_VERSION < 3
	width += GTK_WIDGET(widget)->style->xthickness * 2;
	height += GTK_WIDGET(widget)->style->ythickness * 2;
#endif
	if (ns->rounded_rectangle){
		width += ns->roundness;
		height += ns->roundness;
	}
	gtk_widget_set_size_request(GTK_WIDGET(widget), width, height);
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
void gtk_color_set_color(GtkColor* widget, Color* color, const char* text)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	color_copy(color, &ns->color);
	if (ns->secondary_color){
	}else{
		if (ns->transformation_chain){
			Color c;
			ns->transformation_chain->apply(&ns->color, &c);
			color_get_contrasting(&c, &ns->text_color);
		}else{
			color_get_contrasting(&ns->color, &ns->text_color);
		}
	}
	if (ns->text)
		g_free(ns->text);
	ns->text = 0;
	if (text)
		ns->text = g_strdup(text);
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
void gtk_color_set_rounded(GtkColor* widget, bool rounded_rectangle)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	ns->rounded_rectangle = rounded_rectangle;
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
void gtk_color_set_hcenter(GtkColor* widget, bool hcenter)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	ns->h_center = hcenter;
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
static void cairo_rounded_rectangle(cairo_t *cr, double x, double y, double width, double height, double roundness)
{
	double strength = 0.3;
	cairo_move_to(cr, x+roundness, y);
	cairo_line_to(cr, x+width-roundness, y);
	cairo_curve_to(cr, x+width-roundness*strength, y, x+width, y+roundness*strength, x+width, y+roundness);
	cairo_line_to(cr, x+width, y+height-roundness);
	cairo_curve_to(cr, x+width, y+height-roundness*strength, x+width-roundness*strength, y+height, x+width-roundness, y+height);
	cairo_line_to(cr, x+roundness, y+height);
	cairo_curve_to(cr, x+roundness*strength, y+height, x, y+height-roundness*strength, x, y+height-roundness);
	cairo_line_to(cr, x, y+roundness);
	cairo_curve_to(cr, x, y+roundness*strength, x+roundness*strength, y, x+roundness, y);
	cairo_close_path (cr);
}
static gboolean draw(GtkWidget *widget, cairo_t *cr)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	Color color;
#if GTK_MAJOR_VERSION >= 3
	int width = gtk_widget_get_allocated_width(widget), height = gtk_widget_get_allocated_height(widget);
#else
	GtkAllocation rectangle;
	gtk_widget_get_allocation(widget, &rectangle);
	int width = rectangle.width - widget->style->xthickness * 2 - 1, height = rectangle.height - widget->style->ythickness * 2 - 1;
#endif
	if (ns->rounded_rectangle){
		cairo_rounded_rectangle(cr, 0, 0, width, height, ns->roundness);
		if (ns->transformation_chain){
			ns->transformation_chain->apply(&ns->color, &color);
		}else{
			color_copy(&ns->color, &color);
		}
		cairo_set_source_rgb(cr, boost::math::round(color.rgb.red * 255.0) / 255.0, boost::math::round(color.rgb.green * 255.0) / 255.0, boost::math::round(color.rgb.blue * 255.0) / 255.0);
		cairo_fill_preserve(cr);
		if (gtk_widget_has_focus(widget)){
#if GTK_MAJOR_VERSION >= 3
			//TODO: GTK3 get border color
#else
			cairo_set_source_rgb(cr, widget->style->fg[GTK_STATE_NORMAL].red / 65536.0, widget->style->fg[GTK_STATE_NORMAL].green / 65536.0, widget->style->fg[GTK_STATE_NORMAL].blue / 65536.0);
#endif
			cairo_set_line_width(cr, 3);
		}else{
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_set_line_width(cr, 1);
		}
		cairo_stroke(cr);
	}else{
		if (ns->transformation_chain){
			ns->transformation_chain->apply(&ns->color, &color);
		}else{
			color_copy(&ns->color, &color);
		}
		cairo_set_source_rgb(cr, color.rgb.red, color.rgb.green, color.rgb.blue);
		cairo_paint(cr);
	}
	if (ns->text){
		PangoLayout *layout;
		PangoFontDescription *font_description;
		font_description = pango_font_description_new();
		layout = pango_cairo_create_layout(cr);
		pango_font_description_set_family(font_description, "monospace");
		pango_font_description_set_weight(font_description, PANGO_WEIGHT_NORMAL);
		pango_font_description_set_absolute_size(font_description, 14 * PANGO_SCALE);
		pango_layout_set_font_description(layout, font_description);
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		if (ns->transformation_chain){
			if (ns->secondary_color){
				ns->transformation_chain->apply(&ns->text_color, &color);
			}else{
				color_copy(&ns->text_color, &color);
			}
		}else{
			color_copy(&ns->text_color, &color);
		}
		cairo_set_source_rgb(cr, color.rgb.red, color.rgb.green, color.rgb.blue);
		pango_layout_set_markup(layout, ns->text, -1);
		pango_layout_set_width(layout, width * PANGO_SCALE);
		pango_layout_set_height(layout, height * PANGO_SCALE);
		int layout_width, layout_height;
		pango_layout_get_pixel_size(layout, &layout_width, &layout_height);
		cairo_move_to(cr, 0, (height - layout_height) / 2);
		if (ns->h_center)
			pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		else
			pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
		pango_cairo_update_layout(cr, layout);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);
		pango_font_description_free(font_description);
	}
	return FALSE;
}
#if GTK_MAJOR_VERSION < 3
static gboolean expose(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);
	cairo_translate(cr, widget->style->xthickness + 0.5, widget->style->ythickness + 0.5);
	gboolean result = draw(widget, cr);
	cairo_destroy(cr);
	return result;
}
#endif
static gboolean button_release(GtkWidget *widget, GdkEventButton *event)
{
	gtk_widget_grab_focus(widget);
	return FALSE;
}
static gboolean button_press(GtkWidget *widget, GdkEventButton *event)
{
	gtk_widget_grab_focus(widget);
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
		g_signal_emit(widget, signals[ACTIVATED], 0);
	}
	return FALSE;
}
void gtk_color_set_transformation_chain(GtkColor* widget, transformation::Chain *chain)
{
	GtkColorPrivate *ns = GET_PRIVATE(widget);
	ns->transformation_chain = chain;
	if (!ns->secondary_color){
		if (ns->transformation_chain){
			Color c;
			ns->transformation_chain->apply(&ns->color, &c);
			color_get_contrasting(&c, &ns->text_color);
		}else{
			color_get_contrasting(&ns->color, &ns->text_color);
		}
	}
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
