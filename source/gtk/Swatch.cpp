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

#include "Swatch.h"
#include "../Color.h"
#include "../MathUtil.h"
#include <math.h>

#define GTK_SWATCH_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_SWATCH, GtkSwatchPrivate))

G_DEFINE_TYPE (GtkSwatch, gtk_swatch, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_swatch_expose(GtkWidget *swatch, GdkEventExpose *event);

enum {
	ACTIVE_COLOR_CHANGED, COLOR_CHANGED, COLOR_ACTIVATED, LAST_SIGNAL
};

static guint gtk_swatch_signals[LAST_SIGNAL] = { 0 };

static gboolean gtk_swatch_button_release(GtkWidget *swatch, GdkEventButton *event);

static gboolean gtk_swatch_button_press(GtkWidget *node_system, GdkEventButton *event);

enum {
	TARGET_STRING,
	TARGET_ROOTWIN,
	TARGET_COLOR,
};

static GtkTargetEntry target_list[] = {
	{ (char*)"application/x-color", 0, TARGET_COLOR },
	{ (char*)"text/plain", 0, TARGET_STRING },
	{ (char*)"STRING",     0, TARGET_STRING },
	{ (char*)"application/x-rootwin-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = G_N_ELEMENTS (target_list);

static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint target_type, guint time, gpointer data);
static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t, gpointer user_data);
static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data);
static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data);

static void drag_data_delete(GtkWidget *widget, GdkDragContext *context, gpointer user_data);
static void drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data);
static void drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data);
static void drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data);


/*
 static gboolean
 gtk_swatch_motion_notify (GtkWidget *node_system, GdkEventMotion *event);
 */

typedef struct GtkSwatchPrivate GtkSwatchPrivate;

typedef struct GtkSwatchPrivate {
	Color color[7];
	gint32 current_color;
} GtkSwatchPrivate;

static void gtk_swatch_class_init(GtkSwatchClass *swatch_class) {
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS(swatch_class);
	widget_class = GTK_WIDGET_CLASS(swatch_class);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_swatch_expose;
	widget_class->button_release_event = gtk_swatch_button_release;
	widget_class->button_press_event = gtk_swatch_button_press;

	/*widget_class->button_press_event = gtk_node_view_button_press;
	 widget_class->button_release_event = gtk_node_view_button_release;
	 widget_class->motion_notify_event = gtk_node_view_motion_notify;*/

	g_type_class_add_private(obj_class, sizeof(GtkSwatchPrivate));

	gtk_swatch_signals[ACTIVE_COLOR_CHANGED] = g_signal_new("active_color_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkSwatchClass, active_color_changed), NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

	gtk_swatch_signals[COLOR_CHANGED] = g_signal_new("color_changed", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkSwatchClass, color_changed), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

	gtk_swatch_signals[COLOR_ACTIVATED] = g_signal_new("color_activated", G_OBJECT_CLASS_TYPE(obj_class), G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkSwatchClass, color_activated), NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gtk_swatch_init(GtkSwatch *swatch) {
	gtk_widget_add_events(GTK_WIDGET(swatch), GDK_2BUTTON_PRESS | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
}

GtkWidget *
gtk_swatch_new(void) {
	GtkWidget* widget = (GtkWidget*) g_object_new(GTK_TYPE_SWATCH, NULL);
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(widget);

	gtk_widget_set_size_request(GTK_WIDGET(widget),150+widget->style->xthickness*2,150+widget->style->ythickness*2);

	for (gint32 i = 0; i < 7; ++i)
		color_set(&ns->color[i], i/7.0);
	ns->current_color = 1;
	
	GTK_WIDGET_SET_FLAGS(widget, GTK_CAN_FOCUS);
	
	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), target_list, n_targets, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, target_list, n_targets,	GDK_ACTION_COPY);
	
	g_signal_connect (widget, "drag-data-received", G_CALLBACK(drag_data_received), NULL);
	g_signal_connect (widget, "drag-leave", G_CALLBACK (drag_leave), NULL);
	g_signal_connect (widget, "drag-motion", G_CALLBACK (drag_motion), NULL);
	g_signal_connect (widget, "drag-drop", G_CALLBACK (drag_drop), NULL);
	
	g_signal_connect (widget, "drag-data-get", G_CALLBACK (drag_data_get), NULL);
	g_signal_connect (widget, "drag-data-delete", G_CALLBACK (drag_data_delete), NULL);
	g_signal_connect (widget, "drag-begin", G_CALLBACK (drag_begin), NULL);
	g_signal_connect (widget, "drag-end", G_CALLBACK (drag_end), NULL);

	return widget;
}

void gtk_swatch_set_color_to_main(GtkSwatch* swatch) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(&ns->color[0], &ns->color[ns->current_color]);
	gtk_widget_queue_draw(GTK_WIDGET(swatch));
}

