/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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
#include "Color.h"
using namespace std;

enum {
	VALUES_CHANGED, LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {};
struct GtkRange2DPrivate {
	float x;
	float y;
	char *xname;
	char *yname;
	float block_size;
	bool grab_block;
	cairo_surface_t *cache_range_2d;
#if GTK_MAJOR_VERSION >= 3
	GdkDevice *pointer_grab;
#endif
};
#define GET_PRIVATE(obj) reinterpret_cast<GtkRange2DPrivate *>(gtk_range_2d_get_instance_private(GTK_RANGE_2D(obj)))
G_DEFINE_TYPE_WITH_CODE(GtkRange2D, gtk_range_2d, GTK_TYPE_DRAWING_AREA, G_ADD_PRIVATE(GtkRange2D));
static gboolean button_release(GtkWidget *range_2d, GdkEventButton *event);
static gboolean button_press(GtkWidget *range_2d, GdkEventButton *event);
static gboolean motion_notify(GtkWidget *widget, GdkEventMotion *event);
#if GTK_MAJOR_VERSION >= 3
static gboolean draw(GtkWidget *widget, cairo_t *cr);
#else
static gboolean expose(GtkWidget *range_2d, GdkEventExpose *event);
#endif
static void finalize(GObject *range_2d_obj)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(range_2d_obj);
	if (ns->xname) g_free(ns->xname);
	if (ns->yname) g_free(ns->yname);
	if (ns->cache_range_2d){
		cairo_surface_destroy(ns->cache_range_2d);
		ns->cache_range_2d = 0;
	}
	gpointer parent_class = g_type_class_peek_parent(G_OBJECT_CLASS(GTK_RANGE_2D_GET_CLASS(range_2d_obj)));
	G_OBJECT_CLASS(parent_class)->finalize(range_2d_obj);
}
static void gtk_range_2d_class_init(GtkRange2DClass *range_2d_class)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(range_2d_class);
	obj_class->finalize = finalize;
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(range_2d_class);
	widget_class->button_release_event = button_release;
	widget_class->button_press_event = button_press;
	widget_class->motion_notify_event = motion_notify;
#if GTK_MAJOR_VERSION >= 3
	widget_class->draw = draw;
#else
	widget_class->expose_event = expose;
#endif
	signals[VALUES_CHANGED] = g_signal_new("values_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkRange2DClass, values_changed), nullptr, nullptr, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}
static void gtk_range_2d_init(GtkRange2D *range_2d)
{
	gtk_widget_add_events(GTK_WIDGET(range_2d), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}
GtkWidget* gtk_range_2d_new()
{
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_RANGE_2D, nullptr);
	GtkRange2DPrivate *ns = GET_PRIVATE(widget);
	ns->block_size = 128;
	ns->grab_block = false;
	ns->xname = 0;
	ns->yname = 0;
#if GTK_MAJOR_VERSION >= 3
	gtk_widget_set_size_request(GTK_WIDGET(widget), static_cast<int>(ns->block_size), static_cast<int>(ns->block_size));
#else
	gtk_widget_set_size_request(GTK_WIDGET(widget), static_cast<int>(ns->block_size + widget->style->xthickness * 2), static_cast<int>(ns->block_size + widget->style->ythickness * 2));
#endif
	ns->cache_range_2d = 0;
#if GTK_MAJOR_VERSION >= 3
	ns->pointer_grab = nullptr;
#endif
	gtk_widget_set_can_focus(widget, true);
	return widget;
}
void gtk_range_2d_set_values(GtkRange2D* range_2d, double x, double y)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(range_2d);
	ns->x = static_cast<float>(x);
	ns->y = static_cast<float>(1 - y);
	gtk_widget_queue_draw(GTK_WIDGET(range_2d));
}
double gtk_range_2d_get_x(GtkRange2D* range_2d)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(range_2d);
	return ns->x;
}
double gtk_range_2d_get_y(GtkRange2D* range_2d)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(range_2d);
	return 1 - ns->y;
}
static void draw_dot(cairo_t *cr, double x, double y, double size)
{
	cairo_arc(cr, x, y, size - 1, 0, 2 * math::PI);
	cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
	cairo_set_line_width(cr, 2);
	cairo_stroke(cr);
	cairo_arc(cr, x, y, size, 0, 2 * math::PI);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
}
static void draw_sat_val_block(GtkRange2DPrivate *ns, cairo_t *cr, double pos_x, double pos_y, double size)
{
	cairo_surface_t *surface;
	if (ns->cache_range_2d){
		surface = ns->cache_range_2d;
	}else{
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, static_cast<int>(std::ceil(size)), static_cast<int>(std::ceil(size)));
		unsigned char *data = cairo_image_surface_get_data(surface);
		int stride = cairo_image_surface_get_stride(surface);
		int surface_width = cairo_image_surface_get_width(surface);
		int surface_height = cairo_image_surface_get_height(surface);
		float v;
		uint8_t *line_data;
		for (int y = 0; y < surface_height; ++y){
			line_data = data + stride * y;
			for (int x = 0; x < surface_width; ++x){
				if ((x % 16 < 8) ^ (y % 16 < 8) ){
					v = math::mix(0.5f, 1.0f, static_cast<float>(std::pow(x / size, 2)));
				}else{
					v = math::mix(0.5f, 0.0f, static_cast<float>(std::pow(1 - (y / size), 2)));
				}
				line_data[2] = static_cast<uint8_t>(v * 255);
				line_data[1] = static_cast<uint8_t>(v / 2 * 255);
				line_data[0] = static_cast<uint8_t>(v / 4 * 255);
				line_data[3] = 0xFF;
				line_data += 4;
			}
		}
		ns->cache_range_2d = surface;
	}
	cairo_surface_mark_dirty(surface);
	cairo_save(cr);
	cairo_set_source_surface(cr, surface, pos_x, pos_y);
	cairo_rectangle(cr, pos_x, pos_y, size, size);
	cairo_fill(cr);
	cairo_restore(cr);
}
static gboolean motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(widget);
	if (ns->grab_block){
#if GTK_MAJOR_VERSION >= 3
		double dx = event->x;
		double dy = event->y;
#else
		double dx = (event->x - widget->style->xthickness);
		double dy = (event->y - widget->style->ythickness);
#endif
		ns->x = math::clamp(static_cast<float>(dx / ns->block_size));
		ns->y = math::clamp(static_cast<float>(dy / ns->block_size));
		g_signal_emit(widget, signals[VALUES_CHANGED], 0);
		gtk_widget_queue_draw(widget);
		return true;
	}
	return false;
}
static gboolean draw(GtkWidget *widget, cairo_t *cr)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(widget);
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
#if GTK_MAJOR_VERSION >= 3
		int width = gtk_widget_get_allocated_width(widget), height = gtk_widget_get_allocated_height(widget);
		int padding_x = 0, padding_y = 0;
