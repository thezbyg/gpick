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

#include "uiColorComponent.h"
#include "uiUtilities.h"

#include "Color.h"
#include "MathUtil.h"
#include <math.h>
#include <string.h>

#define GTK_COLOR_COMPONENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COLOR_COMPONENT, GtkColorComponentPrivate))

G_DEFINE_TYPE (GtkColorComponent, gtk_color_component, GTK_TYPE_DRAWING_AREA);

static gboolean
gtk_color_component_expose (GtkWidget *widget, GdkEventExpose *event);

static gboolean
gtk_color_component_button_release (GtkWidget *widget, GdkEventButton *event);

static gboolean
gtk_color_component_button_press (GtkWidget *node_system, GdkEventButton *event);

static gboolean
gtk_color_component_motion_notify (GtkWidget *node_system, GdkEventMotion *event);

static void
gtk_color_component_emit_color_change(GtkWidget *widget, gfloat new_value);


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

static guint gtk_color_component_signals[LAST_SIGNAL] = { 0 };


typedef struct GtkColorComponentPrivate GtkColorComponentPrivate;

typedef struct GtkColorComponentPrivate
{
	Color color;
	GtkColorComponentComp component;

	gfloat value;

}GtkColorComponentPrivate;

static void
gtk_color_component_class_init (GtkColorComponentClass *color_component_class)
{
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS (color_component_class);
	widget_class = GTK_WIDGET_CLASS (color_component_class);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_color_component_expose;
	widget_class->button_release_event = gtk_color_component_button_release;
	widget_class->button_press_event = gtk_color_component_button_press;
	widget_class->motion_notify_event = gtk_color_component_motion_notify;

	g_type_class_add_private (obj_class, sizeof (GtkColorComponentPrivate));


	gtk_color_component_signals[COLOR_CHANGED] = g_signal_new (
	     "color-changed",
	     G_OBJECT_CLASS_TYPE (obj_class),
	     G_SIGNAL_RUN_FIRST,
	     G_STRUCT_OFFSET (GtkColorComponentClass, color_changed),
	     NULL, NULL,
	     g_cclosure_marshal_VOID__POINTER,
	     G_TYPE_NONE, 1,
	     G_TYPE_POINTER);
}

static void
gtk_color_component_init (GtkColorComponent *color_component)
{
	gtk_widget_add_events (GTK_WIDGET (color_component),
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
}





GtkWidget *
gtk_color_component_new (GtkColorComponentComp component)
{
	GtkWidget* widget=(GtkWidget*)g_object_new (GTK_TYPE_COLOR_COMPONENT, NULL);
	GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	ns->component=component;

	gtk_widget_set_size_request(GTK_WIDGET(widget),200,24);

	return widget;
}

void
gtk_color_component_set_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(color, &ns->color);

	Color c1;

	switch (ns->component) {
		case red:
			ns->value=ns->color.rgb.red;
			break;
		case green:
			ns->value=ns->color.rgb.green;
			break;
		case blue:
			ns->value=ns->color.rgb.blue;
			break;
		case hue:
			color_rgb_to_hsv(&ns->color, &c1);
			ns->value=c1.hsv.hue;
			break;
		case saturation:
			color_rgb_to_hsv(&ns->color, &c1);
			ns->value=c1.hsv.saturation;
			break;
		case value:
			color_rgb_to_hsv(&ns->color, &c1);
			ns->value=c1.hsv.value;
			break;

	}


	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}



