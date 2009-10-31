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

#include "LayoutPreview.h"
#include "../layout/Box.h"
#include "../Rect2.h"

#include <list>
using namespace std;
using namespace math;
using namespace layout;

#define GTK_LAYOUT_PREVEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_LAYOUT_PREVIEW, GtkLayoutPreviewPrivate))

G_DEFINE_TYPE (GtkLayoutPreview, gtk_layout_preview, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_layout_preview_expose(GtkWidget *layout_preview, GdkEventExpose *event);
static gboolean gtk_layout_preview_button_release(GtkWidget *layout_preview, GdkEventButton *event);
static gboolean gtk_layout_preview_button_press(GtkWidget *layout_preview, GdkEventButton *event);


enum {
	EMPTY,
	LAST_SIGNAL
};

static guint gtk_layout_preview_signals[LAST_SIGNAL] = { 0 };

typedef struct GtkLayoutPreviewPrivate GtkLayoutPreviewPrivate;

typedef struct GtkLayoutPreviewPrivate{
	Box* box;
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

}

static void gtk_layout_preview_init(GtkLayoutPreview *layout_preview){
	gtk_widget_add_events(GTK_WIDGET(layout_preview), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}

GtkWidget* gtk_layout_preview_new(void){
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_LAYOUT_PREVIEW, NULL);
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);

	
	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	return widget;
}

static void cairo_rounded_rectangle(cairo_t *cr, double x, double y, double width, double height, double roundness){
	
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

static gboolean gtk_layout_preview_expose(GtkWidget *widget, GdkEventExpose *event){
	
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	cairo_t *cr;
	cr = gdk_cairo_create(widget->window);
	
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);
	
	if (ns->box){
		Rect2<float> area = Rect2<float>(0, 0, 640, 480);
		
		ns->box->Draw(cr, area);	
		
	}
	
	//cairo_set_source_rgb(cr, 0, ns->color[0].rgb.green, ns->color[0].rgb.blue);
	//cairo_rounded_rectangle(cr, 0, 0, 400, 400, 10);
	//cairo_fill(cr);
	
	cairo_destroy(cr);
	
	return true;
}

static gboolean gtk_layout_preview_button_release(GtkWidget *layout_preview, GdkEventButton *event){
	return true;
}

static gboolean gtk_layout_preview_button_press(GtkWidget *layout_preview, GdkEventButton *event){
	return true;
}

int gtk_layout_preview_set_box(GtkLayoutPreview* widget, layout::Box* box){
	GtkLayoutPreviewPrivate *ns = GTK_LAYOUT_PREVEW_GET_PRIVATE(widget);
	
	ns->box = box;
	return 0;
}