void gtk_swatch_move_active(GtkSwatch* swatch, gint32 direction) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);

	if (direction < 0) {
		if (ns->current_color == 1) {
			ns->current_color = 7 - 1;
		} else {
			ns->current_color--;
		}
	} else {
		ns->current_color++;

		if (ns->current_color >= 7)
			ns->current_color = 1;

	}

}

void gtk_swatch_get_color(GtkSwatch* swatch, guint32 index, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(&ns->color[index], color);
}

void gtk_swatch_get_main_color(GtkSwatch* swatch, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(&ns->color[0], color);
}

gint32 gtk_swatch_get_active_index(GtkSwatch* swatch) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	return ns->current_color;
}

void gtk_swatch_get_active_color(GtkSwatch* swatch, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(&ns->color[ns->current_color], color);
}

void gtk_swatch_set_color(GtkSwatch* swatch, guint32 index, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(color, &ns->color[index]);

	gtk_widget_queue_draw(GTK_WIDGET(swatch));
}

void gtk_swatch_set_main_color(GtkSwatch* swatch, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(color, &ns->color[0]);

	gtk_widget_queue_draw(GTK_WIDGET(swatch));
}

void gtk_swatch_set_active_index(GtkSwatch* swatch, guint32 index) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	ns->current_color = index;
	//if (ns->current_color<=0) ns->current_color=1;
	gtk_widget_queue_draw(GTK_WIDGET(swatch));
}

void gtk_swatch_set_active_color(GtkSwatch* swatch, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);
	color_copy(color, &ns->color[ns->current_color]);

	gtk_widget_queue_draw(GTK_WIDGET(swatch));
	g_signal_emit(GTK_WIDGET(swatch), gtk_swatch_signals[COLOR_CHANGED], 0);
}

void gtk_swatch_set_main_color(GtkSwatch* swatch, guint index, Color* color) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(swatch);

	color_copy(color, &ns->color[0]);

	gtk_widget_queue_draw(GTK_WIDGET(swatch));
}

static void gtk_swatch_draw_hexagon(cairo_t *cr, float x, float y, float radius) {
	cairo_new_sub_path(cr);
	for (int i = 0; i < 6; ++i) {
		cairo_line_to(cr, x + sin(i * PI / 3) * radius, y + cos(i * PI / 3) * radius);
	}
	cairo_close_path(cr);
}

