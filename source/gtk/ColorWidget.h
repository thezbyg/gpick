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

#ifndef COLORWIDGET_H_
#define COLORWIDGET_H_


#include <gtk/gtk.h>
#include "../Color.h"


G_BEGIN_DECLS

#define GTK_TYPE_COLOR		(gtk_color_get_type ())
#define GTK_COLOR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_COLOR, GtkColor))
#define GTK_COLOR_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GTK_COLOR, GtkColorClass))
#define GTK_IS_COLOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_COLOR))
#define GTK_IS_COLOR_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GTK_TYPE_COLOR))
#define GTK_COLOR_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_COLOR, GtkColorClass))

typedef struct GtkColor			GtkColor;
typedef struct GtkColorClass		GtkColorClass;

typedef gpointer GtkColorObject;

typedef struct GtkColor
{
	GtkDrawingArea parent;

	/* < private > */
}GtkColor;

typedef struct GtkColorClass
{
	GtkDrawingAreaClass parent_class;
	
	void  (*activated)(GtkWidget* widget, gpointer userdata);
}GtkColorClass;

GtkWidget* gtk_color_new (void);
void gtk_color_set_color(GtkColor* widget, Color* color, gchar* text);
void gtk_color_get_color(GtkColor* widget, Color* color);
void gtk_color_set_rounded(GtkColor* widget, bool rounded_rectangle);
void gtk_color_set_hcenter(GtkColor* widget, bool hcenter);
GType gtk_color_get_type(void);

G_END_DECLS


#endif /* COLORWIDGET_H_ */
