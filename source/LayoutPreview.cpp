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

#include "LayoutPreview.h"
#include "DragDrop.h"
#include "uiColorInput.h"
#include "CopyPaste.h"

#include "uiUtilities.h"
#include "ColorList.h"
#include "MathUtil.h"

#include "gtk/LayoutPreview.h"
#include "layout/Layout.h"

#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;
using namespace layout;

struct Arguments{
	ColorSource source;
	
	GtkWidget* main;
	
	GtkWidget *layout;
	
	GlobalState* gs;
};



static int source_destroy(struct Arguments *args){
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static int source_get_color(struct Arguments *args, ColorObject** color){
	Color c;
	//gtk_color_get_color(GTK_COLOR(args->colors[0]), &c);
	*color = color_list_new_color_object(args->gs->colors, &c);
	return 0;
}

static int set_rgb_color(struct Arguments *args, struct ColorObject* color, uint32_t color_index){
	Color c;
	color_object_get_color(color, &c);
	return 0;
}
	
	
static int source_set_color(struct Arguments *args, struct ColorObject* color){
	return set_rgb_color(args, color, 0);
}

static int source_deactivate(struct Arguments *args){

	return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	struct Arguments* args=(struct Arguments*)dd->userdata;
	Color c;
	//gtk_color_get_color(GTK_COLOR(dd->widget), &c);
	struct ColorObject* colorobject = color_object_new(dd->handler_map);
	color_object_set_color(colorobject, &c);
	return colorobject;	
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	struct Arguments* args=(struct Arguments*)dd->userdata;
	//set_rgb_color(args, colorobject, (uintptr_t)dd->userdata2);
	return 0;
}

ColorSource* layout_preview_new(GlobalState* gs, GtkWidget **out_widget){
	struct Arguments* args=new struct Arguments;

	color_source_init(&args->source);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	
	GtkWidget *table, *vbox, *hbox, *widget;
	
	
	gint table_y;
	table = gtk_table_new(4, 4, false);
	table_y=0;
		
	//gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hue:",0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL),GTK_FILL,5,5);

	gtk_table_attach(GTK_TABLE(table), args->layout = gtk_layout_preview_new(),0,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GtkAttachOptions(GTK_FILL | GTK_EXPAND),0,0);
	table_y++;
	
	Layouts* layouts = (Layouts*)dynv_system_get(gs->params, "ptr", "Layouts");
	Box* box = layouts_get(layouts, "std_template_simple");
	//cout<<box<<" "<<layouts<<endl;
	gtk_layout_preview_set_box(GTK_LAYOUT_PREVIEW(args->layout), box);

	args->gs = gs;
	
	gtk_widget_show_all(table);
	
	//update(0, args);
	
	args->main = table;
	*out_widget = table;
	
	return (ColorSource*)args;
}