static gboolean gtk_swatch_expose(GtkWidget *widget, GdkEventExpose *event) {
	
	GtkStateType state;
	
	if (GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_ACTIVE;

	
	cairo_t *cr;

	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(widget);
	
	gtk_paint_shadow(widget->style, widget->window, state, GTK_SHADOW_IN, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);

	cr = gdk_cairo_create(widget->window);

	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 12);

	cairo_matrix_t matrix;
	cairo_get_matrix(cr, &matrix);
	cairo_translate(cr, 75+widget->style->xthickness, 75+widget->style->ythickness);

	int edges = 6;

	cairo_set_source_rgb(cr, 0, 0, 0);

	float radius_multi = 50 * cos((180 / edges) / (180 / PI));
	float rotation = -(PI/6 * 4);

	//Draw stroke
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_width(cr, 3);
	for (int i = 1; i < 7; ++i) {
		if (i == ns->current_color)
			continue;
		gtk_swatch_draw_hexagon(cr, radius_multi * cos(rotation + i * (2 * PI) / edges), radius_multi * sin(rotation + i * (2 * PI) / edges), 27);
	}

	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 1, 1, 1);
	gtk_swatch_draw_hexagon(cr, radius_multi * cos(rotation + (ns->current_color) * (2 * PI) / edges), radius_multi * sin(rotation + (ns->current_color) * (2
			* PI) / edges), 27);
	cairo_stroke(cr);

	//Draw fill
	for (int i = 1; i < 7; ++i) {
		if (i == ns->current_color)
			continue;
		cairo_set_source_rgb(cr, ns->color[i].rgb.red, ns->color[i].rgb.green, ns->color[i].rgb.blue);
		gtk_swatch_draw_hexagon(cr, radius_multi * cos(rotation + i * (2 * PI) / edges), radius_multi * sin(rotation + i * (2 * PI) / edges), 25.5);
		cairo_fill(cr);
	}

	cairo_set_source_rgb(cr, ns->color[ns->current_color].rgb.red, ns->color[ns->current_color].rgb.green, ns->color[ns->current_color].rgb.blue);
	gtk_swatch_draw_hexagon(cr, radius_multi * cos(rotation + (ns->current_color) * (2 * PI) / edges), radius_multi * sin(rotation + (ns->current_color) * (2
			* PI) / edges), 25.5);
	cairo_fill(cr);

	//Draw center
	cairo_set_source_rgb(cr, ns->color[0].rgb.red, ns->color[0].rgb.green, ns->color[0].rgb.blue);
	gtk_swatch_draw_hexagon(cr, 0, 0, 25.5);
	cairo_fill(cr);

	//Draw numbers
	char numb[2] = " ";
	for (int i = 1; i < 7; ++i) {
		Color c;
		color_get_contrasting(&ns->color[i], &c);

		cairo_text_extents_t extends;
		numb[0] = '0' + i;
		cairo_text_extents(cr, numb, &extends);
		cairo_set_source_rgb(cr, c.rgb.red, c.rgb.green, c.rgb.blue);
		cairo_move_to(cr, radius_multi * cos(rotation + i * (2 * PI) / edges) - extends.width / 2, radius_multi * sin(rotation + i * (2 * PI) / edges)
				+ extends.height / 2);
		cairo_show_text(cr, numb);
	}

	cairo_set_matrix(cr, &matrix);

	cairo_destroy(cr);
	
	if (GTK_WIDGET_HAS_FOCUS(widget)){
		gtk_paint_focus(widget->style, widget->window, state, &event->area, widget, 0, widget->style->xthickness, widget->style->ythickness, 150, 150);
	}

	return FALSE;
}

static int swatch_get_color_by_position(gint x, gint y){
	vector2 a, b;
	vector2_set(&a, 1, 0);
	vector2_set(&b, x - 75, y - 75);
	float distance = vector2_length(&b);
	
	if (distance<20){			//center color
		return 0;
	}else if (distance>70){		//outside
		return -1;			
	}else{
		vector2_normalize(&b, &b);

		float angle = acos(vector2_dot(&a, &b));
		if (b.y < 0)
			angle = 2 * PI - angle;
		angle += (PI/6) * 3;

		if (angle < 0)
			angle += PI * 2;
		if (angle > 2 * PI)
			angle -= PI * 2;
		
		return 1 + (int) floor(angle / ((PI*2) / 6));
	}
}

static gboolean gtk_swatch_button_press(GtkWidget *widget, GdkEventButton *event) {
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(widget);
	
	int new_color = swatch_get_color_by_position(event->x - widget->style->xthickness, event->y - widget->style->ythickness);

	gtk_widget_grab_focus(widget);

	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
		if (new_color>0){
			g_signal_emit(widget, gtk_swatch_signals[COLOR_ACTIVATED], 0);
		}
	}else if ((event->type == GDK_BUTTON_PRESS) && ((event->button == 1) || (event->button == 3))) {
		if (new_color==0){
			gdk_pointer_grab(widget->window, FALSE, GdkEventMask(GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK	), NULL, NULL, event->time);
		}else if (new_color<0){
			g_signal_emit(widget, gtk_swatch_signals[ACTIVE_COLOR_CHANGED], 0, ns->current_color);
		}else{
			if (new_color != ns->current_color){
				ns->current_color = new_color;

				g_signal_emit(widget, gtk_swatch_signals[ACTIVE_COLOR_CHANGED], 0, ns->current_color);
				
				gtk_widget_queue_draw(GTK_WIDGET(widget));
			}
		}
	}
	return FALSE;
}

static gboolean gtk_swatch_button_release(GtkWidget *widget, GdkEventButton *event) {
	//GtkSwatchPrivate *ns=GTK_SWATCH_GET_PRIVATE(widget);

	if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 1)) {
		//gtk_swatch_set_color_to_main(GTK_SWATCH(widget));

	}else if ((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
		g_signal_emit_by_name(widget, "popup-menu");
	}
	gdk_pointer_ungrab(event->time);
	return FALSE;
}

