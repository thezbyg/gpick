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
	void  (* active_color_changed)(GtkWidget* widget, gint32 active_color, gpointer userdata);
	void  (* color_changed)(GtkWidget* widget, gpointer userdata);
	void  (* color_activated)(GtkWidget* widget, gpointer userdata);
	void  (* center_activated)(GtkWidget* widget, gpointer userdata);
}GtkColorWheelClass;

GtkWidget* gtk_color_wheel_new(void);

typedef enum{
	HSL_RGB,
	HSL_RYB,
}ColorWheelSpace;

void gtk_color_wheel_set_color_space(GtkColorWheel *color_wheel, ColorWheelSpace color_space);
void gtk_color_wheel_set_color(GtkColorWheel *color_wheel, guint32 index, double hue, double saturation, double lightness );

GType gtk_color_wheel_get_type(void);

G_END_DECLS


#endif /* COLOR_WHEEL_H_ */

