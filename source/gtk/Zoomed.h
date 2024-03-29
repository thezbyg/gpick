/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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

#ifndef GPICK_GTK_ZOOMED_H_
#define GPICK_GTK_ZOOMED_H_

#include <gtk/gtk.h>
#include <cstdint>
#include "Color.h"
#include "math/Rectangle.h"
#include "math/Vector.h"

#define GTK_TYPE_ZOOMED (gtk_zoomed_get_type())
#define GTK_ZOOMED(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_ZOOMED, GtkZoomed))
#define GTK_ZOOMED_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST((obj), GTK_ZOOMED, GtkZoomedClass))
#define GTK_IS_ZOOMED(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_ZOOMED))
#define GTK_IS_ZOOMED_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((obj), GTK_TYPE_ZOOMED))
#define GTK_ZOOMED_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_ZOOMED, GtkZoomedClass))
struct GtkZoomed
{
	GtkDrawingArea parent;
};
struct GtkZoomedClass
{
	GtkDrawingAreaClass parent_class;
	void (* color_changed)(GtkWidget* widget, Color* c, gpointer userdata);
	void (*activated)(GtkWidget* widget, gpointer userdata);
};
GtkWidget* gtk_zoomed_new();
void gtk_zoomed_set_zoom(GtkZoomed* zoomed, gfloat zoom);
gfloat gtk_zoomed_get_zoom(GtkZoomed* zoomed);
void gtk_zoomed_set_fade(GtkZoomed* zoomed, bool fade);
int32_t gtk_zoomed_get_size(GtkZoomed *zoomed);
void gtk_zoomed_set_size(GtkZoomed *zoomed, int32_t width_height);
void gtk_zoomed_set_mark(GtkZoomed *zoomed, int index, math::Vector2i &position);
void gtk_zoomed_clear_mark(GtkZoomed *zoomed, int index);
void gtk_zoomed_update(GtkZoomed* zoomed, math::Vector2i &pointer, math::Rectangle<int>& screen_rect, math::Vector2i &offset, cairo_surface_t *surface);
void gtk_zoomed_get_screen_rect(GtkZoomed* zoomed, math::Vector2i &pointer, math::Rectangle<int>& screen_rect, math::Rectangle<int> *rect);
GType gtk_zoomed_get_type();

#endif /* GPICK_GTK_ZOOMED_H_ */
