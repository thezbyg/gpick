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

#include "ScreenReader.h"
#include <gtk/gtk.h>
#include <algorithm>
#include <iostream>
struct ScreenReader {
	cairo_surface_t *surface;
	int maxSize;
	GdkScreen *screen;
	math::Rectangle<int> readArea;
};
struct ScreenReader *screen_reader_new() {
	ScreenReader *screen = new ScreenReader;
	screen->maxSize = 0;
	screen->surface = 0;
	screen->screen = 0;
	return screen;
}
void screen_reader_destroy(ScreenReader *screen) {
	if (screen->surface) cairo_surface_destroy(screen->surface);
	delete screen;
}
void screen_reader_add_rect(ScreenReader *screen, GdkScreen *gdkScreen, math::Rectangle<int> &rect) {
	if (screen->screen && (screen->screen == gdkScreen)) {
		screen->readArea += rect;
	} else {
		screen->readArea += rect;
		screen->screen = gdkScreen;
	}
}
void screen_reader_reset_rect(ScreenReader *screen) {
	screen->readArea = math::Rectangle<int>();
	screen->screen = NULL;
}
void screen_reader_update_surface(ScreenReader *screen, math::Rectangle<int> *updateRect) {
	if (!screen->screen) return;
	int left = screen->readArea.getX();
	int top = screen->readArea.getY();
	int width = screen->readArea.getWidth();
	int height = screen->readArea.getHeight();
	if (width > screen->maxSize || height > screen->maxSize) {
		if (screen->surface) cairo_surface_destroy(screen->surface);
		screen->maxSize = (std::max(width, height) / 150 + 1) * 150;
		screen->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, screen->maxSize, screen->maxSize);
	}
	GdkWindow *rootWindow = gdk_screen_get_root_window(screen->screen);
	cairo_t *rootCairo = gdk_cairo_create(rootWindow);
	cairo_surface_t *rootSurface = cairo_get_target(rootCairo);
	if (cairo_surface_status(rootSurface) != CAIRO_STATUS_SUCCESS) {
		std::cerr << "can not get root window surface" << std::endl;
		return;
	}
	cairo_surface_mark_dirty_rectangle(rootSurface, left, top, width, height);
	cairo_t *cr = cairo_create(screen->surface);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, rootSurface, -left, -top);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);
	cairo_destroy(cr);
	cairo_destroy(rootCairo);
	*updateRect = screen->readArea;
}
cairo_surface_t *screen_reader_get_surface(ScreenReader *screen) {
	return screen->surface;
}
