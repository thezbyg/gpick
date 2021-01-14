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

#include "ColorComponent.h"
#include "uiUtilities.h"
#include "Color.h"
#include "Paths.h"
#include <cmath>
#include <vector>
using namespace std;

enum {
	COLOR_CHANGED,
	INPUT_CLICKED,
	LAST_SIGNAL
};
static const int MaxNumberOfComponents = 4;
static guint signals[LAST_SIGNAL] = {};
struct GtkColorComponentPrivate {
	Color orig_color;
	Color color;
	GtkColorComponentComp component;
	int n_components;
	int capture_on;
	gint last_event_position;
	bool changing_color;
	bool out_of_gamut_mask;
	ReferenceIlluminant lab_illuminant;
	ReferenceObserver lab_observer;
	cairo_surface_t *pattern_surface;
	cairo_pattern_t *pattern;
	const char *label[MaxNumberOfComponents][2];
	gchar *text[MaxNumberOfComponents];
	double range[MaxNumberOfComponents];
	double offset[MaxNumberOfComponents];
#if GTK_MAJOR_VERSION >= 3
	GdkDevice *pointer_grab;
#endif
};
#define GET_PRIVATE(obj) reinterpret_cast<GtkColorComponentPrivate *>(gtk_color_component_get_instance_private(GTK_COLOR_COMPONENT(obj)))
G_DEFINE_TYPE_WITH_CODE(GtkColorComponent, gtk_color_component, GTK_TYPE_DRAWING_AREA, G_ADD_PRIVATE(GtkColorComponent));
struct GtkColorComponentPrivate;
static gboolean button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean button_press(GtkWidget *node_system, GdkEventButton *event);
static gboolean motion_notify(GtkWidget *node_system, GdkEventMotion *event);
static void update_rgb_color(GtkColorComponentPrivate *ns, Color *c);
static void finalize(GObject *color_obj);
#if GTK_MAJOR_VERSION >= 3
static gboolean draw(GtkWidget *widget, cairo_t *cr);
#else
static gboolean expose(GtkWidget *widget, GdkEventExpose *event);
static void size_request(GtkWidget *widget, GtkRequisition *requisition);
#endif
static void gtk_color_component_class_init(GtkColorComponentClass *color_component_class)
{
	GObjectClass *obj_class = G_OBJECT_CLASS (color_component_class);
	obj_class->finalize = finalize;
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (color_component_class);
	widget_class->button_release_event = button_release;
	widget_class->button_press_event = button_press;
	widget_class->motion_notify_event = motion_notify;
#if GTK_MAJOR_VERSION >= 3
	widget_class->draw = draw;
#else
	widget_class->expose_event = expose;
	widget_class->size_request = size_request;
#endif
	signals[COLOR_CHANGED] = g_signal_new(
			"color-changed",
			G_OBJECT_CLASS_TYPE(obj_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorComponentClass, color_changed),
			nullptr, nullptr,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1,
			G_TYPE_POINTER);
	signals[INPUT_CLICKED] = g_signal_new(
			"input-clicked",
			G_OBJECT_CLASS_TYPE(obj_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET(GtkColorComponentClass, input_clicked),
			nullptr, nullptr,
			g_cclosure_marshal_VOID__INT,
			G_TYPE_NONE, 1,
			G_TYPE_INT);
}
static void gtk_color_component_init(GtkColorComponent *color_component)
{
	gtk_widget_add_events(GTK_WIDGET(color_component), GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK);
}
static void finalize(GObject *color_obj)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_obj);
	for (int i = 0; i != sizeof(ns->text) / sizeof(gchar*); i++){
		if (ns->text[i]){
			g_free(ns->text[i]);
			ns->text[i] = 0;
		}
	}
	if (ns->pattern_surface)
		cairo_surface_destroy(ns->pattern_surface);
	if (ns->pattern)
		cairo_pattern_destroy(ns->pattern);
	gpointer parent_class = g_type_class_peek_parent(G_OBJECT_CLASS(GTK_COLOR_COMPONENT_GET_CLASS(color_obj)));
	G_OBJECT_CLASS(parent_class)->finalize(color_obj);
}
GtkWidget *gtk_color_component_new(GtkColorComponentComp component)
{
	GtkWidget* widget = (GtkWidget*)g_object_new(GTK_TYPE_COLOR_COMPONENT, nullptr);
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
	auto patternFilename = buildFilename("gpick-gray-pattern.png");
	ns->pattern_surface = cairo_image_surface_create_from_png(patternFilename.c_str());
	ns->pattern = cairo_pattern_create_for_surface(ns->pattern_surface);
	cairo_pattern_set_extend(ns->pattern, CAIRO_EXTEND_REPEAT);
	ns->component = component;
	ns->last_event_position = -1;
	ns->changing_color = false;
	ns->lab_illuminant = ReferenceIlluminant::D50;
	ns->lab_observer = ReferenceObserver::_2;
	ns->out_of_gamut_mask = false;
#if GTK_MAJOR_VERSION >= 3
	ns->pointer_grab = nullptr;
#endif
	for (int i = 0; i != sizeof(ns->text) / sizeof(gchar*); i++){
		ns->text[i] = 0;
	}
	for (int i = 0; i != sizeof(ns->label) / sizeof(const char*[2]); i++){
		ns->label[i][0] = 0;
		ns->label[i][1] = 0;
	}
	switch (component){
		case GtkColorComponentComp::lab:
			ns->n_components = 3;
			ns->range[0] = 100;
			ns->offset[0] = 0;
			ns->range[1] = ns->range[2] = 290;
			ns->offset[1] = ns->offset[2] = -145;
			break;
		case GtkColorComponentComp::lch:
			ns->n_components = 3;
			ns->range[0] = 100;
			ns->offset[0] = 0;
			ns->range[1] = 136;
			ns->range[2] = 360;
			ns->offset[1] = ns->offset[2] = 0;
			break;
		case GtkColorComponentComp::xyz:
			//TODO: implement
			break;
		case GtkColorComponentComp::cmyk:
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
void gtk_color_component_get_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	*color = ns->orig_color;
}
void gtk_color_component_get_transformed_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	*color = ns->color;
}
void gtk_color_component_set_transformed_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	*color = ns->color;
	update_rgb_color(ns, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
int gtk_color_component_get_component_id_at(GtkColorComponent* color_component, gint x, gint y)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	int component = y / 16;
	if (component < 0) component = 0;
	else if (component >= ns->n_components) component = ns->n_components - 1;
	return component;
}
const char* gtk_color_component_get_text(GtkColorComponent* color_component, gint component_id)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	return ns->text[component_id];
}
void gtk_color_component_set_text(GtkColorComponent* color_component, const char **text)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	for (int i = 0; i != sizeof(ns->text) / sizeof(gchar*); i++){
		if (!text[i]) break;
		if (ns->text[i]){
			g_free(ns->text[i]);
		}
		ns->text[i] = g_strdup(text[i]);
	}
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
void gtk_color_component_set_label(GtkColorComponent* color_component, const char **label)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	for (int i = 0; i != MaxNumberOfComponents * 2; i++){
		if (!label[i]) break;
		ns->label[i >> 1][i & 1] = label[i];
	}
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
void gtk_color_component_set_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	ns->orig_color = *color;
	switch (ns->component){
		case GtkColorComponentComp::rgb:
			ns->color = ns->orig_color;
			break;
		case GtkColorComponentComp::hsl:
			ns->color = ns->orig_color.rgbToHsl();
			break;
		case GtkColorComponentComp::hsv:
			ns->color = ns->orig_color.rgbToHsv();
			break;
		case GtkColorComponentComp::cmyk:
			ns->color = ns->orig_color.rgbToCmyk();
			break;
		case GtkColorComponentComp::lab:
			{
				auto adaptationMatrix = Color::getChromaticAdaptationMatrix(Color::getReference(ReferenceIlluminant::D65, ReferenceObserver::_2), Color::getReference(ns->lab_illuminant, ns->lab_observer));
				ns->color = ns->orig_color.rgbToLab(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::sRGBMatrix, adaptationMatrix);
			}
			break;
		case GtkColorComponentComp::xyz:
			//TODO: implement
			break;
		case GtkColorComponentComp::lch:
			{
				auto adaptationMatrix = Color::getChromaticAdaptationMatrix(Color::getReference(ReferenceIlluminant::D65, ReferenceObserver::_2), Color::getReference(ns->lab_illuminant, ns->lab_observer));
				ns->color = ns->orig_color.rgbToLch(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::sRGBMatrix, adaptationMatrix);
			}
			break;
	}
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
static void interpolate_colors(Color *color1, Color *color2, float position, Color *result)
{
	result->rgb.red = color1->rgb.red * (1 - position) + color2->rgb.red * position;
	result->rgb.green= color1->rgb.green * (1 - position) + color2->rgb.green * position;
	result->rgb.blue = color1->rgb.blue * (1 - position) + color2->rgb.blue * position;
}
#if GTK_MAJOR_VERSION >= 3
#else
static void size_request(GtkWidget *widget, GtkRequisition *requisition)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
	gint width = 240 + widget->style->xthickness * 2;
	gint height = ns->n_components * 16 + widget->style->ythickness * 2;
	requisition->width = width;
	requisition->height = height;
}
#endif
static int get_x_offset(GtkWidget *widget)
{
#if GTK_MAJOR_VERSION >= 3
	return gtk_widget_get_allocated_width(widget) - 240;
#else
	return widget->allocation.width - widget->style->xthickness * 2 - 240;
#endif
}
static gboolean draw(GtkWidget *widget, cairo_t *cr)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
	Color c[MaxNumberOfComponents];
	double pointer_pos[MaxNumberOfComponents];
	int steps;
	int i, j;
	for (int i = 0; i < ns->n_components; ++i){
		pointer_pos[i] = (ns->color[i] - ns->offset[i]) / ns->range[i];
	}
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 200, ns->n_components * 16);
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	int surface_width = cairo_image_surface_get_width(surface);
	unsigned char *col_ptr;
	Color *rgb_points = new Color[ns->n_components * 200];
	double int_part;
	math::Matrix3d adaptationMatrix;
	std::vector<std::vector<bool>> out_of_gamut(MaxNumberOfComponents, std::vector<bool>(false, 1));
	switch (ns->component) {
		case GtkColorComponentComp::rgb:
			steps = 1;
			for (i = 0; i < 3; ++i){
				c[i] = ns->color;
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
		case GtkColorComponentComp::hsv:
			steps = 100;
			for (i = 0; i < 3; ++i){
				c[i] = ns->color;
			}
			for (i = 0; i <= steps; ++i){
				c[0].hsv.hue = c[1].hsv.saturation = c[2].hsv.value = i / static_cast<float>(steps);
				for (j = 0; j < 3; ++j){
					rgb_points[j * (steps + 1) + i] = c[j].hsvToRgb();
				}
			}
			for (i = 0; i < surface_width; ++i){
				float position = static_cast<float>(std::modf(i * static_cast<float>(steps) / surface_width, &int_part));
				int index = i * steps / surface_width;
				interpolate_colors(&rgb_points[0 * (steps + 1) + index], &rgb_points[0 * (steps + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (steps + 1) + index], &rgb_points[1 * (steps + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (steps + 1) + index], &rgb_points[2 * (steps + 1) + index + 1], position, &c[2]);
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
		case GtkColorComponentComp::hsl:
			steps = 100;
			for (i = 0; i < 3; ++i){
				c[i] = ns->color;
			}
			for (i = 0; i <= steps; ++i){
				c[0].hsl.hue = c[1].hsl.saturation = c[2].hsl.lightness = i / static_cast<float>(steps);
				for (j = 0; j < 3; ++j){
					rgb_points[j * (steps + 1) + i] = c[j].hslToRgb();
				}
			}
			for (i = 0; i < surface_width; ++i){
				float position = static_cast<float>(std::modf(i * static_cast<float>(steps) / surface_width, &int_part));
				int index = i * steps / surface_width;
				interpolate_colors(&rgb_points[0 * (steps + 1) + index], &rgb_points[0 * (steps + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (steps + 1) + index], &rgb_points[1 * (steps + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (steps + 1) + index], &rgb_points[2 * (steps + 1) + index + 1], position, &c[2]);
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
		case GtkColorComponentComp::cmyk:
			steps = 100;
			for (i = 0; i < 4; ++i){
				c[i] = ns->color;
			}
			for (i = 0; i <= steps; ++i){
				c[0].cmyk.c = c[1].cmyk.m = c[2].cmyk.y = c[3].cmyk.k = i / static_cast<float>(steps);
				for (j = 0; j < 4; ++j){
					rgb_points[j * (steps + 1) + i] = c[j].cmykToRgb();
				}
			}
			for (i = 0; i < surface_width; ++i){
				float position = static_cast<float>(std::modf(i * static_cast<float>(steps) / surface_width, &int_part));
				int index = i * steps / surface_width;
				interpolate_colors(&rgb_points[0 * (steps + 1) + index], &rgb_points[0 * (steps + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (steps + 1) + index], &rgb_points[1 * (steps + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (steps + 1) + index], &rgb_points[2 * (steps + 1) + index + 1], position, &c[2]);
				interpolate_colors(&rgb_points[3 * (steps + 1) + index], &rgb_points[3 * (steps + 1) + index + 1], position, &c[3]);
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
		case GtkColorComponentComp::lab:
			steps = 100;
			adaptationMatrix = Color::getChromaticAdaptationMatrix(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::getReference(ReferenceIlluminant::D65, ReferenceObserver::_2));
			for (j = 0; j < 3; ++j){
				c[j] = ns->color;
				out_of_gamut[j] = std::vector<bool>(steps + 1, false);
				for (i = 0; i <= steps; ++i){
					c[j][j] = static_cast<float>((i / static_cast<float>(steps)) * ns->range[j] + ns->offset[j]);
					rgb_points[j * (steps + 1) + i] = c[j].labToRgb(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::sRGBInvertedMatrix, adaptationMatrix);
					if (rgb_points[j * (steps + 1) + i].isOutOfRgbGamut()){
						out_of_gamut[j][i] = true;
					}
					rgb_points[j * (steps + 1) + i].normalizeRgbInplace();
				}
			}
			for (i = 0; i < surface_width; ++i){
				float position = static_cast<float>(std::modf(i * static_cast<float>(steps) / surface_width, &int_part));
				int index = i * steps / surface_width;
				interpolate_colors(&rgb_points[0 * (steps + 1) + index], &rgb_points[0 * (steps + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (steps + 1) + index], &rgb_points[1 * (steps + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (steps + 1) + index], &rgb_points[2 * (steps + 1) + index + 1], position, &c[2]);
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
		case GtkColorComponentComp::lch:
			steps = 100;
			adaptationMatrix = Color::getChromaticAdaptationMatrix(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::getReference(ReferenceIlluminant::D65, ReferenceObserver::_2));
			for (j = 0; j < 3; ++j){
				c[j] = ns->color;
				out_of_gamut[j] = std::vector<bool>(steps + 1, false);
				for (i = 0; i <= steps; ++i){
					c[j][j] = static_cast<float>((i / static_cast<float>(steps)) * ns->range[j] + ns->offset[j]);
					rgb_points[j * (steps + 1) + i] = c[j].lchToRgb(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::sRGBInvertedMatrix, adaptationMatrix);
					if (rgb_points[j * (steps + 1) + i].isOutOfRgbGamut()){
						out_of_gamut[j][i] = true;
					}
					rgb_points[j * (steps + 1) + i].normalizeRgbInplace();
				}
			}
			for (i = 0; i < surface_width; ++i){
				float position = static_cast<float>(std::modf(i * static_cast<float>(steps) / surface_width, &int_part));
				int index = i * steps / surface_width;
				interpolate_colors(&rgb_points[0 * (steps + 1) + index], &rgb_points[0 * (steps + 1) + index + 1], position, &c[0]);
				interpolate_colors(&rgb_points[1 * (steps + 1) + index], &rgb_points[1 * (steps + 1) + index + 1], position, &c[1]);
				interpolate_colors(&rgb_points[2 * (steps + 1) + index], &rgb_points[2 * (steps + 1) + index + 1], position, &c[2]);
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
	int offset_x = get_x_offset(widget);
	cairo_set_source_surface(cr, surface, offset_x, 0);
	cairo_surface_destroy(surface);
	for (i = 0; i < ns->n_components; ++i){
		cairo_rectangle(cr, offset_x, 16 * i, 200, 15);
		cairo_fill(cr);
	}
	cairo_restore(cr);
	for (i = 0; i < ns->n_components; ++i){
		cairo_matrix_t matrix;
		cairo_matrix_init_translate(&matrix, -offset_x - 64, -64 + 5 * i);
		cairo_pattern_set_matrix(ns->pattern, &matrix);
		if (ns->out_of_gamut_mask){
			int first_out_of_gamut = 0;
			bool out_of_gamut_found = false;
			cairo_set_source(cr, ns->pattern);
			for (size_t j = 0; j < out_of_gamut[i].size(); j++){
				if (out_of_gamut[i][j]){
					if (!out_of_gamut_found){
						out_of_gamut_found = true;
						first_out_of_gamut = j;
					}
				}else{
					if (out_of_gamut_found){
						cairo_rectangle(cr, offset_x + (first_out_of_gamut * 200.0 / out_of_gamut[i].size()), 16 * i, (j - first_out_of_gamut) * 200.0 / out_of_gamut[i].size(), 15);
						cairo_fill(cr);
						out_of_gamut_found = false;
					}
				}
			}
			if (out_of_gamut_found){
				cairo_rectangle(cr, offset_x + (first_out_of_gamut * 200.0 / out_of_gamut[i].size()), 16 * i, (out_of_gamut[i].size() - first_out_of_gamut) * 200.0 / out_of_gamut[i].size(), 15);
				cairo_fill(cr);
			}
		}
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
#if GTK_MAJOR_VERSION >= 3
			//TODO: GTK3 font color
#else
			gdk_cairo_set_source_color(cr, &widget->style->text[0]);
#endif
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
	return TRUE;
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
GtkColorComponentComp gtk_color_component_get_component(GtkColorComponent* color_component)
{
	return GET_PRIVATE(color_component)->component;
}
static void update_rgb_color(GtkColorComponentPrivate *ns, Color *c)
{
	switch (ns->component){
		case GtkColorComponentComp::rgb:
			*c = ns->color.normalizeRgb();
			break;
		case GtkColorComponentComp::hsv:
			*c = ns->color.hsvToRgb().normalizeRgbInplace();
			break;
		case GtkColorComponentComp::hsl:
			*c = ns->color.hslToRgb().normalizeRgbInplace();
			break;
		case GtkColorComponentComp::cmyk:
			*c = ns->color.cmykToRgb().normalizeRgbInplace();
			break;
		case GtkColorComponentComp::lab:
			{
				auto adaptationMatrix = Color::getChromaticAdaptationMatrix(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::getReference(ReferenceIlluminant::D65, ReferenceObserver::_2));
				*c = ns->color.labToRgb(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::sRGBInvertedMatrix, adaptationMatrix).normalizeRgbInplace();
			}
			break;
		case GtkColorComponentComp::xyz:
			//TODO: implement
			break;
		case GtkColorComponentComp::lch:
			{
				auto adaptationMatrix = Color::getChromaticAdaptationMatrix(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::getReference(ReferenceIlluminant::D65, ReferenceObserver::_2));
				*c = ns->color.lchToRgb(Color::getReference(ns->lab_illuminant, ns->lab_observer), Color::sRGBInvertedMatrix, adaptationMatrix).normalizeRgbInplace();
			}
			break;
	}
}
void gtk_color_component_get_raw_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	*color = ns->color;
}
void gtk_color_component_set_raw_color(GtkColorComponent* color_component, Color* color)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	ns->color = *color;
	Color c;
	update_rgb_color(ns, &c);
	ns->orig_color = c;
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
	g_signal_emit(GTK_WIDGET(color_component), signals[COLOR_CHANGED], 0, &c);
}
static void emit_color_change(GtkWidget *widget, int component, double value)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
	Color c;
	ns->color[component] = static_cast<float>(value * ns->range[component] + ns->offset[component]);
	update_rgb_color(ns, &c);
	g_signal_emit(widget, signals[COLOR_CHANGED], 0, &c);
}
static gboolean button_release(GtkWidget *widget, GdkEventButton *event)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
#if GTK_MAJOR_VERSION >= 3
	if (ns->pointer_grab){
		gdk_seat_ungrab(gdk_device_get_seat(ns->pointer_grab));
		ns->pointer_grab = nullptr;
	}
#else
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
#endif
	ns->changing_color = false;
	return false;
}
static gboolean button_press(GtkWidget *widget, GdkEventButton *event)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)){
		int component = static_cast<int>(event->y / 16);
		if (component < 0) component = 0;
		else if (component >= ns->n_components) component = ns->n_components - 1;
		int offset_x = get_x_offset(widget);
		if (event->x < offset_x || event->x > 200 + offset_x) {
			g_signal_emit(widget, signals[INPUT_CLICKED], 0, component);
			return FALSE;
		}
		ns->changing_color = true;
		ns->last_event_position = static_cast<gint>(event->x);
		double value;
		value = (event->x - offset_x) / 200.0;
		if (value < 0) value = 0;
		else if (value > 1) value = 1;
		ns->capture_on = component;
#if GTK_MAJOR_VERSION >= 3
		ns->pointer_grab = event->device;
		gdk_seat_grab(gdk_device_get_seat(event->device), gtk_widget_get_window(widget), GDK_SEAT_CAPABILITY_ALL, false, nullptr, nullptr, nullptr, nullptr);
#else
		gdk_pointer_grab(gtk_widget_get_window(widget), false, GdkEventMask(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK), nullptr, nullptr, GDK_CURRENT_TIME);
#endif
		emit_color_change(widget, component, value);
		gtk_widget_queue_draw(widget);
		return TRUE;
	}
	return FALSE;
}
static gboolean motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(widget);
	if (ns->changing_color && (event->state & GDK_BUTTON1_MASK)){
		int offset_x = get_x_offset(widget);
		if ((event->x < offset_x && ns->last_event_position < offset_x) || ((event->x > 200 + offset_x) && (ns->last_event_position > 200 + offset_x))) return FALSE;
		ns->last_event_position = static_cast<gint>(event->x);
		double value;
		value = (event->x - offset_x) / 200.0;
		if (value < 0) value = 0;
		else if (value > 1) value = 1;
		emit_color_change(widget, ns->capture_on, value);
		gtk_widget_queue_draw(widget);
		return TRUE;
	}
	return FALSE;
}
void gtk_color_component_set_lab_illuminant(GtkColorComponent* color_component, ReferenceIlluminant illuminant)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	ns->lab_illuminant = illuminant;
	gtk_color_component_set_color(color_component, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
void gtk_color_component_set_lab_observer(GtkColorComponent* color_component, ReferenceObserver observer)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	ns->lab_observer = observer;
	gtk_color_component_set_color(color_component, &ns->orig_color);
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
void gtk_color_component_set_out_of_gamut_mask(GtkColorComponent* color_component, bool mask_enabled)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	ns->out_of_gamut_mask = mask_enabled;
	gtk_widget_queue_draw(GTK_WIDGET(color_component));
}
bool gtk_color_component_get_out_of_gamut_mask(GtkColorComponent* color_component)
{
	GtkColorComponentPrivate *ns = GET_PRIVATE(color_component);
	return ns->out_of_gamut_mask;
}
