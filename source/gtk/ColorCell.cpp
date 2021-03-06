/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#include "ColorCell.h"
#include "ColorObject.h"
#include <boost/math/special_functions/round.hpp>

static void init(CustomCellRendererColor *cellcolor);
static void class_init(CustomCellRendererColorClass *klass);
static void get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);
static void set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);
static void finalize(GObject *gobject);
#if GTK_MAJOR_VERSION >= 3
static void get_preferred_width(GtkCellRenderer *cell, GtkWidget *widget, gint *minimum_size, gint *natural_size);
static void get_preferred_height(GtkCellRenderer *cell, GtkWidget *widget, gint *minimum_size, gint *natural_size);
static void render(GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget, const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags);
#else
static void get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height);
static void render(GtkCellRenderer *cell, GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags);
#endif

enum
{
	PROP_COLOR = 1,
};
static gpointer parent_class;

GType custom_cell_renderer_color_get_type()
{
	static GType cell_color_type = 0;
	if (cell_color_type == 0){
		static const GTypeInfo cell_color_info = { sizeof(CustomCellRendererColorClass), nullptr, /* base_init */
		nullptr, /* base_finalize */
		(GClassInitFunc) class_init, nullptr, /* class_finalize */
		nullptr, /* class_data */
		sizeof(CustomCellRendererColor), 0, /* n_preallocs */
		(GInstanceInitFunc) init, };
		cell_color_type = g_type_register_static(GTK_TYPE_CELL_RENDERER, "CustomCellRendererColor", &cell_color_info, (GTypeFlags) 0);
	}
	return cell_color_type;
}
static void init(CustomCellRendererColor *cellrenderercolor)
{
#if GTK_MAJOR_VERSION >= 3
#else
	GTK_CELL_RENDERER(cellrenderercolor)->mode = GTK_CELL_RENDERER_MODE_INERT;
	GTK_CELL_RENDERER(cellrenderercolor)->xpad = 2;
	GTK_CELL_RENDERER(cellrenderercolor)->ypad = 2;
#endif
	cellrenderercolor->width = 32;
	cellrenderercolor->height = 16;
	cellrenderercolor->color = nullptr;
}
static void class_init(CustomCellRendererColorClass *klass)
{
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = finalize;
	object_class->get_property = get_property;
	object_class->set_property = set_property;
#if GTK_MAJOR_VERSION >= 3
	cell_class->get_preferred_width = get_preferred_width;
	cell_class->get_preferred_height = get_preferred_height;
#else
	cell_class->get_size = get_size;
#endif
	cell_class->render = render;
	g_object_class_install_property(object_class, PROP_COLOR, g_param_spec_pointer("color", "Color", "ColorObject pointer", (GParamFlags) G_PARAM_READWRITE));
}
static void finalize(GObject *object)
{
	(*G_OBJECT_CLASS(parent_class)->finalize)(object);
}
static void get_property(GObject *object, guint param_id, GValue *value, GParamSpec *psec)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(object);
	switch (param_id){
	case PROP_COLOR:
		g_value_set_pointer(value, cellcolor->color);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, psec);
		break;
	}
}
static void set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(object);
	switch (param_id){
	case PROP_COLOR:
		cellcolor->color = (ColorObject*)g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
		break;
	}
}
GtkCellRenderer *custom_cell_renderer_color_new()
{
	return (GtkCellRenderer*)g_object_new(CUSTOM_TYPE_CELL_RENDERER_COLOR, nullptr);
}
void custom_cell_renderer_color_set_size(GtkCellRenderer *cell, gint width, gint height)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(cell);
	cellcolor->width = width;
	cellcolor->height = height;
}
#if GTK_MAJOR_VERSION >= 3
static void get_preferred_width(GtkCellRenderer *cell, GtkWidget *widget, gint *minimum_size, gint *natural_size)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(cell);
	if (minimum_size)
		*minimum_size = 1;
	*natural_size = cellcolor->width;
}
static void get_preferred_height(GtkCellRenderer *cell, GtkWidget *widget, gint *minimum_size, gint *natural_size)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(cell);
	if (minimum_size)
		*minimum_size = 1;
	*natural_size = cellcolor->height;
}
static void render(GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget, const GdkRectangle *background_area, const GdkRectangle *cell_area, GtkCellRendererState flags)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(cell);
	cairo_rectangle(cr, cell_area->x, cell_area->y, cell_area->width, cell_area->height);
	Color color = cellcolor->color->getColor();
	cairo_set_source_rgb(cr, round(color.rgb.red * 255.0) / 255.0, round(color.rgb.green * 255.0) / 255.0, round(color.rgb.blue * 255.0) / 255.0);
	cairo_fill(cr);
}
#else
static void get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height)
{
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(cell);
	gint calc_width;
	gint calc_height;
	calc_width = (gint) cell->xpad * 2 + cellcolor->width;
	calc_height = (gint) cell->ypad * 2 + cellcolor->height;
	if (width)
		*width = calc_width;
	if (height)
		*height = calc_height;
	if (cell_area){
		if (x_offset){
			*x_offset = gint(cell->xalign * (cell_area->width - calc_width));
			*x_offset = MAX(*x_offset, 0);
		}
		if (y_offset){
			*y_offset = gint(cell->yalign * (cell_area->height - calc_height));
			*y_offset = MAX(*y_offset, 0);
		}
	}
}
static void render(GtkCellRenderer *cell, GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags)
{
	using boost::math::round;
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(cell);
	cairo_t *cr;
	cr = gdk_cairo_create(window);
	cairo_rectangle(cr, expose_area->x, expose_area->y, expose_area->width, expose_area->height);
	cairo_clip(cr);
	cairo_rectangle(cr, expose_area->x, expose_area->y, expose_area->width, expose_area->height);
	Color color = cellcolor->color->getColor();
	cairo_set_source_rgb(cr, round(color.rgb.red * 255.0) / 255.0, round(color.rgb.green * 255.0) / 255.0, round(color.rgb.blue * 255.0) / 255.0);
	cairo_fill(cr);
	cairo_destroy(cr);
}
#endif
