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

#ifndef UIZOOMED_H_
#define UIZOOMED_H_


#include <gtk/gtk.h>
#include "Color.h"


G_BEGIN_DECLS

#define GTK_TYPE_ZOOMED		(gtk_zoomed_get_type ())
#define GTK_ZOOMED(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_ZOOMED, GtkZoomed))
#define GTK_ZOOMED_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GTK_ZOOMED, GtkZoomedClass))
#define GTK_IS_ZOOMED(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_ZOOMED))
#define GTK_IS_ZOOMED_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GTK_TYPE_ZOOMED))
#define GTK_ZOOMED_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_ZOOMED, GtkZoomedClass))

typedef struct GtkZoomed			GtkZoomed;
typedef struct GtkZoomedClass		GtkZoomedClass;

typedef gpointer GtkZoomedObject;

typedef struct GtkZoomed
{
	GtkDrawingArea parent;

	/* < private > */
}GtkZoomed;

typedef struct GtkZoomedClass
{
	GtkDrawingAreaClass parent_class;
	void  (* color_changed)(GtkWidget* widget, Color* c, gpointer userdata);
}GtkZoomedClass;


GtkWidget* gtk_zoomed_new ();

void gtk_zoomed_set_zoom (GtkZoomed* zoomed, gfloat zoom);

void gtk_zoomed_update (GtkZoomed* zoomed);

GType gtk_zoomed_get_type(void);

G_END_DECLS

#endif /* UIZOOMED_H_ */
