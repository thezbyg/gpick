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

#include "ColorComponent.h"
#include "../uiUtilities.h"

#include "../Color.h"
#include "../MathUtil.h"
#include <math.h>
#include <string.h>

#include <iostream>
using namespace std;

#define GTK_COLOR_COMPONENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_TYPE_COLOR_COMPONENT, GtkColorComponentPrivate))

G_DEFINE_TYPE (GtkColorComponent, gtk_color_component, GTK_TYPE_DRAWING_AREA);

typedef struct GtkColorComponentPrivate GtkColorComponentPrivate;

static gboolean gtk_color_component_expose (GtkWidget *widget, GdkEventExpose *event);
static gboolean gtk_color_component_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_color_component_button_press (GtkWidget *node_system, GdkEventButton *event);
static gboolean gtk_color_component_motion_notify (GtkWidget *node_system, GdkEventMotion *event);
static void update_rgb_color(GtkColorComponentPrivate *ns, Color *c);

enum{
  COLOR_CHANGED,
  LAST_SIGNAL
};

static guint gtk_color_component_signals[LAST_SIGNAL] = { 0 };


typedef struct GtkColorComponentPrivate{
	Color orig_color;
	Color color;
	GtkColorComponentComp component;
	int n_components;
	int capture_on;

	double range[4];
	double offset[4];
}GtkColorComponentPrivate;

static void gtk_color_component_class_init (GtkColorComponentClass *color_component_class){
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS (color_component_class);
	widget_class = GTK_WIDGET_CLASS (color_component_class);

	/* GtkWidget signals */

	widget_class->expose_event = gtk_color_component_expose;
	widget_class->button_release_event = gtk_color_component_button_release;
	widget_class->button_press_event = gtk_color_component_button_press;
	widget_class->motion_notify_event = gtk_color_component_motion_notify;

	g_type_class_add_private(obj_class, sizeof(GtkColorComponentPrivate));


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

static void gtk_color_component_init (GtkColorComponent *color_component){
	gtk_widget_add_events (GTK_WIDGET (color_component),
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
}


GtkWidget *gtk_color_component_new (GtkColorComponentComp component){
	GtkWidget* widget = (GtkWidget*)g_object_new(GTK_TYPE_COLOR_COMPONENT, NULL);
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	ns->component = component;

	switch (component){
		case lab:
			ns->n_components = 3;
			ns->range[0] = 100;
			ns->offset[0] = 0;
			ns->range[1] = ns->range[2] = 290;
			ns->offset[1] = ns->offset[2] = -145;
			break;

		case xyz:

			break;

		case cmyk:
			ns->n_components = 4;
			ns->range[0] = ns->range[1] = ns->range[2] = ns->range[3] = 1;
			ns->offset[0] = ns->offset[1] = ns->offset[2] = ns->offset[3] = 0;
			break;

		default:
			ns->n_components = 3;
			ns->range[0] = ns->range[1] = ns->range[2] = ns->range[3] = 1;
			ns->offset[0] = ns->offset[1] = ns->offset[2] = ns->offset[3] = 0;
	}

	gtk_widget_set_size_request(GTK_WIDGET(widget), 200, 16 * ns->n_components);

	return widget;
}

void gtk_color_component_get_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(&ns->orig_color, color);
}

void gtk_color_component_get_transformed_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(&ns->color, color);
}

void gtk_color_component_set_transformed_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(color, &ns->color);
	update_rgb_color(ns, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

int gtk_color_component_get_component_id_at(GtkColorComponent* color_component, gint x, gint y){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);

	int component = y / 16;
	if (component < 0) component = 0;
	else if (component >= ns->n_components) component = ns->n_components - 1;

	return component;
}

void gtk_color_component_set_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(color, &ns->orig_color);

	Color c1;

	switch (ns->component){
		case rgb:
			color_copy(&ns->orig_color, &ns->color);
			break;
		case hsl:
			color_rgb_to_hsl(&ns->orig_color, &ns->color);
			break;
		case hsv:
			color_rgb_to_hsv(&ns->orig_color, &ns->color);
			break;
		case cmyk:
			color_rgb_to_cmyk(&ns->orig_color, &ns->color);
			break;
		case lab:
			matrix3x3 adaptation_matrix, working_space_matrix;
			vector3 d50, d65;
			vector3_set(&d50, 96.442, 100.000,  82.821);
			vector3_set(&d65, 95.047, 100.000, 108.883);
			color_get_chromatic_adaptation_matrix(&d50, &d65, &adaptation_matrix);
			color_get_working_space_matrix(0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, &d65, &working_space_matrix);

			color_rgb_to_xyz(&ns->orig_color, &c1, &working_space_matrix);
			color_xyz_chromatic_adaptation(&c1, &c1, &adaptation_matrix);

			color_xyz_to_lab(&c1, &ns->color, &d50);

			break;
		case xyz:
			/* todo */
			break;
	}


	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

