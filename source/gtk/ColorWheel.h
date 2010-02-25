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

#ifndef COLOR_WHEEL_H_
#define COLOR_WHEEL_H_


#include <gtk/gtk.h>
#include "../Color.h"
#include "../ColorWheelType.h"


G_BEGIN_DECLS

#define GTK_TYPE_COLOR_WHEEL		(gtk_color_wheel_get_type ())
#define GTK_COLOR_WHEEL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_COLOR_WHEEL, GtkColorWheel))
#define GTK_COLOR_WHEEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GTK_COLOR_WHEEL, GtkColorWheelClass))
#define GTK_IS_COLOR_WHEEL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_COLOR_WHEEL))
#define GTK_IS_COLOR_WHEEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GTK_TYPE_COLOR_WHEEL))
#define GTK_COLOR_WHEEL_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_COLOR_WHEEL, GtkColorWheelClass))

typedef struct GtkColorWheel			GtkColorWheel;
typedef struct GtkColorWheelClass		GtkColorWheelClass;

typedef gpointer GtkColorWheelObject;

typedef struct GtkColorWheel
{
	GtkDrawingArea parent;

	/* < private > */
}GtkColorWheel;

typedef struct GtkColorWheelClass{
	GtkDrawingAreaClass parent_class;
	void  (*hue_changed)(GtkWidget* widget, gint color_id, gpointer userdata);
	void  (*saturation_value_changed)(GtkWidget* widget, gint color_id, gpointer userdata);
}GtkColorWheelClass;

GtkWidget* gtk_color_wheel_new(void);

void gtk_color_wheel_set_value(GtkColorWheel* color_wheel, guint32 index, double value);
void gtk_color_wheel_set_hue(GtkColorWheel* color_wheel, guint32 index, double hue);
void gtk_color_wheel_set_saturation(GtkColorWheel* color_wheel, guint32 index, double saturation);
void gtk_color_wheel_set_selected(GtkColorWheel* color_wheel, guint32 index);

double gtk_color_wheel_get_hue(GtkColorWheel* color_wheel, guint32 index);
double gtk_color_wheel_get_saturation(GtkColorWheel* color_wheel, guint32 index);
double gtk_color_wheel_get_value(GtkColorWheel* color_wheel, guint32 index);

void gtk_color_wheel_set_block_editable(GtkColorWheel* color_wheel, bool editable);
bool gtk_color_wheel_get_block_editable(GtkColorWheel* color_wheel);

void gtk_color_wheel_set_n_colors(GtkColorWheel* color_wheel, guint32 number_of_colors);

void gtk_color_wheel_set_color_wheel_type(GtkColorWheel *color_wheel, const ColorWheelType *color_wheel_type);

GType gtk_color_wheel_get_type(void);

G_END_DECLS


#endif /* COLOR_WHEEL_H_ */

