/*
 * Copyright (c) 2009-2025, Albertas Vyšniauskas
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

#include "ImageView.h"
#include <algorithm>
struct GtkImageViewPrivate {
	GdkPixbuf *image;
};
#define GET_PRIVATE(obj) *static_cast<GtkImageViewPrivate *>(gtk_image_view_get_instance_private(GTK_IMAGE_VIEW(obj)))
G_DEFINE_TYPE_WITH_CODE(GtkImageView, gtk_image_view, GTK_TYPE_DRAWING_AREA, G_ADD_PRIVATE(GtkImageView));
static void finalize(GObject *imageViewObject);
#if GTK_MAJOR_VERSION >= 3
static gboolean draw(GtkWidget *widget, cairo_t *cr);
#else
static gboolean expose(GtkWidget *widget, GdkEventExpose *event);
#endif
static void gtk_image_view_class_init(GtkImageViewClass *imageViewClass) {
	GObjectClass *objectClass = G_OBJECT_CLASS(imageViewClass);
	objectClass->finalize = finalize;
	GtkWidgetClass *widgetClass = GTK_WIDGET_CLASS(imageViewClass);
#if GTK_MAJOR_VERSION >= 3
	widgetClass->draw = draw;
#else
	widgetClass->expose_event = expose;
#endif
}
static void gtk_image_view_init(GtkImageView *imageView) {
}
GtkWidget *gtk_image_view_new() {
	GtkWidget *widget = (GtkWidget *)g_object_new(GTK_TYPE_IMAGE_VIEW, nullptr);
	auto &instance = GET_PRIVATE(widget);
	instance.image = nullptr;
	return widget;
}
static void finalize(GObject *imageViewObject) {
	auto &instance = GET_PRIVATE(imageViewObject);
	if (instance.image) {
		g_object_unref(instance.image);
		instance.image = nullptr;
	}
	G_OBJECT_CLASS(g_type_class_peek_parent(G_OBJECT_CLASS(GTK_IMAGE_VIEW_GET_CLASS(imageViewObject))))->finalize(imageViewObject);
}
static gboolean draw(GtkWidget *widget, cairo_t *cr) {
	auto &instance = GET_PRIVATE(widget);
#if GTK_MAJOR_VERSION >= 3
	int width = gtk_widget_get_allocated_width(widget), height = gtk_widget_get_allocated_height(widget);
#else
	GtkAllocation rectangle;
	gtk_widget_get_allocation(widget, &rectangle);
	int width = rectangle.width - widget->style->xthickness * 2 - 1, height = rectangle.height - widget->style->ythickness * 2 - 1;
#endif
	if (instance.image) {
		int imageWidth = gdk_pixbuf_get_width(instance.image);
		int imageHeight = gdk_pixbuf_get_height(instance.image);
		double scale = std::min(1.0, std::min(width / static_cast<double>(imageWidth), height / static_cast<double>(imageHeight)));
		cairo_translate(cr, (width - imageWidth * scale) / 2, (height - imageHeight * scale) / 2);
		cairo_scale(cr, scale, scale);
		gdk_cairo_set_source_pixbuf(cr, instance.image, 0, 0);
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BEST);
		cairo_paint(cr);
	}
	return true;
}
#if GTK_MAJOR_VERSION < 3
static gboolean expose(GtkWidget *widget, GdkEventExpose *event) {
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);
	cairo_translate(cr, widget->style->xthickness + 0.5, widget->style->ythickness + 0.5);
	gboolean result = draw(widget, cr);
	cairo_destroy(cr);
	return result;
}
#endif
void gtk_image_view_set_image(GtkImageView *imageView, GdkPixbuf *image) {
	auto &instance = GET_PRIVATE(imageView);
	if (instance.image) {
		g_object_unref(instance.image);
		instance.image = nullptr;
	}
	if (image)
		instance.image = g_object_ref(image);
	gtk_widget_queue_draw(GTK_WIDGET(imageView));
}
