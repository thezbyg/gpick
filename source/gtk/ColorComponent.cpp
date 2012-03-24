/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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
static void gtk_color_component_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void gtk_color_component_finalize(GObject *color_obj);

enum{
  COLOR_CHANGED,
  INPUT_CLICKED,
  LAST_SIGNAL
};

static const int MaxNumberOfComponents = 4;

static guint gtk_color_component_signals[LAST_SIGNAL] = {0, 0};

typedef struct GtkColorComponentPrivate{
	Color orig_color;
	Color color;
	GtkColorComponentComp component;
	int n_components;
	int capture_on;
	gint last_event_position;
	bool changing_color;

	ReferenceIlluminant lab_illuminant;
	ReferenceObserver lab_observer;

	const char *label[MaxNumberOfComponents][2];
	gchar *text[MaxNumberOfComponents];
	double range[MaxNumberOfComponents];
	double offset[MaxNumberOfComponents];
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
	widget_class->size_request = gtk_color_component_size_request;

	g_type_class_add_private(obj_class, sizeof(GtkColorComponentPrivate));

	obj_class->finalize = gtk_color_component_finalize;

	gtk_color_component_signals[COLOR_CHANGED] = g_signal_new(
			"color-changed",
			G_OBJECT_CLASS_TYPE(obj_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorComponentClass, color_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1,
			G_TYPE_POINTER);
	gtk_color_component_signals[INPUT_CLICKED] = g_signal_new(
			"input-clicked",
			G_OBJECT_CLASS_TYPE(obj_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorComponentClass, input_clicked),
			NULL, NULL,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE, 1,
			G_TYPE_INT);
}

static void gtk_color_component_init (GtkColorComponent *color_component){
	gtk_widget_add_events (GTK_WIDGET (color_component),
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
}

static void gtk_color_component_finalize(GObject *color_obj){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_obj);
	for (int i = 0; i != sizeof(ns->text) / sizeof(gchar*); i++){
		if (ns->text[i]){
			g_free(ns->text[i]);
			ns->text[i] = 0;
		}
	}
	gpointer parent_class = g_type_class_peek_parent(G_OBJECT_CLASS(GTK_COLOR_COMPONENT_GET_CLASS(color_obj)));
	G_OBJECT_CLASS(parent_class)->finalize(color_obj);
}


GtkWidget *gtk_color_component_new (GtkColorComponentComp component){
	GtkWidget* widget = (GtkWidget*)g_object_new(GTK_TYPE_COLOR_COMPONENT, NULL);
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	ns->component = component;
	ns->last_event_position = -1;
	ns->changing_color = false;
	ns->lab_illuminant = REFERENCE_ILLUMINANT_D50;
	ns->lab_observer= REFERENCE_OBSERVER_2;

	for (int i = 0; i != sizeof(ns->text) / sizeof(gchar*); i++){
		ns->text[i] = 0;
	}
	for (int i = 0; i != sizeof(ns->label) / sizeof(const char*[2]); i++){
		ns->label[i][0] = 0;
		ns->label[i][1] = 0;
	}

	switch (component){
		case lab:
			ns->n_components = 3;
			ns->range[0] = 100;
			ns->offset[0] = 0;
			ns->range[1] = ns->range[2] = 290;
			ns->offset[1] = ns->offset[2] = -145;
			break;

		case lch:
			ns->n_components = 3;
			ns->range[0] = 100;
			ns->offset[0] = 0;
			ns->range[1] = 100;
			ns->range[2] = 360;
			ns->offset[1] = ns->offset[2] = 0;
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

	gtk_widget_set_size_request(GTK_WIDGET(widget), 242, 16 * ns->n_components);

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

const char* gtk_color_component_get_text(GtkColorComponent* color_component, gint component_id){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	return ns->text[component_id];
}

void gtk_color_component_set_text(GtkColorComponent* color_component, const char **text){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	for (int i = 0; i != sizeof(ns->text) / sizeof(gchar*); i++){
		if (!text[i]) break;
		if (ns->text[i]){
			g_free(ns->text[i]);
		}
		ns->text[i] = g_strdup(text[i]);
	}
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

void gtk_color_component_set_label(GtkColorComponent* color_component, const char **label){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	for (int i = 0; i != MaxNumberOfComponents * 2; i++){
		if (!label[i]) break;
		ns->label[i >> 1][i & 1] = label[i];
	}
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

void gtk_color_component_set_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(color, &ns->orig_color);

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
			{
				matrix3x3 adaptation_matrix;
				color_get_chromatic_adaptation_matrix(color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), color_get_reference(ns->lab_illuminant, ns->lab_observer), &adaptation_matrix);
				color_rgb_to_lab(&ns->orig_color, &ns->color, color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_sRGB_transformation_matrix(), &adaptation_matrix);
			}
			break;
		case xyz:
			/* todo */
			break;
		case lch:
			{
				matrix3x3 adaptation_matrix;
				color_get_chromatic_adaptation_matrix(color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), color_get_reference(ns->lab_illuminant, ns->lab_observer), &adaptation_matrix);
				color_rgb_to_lch(&ns->orig_color, &ns->color, color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_sRGB_transformation_matrix(), &adaptation_matrix);
			}
			break;
	}


	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

static void interpolate_colors(Color *color1, Color *color2, float position, Color *result){
	result->rgb.red = color1->rgb.red * (1 - position) + color2->rgb.red * position;
	result->rgb.green= color1->rgb.green * (1 - position) + color2->rgb.green * position;
	result->rgb.blue = color1->rgb.blue * (1 - position) + color2->rgb.blue * position;
}

static void gtk_color_component_size_request (GtkWidget *widget, GtkRequisition *requisition){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	gint width = 240 + widget->style->xthickness * 2;
	gint height = ns->n_components * 16 + widget->style->ythickness * 2;

	requisition->width = width;
	requisition->height = height;
}

static gboolean gtk_color_component_expose (GtkWidget *widget, GdkEventExpose *event){
	cairo_t *cr;

	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	cr = gdk_cairo_create(widget->window);
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	Color c[MaxNumberOfComponents];
	double pointer_pos[MaxNumberOfComponents];

	float steps;
	int i, j;

	for (int i = 0; i < ns->n_components; ++i){
		pointer_pos[i] = (ns->color.ma[i] - ns->offset[i]) / ns->range[i];
	}

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 200, ns->n_components * 16);
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	int surface_width = cairo_image_surface_get_width(surface);

	unsigned char *col_ptr;

	Color *rgb_points = new Color[ns->n_components * 200];

	double int_part;
	matrix3x3 adaptation_matrix;

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

			color_get_chromatic_adaptation_matrix(color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), &adaptation_matrix);

			for (i = 0; i < 3; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i <= steps; ++i){
				c[0].lab.L = (i / steps) * ns->range[0] + ns->offset[0];
        color_lab_to_rgb(&c[0], &rgb_points[0 * (int(steps) + 1) + i], color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
				color_rgb_normalize(&rgb_points[0 * (int(steps) + 1) + i]);
			}

			for (i = 0; i <= steps; ++i){
				c[1].lab.a = (i / steps) * ns->range[1] + ns->offset[1];
        color_lab_to_rgb(&c[1], &rgb_points[1 * (int(steps) + 1) + i], color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
				color_rgb_normalize(&rgb_points[1 * (int(steps) + 1) + i]);
			}
			for (i = 0; i <= steps; ++i){
				c[2].lab.b = (i / steps) * ns->range[2] + ns->offset[2];
        color_lab_to_rgb(&c[2], &rgb_points[2 * (int(steps) + 1) + i], color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
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

		case lch:
			steps = 100;

			color_get_chromatic_adaptation_matrix(color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), &adaptation_matrix);

			for (i = 0; i < 3; ++i){
				color_copy(&ns->color, &c[i]);
			}
			for (i = 0; i <= steps; ++i){
				c[0].lch.L = (i / steps) * ns->range[0] + ns->offset[0];
        color_lch_to_rgb(&c[0], &rgb_points[0 * (int(steps) + 1) + i], color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
				color_rgb_normalize(&rgb_points[0 * (int(steps) + 1) + i]);
			}

			for (i = 0; i <= steps; ++i){
				c[1].lch.C = (i / steps) * ns->range[1] + ns->offset[1];
        color_lch_to_rgb(&c[1], &rgb_points[1 * (int(steps) + 1) + i], color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
				color_rgb_normalize(&rgb_points[1 * (int(steps) + 1) + i]);
			}
			for (i = 0; i <= steps; ++i){
				c[2].lch.h = (i / steps) * ns->range[2] + ns->offset[2];
        color_lch_to_rgb(&c[2], &rgb_points[2 * (int(steps) + 1) + i], color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
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

	int offset_x = widget->allocation.width - widget->style->xthickness * 2 - 240;

	cairo_set_source_surface(cr, surface, offset_x, 0);
	cairo_surface_destroy(surface);

	for (i = 0; i < ns->n_components; ++i){
		cairo_rectangle(cr, offset_x, 16 * i, 200, 15);
		cairo_fill(cr);
	}

	cairo_restore(cr);

	for (i = 0; i < ns->n_components; ++i){
		cairo_move_to(cr, offset_x + 200 * pointer_pos[i], 16 * i + 9);
		cairo_line_to(cr, offset_x + 200 * pointer_pos[i] + 3, 16 * i + 16);
		cairo_line_to(cr, offset_x + 200 * pointer_pos[i] - 3, 16 * i + 16);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_fill_preserve(cr);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);

		if (ns->text[i] || ns->label[i]){
			PangoLayout *layout;
			PangoFontDescription *font_description;
			font_description = pango_font_description_new();
			layout = pango_cairo_create_layout(cr);

			pango_font_description_set_family(font_description, "sans");
			pango_font_description_set_weight(font_description, PANGO_WEIGHT_NORMAL);
			pango_font_description_set_absolute_size(font_description, 12 * PANGO_SCALE);
			pango_layout_set_font_description(layout, font_description);
			pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
			pango_layout_set_single_paragraph_mode(layout, true);

			gdk_cairo_set_source_color(cr, &widget->style->text[0]);
			int width, height;

			if (ns->text[i]){
				pango_layout_set_text(layout, ns->text[i], -1);
				pango_layout_set_width(layout, 40 * PANGO_SCALE);
				pango_layout_set_height(layout, 16 * PANGO_SCALE);
				pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
				pango_cairo_update_layout(cr, layout);

				pango_layout_get_pixel_size(layout, &width, &height);
				cairo_move_to(cr, 200 + offset_x, i * 16);
				pango_cairo_show_layout(cr, layout);
			}

			if (ns->label[i] && offset_x > 10){
				if (offset_x > 50){
					pango_layout_set_text(layout, ns->label[i][1], -1);
				}else{
					pango_layout_set_text(layout, ns->label[i][0], -1);
				}
				pango_layout_set_width(layout, (offset_x - 10) * PANGO_SCALE);
				pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
				pango_layout_set_height(layout, 16 * PANGO_SCALE);
				pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
				pango_cairo_update_layout(cr, layout);

				pango_layout_get_pixel_size(layout, &width, &height);
				cairo_move_to(cr, 5, i * 16);
				pango_cairo_show_layout(cr, layout);
			}

			g_object_unref(layout);
			pango_font_description_free(font_description);
		}
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
			{
				matrix3x3 adaptation_matrix;
				color_get_chromatic_adaptation_matrix(color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), &adaptation_matrix);
				color_lab_to_rgb(&ns->color, c, color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
				color_rgb_normalize(c);
			}
			break;
		case xyz:
      /* TODO */
			break;
		case lch:
			{
				matrix3x3 adaptation_matrix;
				color_get_chromatic_adaptation_matrix(color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_reference(REFERENCE_ILLUMINANT_D65, REFERENCE_OBSERVER_2), &adaptation_matrix);
				color_lch_to_rgb(&ns->color, c, color_get_reference(ns->lab_illuminant, ns->lab_observer), color_get_inverted_sRGB_transformation_matrix(), &adaptation_matrix);
				color_rgb_normalize(c);
			}
			break;

	}
}

void gtk_color_component_get_raw_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(&ns->color, color);
}

void gtk_color_component_set_raw_color(GtkColorComponent* color_component, Color* color){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
	color_copy(color, &ns->color);
	Color c;
	update_rgb_color(ns, &c);
	color_copy(&c, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
	g_signal_emit(GTK_WIDGET(color_component), gtk_color_component_signals[COLOR_CHANGED], 0, &c);
}

static void gtk_color_component_emit_color_change(GtkWidget *widget, int component, double value){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	Color c;
	ns->color.ma[component] = value * ns->range[component] + ns->offset[component];
	update_rgb_color(ns, &c);

	g_signal_emit(widget, gtk_color_component_signals[COLOR_CHANGED], 0, &c);
}

static gboolean gtk_color_component_button_release (GtkWidget *widget, GdkEventButton *event){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	ns->changing_color = false;
	return false;
}

static gboolean gtk_color_component_button_press (GtkWidget *widget, GdkEventButton *event){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){
		int component = event->y / 16;
		if (component < 0) component = 0;
		else if (component >= ns->n_components) component = ns->n_components - 1;

		int offset_x = widget->allocation.width - widget->style->xthickness * 2 - 240;

		if (event->x < offset_x || event->x > 200 + offset_x) {
			g_signal_emit(widget, gtk_color_component_signals[INPUT_CLICKED], 0, component);
			return FALSE;
		}
		ns->changing_color = true;
		ns->last_event_position = event->x;
		double value;

		value = (event->x - offset_x) / 200.0;
		if (value < 0) value = 0;
		else if (value > 1) value = 1;


		ns->capture_on = component;
		gdk_pointer_grab(gtk_widget_get_window(widget), false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK), NULL, NULL, GDK_CURRENT_TIME);

		gtk_color_component_emit_color_change(widget, component, value);
		gtk_widget_queue_draw(widget);
		return TRUE;
	}
	return FALSE;
}

static gboolean gtk_color_component_motion_notify (GtkWidget *widget, GdkEventMotion *event){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(widget);

	if (ns->changing_color && (event->state & GDK_BUTTON1_MASK)){
		int offset_x = widget->allocation.width - widget->style->xthickness * 2 - 240;
		if ((event->x < offset_x && ns->last_event_position < offset_x) || ((event->x > 200 + offset_x) && (ns->last_event_position > 200 + offset_x))) return FALSE;
		ns->last_event_position = event->x;
		double value;

		value = (event->x - offset_x) / 200.0;
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

void gtk_color_component_set_lab_illuminant(GtkColorComponent* color_component, ReferenceIlluminant illuminant){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
  ns->lab_illuminant = illuminant;
	gtk_color_component_set_color(color_component, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

void gtk_color_component_set_lab_observer(GtkColorComponent* color_component, ReferenceObserver observer){
	GtkColorComponentPrivate *ns = GTK_COLOR_COMPONENT_GET_PRIVATE(color_component);
  ns->lab_observer = observer;
	gtk_color_component_set_color(color_component, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}

