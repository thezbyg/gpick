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

#include "LayoutPreview.h"
#include "../layout/System.h"
#include "../Rect2.h"

#include <list>
#include <typeinfo>
using namespace std;
using namespace math;
using namespace layout;

#define GTK_LAYOUT_PREVEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_LAYOUT_PREVIEW, GtkLayoutPreviewPrivate))

G_DEFINE_TYPE (GtkLayoutPreview, gtk_layout_preview, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_layout_preview_expose(GtkWidget *layout_preview, GdkEventExpose *event);
static gboolean gtk_layout_preview_button_release(GtkWidget *layout_preview, GdkEventButton *event);
static gboolean gtk_layout_preview_button_press(GtkWidget *layout_preview, GdkEventButton *event);


enum {
	COLOR_CHANGED,
	EMPTY,
	LAST_SIGNAL
};

static guint gtk_layout_preview_signals[LAST_SIGNAL] = { 0 };

typedef struct GtkLayoutPreviewPrivate GtkLayoutPreviewPrivate;

typedef struct GtkLayoutPreviewPrivate{
	System* system;
	Rect2<float> area;
	Style* selected_style;
	Box* selected_box;
}GtkLayoutPreviewPrivate;


static void gtk_layout_preview_class_init(GtkLayoutPreviewClass *klass){
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_layout_preview_expose;
	widget_class->button_release_event = gtk_layout_preview_button_release;
	widget_class->button_press_event = gtk_layout_preview_button_press;

	g_type_class_add_private(obj_class, sizeof(GtkLayoutPreviewPrivate));

	gtk_layout_preview_signals[COLOR_CHANGED] = g_signal_new("color_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkLayoutPreviewClass, color_changed), NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void gtk_layout_preview_init(GtkLayoutPreview *layout_preview){
	gtk_widget_add_events(GTK_WIDGET(layout_preview), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}

static void gtk_layout_preview_destroy(GtkLayoutPreview *widget){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (ns->system) System::unref(ns->system);
}

GtkWidget* gtk_layout_preview_new(void){
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_LAYOUT_PREVIEW, NULL);
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);

	ns->area = Rect2<float>(0, 0, 1, 1);
	ns->selected_style = 0;
	ns->selected_box = 0;
	ns->system = 0;

	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK (gtk_layout_preview_destroy), NULL);

	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	return widget;
}



static bool set_selected_box(GtkLayoutPreviewPrivate *ns, Box* box){
	bool changed = false;

	if (box && box->style){
		if (ns->selected_style) ns->selected_style->SetState(false, 0);
		ns->selected_style = box->style;
		ns->selected_style->SetState(true, box);
		if (ns->selected_box != box) changed=true;

		ns->selected_box = box;
	}else{
		if (ns->selected_style){
			ns->selected_style->SetState(false, 0);
			changed = true;
		}
		ns->selected_style = 0;
		ns->selected_box = 0;
	}
	return changed;
}

static gboolean gtk_layout_preview_expose(GtkWidget *widget, GdkEventExpose *event){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);

	cairo_t *cr;
	cr = gdk_cairo_create(widget->window);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);


	if (ns->system && ns->system->box){
		ns->area = Rect2<float>(0, 0, 1, 1);
		ns->system->box->Draw(cr, ns->area);
	}

	cairo_destroy(cr);

	return true;
}

static gboolean gtk_layout_preview_button_release(GtkWidget *layout_preview, GdkEventButton *event){
	return true;
}

static gboolean gtk_layout_preview_button_press(GtkWidget *widget, GdkEventButton *event){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);

	if (ns->system){

		Vec2<float> point = Vec2<float>((event->x-ns->area.getX()) / ns->area.getWidth(), (event->y-ns->area.getY()) / ns->area.getHeight());

		if (set_selected_box(ns, ns->system->GetBoxAt(point))){
			gtk_widget_queue_draw(GTK_WIDGET(widget));
		}
	}

	return false;
}

