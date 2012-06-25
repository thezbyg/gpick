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

#ifndef RANGE_2D_H_
#define RANGE_2D_H_


#include <gtk/gtk.h>
#include "../Color.h"


G_BEGIN_DECLS

#define GTK_TYPE_RANGE_2D		(gtk_range_2d_get_type ())
#define GTK_RANGE_2D(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_RANGE_2D, GtkRange2D))
#define GTK_RANGE_2D_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GTK_RANGE_2D, GtkRange2DClass))
#define GTK_IS_RANGE_2D(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_RANGE_2D))
#define GTK_IS_RANGE_2D_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GTK_TYPE_RANGE_2D))
#define GTK_RANGE_2D_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_RANGE_2D, GtkRange2DClass))

typedef struct GtkRange2D			GtkRange2D;
typedef struct GtkRange2DClass		GtkRange2DClass;

typedef gpointer GtkRange2DObject;

typedef struct GtkRange2D
{
	GtkDrawingArea parent;

	/* < private > */
}GtkRange2D;

typedef struct GtkRange2DClass{
	GtkDrawingAreaClass parent_class;
	void  (*values_changed)(GtkWidget *widget, gpointer userdata);
}GtkRange2DClass;

GtkWidget* gtk_range_2d_new(void);

void gtk_range_2d_set_values(GtkRange2D *range_2d, double x, double y);
void gtk_range_2d_set_axis(GtkRange2D *range_2d, const char *x, const char *y);

double gtk_range_2d_get_x(GtkRange2D *range_2d);
double gtk_range_2d_get_y(GtkRange2D *range_2d);

GType gtk_range_2d_get_type(void);

G_END_DECLS


#endif /* RANGE_2D_H_ */


