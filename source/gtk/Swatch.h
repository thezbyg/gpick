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

#ifndef GPICK_GTK_SWATCH_H_
#define GPICK_GTK_SWATCH_H_

#include <gtk/gtk.h>
#include "../Color.h"
#include "../transformation/Chain.h"
#define GTK_TYPE_SWATCH (gtk_swatch_get_type())
#define GTK_SWATCH(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_SWATCH, GtkSwatch))
#define GTK_SWATCH_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), GTK_SWATCH, GtkSwatchClass))
#define GTK_IS_SWATCH(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_SWATCH))
#define GTK_IS_SWATCH_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj), GTK_TYPE_SWATCH))
#define GTK_SWATCH_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_SWATCH, GtkSwatchClass))
struct GtkSwatch
{
	GtkDrawingArea parent;
};
struct GtkSwatchClass
{
	GtkDrawingAreaClass parent_class;
	void (*active_color_changed)(GtkWidget* widget, gint32 active_color, gpointer userdata);
	void (*color_changed)(GtkWidget* widget, gpointer userdata);
	void (*color_activated)(GtkWidget* widget, gpointer userdata);
	void (*center_activated)(GtkWidget* widget, gpointer userdata);
};
GtkWidget* gtk_swatch_new();
void gtk_swatch_set_color(GtkSwatch* swatch, guint32 index, Color* color);
void gtk_swatch_set_color(GtkSwatch* swatch, guint32 index, const Color &color);
void gtk_swatch_set_main_color(GtkSwatch* swatch, Color* color);
void gtk_swatch_set_active_index(GtkSwatch* swatch, guint32 index);
void gtk_swatch_set_active_color(GtkSwatch* swatch, Color* color);
void gtk_swatch_move_active(GtkSwatch* swatch, gint32 direction);
void gtk_swatch_set_color_to_main(GtkSwatch* swatch);
void gtk_swatch_get_color(GtkSwatch* swatch, guint32 index, Color* color);
void gtk_swatch_get_main_color(GtkSwatch* swatch, Color* color);
gint32 gtk_swatch_get_active_index(GtkSwatch* swatch);
void gtk_swatch_get_active_color(GtkSwatch* swatch, Color* color);
void gtk_swatch_set_color_count(GtkSwatch* swatch, gint32 colors);
gint gtk_swatch_get_color_at(GtkSwatch* swatch, gint x, gint y);
void gtk_swatch_set_transformation_chain(GtkSwatch* widget, transformation::Chain *chain);
void gtk_swatch_set_active(GtkSwatch* swatch, gboolean active);
GType gtk_swatch_get_type();

#endif /* GPICK_GTK_SWATCH_H_ */