int gtk_layout_preview_set_system(GtkLayoutPreview* widget, System* system){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (ns->system){
		System::unref(ns->system);
	}
	if (system){
		ns->system = static_cast<System*>(system->ref());
		if (ns->system->box)
			gtk_widget_set_size_request(GTK_WIDGET(widget), ns->system->box->rect.getWidth(), ns->system->box->rect.getHeight());
	}else ns->system = 0;
	return 0;
}



int gtk_layout_preview_set_color_at(GtkLayoutPreview* widget, Color* color, gdouble x, gdouble y){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (!ns->system) return -1;

	Vec2<float> point = Vec2<float>((x-ns->area.getX()) / ns->area.getWidth(), (y-ns->area.getY()) / ns->area.getHeight());
	Box* box = ns->system->GetBoxAt(point);
	if (box && box->style && !box->locked){
		color_copy(color, &box->style->color);

		/*if (typeid(*box)==typeid(Fill)){
			color_copy(color, &box->style->background_color);
		}else if (typeid(*box)==typeid(Text)){
			color_copy(color, &box->style->text_color);
		}*/
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return 0;
	}

	return -1;
}

int gtk_layout_preview_set_color_named(GtkLayoutPreview* widget, Color* color, const char *name){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (!ns->system) return -1;

	Box* box = ns->system->GetNamedBox(name);
	if (box && box->style && !box->locked){
		color_copy(color, &box->style->color);
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return 0;
	}

	return -1;
}

int gtk_layout_preview_set_focus_named(GtkLayoutPreview* widget, const char *name){

	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (!ns->system) return -1;

	Box* box;
	if (set_selected_box(ns, box = ns->system->GetNamedBox(name))){
		gtk_widget_queue_draw(GTK_WIDGET(widget));

		return (box)?(0):(-1);
	}
	return -1;
}


int gtk_layout_preview_set_focus_at(GtkLayoutPreview* widget, gdouble x, gdouble y){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (!ns->system) return -1;

	Vec2<float> point = Vec2<float>((x-ns->area.getX()) / ns->area.getWidth(), (y-ns->area.getY()) / ns->area.getHeight());
	Box* box;
	if (set_selected_box(ns, box = ns->system->GetBoxAt(point))){
		gtk_widget_queue_draw(GTK_WIDGET(widget));

		return (box)?(0):(-1);
	}
	return -1;
}

int gtk_layout_preview_get_current_color(GtkLayoutPreview* widget, Color* color){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);

	if (ns->system && ns->selected_style){
		Box* box = ns->selected_box;

		color_copy(&box->style->color, color);

		/*if (typeid(*box)==typeid(Fill)){
			color_copy(&box->style->background_color, color);
		}else if (typeid(*box)==typeid(Text)){
			color_copy(&box->style->text_color, color);
		}*/

		return 0;

	}
	return -1;
}


int gtk_layout_preview_set_current_color(GtkLayoutPreview* widget, Color* color){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);

	if (ns->system && ns->selected_style && !ns->selected_box->locked){
		Box* box = ns->selected_box;

		color_copy(color, &box->style->color);

		/*if (typeid(*box)==typeid(Fill)){
			color_copy(color, &box->style->background_color);
		}else if (typeid(*box)==typeid(Text)){
			color_copy(color, &box->style->text_color);
		}*/
		gtk_widget_queue_draw(GTK_WIDGET(widget));
		return 0;
	}
	return -1;
}

bool gtk_layout_preview_is_selected(GtkLayoutPreview* widget){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (ns->system && ns->selected_style && ns->selected_box){
		return true;
	}
	return false;
}

bool gtk_layout_preview_is_editable(GtkLayoutPreview* widget){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	if (ns->system && ns->selected_style && ns->selected_box){
		return !ns->selected_box->locked;
	}
	return false;
}

