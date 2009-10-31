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

#ifndef GTK_LAYOUTPREVIEW_H_
#define GTK_LAYOUTPREVIEW_H_


#include <gtk/gtk.h>
#include "../ColorObject.h"
#include "../layout/Box.h"

G_BEGIN_DECLS

#define GTK_TYPE_LAYOUT_PREVIEW				(gtk_layout_preview_get_type ())
#define GTK_LAYOUT_PREVIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_LAYOUT_PREVIEW, GtkLayoutPreview))
#define GTK_LAYOUT_PREVIEW_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST ((obj), GTK_LAYOUT_PREVIEW, GtkLayoutPreviewClass))
#define GTK_IS_LAYOUT_PREVIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_LAYOUT_PREVIEW))
#define GTK_IS_LAYOUT_PREVIEW_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GTK_TYPE_LAYOUT_PREVIEW))
#define GTK_LAYOUT_PREVIEW_GET_CLASS		(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_LAYOUT_PREVIEW, GtkLayoutPreviewClass))

typedef struct GtkLayoutPreview				GtkLayoutPreview;
typedef struct GtkLayoutPreviewClass		GtkLayoutPreviewClass;

typedef gpointer GtkLayoutPreviewObject;

typedef struct GtkLayoutPreview
{
	GtkDrawingArea parent;

	/* < private > */
}GtkLayoutPreview;

typedef struct GtkLayoutPreviewClass
{
	GtkDrawingAreaClass parent_class;
	
	void  (* active_color_changed)(GtkWidget* widget, gint32 active_color, gpointer userdata);
	void  (* color_changed)(GtkWidget* widget, gpointer userdata);
	void  (* color_activated)(GtkWidget* widget, gpointer userdata);
}GtkLayoutPreviewClass;

GtkWidget* gtk_layout_preview_new(void);
GType gtk_layout_preview_get_type(void);

int gtk_layout_preview_set_box(GtkLayoutPreview* widget, layout::Box* box);

G_END_DECLS


#endif /* GTK_LAYOUTPREVIEW_H_ */
