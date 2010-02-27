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

#ifndef COLORCELL_H_
#define COLORCELL_H_

#include <gtk/gtk.h>
#include "../ColorObject.h"

#define CUSTOM_TYPE_CELL_RENDERER_COLOR				(custom_cell_renderer_color_get_type())
#define CUSTOM_CELL_RENDERER_COLOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_COLOR, CustomCellRendererColor))
#define CUSTOM_CELL_RENDERER_COLOR_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST ((obj),  CUSTOM_TYPE_CELL_RENDERER_COLOR, CustomCellRendererColorClass))
#define CUSTOM_IS_CELL_COLOR_COLOR(obj)				(G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_COLOR))
#define CUSTOM_IS_CELL_COLOR_COLOR_CLASS(obj)		(G_TYPE_CHECK_CLASS_TYPE ((obj),  CUSTOM_TYPE_CELL_RENDERER_COLOR))
#define CUSTOM_CELL_RENDERER_COLOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_COLOR, CustomCellRendererColorClass))

typedef struct _CustomCellRendererColor CustomCellRendererColor;
typedef struct _CustomCellRendererColorClass CustomCellRendererColorClass;

struct _CustomCellRendererColor
{
	GtkCellRenderer	parent;
	struct ColorObject*	color;
	int width;
	int height;
};

struct _CustomCellRendererColorClass
{
GtkCellRendererClass  parent_class;
};

GType custom_cell_renderer_color_get_type (void);
GtkCellRenderer *custom_cell_renderer_color_new (void);

void custom_cell_renderer_color_set_size(GtkCellRenderer *cell,  gint width, gint height);


#endif /* COLORCELL_H_ */