static gboolean
gtk_color_component_expose (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;

	GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	/* get a cairo_t */
	cr = gdk_cairo_create (widget->window);

	cairo_rectangle (cr,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	cairo_clip (cr);

	cairo_pattern_t* pattern=cairo_pattern_create_linear(0,0,200,0);

	Color c1,c2;

	float steps;
	int i;

	float pointer_pos=ns->value;

	switch (ns->component) {
		case red:
			steps=1;
			color_copy(&ns->color, &c1);
			//pointer_pos=c1.rgb.red;
			for (i=0;i<=steps;++i){
				c1.rgb.red=i/steps;
				cairo_pattern_add_color_stop_rgb(pattern, i/steps, c1.rgb.red,c1.rgb.green,c1.rgb.blue);
			}
			break;
		case green:
			steps=1;
			color_copy(&ns->color, &c1);
			//pointer_pos=c1.rgb.green;
			for (i=0;i<=steps;++i){
				c1.rgb.green=i/steps;
				cairo_pattern_add_color_stop_rgb(pattern, i/steps, c1.rgb.red,c1.rgb.green,c1.rgb.blue);
			}
			break;
		case blue:
			steps=1;
			color_copy(&ns->color, &c1);
			//pointer_pos=c1.rgb.blue;
			for (i=0;i<=steps;++i){
				c1.rgb.blue=i/steps;
				cairo_pattern_add_color_stop_rgb(pattern, i/steps, c1.rgb.red,c1.rgb.green,c1.rgb.blue);
			}
			break;

		case hue:
			steps=200;
			color_rgb_to_hsv(&ns->color, &c1);
			//pointer_pos=c1.hsv.hue;
			for (i=0;i<=steps;++i){
				c1.hsv.hue=i/steps;
				color_hsv_to_rgb(&c1, &c2);
				cairo_pattern_add_color_stop_rgb(pattern, i/steps, c2.rgb.red,c2.rgb.green,c2.rgb.blue);
			}
			break;

		case saturation:
			steps=100;
			color_rgb_to_hsv(&ns->color, &c1);
			//pointer_pos=c1.hsv.saturation;
			for (i=0;i<=steps;++i){
				c1.hsv.saturation=i/steps;
				color_hsv_to_rgb(&c1, &c2);
				cairo_pattern_add_color_stop_rgb(pattern, i/steps, c2.rgb.red,c2.rgb.green,c2.rgb.blue);
			}
			break;
		case value:
			steps=100;
			color_rgb_to_hsv(&ns->color, &c1);
			//pointer_pos=c1.hsv.value;
			for (i=0;i<=steps;++i){
				c1.hsv.value=i/steps;
				color_hsv_to_rgb(&c1, &c2);
				cairo_pattern_add_color_stop_rgb(pattern, i/steps, c2.rgb.red,c2.rgb.green,c2.rgb.blue);
			}
			break;
		default:
			break;
	}

	//cairo_new_path(cr);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr, 0, 0, 200, 24);
	cairo_fill (cr);
	cairo_pattern_destroy(pattern);

	//cairo_new_path(cr);
	cairo_move_to(cr, 200*pointer_pos, 17);
	cairo_line_to(cr, 200*pointer_pos+3, 25);
	cairo_line_to(cr, 200*pointer_pos-3, 25);
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_set_line_width(cr,1);
	cairo_stroke(cr);


	cairo_destroy (cr);

	return TRUE;
}

static void
gtk_color_component_popup_menu_detach(GtkWidget *attach_widget, GtkMenu *menu)
{
	gtk_widget_destroy(GTK_WIDGET(menu));
}

static void
gtk_color_component_popup_copy_1(GtkWidget *widget,  gpointer item)
{
	GtkColorComponentPrivate *ns=(GtkColorComponentPrivate*)item;
	gchar* tmp=g_strdup_printf("%f",ns->value);
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), tmp, strlen(tmp));
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    g_free(tmp);
}

static void
gtk_color_component_popup_copy_100(GtkWidget *widget,  gpointer item)
{
	GtkColorComponentPrivate *ns=(GtkColorComponentPrivate*)item;
	gchar* tmp=g_strdup_printf("%d",gint32(ns->value*100));
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), tmp, strlen(tmp));
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    g_free(tmp);
}

static void
gtk_color_component_popup_copy_255(GtkWidget *widget,  gpointer item)
{
	GtkColorComponentPrivate *ns=(GtkColorComponentPrivate*)item;
	gchar* tmp=g_strdup_printf("%d",gint32(ns->value*255));
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), tmp, strlen(tmp));
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    g_free(tmp);
}

static void
gtk_color_component_popup_copy_360(GtkWidget *widget,  gpointer item)
{
	GtkColorComponentPrivate *ns=(GtkColorComponentPrivate*)item;
	gchar* tmp=g_strdup_printf("%d",gint32(ns->value*360));
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), tmp, strlen(tmp));
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    g_free(tmp);
}

static void
gtk_color_component_popup_copy_FF(GtkWidget *widget,  gpointer item)
{
	GtkColorComponentPrivate *ns=(GtkColorComponentPrivate*)item;
	gchar* tmp=g_strdup_printf("%02X",gint32(ns->value*255));
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), tmp, strlen(tmp));
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    g_free(tmp);
}


static void
gtk_color_component_popup_paste_1(GtkWidget *widget,  gpointer item)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(item);

	gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
		gfloat value;
		sscanf(text,"%f",&value);
		gtk_color_component_emit_color_change(GTK_WIDGET(item), value);
	}
}
static void
gtk_color_component_popup_paste_100(GtkWidget *widget,  gpointer item)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(item);

	gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
		gfloat value;
		sscanf(text,"%f",&value);
		gtk_color_component_emit_color_change(GTK_WIDGET(item), value/100.0);
	}
}
static void
gtk_color_component_popup_paste_255(GtkWidget *widget,  gpointer item)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(item);

	gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
		gfloat value;
		sscanf(text,"%f",&value);
		gtk_color_component_emit_color_change(GTK_WIDGET(item), value/255.0);
	}
}
static void
gtk_color_component_popup_paste_360(GtkWidget *widget,  gpointer item)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(item);

	gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
		gfloat value;
		sscanf(text,"%f",&value);
		gtk_color_component_emit_color_change(GTK_WIDGET(item), value/360.0);
	}
}
static void
gtk_color_component_popup_paste_FF(GtkWidget *widget,  gpointer item)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(item);

	gchar *text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
	if (text){
		gint32 value;
		sscanf(text,"%x",&value);
		gtk_color_component_emit_color_change(GTK_WIDGET(item), value/255.0);
	}
}




