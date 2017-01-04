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

#include "ScreenReader.h"
#include "Rect2.h"
#include <gtk/gtk.h>
#include <algorithm>
#include <iostream>
using namespace math;
using namespace std;

struct ScreenReader
{
	cairo_surface_t *surface;
	int max_size;
	GdkScreen *screen;
	Rect2<int> read_area;
};
struct ScreenReader* screen_reader_new()
{
	ScreenReader* screen = new ScreenReader;
	screen->max_size = 0;
	screen->surface = 0;
	screen->screen = 0;
	return screen;
}
void screen_reader_destroy(ScreenReader *screen)
{
	if (screen->surface) cairo_surface_destroy(screen->surface);
	delete screen;
}
void screen_reader_add_rect(ScreenReader *screen, GdkScreen *gdk_screen, Rect2<int>& rect)
{
	if (screen->screen && (screen->screen == gdk_screen)){
		screen->read_area += rect;
	}else{
		screen->read_area += rect;
		screen->screen = gdk_screen;
	}
}
void screen_reader_reset_rect(ScreenReader *screen)
{
	screen->read_area = Rect2<int>();
	screen->screen = NULL;
}
void screen_reader_update_surface(ScreenReader *screen, Rect2<int>* update_rect)
{
	if (!screen->screen) return;
	int left = screen->read_area.getX();
	int top = screen->read_area.getY();
	int width = screen->read_area.getWidth();
	int height = screen->read_area.getHeight();
	if (width > screen->max_size || height > screen->max_size){
		if (screen->surface) cairo_surface_destroy(screen->surface);
		screen->max_size = (std::max(width, height) / 150 + 1) * 150;
		screen->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, screen->max_size, screen->max_size);
	}
	GdkWindow* root_window = gdk_screen_get_root_window(screen->screen);
	cairo_t *root_cr = gdk_cairo_create(root_window);
	cairo_surface_t *root_surface = cairo_get_target(root_cr);
	if (cairo_surface_status(root_surface) != CAIRO_STATUS_SUCCESS){
		cerr << "can not get root window surface" << endl;
		return;
	}
	cairo_t *cr = cairo_create(screen->surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, root_surface, -left, -top);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	cairo_destroy(cr);
	cairo_destroy(root_cr);
	*update_rect = screen->read_area;
}
cairo_surface_t* screen_reader_get_surface(ScreenReader *screen)
{
	return screen->surface;
}
