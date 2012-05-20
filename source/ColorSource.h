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

#ifndef COLORSOURCE_H_
#define COLORSOURCE_H_

#include "ColorObject.h"
#include "GlobalStateStruct.h"
#include <gtk/gtk.h>

typedef struct ColorSourceSlot{
	const char *identificator;
	const char *hr_name;

	uint32_t id;

	struct{
		bool read;
		bool write;
	}supports;
}ColorSourceSlot;

typedef struct ColorSource{
	char *identificator;
	char *hr_name;

	int (*set_color)(ColorSource *source, ColorObject *color);
	int (*get_color)(ColorSource *source, ColorObject **color);

	int (*set_nth_color)(ColorSource *source, uint32_t color_n, ColorObject *color);
	int (*get_nth_color)(ColorSource *source, uint32_t color_n, ColorObject **color);

	int (*activate)(ColorSource *source);
	int (*deactivate)(ColorSource *source);

	int (*query_slots)(ColorSource *source, ColorSourceSlot *slot);
	int (*set_slot_color)(ColorSource *source, uint32_t slot_id, ColorObject *color);

	ColorSource* (*implement)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace);
	int (*destroy)(ColorSource *source);

	bool single_instance_only;
	bool needs_viewport;
	int default_accelerator;

	GtkWidget *widget;
	void* userdata;
}ColorSource;

int color_source_init(ColorSource* source, const char *identificator, const char *name);

int color_source_activate(ColorSource *source);
int color_source_deactivate(ColorSource *source);

int color_source_set_color(ColorSource *source, ColorObject *color);
int color_source_set_nth_color(ColorSource *source, uint32_t color_n, ColorObject *color);
int color_source_get_color(ColorSource *source, ColorObject *color);
int color_source_get_nth_color(ColorSource *source, uint32_t color_n, ColorObject **color);

int color_source_get_default_accelerator(ColorSource *source);

ColorSource* color_source_implement(ColorSource* source, GlobalState *gs, struct dynvSystem *dynv_namespace);
GtkWidget* color_source_get_widget(ColorSource* source);
int color_source_destroy(ColorSource* source);

#endif /* COLORSOURCE_H_ */