static gboolean
gtk_color_component_popup_show (GtkWidget *widget, GdkEventButton* event, gpointer ptr){
	GtkWidget *menu;
	GtkWidget* item ;
	gint32 button, event_time;

	menu = gtk_menu_new ();

	GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

    item = gtk_menu_item_new_with_image ("Copy [0..1]", gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_copy_1),ptr);

    item = gtk_menu_item_new_with_image ("Copy [0..100]", gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_copy_100),ptr);

    item = gtk_menu_item_new_with_image ("Copy [0..255]", gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_copy_255),ptr);

    if ( ns->component == hue ){
		item = gtk_menu_item_new_with_image ("Copy [0..360]", gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_copy_360),ptr);
    }

    item = gtk_menu_item_new_with_image ("Copy Hexagonal [00..FF]", gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_copy_FF),ptr);

    item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gboolean paste_enabled = gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));

    item = gtk_menu_item_new_with_image ("Paste [0..1]", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_set_sensitive(item, paste_enabled);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_paste_1),widget);

    item = gtk_menu_item_new_with_image ("Paste [0..100]", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_set_sensitive(item, paste_enabled);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_paste_100),widget);

    item = gtk_menu_item_new_with_image ("Paste [0..255]", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_set_sensitive(item, paste_enabled);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_paste_255),widget);

    if ( ns->component == hue ){
		item = gtk_menu_item_new_with_image ("Paste [0..360]", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		gtk_widget_set_sensitive(item, paste_enabled);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_paste_360),widget);
    }

    item = gtk_menu_item_new_with_image ("Paste Hexagonal [00..FF]", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_set_sensitive(item, paste_enabled);
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gtk_color_component_popup_paste_FF),widget);


    gtk_widget_show_all (GTK_WIDGET(menu));

	if (event){
		button = event->button;
		event_time = event->time;
	}else{
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_attach_to_widget (GTK_MENU (menu), widget, gtk_color_component_popup_menu_detach);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  button, event_time);

	return 1;
}

static void
gtk_color_component_emit_color_change(GtkWidget *widget, gfloat new_value)
{
	GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	Color c,c2;
	color_copy(&ns->color, &c);

	switch (ns->component) {
		case red:
			c.rgb.red=new_value;
			break;
		case green:
			c.rgb.green=new_value;
			break;
		case blue:
			c.rgb.blue=new_value;
			break;

		case hue:
			color_rgb_to_hsv(&c, &c2);
			c2.hsv.hue=new_value;
			color_hsv_to_rgb(&c2, &c);
			break;
		case saturation:
			color_rgb_to_hsv(&c, &c2);
			c2.hsv.saturation=new_value;
			color_hsv_to_rgb(&c2, &c);
			break;
		case value:
			color_rgb_to_hsv(&c, &c2);
			c2.hsv.value=new_value;
			color_hsv_to_rgb(&c2, &c);
			break;
	}

	g_signal_emit (widget, gtk_color_component_signals[COLOR_CHANGED],0,&c);
}

static gboolean
gtk_color_component_button_release (GtkWidget *widget, GdkEventButton *event)
{
	GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if ((event->type==GDK_BUTTON_RELEASE)&&(event->button==3)){
		gtk_color_component_popup_show(widget, event, ns);
		return TRUE;
	}
	return FALSE;
}

static gboolean
gtk_color_component_button_press (GtkWidget *widget, GdkEventButton *event)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if ((event->type==GDK_BUTTON_PRESS)&&(event->button==1)){
		gfloat value;

		value=event->x/200.0;
		if (value<0) value=0;
		if (value>1) value=1;

		gtk_color_component_emit_color_change(widget, value);

		//g_signal_emit (widget, gtk_color_component_signals[COLOR_CHANGED],0,value);

		//gtk_widget_queue_draw(widget);
		return TRUE;
	}
	return FALSE;
}

static gboolean
gtk_color_component_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	//GtkColorComponentPrivate *ns=GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if ((event->state&GDK_BUTTON1_MASK)){

		gfloat value;

		value=event->x/200.0;
		if (value<0) value=0;
		if (value>1) value=1;

		gtk_color_component_emit_color_change(widget, value);

		//g_signal_emit (widget, gtk_color_component_signals[VALUE_CHANGED],0,value);

		//gtk_widget_queue_draw(widget);
		return TRUE;

	}

	return FALSE;


}