static int hex2dec(char h){
	if (h>='0' && h<='9'){
		return h-'0';
	}else if (h>='a' && h<='f'){
		return h-'a'+10;
	}else if (h>='A' && h<='F'){
		return h-'A'+10;
	}
	return -1;
}

static void parse_hex6digit(char* str, Color* c){

	int red = hex2dec(str[1])<<4 | hex2dec(str[2]);
	int green = hex2dec(str[3])<<4 | hex2dec(str[4]);
	int blue = hex2dec(str[5])<<4 | hex2dec(str[6]);
	
	c->rgb.red = red/255.0;
	c->rgb.green = green/255.0;
	c->rgb.blue = blue/255.0;	
}

static void parse_hex3digit(char* str, Color* c){

	int red = hex2dec(str[1]);
	int green = hex2dec(str[2]);
	int blue = hex2dec(str[3]);
	
	c->rgb.red = red/15.0;
	c->rgb.green = green/15.0;
	c->rgb.blue = blue/15.0;	
}

static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint target_type, guint time, gpointer data){
	bool success = false;

	if ((selection_data != NULL) && (selection_data->length >= 0)){
		
		int new_color = swatch_get_color_by_position(x - widget->style->xthickness, y - widget->style->ythickness);
		if (new_color>0) {

			context->action = GDK_ACTION_COPY;

			switch (target_type){
			case TARGET_STRING:
				{
					gchar* data = (gchar*)selection_data->data;
					if (data[selection_data->length]!=0) break;	//not null terminated
					
					Color color;
					GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(widget);
					
					if (g_regex_match_simple("^#[\\dabcdef]{6}$", data, GRegexCompileFlags(G_REGEX_MULTILINE | G_REGEX_CASELESS), G_REGEX_MATCH_NOTEMPTY)){
						parse_hex6digit( data, &color);					
						color_copy(&color, &ns->color[new_color]);
						gtk_widget_queue_draw(widget);					
					}else if (g_regex_match_simple("^#[\\dabcdef]{3}$", data, GRegexCompileFlags(G_REGEX_MULTILINE | G_REGEX_CASELESS), G_REGEX_MATCH_NOTEMPTY)){
						parse_hex3digit( data, &color);
						color_copy(&color, &ns->color[new_color]);
						gtk_widget_queue_draw(widget);	
					}
				}
				success = true;
				break;
				
			case TARGET_COLOR:
				{
					guint16* data = (guint16*)selection_data->data;

					Color color;
					GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(widget);

					color.rgb.red = data[0] / (double)0xFFFF;
					color.rgb.green = data[1] / (double)0xFFFF;				
					color.rgb.blue = data[2] / (double)0xFFFF;

					
					color_copy(&color, &ns->color[new_color]);
					gtk_widget_queue_draw(widget);	
				}
				success = true;
				break;	
				
			default:
				g_assert_not_reached ();
			}
		}
	}
	gtk_drag_finish (context, success, false, time);
}


static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t, gpointer user_data){
	return  FALSE;
}


static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data){
}


static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data){
	
	GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
	
	if (target != GDK_NONE){
		gtk_drag_get_data(widget, context, target, time);
		return TRUE;
	}
	return FALSE;
}




static void drag_data_delete(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
}

static void drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data){

	g_assert (selection_data != NULL);
	
	GtkSwatchPrivate *ns = GTK_SWATCH_GET_PRIVATE(widget);
	Color color;
	color_copy(&ns->color[ns->current_color], &color);
	
	switch (target_type){
	case TARGET_STRING:
		{
			char text[8];
			snprintf(text, 8, "#%02x%02x%02x", int(color.rgb.red*255), int(color.rgb.green*255), int(color.rgb.blue*255));
			gtk_selection_data_set_text(selection_data, text, 8);
		}
		break;
		
	case TARGET_COLOR:
		{
			guint16 data_color[4];

			data_color[0] = int(color.rgb.red * 0xFFFF);
			data_color[1] = int(color.rgb.green * 0xFFFF);
			data_color[2] = int(color.rgb.blue * 0xFFFF);
			data_color[3] = 0xffff;
			
			gtk_selection_data_set (selection_data, gdk_atom_intern ("application/x-color", TRUE), 16, (guchar *)data_color, 8);
		}
		break;

	case TARGET_ROOTWIN:
		g_print ("Dropped on the root window!\n");
		break;

	default:
		g_assert_not_reached ();
	}
}

static void drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
}

static void drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
}

