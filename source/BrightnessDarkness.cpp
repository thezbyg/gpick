/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#include "BrightnessDarkness.h"
#include "DragDrop.h"

#include "GlobalStateStruct.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "MathUtil.h"
#include "ColorRYB.h"
#include "gtk/ColorWidget.h"
#include "gtk/Range2D.h"
#include "uiColorInput.h"
#include "CopyPaste.h"
#include "Converter.h"
#include "DynvHelpers.h"

#include "gtk/LayoutPreview.h"
#include "layout/Layout.h"
#include "layout/Style.h"

#include "uiApp.h"

#include <gdk/gdkkeysyms.h>

#include <math.h>
#include <string.h>
#include <sstream>
#include <iostream>

using namespace std;
using namespace layout;

typedef struct BrightnessDarknessArgs{
	ColorSource source;

	GtkWidget* main;
	GtkWidget* statusbar;

	Color color;

	GtkWidget *brightness_darkness;
	GtkWidget *layout_view;

	System* layout_system;
	Layouts* layouts;

	struct dynvSystem *params;
	GlobalState* gs;
}BrightnessDarknessArgs;


static void calc(BrightnessDarknessArgs *args, bool preview, bool save_settings){
	double brightness = gtk_range_2d_get_x(GTK_RANGE_2D(args->brightness_darkness));
	double darkness = gtk_range_2d_get_y(GTK_RANGE_2D(args->brightness_darkness));

	if (save_settings){
		dynv_set_float(args->params, "brightness", brightness);
		dynv_set_float(args->params, "darkness", brightness);
	}

	Color color, hsl_orig, hsl, r;
	color_copy(&args->color, &color);
	color_rgb_to_hsl(&color, &hsl_orig);

	Box* box;
	char tmp[4];
	for (int i = 1; i <= 4; i++){
		color_copy(&hsl_orig, &hsl);
		hsl.hsl.lightness = mix_float(hsl.hsl.lightness, mix_float(hsl.hsl.lightness, 1, brightness), i / 4.0); //clamp_float(hsl.hsl.lightness + brightness / 8.0 * i, 0, 1);
        color_hsl_to_rgb(&hsl, &r);

		snprintf(tmp, sizeof(tmp), "b%d", i);
		box = args->layout_system->GetNamedBox(tmp);
		if (box && box->style){
			color_copy(&r, &box->style->color);
		}


		color_copy(&hsl_orig, &hsl);
		hsl.hsl.lightness = mix_float(hsl.hsl.lightness, mix_float(hsl.hsl.lightness, 0, darkness), i / 4.0); //clamp_float(hsl.hsl.lightness - darkness / 8.0 * i, 0, 1);
        color_hsl_to_rgb(&hsl, &r);

		snprintf(tmp, sizeof(tmp), "c%d", i);
		box = args->layout_system->GetNamedBox(tmp);
		if (box && box->style){
			color_copy(&r, &box->style->color);
		}
	}


	gtk_widget_queue_draw(GTK_WIDGET(args->layout_view));

}

static void update(GtkWidget *widget, BrightnessDarknessArgs *args ){
	calc(args, true, false);
}

static int source_get_color(BrightnessDarknessArgs *args, struct ColorObject** color){
	Color c;
	if (gtk_layout_preview_get_current_color(GTK_LAYOUT_PREVIEW(args->layout_view), &c) == 0){
		*color = color_list_new_color_object(args->gs->colors, &c);

		string name = color_names_get(args->gs->color_names, &c, dynv_get_bool_wd(args->gs->params, "gpick.color_names.imprecision_postfix", true));
		dynv_set_string((*color)->params, "name", name.c_str());
		return 0;
	}
	return -1;
}

static int source_set_color(BrightnessDarknessArgs *args, struct ColorObject* color){
	Color c;
	color_object_get_color(color, &c);
	color_copy(&c, &args->color);
	gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(args->layout_view), &c, "main");
    calc(args, true, false);
	return 0;
}

static struct ColorObject* get_color_object(struct DragDrop* dd){
	BrightnessDarknessArgs* args = (BrightnessDarknessArgs*)dd->userdata;
	struct ColorObject* colorobject;
	if (source_get_color(args, &colorobject) == 0){
		return colorobject;
	}
	return 0;
}

