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

#include "LayoutPreview.h"
#include "layout/System.h"
#include "layout/Box.h"
#include "layout/Style.h"
#include "layout/Context.h"
#include "transformation/Chain.h"
#include <typeinfo>
enum {
	COLOR_CHANGED,
	EMPTY,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = {};
struct GtkLayoutPreviewPrivate {
	common::Ref<layout::System> system;
	math::Rectangle<float> area;
	common::Ref<layout::Style> selectedStyle;
	common::Ref<layout::Box> selectedBox;
	bool fill;
	transformation::Chain *transformationChain;
	GtkLayoutPreviewPrivate() {
		area = math::Rectangle<float>(0, 0, 1, 1);
		fill = false;
		transformationChain = nullptr;
	}
	~GtkLayoutPreviewPrivate() {
	}
};
#define GET_PRIVATE(obj) reinterpret_cast<GtkLayoutPreviewPrivate *>(gtk_layout_preview_get_instance_private(GTK_LAYOUT_PREVIEW(obj)))
G_DEFINE_TYPE_WITH_CODE(GtkLayoutPreview, gtk_layout_preview, GTK_TYPE_DRAWING_AREA, G_ADD_PRIVATE(GtkLayoutPreview));
static gboolean button_release(GtkWidget *layout_preview, GdkEventButton *event);
static gboolean button_press(GtkWidget *layout_preview, GdkEventButton *event);
#if GTK_MAJOR_VERSION >= 3
static gboolean draw(GtkWidget *widget, cairo_t *cr);
#else
static gboolean expose(GtkWidget *layout_preview, GdkEventExpose *event);
#endif
static void gtk_layout_preview_class_init(GtkLayoutPreviewClass *klass) {
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->button_release_event = button_release;
	widget_class->button_press_event = button_press;
#if GTK_MAJOR_VERSION >= 3
	widget_class->draw = draw;
#else
	widget_class->expose_event = expose;
#endif
	signals[COLOR_CHANGED] = g_signal_new("color_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkLayoutPreviewClass, color_changed), nullptr, nullptr, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
}
static void gtk_layout_preview_init(GtkLayoutPreview *layout_preview) {
	gtk_widget_add_events(GTK_WIDGET(layout_preview), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}
static void destroy(GtkLayoutPreview *widget) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	ns->~GtkLayoutPreviewPrivate();
}
GtkWidget *gtk_layout_preview_new() {
	GtkWidget *widget = (GtkWidget *)g_object_new(GTK_TYPE_LAYOUT_PREVIEW, nullptr);
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	new (ns) GtkLayoutPreviewPrivate();
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(destroy), nullptr);
	gtk_widget_set_can_focus(widget, true);
	return widget;
}
static bool setSelectedBox(GtkLayoutPreviewPrivate *ns, common::Ref<layout::Box> box) {
	bool changed = false;
	if (box && box->style()) {
		ns->selectedStyle = box->style();
		if (ns->selectedBox != box)
			changed = true;
		ns->selectedBox = box;
		ns->system->setSelected(box);
	} else {
		if (ns->selectedStyle)
			changed = true;
		ns->selectedStyle = common::nullRef;
		ns->selectedBox = common::nullRef;
		ns->system->setSelected(common::nullRef);
	}
	return changed;
}
static math::Vector2i getSize(GtkWidget *widget) {
#if GTK_MAJOR_VERSION >= 3
	return math::Vector2i(gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
#else
	GtkAllocation rectangle;
	gtk_widget_get_allocation(widget, &rectangle);
	return math::Vector2i(rectangle.width - widget->style()->xthickness * 2 - 1, rectangle.height - widget->style()->ythickness * 2 - 1);
#endif
}
static gboolean draw(GtkWidget *widget, cairo_t *cr) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system && ns->system->box()) {
		ns->area = math::Rectangle<float>(0, 0, 1, 1);
		if (ns->fill) {
			auto widgetSize = getSize(widget);
			auto layoutSize = ns->system->box()->rect().size();
			ns->area = math::Rectangle<float>(0, 0, widgetSize.x / layoutSize.x, widgetSize.y / layoutSize.y);
		}
		layout::Context context(*ns->system, cr, ns->transformationChain);
		ns->system->box()->draw(context, ns->area);
	}
	return true;
}
#if GTK_MAJOR_VERSION < 3
static gboolean expose(GtkWidget *widget, GdkEventExpose *event) {
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);
	gboolean result = draw(widget, cr);
	cairo_destroy(cr);
	return result;
}
#endif
static gboolean button_release(GtkWidget *layout_preview, GdkEventButton *event) {
	return true;
}
static gboolean button_press(GtkWidget *widget, GdkEventButton *event) {
	gtk_widget_grab_focus(widget);
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system) {
		math::Vector2f point = math::Vector2f(static_cast<float>((event->x - ns->area.getX()) / ns->area.getWidth()), static_cast<float>((event->y - ns->area.getY()) / ns->area.getHeight()));
		if (setSelectedBox(ns, ns->system->getBoxAt(point))) {
			gtk_widget_queue_draw(GTK_WIDGET(widget));
		}
	}
	return false;
}
int gtk_layout_preview_set_system(GtkLayoutPreview *widget, common::Ref<layout::System> system) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	ns->system = system;
	if (ns->system && ns->system->box()) {
		gtk_widget_set_size_request(GTK_WIDGET(widget), static_cast<int>(ns->system->box()->rect().getWidth()), static_cast<int>(ns->system->box()->rect().getHeight()));
	}
	gtk_widget_queue_draw(GTK_WIDGET(widget));
	return 0;
}
int gtk_layout_preview_set_color_at(GtkLayoutPreview *widget, Color *color, gdouble x, gdouble y) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (!ns->system)
		return -1;
	math::Vector2f point = math::Vector2f(static_cast<float>((x - ns->area.getX()) / ns->area.getWidth()), static_cast<float>((y - ns->area.getY()) / ns->area.getHeight()));
	common::Ref<layout::Box> box = ns->system->getBoxAt(point);
	if (box && box->style() && !box->locked()) {
		box->style()->setColor(*color);
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return 0;
	}
	return -1;
}
int gtk_layout_preview_set_color_named(GtkLayoutPreview *widget, const Color &color, const char *name) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (!ns->system)
		return -1;
	common::Ref<layout::Box> box = ns->system->getNamedBox(name);
	if (box && box->style() && !box->locked()) {
		box->style()->setColor(color);
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return 0;
	}
	return -1;
}
int gtk_layout_preview_set_focus_named(GtkLayoutPreview *widget, const char *name) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (!ns->system)
		return -1;
	common::Ref<layout::Box> box;
	if (setSelectedBox(ns, box = ns->system->getNamedBox(name))) {
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return (box) ? (0) : (-1);
	}
	return -1;
}
int gtk_layout_preview_set_focus_at(GtkLayoutPreview *widget, gdouble x, gdouble y) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (!ns->system)
		return -1;
	math::Vector2f point = math::Vector2f(static_cast<float>((x - ns->area.getX()) / ns->area.getWidth()), static_cast<float>((y - ns->area.getY()) / ns->area.getHeight()));
	common::Ref<layout::Box> box;
	if (setSelectedBox(ns, box = ns->system->getBoxAt(point))) {
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return (box) ? (0) : (-1);
	}
	return -1;
}
int gtk_layout_preview_get_current_style(GtkLayoutPreview *widget, common::Ref<layout::Style> &style) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system && ns->selectedStyle) {
		style = ns->selectedStyle;
		return 0;
	}
	return -1;
}
int gtk_layout_preview_get_current_color(GtkLayoutPreview *widget, Color &color) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system && ns->selectedStyle && ns->selectedBox) {
		color = ns->selectedBox->style()->color();
		return 0;
	}
	return -1;
}
int gtk_layout_preview_set_current_color(GtkLayoutPreview *widget, const Color &color) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system && ns->selectedStyle && ns->selectedBox && !ns->selectedBox->locked()) {
		ns->selectedBox->style()->setColor(color);
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return 0;
	}
	return -1;
}
bool gtk_layout_preview_is_selected(GtkLayoutPreview *widget) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system && ns->selectedStyle && ns->selectedBox) {
		return true;
	}
	return false;
}
bool gtk_layout_preview_is_editable(GtkLayoutPreview *widget) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	if (ns->system && ns->selectedStyle && ns->selectedBox) {
		return !ns->selectedBox->locked();
	}
	return false;
}
void gtk_layout_preview_set_transformation_chain(GtkLayoutPreview *widget, transformation::Chain *chain) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	ns->transformationChain = chain;
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
void gtk_layout_preview_set_fill(GtkLayoutPreview *widget, bool fill) {
	GtkLayoutPreviewPrivate *ns = GET_PRIVATE(widget);
	ns->fill = fill;
	gtk_widget_queue_draw(GTK_WIDGET(widget));
}