#else
		int width = widget->allocation.width, height = widget->allocation.height;
		int padding_x = widget->style->xthickness, padding_y = widget->style->ythickness;
#endif
		pango_layout_set_width(layout, (width - padding_x * 2) * PANGO_SCALE);
		pango_layout_set_height(layout, (height - padding_y * 2) * PANGO_SCALE);
		pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		pango_cairo_update_layout(cr, layout);
		int layout_width, layout_height;
		pango_layout_get_pixel_size(layout, &layout_width, &layout_height);
		cairo_move_to(cr, 0, ns->block_size - layout_height - 1);
		pango_cairo_show_layout(cr, layout);
		PangoContext *context = pango_layout_get_context(layout);
		pango_context_set_gravity_hint(context, PANGO_GRAVITY_HINT_STRONG);
		pango_context_set_base_gravity(context, PANGO_GRAVITY_NORTH);
		pango_layout_context_changed(layout);
		pango_layout_set_text(layout, ns->yname, -1);
		pango_layout_get_pixel_size(layout, &layout_width, &layout_height);
		cairo_move_to(cr, layout_height + 1, 0);
		cairo_rotate(cr, 90 / (180.0 / math::PI));
		pango_cairo_update_layout(cr, layout);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);
		pango_font_description_free(font_description);
	}
	return false;
}
#if GTK_MAJOR_VERSION < 3
static gboolean expose(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);
	cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);
	gboolean result = draw(widget, cr);
	cairo_destroy(cr);
	return result;
}
#endif
static gboolean button_press(GtkWidget *widget, GdkEventButton *event)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(widget);
	gtk_widget_grab_focus(widget);
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){
		ns->grab_block = true;
		GdkCursor *cursor = gdk_cursor_new_for_display(gtk_widget_get_display(widget), GDK_CROSS);
#if GTK_MAJOR_VERSION >= 3
		ns->pointer_grab = event->device;
		gdk_seat_grab(gdk_device_get_seat(event->device), gtk_widget_get_window(widget), GDK_SEAT_CAPABILITY_ALL, false, cursor, nullptr, nullptr, nullptr);
#else
		gdk_pointer_grab(gtk_widget_get_window(widget), false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK), nullptr, cursor, GDK_CURRENT_TIME);
#endif
#if GTK_MAJOR_VERSION >= 3
		g_object_unref(cursor);
		double dx = event->x;
		double dy = event->y;
#else
		gdk_cursor_unref(cursor);
		double dx = (event->x - widget->style->xthickness);
		double dy = (event->y - widget->style->ythickness);
#endif
		ns->x = math::clamp(static_cast<float>(dx / ns->block_size));
		ns->y = math::clamp(static_cast<float>(dy / ns->block_size));
		g_signal_emit(widget, signals[VALUES_CHANGED], 0);
		gtk_widget_queue_draw(widget);
		return true;
	}
	return false;
}
static gboolean button_release(GtkWidget *widget, GdkEventButton *event)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(widget);
#if GTK_MAJOR_VERSION >= 3
	if (ns->pointer_grab){
		gdk_seat_ungrab(gdk_device_get_seat(ns->pointer_grab));
		ns->pointer_grab = nullptr;
	}
#else
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
#endif
	ns->grab_block = false;
	return false;
}
void gtk_range_2d_set_axis(GtkRange2D *range_2d, const char *x, const char *y)
{
	GtkRange2DPrivate *ns = GET_PRIVATE(range_2d);
	if (ns->xname) g_free(ns->xname);
	ns->xname = g_strdup(x);
	if (ns->yname) g_free(ns->yname);
	ns->yname = g_strdup(y);
	gtk_widget_queue_draw(GTK_WIDGET(range_2d));
}
