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

#include "ColorCell.h"
#include "../ColorObject.h"

static void custom_cell_renderer_color_init(CustomCellRendererColor *cellcolor);

static void custom_cell_renderer_color_class_init(CustomCellRendererColorClass *klass);

static void custom_cell_renderer_color_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *pspec);

static void custom_cell_renderer_color_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

static void custom_cell_renderer_color_finalize(GObject *gobject);

static void custom_cell_renderer_color_get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width,
		gint *height);

static void custom_cell_renderer_color_render     (GtkCellRenderer            *cell,
							 GdkDrawable                *window,
							 GtkWidget                  *widget,
							 GdkRectangle               *background_area,
							 GdkRectangle               *cell_area,
							 GdkRectangle               *expose_area,
							 GtkCellRendererState        flags);


enum
{
  PROP_COLOR = 1,
};

static   gpointer parent_class;

GType custom_cell_renderer_color_get_type(void) {
	static GType cell_color_type = 0;

	if (cell_color_type == 0){
		static const GTypeInfo cell_color_info = { sizeof(CustomCellRendererColorClass), NULL, /* base_init */
		NULL, /* base_finalize */
		(GClassInitFunc) custom_cell_renderer_color_class_init, NULL, /* class_finalize */
		NULL, /* class_data */
		sizeof(CustomCellRendererColor), 0, /* n_preallocs */
		(GInstanceInitFunc) custom_cell_renderer_color_init, };

		cell_color_type = g_type_register_static(GTK_TYPE_CELL_RENDERER, "CustomCellRendererColor", &cell_color_info, (GTypeFlags) 0);
	}

	return cell_color_type;
}

static void custom_cell_renderer_color_init(CustomCellRendererColor *cellrenderercolor) {
	GTK_CELL_RENDERER(cellrenderercolor)->mode = GTK_CELL_RENDERER_MODE_INERT;
	GTK_CELL_RENDERER(cellrenderercolor)->xpad = 2;
	GTK_CELL_RENDERER(cellrenderercolor)->ypad = 2;
}

static void custom_cell_renderer_color_class_init(CustomCellRendererColorClass *klass) {
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = custom_cell_renderer_color_finalize;

	object_class->get_property = custom_cell_renderer_color_get_property;
	object_class->set_property = custom_cell_renderer_color_set_property;

	cell_class->get_size = custom_cell_renderer_color_get_size;
	cell_class->render = custom_cell_renderer_color_render;

	g_object_class_install_property(object_class, PROP_COLOR, g_param_spec_pointer("color", "Color", "ColorObject pointer", (GParamFlags) G_PARAM_READWRITE));
}


static void custom_cell_renderer_color_finalize(GObject *object) {
	(*G_OBJECT_CLASS(parent_class)->finalize)(object);
}

static void custom_cell_renderer_color_get_property(GObject *object, guint param_id, GValue *value, GParamSpec *psec) {
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR(object);

	switch (param_id) {
	case PROP_COLOR:
		g_value_set_pointer(value, cellcolor->color);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, psec);
		break;
	}
}

static void custom_cell_renderer_color_set_property(GObject *object, guint param_id, const GValue *value, GParamSpec *pspec) {
	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR (object);

	switch (param_id) {
	case PROP_COLOR:
		cellcolor->color = (struct ColorObject*) g_value_get_pointer(value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
		break;
	}
}

GtkCellRenderer *
custom_cell_renderer_color_new(void) {
	return (GtkCellRenderer *) g_object_new(CUSTOM_TYPE_CELL_RENDERER_COLOR, NULL);
}


#define FIXED_WIDTH   32
#define FIXED_HEIGHT  16

static void custom_cell_renderer_color_get_size(GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width,
		gint *height) {
	gint calc_width;
	gint calc_height;

	calc_width = (gint) cell->xpad * 2 + FIXED_WIDTH;
	calc_height = (gint) cell->ypad * 2 + FIXED_HEIGHT;

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


static void custom_cell_renderer_color_render(GtkCellRenderer *cell, GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area,
		GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags) {

	CustomCellRendererColor *cellcolor = CUSTOM_CELL_RENDERER_COLOR (cell);

	/*GtkStateType state;
	gint width, height;
	gint x_offset, y_offset;

	custom_cell_renderer_color_get_size(cell, widget, cell_area, &x_offset, &y_offset, &width, &height);

	if (GTK_WIDGET_HAS_FOCUS(widget))
		state = GTK_STATE_ACTIVE;
	else
		state = GTK_STATE_NORMAL;

	width -= cell->xpad * 2;
	height -= cell->ypad * 2;*/

	cairo_t *cr;
	cr = gdk_cairo_create(window);
	cairo_rectangle(cr, expose_area->x, expose_area->y, expose_area->width, expose_area->height);
	cairo_clip(cr);

	cairo_rectangle(cr, expose_area->x, expose_area->y, expose_area->width, expose_area->height);
	Color c;
	color_object_get_color(cellcolor->color, &c);
	cairo_set_source_rgb(cr, c.rgb.red, c.rgb.green, c.rgb.blue);
	cairo_fill(cr);

	cairo_destroy(cr);

	/*
	gtk_paint_box(widget->style, window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, "trough", cell_area->x + x_offset + cell->xpad, cell_area->y + y_offset
			+ cell->ypad, width - 1, height - 1);


	 gtk_paint_box (widget->style,
	 window,
	 state, GTK_SHADOW_OUT,
	 NULL, widget, "bar",
	 cell_area->x + x_offset + cell->xpad,
	 cell_area->y + y_offset + cell->ypad,
	 width * 0.5,
	 height - 1);
	 */
}