static int set_color_object_at(struct DragDrop* dd, struct ColorObject* colorobject, int x, int y, bool move){
	BrightnessDarknessArgs* args=(BrightnessDarknessArgs*)dd->userdata;
	Color color;
	color_object_get_color(colorobject, &color);
	color_copy(&color, &args->color);
	gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(args->layout_view), &color, "main");

    calc(args, true, false);

	return 0;
}

static bool test_at(struct DragDrop* dd, int x, int y){
	BrightnessDarknessArgs* args=(BrightnessDarknessArgs*)dd->userdata;
	gtk_layout_preview_set_focus_named(GTK_LAYOUT_PREVIEW(args->layout_view), "main");
	return gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(args->layout_view));
}




static void edit_cb(GtkWidget *widget,  gpointer item) {
	BrightnessDarknessArgs* args=(BrightnessDarknessArgs*)item;

	struct ColorObject *color_object;
	struct ColorObject* new_color_object = 0;
	if (source_get_color(args, &color_object)==0){
		if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), args->gs, color_object, &new_color_object )==0){
			source_set_color(args, new_color_object);
			color_object_release(new_color_object);
		}
		color_object_release(color_object);
	}
}

static void paste_cb(GtkWidget *widget, BrightnessDarknessArgs* args) {
	struct ColorObject* color_object;
	if (copypaste_get_color_object(&color_object, args->gs)==0){
		source_set_color(args, color_object);
		color_object_release(color_object);
	}
}

static void add_to_palette_cb(GtkWidget *widget,  gpointer item) {
	BrightnessDarknessArgs* args=(BrightnessDarknessArgs*)item;

	struct ColorObject *color_object;
	if (source_get_color(args, &color_object)==0){
		dynv_set_string(color_object->params, "name", "");
		color_list_add_color_object(args->gs->colors, color_object, 1);
		color_object_release(color_object);
	}
}

static void add_all_to_palette_cb(GtkWidget *widget, BrightnessDarknessArgs *args) {

	struct ColorObject *color_object;

	for (list<Style*>::iterator i = args->layout_system->styles.begin(); i != args->layout_system->styles.end(); i++){

		color_object = color_list_new_color_object(args->gs->colors, &(*i)->color);
		dynv_set_string(color_object->params, "name", (*i)->ident_name.c_str());
		color_list_add_color_object(args->gs->colors, color_object, 1);
		color_object_release(color_object);
	}
}

static gboolean button_press_cb (GtkWidget *widget, GdkEventButton *event, BrightnessDarknessArgs* args) {
	GtkWidget *menu;

	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS){
		struct ColorObject *color_object;

		if (source_get_color(args, &color_object)==0){
			color_list_add_color_object(args->gs->colors, color_object, 1);
			color_object_release(color_object);
		}

		return true;

	}else if (event->button == 3 && event->type == GDK_BUTTON_PRESS){

		GtkWidget* item ;
		gint32 button, event_time;

		menu = gtk_menu_new ();

		bool selection_avail = gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(args->layout_view));
		bool edit_avail = gtk_layout_preview_is_editable(GTK_LAYOUT_PREVIEW(args->layout_view));

	    item = gtk_menu_item_new_with_image ("_Add to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_to_palette_cb), args);
		if (!selection_avail) gtk_widget_set_sensitive(item, false);

	    item = gtk_menu_item_new_with_image ("_Add all to palette", gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_all_to_palette_cb), args);

	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

	    item = gtk_menu_item_new_with_mnemonic ("_Copy to clipboard");
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		if (selection_avail){
			struct ColorObject* color_object;
			source_get_color(args, &color_object);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), converter_create_copy_menu (color_object, 0, args->gs));
			color_object_release(color_object);
		}else{
			gtk_widget_set_sensitive(item, false);
		}

		if (edit_avail){

			gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

			item = gtk_menu_item_new_with_image ("_Edit...", gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (edit_cb), args);
			if (!selection_avail) gtk_widget_set_sensitive(item, false);

			item = gtk_menu_item_new_with_image ("_Paste", gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (paste_cb), args);
			if (!selection_avail) gtk_widget_set_sensitive(item, false);

			if (copypaste_is_color_object_available(args->gs)!=0){
				gtk_widget_set_sensitive(item, false);
			}

		}

		gtk_widget_show_all (GTK_WIDGET(menu));

		button = event->button;
		event_time = event->time;

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, event_time);

		g_object_ref_sink(menu);
		g_object_unref(menu);

		return TRUE;
	}
	return FALSE;
}