static void interpolate_colors(Color *color1, Color *color2, float position, Color *result){
	result->rgb.red = color1->rgb.red * (1 - position) + color2->rgb.red * position;
	result->rgb.green= color1->rgb.green * (1 - position) + color2->rgb.green * position;
	result->rgb.blue = color1->rgb.blue * (1 - position) + color2->rgb.blue * position;
}

static gboolean gtk_color_component_expose (GtkWidget *widget, GdkEventExpose *event){
	cairo_t *cr;

	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	cr = gdk_cairo_create (widget->window);
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	Color c[4];
	Color c2[4];
	double pointer_pos[4];

	float steps;
	int i, j;

	for (int i = 0; i < ns->n_components; ++i){
		pointer_pos[i] = (ns->color.ma[i] - ns->offset[i]) / ns->range[i];
	}

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 200, ns->n_components * 16);
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	int surface_width = cairo_image_surface_get_width(surface);
	int surface_height = cairo_image_surface_get_height(surface);

	unsigned char *col_ptr;

	Color *rgb_points = new Color[ns->n_components * 200];

	double int_part;

	switch (ns->component) {
		case rgb:
			steps = 1;
			for (i = 0; i < 3; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i < surface_width; ++i){
				c[0].rgb.red = c[1].rgb.green = c[2].rgb.blue = (float)i / (float)(surface_width - 1);
				col_ptr = data + i * 4;

				for (int y = 0; y < ns->n_components * 16; ++y){
          if ((y & 0x0f) != 0x0f){
						col_ptr[2] = (unsigned char)(c[y / 16].rgb.red * 255);
						col_ptr[1] = (unsigned char)(c[y / 16].rgb.green * 255);
						col_ptr[0] = (unsigned char)(c[y / 16].rgb.blue * 255);
						col_ptr[3] = 0xff;
					}else{
						col_ptr[0] = 0x00;
						col_ptr[1] = 0x00;
						col_ptr[2] = 0x00;
						col_ptr[3] = 0x00;
					}
					col_ptr += stride;
				}

			}
			break;

 		case hsv:
			steps = 100;
			for (i = 0; i < 3; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i <= steps; ++i){
				c[0].hsv.hue = c[1].hsv.saturation = c[2].hsv.value = i / steps;

				for (j = 0; j < 3; ++j){
					color_hsv_to_rgb(&c[j], &rgb_points[j * (int(steps) + 1) + i]);
				}
			}
			for (i = 0; i < surface_width; ++i){

				float position = modf(i * steps / surface_width, &int_part);
        int index = i * int(steps) / surface_width;

				interpolate_colors(&rgb_points[0 * (int(steps) + 1) + index], &rgb_points[0 * (int(steps) + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (int(steps) + 1) + index], &rgb_points[1 * (int(steps) + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (int(steps) + 1) + index], &rgb_points[2 * (int(steps) + 1) + index + 1], position, &c[2]);

				col_ptr = data + i * 4;

				for (int y = 0; y < ns->n_components * 16; ++y){
          if ((y & 0x0f) != 0x0f){
						col_ptr[2] = (unsigned char)(c[y / 16].rgb.red * 255);
						col_ptr[1] = (unsigned char)(c[y / 16].rgb.green * 255);
						col_ptr[0] = (unsigned char)(c[y / 16].rgb.blue * 255);
						col_ptr[3] = 0xff;
					}else{
						col_ptr[0] = 0x00;
						col_ptr[1] = 0x00;
						col_ptr[2] = 0x00;
						col_ptr[3] = 0x00;
					}
					col_ptr += stride;
				}
			}
			break;

		case hsl:
			steps = 100;
			for (i = 0; i < 3; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i <= steps; ++i){
				c[0].hsl.hue = c[1].hsl.saturation = c[2].hsl.lightness = i / steps;

				for (j = 0; j < 3; ++j){
					color_hsl_to_rgb(&c[j], &rgb_points[j * (int(steps) + 1) + i]);
				}
			}
			for (i = 0; i < surface_width; ++i){

				float position = modf(i * steps / surface_width, &int_part);
        int index = i * int(steps) / surface_width;

				interpolate_colors(&rgb_points[0 * (int(steps) + 1) + index], &rgb_points[0 * (int(steps) + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (int(steps) + 1) + index], &rgb_points[1 * (int(steps) + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (int(steps) + 1) + index], &rgb_points[2 * (int(steps) + 1) + index + 1], position, &c[2]);

				col_ptr = data + i * 4;

				for (int y = 0; y < ns->n_components * 16; ++y){
          if ((y & 0x0f) != 0x0f){
						col_ptr[2] = (unsigned char)(c[y / 16].rgb.red * 255);
						col_ptr[1] = (unsigned char)(c[y / 16].rgb.green * 255);
						col_ptr[0] = (unsigned char)(c[y / 16].rgb.blue * 255);
						col_ptr[3] = 0xff;
					}else{
						col_ptr[0] = 0x00;
						col_ptr[1] = 0x00;
						col_ptr[2] = 0x00;
						col_ptr[3] = 0x00;
					}
					col_ptr += stride;
				}
			}
			break;

		case cmyk:
			steps = 100;
			for (i = 0; i < 4; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i <= steps; ++i){
				c[0].cmyk.c = c[1].cmyk.m = c[2].cmyk.y = c[3].cmyk.k = i / steps;

				for (j = 0; j < 4; ++j){
					color_cmyk_to_rgb(&c[j], &rgb_points[j * (int(steps) + 1) + i]);
				}
			}
			for (i = 0; i < surface_width; ++i){

				float position = modf(i * steps / surface_width, &int_part);
        int index = i * int(steps) / surface_width;

				interpolate_colors(&rgb_points[0 * (int(steps) + 1) + index], &rgb_points[0 * (int(steps) + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (int(steps) + 1) + index], &rgb_points[1 * (int(steps) + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (int(steps) + 1) + index], &rgb_points[2 * (int(steps) + 1) + index + 1], position, &c[2]);
				interpolate_colors(&rgb_points[3 * (int(steps) + 1) + index], &rgb_points[3 * (int(steps) + 1) + index + 1], position, &c[3]);

				col_ptr = data + i * 4;

				for (int y = 0; y < ns->n_components * 16; ++y){
          if ((y & 0x0f) != 0x0f){
						col_ptr[2] = (unsigned char)(c[y / 16].rgb.red * 255);
						col_ptr[1] = (unsigned char)(c[y / 16].rgb.green * 255);
						col_ptr[0] = (unsigned char)(c[y / 16].rgb.blue * 255);
						col_ptr[3] = 0xff;
					}else{
						col_ptr[0] = 0x00;
						col_ptr[1] = 0x00;
						col_ptr[2] = 0x00;
						col_ptr[3] = 0x00;
					}
					col_ptr += stride;
				}
			}
			break;

		case lab:
			steps = 100;

			matrix3x3 adaptation_matrix, working_space_matrix, working_space_matrix_inv;
			vector3 d50, d65;
			vector3_set(&d50, 96.442, 100.000,  82.821);
			vector3_set(&d65, 95.047, 100.000, 108.883);
			color_get_chromatic_adaptation_matrix(&d65, &d50, &adaptation_matrix);
			color_get_working_space_matrix(0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, &d65, &working_space_matrix);

			matrix3x3_inverse(&working_space_matrix, &working_space_matrix_inv);

			for (i = 0; i < 3; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i <= steps; ++i){
				c[0].lab.L = (i / steps) * ns->range[0] + ns->offset[0];
				color_lab_to_xyz(&c[0], &c2[0], &d50);
				color_xyz_chromatic_adaptation(&c2[0], &c2[0], &adaptation_matrix);
				color_xyz_to_rgb(&c2[0], &rgb_points[0 * (int(steps) + 1) + i], &working_space_matrix_inv);
				color_rgb_normalize(&rgb_points[0 * (int(steps) + 1) + i]);
			}

			for (i = 0; i <= steps; ++i){
				c[1].lab.a = (i / steps) * ns->range[1] + ns->offset[1];
				color_lab_to_xyz(&c[1], &c2[1], &d50);
				color_xyz_chromatic_adaptation(&c2[1], &c2[1], &adaptation_matrix);
				color_xyz_to_rgb(&c2[1], &rgb_points[1 * (int(steps) + 1) + i], &working_space_matrix_inv);
				color_rgb_normalize(&rgb_points[1 * (int(steps) + 1) + i]);
			}
			for (i = 0; i <= steps; ++i){
				c[2].lab.b = (i / steps) * ns->range[2] + ns->offset[2];
				color_lab_to_xyz(&c[2], &c2[2], &d50);
				color_xyz_chromatic_adaptation(&c2[2], &c2[2], &adaptation_matrix);
				color_xyz_to_rgb(&c2[2], &rgb_points[2 * (int(steps) + 1) + i], &working_space_matrix_inv);
				color_rgb_normalize(&rgb_points[2 * (int(steps) + 1) + i]);
			}
			for (i = 0; i < surface_width; ++i){

				float position = modf(i * steps / surface_width, &int_part);
        int index = i * int(steps) / surface_width;

				interpolate_colors(&rgb_points[0 * (int(steps) + 1) + index], &rgb_points[0 * (int(steps) + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (int(steps) + 1) + index], &rgb_points[1 * (int(steps) + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (int(steps) + 1) + index], &rgb_points[2 * (int(steps) + 1) + index + 1], position, &c[2]);

				col_ptr = data + i * 4;

				for (int y = 0; y < ns->n_components * 16; ++y){
          if ((y & 0x0f) != 0x0f){
						col_ptr[2] = (unsigned char)(c[y / 16].rgb.red * 255);
						col_ptr[1] = (unsigned char)(c[y / 16].rgb.green * 255);
						col_ptr[0] = (unsigned char)(c[y / 16].rgb.blue * 255);
						col_ptr[3] = 0xff;
					}else{
						col_ptr[0] = 0x00;
						col_ptr[1] = 0x00;
						col_ptr[2] = 0x00;
						col_ptr[3] = 0x00;
					}
					col_ptr += stride;
				}
			}
			break;

		default:
			break;
	}

	delete [] rgb_points;

	cairo_surface_mark_dirty(surface);
	cairo_save(cr);

	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_surface_destroy(surface);

	for (i = 0; i < ns->n_components; ++i){
		cairo_rectangle(cr, 0, 16 * i, 200, 15);
		cairo_fill(cr);
	}

	cairo_restore(cr);

	for (i = 0; i < ns->n_components; ++i){
		cairo_move_to(cr, 200*pointer_pos[i], 16 * i + 9);
		cairo_line_to(cr, 200*pointer_pos[i]+3, 16 * i + 16);
		cairo_line_to(cr, 200*pointer_pos[i]-3, 16 * i + 16);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_fill_preserve(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
	}

	cairo_destroy (cr);

	return TRUE;
}


GtkColorComponentComp gtk_color_component_get_component(GtkColorComponent* color_component){
	return GTK_COLOR_COMPONENT_GET_PRIVATE(color_component)->component;
}

static void update_rgb_color(GtkColorComponentPrivate *ns, Color *c){
	switch (ns->component) {
		case rgb:
			color_copy(&ns->color, c);
			color_rgb_normalize(c);
			break;
		case hsv:
			color_hsv_to_rgb(&ns->color, c);
			color_rgb_normalize(c);
			break;
		case hsl:
			color_hsl_to_rgb(&ns->color, c);
			color_rgb_normalize(c);
			break;
		case cmyk:
			color_cmyk_to_rgb(&ns->color, c);
			color_rgb_normalize(c);
			break;
		case lab:
			matrix3x3 adaptation_matrix, working_space_matrix, working_space_matrix_inv;
			vector3 d50, d65;
			vector3_set(&d50, 96.442, 100.000,  82.821);
			vector3_set(&d65, 95.047, 100.000, 108.883);
			color_get_chromatic_adaptation_matrix(&d65, &d50, &adaptation_matrix);
			color_get_working_space_matrix(0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, &d65, &working_space_matrix);

			matrix3x3_inverse(&working_space_matrix, &working_space_matrix_inv);

			Color c2;

			color_lab_to_xyz(&ns->color, &c2, &d50);
			color_xyz_chromatic_adaptation(&c2, &c2, &adaptation_matrix);
			color_xyz_to_rgb(&c2, c, &working_space_matrix_inv);
			color_rgb_normalize(c);

			break;

		case xyz:

			break;

	}
}

static void gtk_color_component_emit_color_change(GtkWidget *widget, int component, double value){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	Color c;
	ns->color.ma[component] = value * ns->range[component] + ns->offset[component];
	update_rgb_color(ns, &c);

	g_signal_emit(widget, gtk_color_component_signals[COLOR_CHANGED], 0, &c);
}

static gboolean gtk_color_component_button_release (GtkWidget *widget, GdkEventButton *event){
	return false;
}

static gboolean gtk_color_component_button_press (GtkWidget *widget, GdkEventButton *event){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){
		double value;

		value = event->x / 200.0;
		if (value < 0) value = 0;
		else if (value > 1) value = 1;

		int component = event->y / 16;
		if (component < 0) component = 0;
		else if (component >= ns->n_components) component = ns->n_components - 1;

		ns->capture_on = component;

		gtk_color_component_emit_color_change(widget, component, value);
		gtk_widget_queue_draw(widget);
		return TRUE;
	}
	return FALSE;
}

static gboolean gtk_color_component_motion_notify (GtkWidget *widget, GdkEventMotion *event){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if ((event->state & GDK_BUTTON1_MASK)){
		double value;

		value = event->x / 200.0;
		if (value < 0) value = 0;
		else if (value > 1) value = 1;

		/*int component = event->y / 16;
		if (component < 0) component = 0;
		else if (component >= ns->n_components) component = ns->n_components - 1;*/

		gtk_color_component_emit_color_change(widget, ns->capture_on, value);
		gtk_widget_queue_draw(widget);
		return TRUE;
	}
	return FALSE;
}

