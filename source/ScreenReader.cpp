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


#include "ScreenReader.h"
#include "Rect2.h"


using namespace math;

struct ScreenReader{
	GdkPixbuf *pixbuf;
	GdkScreen *screen;
	Rect2<int> read_area;
};

struct ScreenReader* screen_reader_new(){
	struct ScreenReader* screen = new struct ScreenReader;
	screen->pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, false, 8, 75, 75);
	screen->screen = NULL;
	return screen;
}

void screen_reader_destroy(struct ScreenReader *screen) {
	if (screen->pixbuf) g_object_unref(screen->pixbuf);
	delete screen;
}

void screen_reader_add_rect(struct ScreenReader *screen, GdkScreen *gdk_screen, Rect2<int>& rect){
	if (screen->screen && (screen->screen == gdk_screen)){
		screen->read_area += rect;
	}else{
		screen->read_area += rect;
		screen->screen = gdk_screen;
	}
}

void screen_reader_reset_rect(struct ScreenReader *screen){
	screen->read_area = Rect2<int>();
	screen->screen = NULL;
}

void screen_reader_update_pixbuf(struct ScreenReader *screen, Rect2<int>* update_rect){
	if (!screen->screen) return;

	GdkWindow* root_window = gdk_screen_get_root_window(screen->screen);
	GdkColormap* colormap = gdk_screen_get_system_colormap(screen->screen);

	int left = screen->read_area.getX();
	int top = screen->read_area.getY();
	int width = screen->read_area.getWidth();
	int height = screen->read_area.getHeight();

	gdk_pixbuf_get_from_drawable(screen->pixbuf, root_window, colormap, left, top, 0, 0, width, height);
	*update_rect = screen->read_area;
}

GdkPixbuf* screen_reader_get_pixbuf(struct ScreenReader *screen){
	return screen->pixbuf;
}