static int source_destroy(BrightnessDarknessArgs *args){

	if (args->layout_system) System::unref(args->layout_system);
	args->layout_system = 0;

	dynv_system_release(args->params);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static int set_rgb_color(BrightnessDarknessArgs *args, struct ColorObject* color, uint32_t color_index){
	color_object_get_color(color, &args->color);
	update(0, args);
	return 0;
}

static int source_activate(BrightnessDarknessArgs *args){

	transformation::Chain *chain = static_cast<transformation::Chain*>(dynv_get_pointer_wdc(args->gs->params, "TransformationChain", 0));
	gtk_layout_preview_set_transformation_chain(GTK_LAYOUT_PREVIEW(args->layout_view), chain);

	gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "empty"), "");
	return 0;
}

static int source_deactivate(BrightnessDarknessArgs *args){
	dynv_set_color(args->params, "color", &args->color);
	calc(args, true, true);
	return 0;
}

static ColorSource* source_implement(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace){
	BrightnessDarknessArgs* args = new BrightnessDarknessArgs;

	args->params = dynv_system_ref(dynv_namespace);
	args->statusbar = (GtkWidget*)dynv_get_pointer_wd(gs->params, "StatusBar", 0);

	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;

	Layouts* layouts = (Layouts*)dynv_get_pointer_wd(gs->params, "Layouts", 0);
	args->layouts = layouts;
	args->layout_system = 0;

	GtkWidget *hbox, *widget;

	hbox = gtk_hbox_new(FALSE, 0);


	struct DragDrop dd;
	dragdrop_init(&dd, gs);

	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;
	dd.test_at = test_at;
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);

	args->brightness_darkness = widget = gtk_range_2d_new();
	gtk_range_2d_set_values(GTK_RANGE_2D(widget), dynv_get_float_wd(dynv_namespace, "brightness", 0.5), dynv_get_float_wd(dynv_namespace, "darkness", 0.5));
	gtk_range_2d_set_axis(GTK_RANGE_2D(widget), "Brightness", "Darkness");
	g_signal_connect(G_OBJECT(widget), "values_changed", G_CALLBACK(update), args);
    gtk_box_pack_start(GTK_BOX(hbox), widget, false, false, 0);

	args->layout_view = widget = gtk_layout_preview_new();
	g_signal_connect_after(G_OBJECT(widget), "button-press-event", G_CALLBACK(button_press_cb), args);
    gtk_box_pack_start(GTK_BOX(hbox), widget, false, false, 0);


	//setup drag&drop

	gtk_drag_dest_set( widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->colors->params);
	dd.userdata2 = (void*)-1;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	args->gs = gs;


	System* layout_system = layouts_get(args->layouts, "std_layout_brightness_darkness");
	gtk_layout_preview_set_system(GTK_LAYOUT_PREVIEW(args->layout_view), layout_system);
	if (args->layout_system) System::unref(args->layout_system);
	args->layout_system = layout_system;

	Color c;
	color_set(&c, 0.5);
	Color *color = dynv_get_color_wdc(dynv_namespace, "color", &c);

	color_copy(color, &args->color);
	gtk_layout_preview_set_color_named(GTK_LAYOUT_PREVIEW(args->layout_view), color, "main");
	calc(args, true, false);

	gtk_widget_show_all(hbox);

	update(0, args);

	args->main = hbox;

	args->source.widget = hbox;

	return (ColorSource*)args;
}

int brightness_darkness_source_register(ColorSourceManager *csm){
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "brightness_darkness", "Brightness Darkness");
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, struct dynvSystem *dynv_namespace))source_implement;
	color_source_manager_add_source(csm, color_source);
	return 0;
}



