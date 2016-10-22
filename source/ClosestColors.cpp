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

#include "ClosestColors.h"
#include "ColorObject.h"
#include "ColorSource.h"
#include "ColorSourceManager.h"
#include "DragDrop.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "gtk/ColorWidget.h"
#include "uiColorInput.h"
#include "CopyPaste.h"
#include "DynvHelpers.h"
#include "Internationalisation.h"
#include "color_names/ColorNames.h"
#include "StandardMenu.h"
#include "Clipboard.h"
#include <gdk/gdkkeysyms.h>
#include <boost/format.hpp>
#include <sstream>
using namespace std;

struct ClosestColorsArgs
{
	ColorSource source;
	GtkWidget *main, *status_bar, *color, *last_focused_color, *color_previews;
	GtkWidget *closest_colors[9];
	dynvSystem *params;
	GlobalState* gs;
};

class ClosestColorsColorNameAssigner: public ToolColorNameAssigner
{
	protected:
		stringstream m_stream;
		const char *m_ident;
	public:
		ClosestColorsColorNameAssigner(GlobalState *gs):
			ToolColorNameAssigner(gs)
		{
		}
		void assign(ColorObject *color_object, const Color *color, const char *ident)
		{
			m_ident = ident;
			ToolColorNameAssigner::assign(color_object, color);
		}
		virtual std::string getToolSpecificName(ColorObject *color_object, const Color *color)
		{
			m_stream.str("");
			m_stream << color_names_get(m_gs->getColorNames(), color, false) << " " << _("closest color") << " " << m_ident;
			return m_stream.str();
		}
};
static int source_set_color(ClosestColorsArgs *args, ColorObject *color_object);
static void calc(ClosestColorsArgs *args, bool preview, bool save_settings)
{
	Color color;
	gtk_color_get_color(GTK_COLOR(args->color), &color);
	vector<pair<const char*, Color>> colors;
	color_names_find_nearest(args->gs->getColorNames(), color, 9, colors);
	for (size_t i = 0; i < 9; ++i){
		if (i < colors.size()){
			gtk_color_set_color(GTK_COLOR(args->closest_colors[i]), &colors[i].second, colors[i].first);
			gtk_widget_set_sensitive(args->closest_colors[i], true);
		}else{
			gtk_widget_set_sensitive(args->closest_colors[i], false);
		}
	}
}
static void update(GtkWidget *widget, ClosestColorsArgs *args)
{
	calc(args, true, false);
}
static void on_color_paste(GtkWidget *widget, ClosestColorsArgs *args)
{
	ColorObject *color_object;
	if (copypaste_get_color_object(&color_object, args->gs) == 0){
		source_set_color(args, color_object);
		color_object->release();
	}
}
static void on_color_edit(GtkWidget *widget, ClosestColorsArgs *args)
{
	GtkWidget* color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));
	Color c;
	gtk_color_get_color(GTK_COLOR(color_widget), &c);
	ColorObject* color_object = color_list_new_color_object(args->gs->getColorList(), &c);
	ColorObject* new_color_object = nullptr;
	if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), args->gs, color_object, &new_color_object) == 0){
		source_set_color(args, color_object);
		new_color_object->release();
	}
	color_object->release();
}
static boost::format format_ignore_arg_errors(const std::string &f_string)
{
	boost::format fmter(f_string);
	fmter.exceptions(boost::io::all_error_bits ^ (boost::io::too_many_args_bit | boost::io::too_few_args_bit));
	return fmter;
}
static string identify_color_widget(GtkWidget *widget, ClosestColorsArgs *args)
{
	if (args->color == widget){
		return _("target");
	}else for (int i = 0; i < 9; ++i){
		if (args->closest_colors[i] == widget){
			try{
				return (format_ignore_arg_errors(_("match %d")) % (i + 1)).str();
			}catch(const boost::io::format_error &e){
				return (format_ignore_arg_errors("match %d") % (i + 1)).str();
			}
		}
	}
	return "unknown";
}
static void add_color_to_palette(GtkWidget *color_widget, ClosestColorsColorNameAssigner &name_assigner, ClosestColorsArgs *args)
{
	Color c;
	ColorObject *color_object;
	string widget_ident;
	gtk_color_get_color(GTK_COLOR(color_widget), &c);
	color_object = color_list_new_color_object(args->gs->getColorList(), &c);
	widget_ident = identify_color_widget(color_widget, args);
	name_assigner.assign(color_object, &c, widget_ident.c_str());
	color_list_add_color_object(args->gs->getColorList(), color_object, 1);
	color_object->release();
}
static void on_color_add_to_palette(GtkWidget *widget, ClosestColorsArgs *args)
{
	GtkWidget *color_widget = GTK_WIDGET(g_object_get_data(G_OBJECT(widget), "color_widget"));
	ClosestColorsColorNameAssigner name_assigner(args->gs);
	add_color_to_palette(color_widget, name_assigner, args);
}
static void on_color_add_all_to_palette(GtkWidget *widget, ClosestColorsArgs *args)
{
	ClosestColorsColorNameAssigner name_assigner(args->gs);
	add_color_to_palette(args->color, name_assigner, args);
	for (int i = 0; i < 9; ++i){
		add_color_to_palette(args->closest_colors[i], name_assigner, args);
	}
}
static gboolean color_focus_in_cb(GtkWidget *widget, GdkEventFocus *event, ClosestColorsArgs *args)
{
	args->last_focused_color = widget;
	return false;
}
static void on_color_activate(GtkWidget *widget, ClosestColorsArgs *args)
{
	ClosestColorsColorNameAssigner name_assigner(args->gs);
	add_color_to_palette(widget, name_assigner, args);
}
static void color_show_menu(GtkWidget *widget, ClosestColorsArgs *args, GdkEventButton *event)
{
	GtkWidget *menu;
	GtkWidget* item;
	menu = gtk_menu_new();
	item = gtk_menu_item_new_with_image(_("_Add to palette"), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_color_add_to_palette), args);
	g_object_set_data(G_OBJECT(item), "color_widget", widget);
	item = gtk_menu_item_new_with_image(_("A_dd all to palette"), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_color_add_all_to_palette), args);
	g_object_set_data(G_OBJECT(item), "color_widget", widget);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	Color c;
	gtk_color_get_color(GTK_COLOR(widget), &c);
	ColorObject* color_object = color_list_new_color_object(args->gs->getColorList(), &c);
	StandardMenu::appendMenu(menu, color_object, args->gs);
	color_object->release();

	if (args->color == widget){
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		item = gtk_menu_item_new_with_image(_("_Edit..."), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_color_edit), args);
		g_object_set_data(G_OBJECT(item), "color_widget", widget);
		item = gtk_menu_item_new_with_image(_("_Paste"), gtk_image_new_from_stock(GTK_STOCK_PASTE, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_color_paste), args);
		g_object_set_data(G_OBJECT(item), "color_widget", widget);
		if (copypaste_is_color_object_available(args->gs) != 0){
			gtk_widget_set_sensitive(item, false);
		}
	}
	gtk_widget_show_all(GTK_WIDGET(menu));
	gint32 button, event_time;
	if (event){
		button = event->button;
		event_time = event->time;
	}else{
		button = 0;
		event_time = gtk_get_current_event_time();
	}
	gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, nullptr, nullptr, button, event_time);
	g_object_ref_sink(menu);
	g_object_unref(menu);
}
static gboolean on_color_button_press(GtkWidget *widget, GdkEventButton *event, ClosestColorsArgs *args)
{
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		color_show_menu(widget, args, event);
	}
	return false;
}
static void on_color_popup_menu(GtkWidget *widget, ClosestColorsArgs *args)
{
	color_show_menu(widget, args, nullptr);
}
static gboolean on_color_key_press(GtkWidget *widget, GdkEventKey *event, ClosestColorsArgs *args)
{
	guint modifiers = gtk_accelerator_get_default_mod_mask();
	Color c;
	ColorObject* color_object;
	GtkWidget* color_widget = widget;
	switch (event->keyval){
		case GDK_KEY_c:
			if ((event->state & modifiers) == GDK_CONTROL_MASK){
				gtk_color_get_color(GTK_COLOR(color_widget), &c);
				color_object = color_list_new_color_object(args->gs->getColorList(), &c);
				Clipboard::set(color_object, args->gs);
				color_object->release();
				return true;
			}
			return false;
			break;
		case GDK_KEY_v:
			if ((event->state & modifiers) == GDK_CONTROL_MASK){
				if (copypaste_get_color_object(&color_object, args->gs) == 0){
					source_set_color(args, color_object);
					color_object->release();
				}
				return true;
			}
			return false;
			break;
		default:
			return false;
		break;
	}
	return false;
}
static int source_destroy(ClosestColorsArgs *args)
{
	Color c;
	gtk_color_get_color(GTK_COLOR(args->color), &c);
	dynv_set_color(args->params, "color", &c);
	dynv_system_release(args->params);
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}
static int source_get_color(ClosestColorsArgs *args, ColorObject **color)
{
	ClosestColorsColorNameAssigner name_assigner(args->gs);
	Color c;
	string widget_ident;
	if (args->last_focused_color){
		gtk_color_get_color(GTK_COLOR(args->last_focused_color), &c);
		widget_ident = identify_color_widget(args->last_focused_color, args);
	}else{
		gtk_color_get_color(GTK_COLOR(args->closest_colors[0]), &c);
		widget_ident = identify_color_widget(args->closest_colors[0], args);
	}
	*color = color_list_new_color_object(args->gs->getColorList(), &c);
	name_assigner.assign(*color, &c, widget_ident.c_str());
	return 0;
}
static int source_set_color(ClosestColorsArgs *args, ColorObject *color_object)
{
	Color color = color_object->getColor();
	gtk_color_set_color(GTK_COLOR(args->color), &color, "");
	update(0, args);
	return 0;
}
static int source_activate(ClosestColorsArgs *args)
{
	auto chain = args->gs->getTransformationChain();
	gtk_color_set_transformation_chain(GTK_COLOR(args->color), chain);
	for (int i = 0; i < 9; ++i){
		gtk_color_set_transformation_chain(GTK_COLOR(args->closest_colors[i]), chain);
	}
	gtk_statusbar_push(GTK_STATUSBAR(args->status_bar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->status_bar), "empty"), "");
	return 0;
}
static int source_deactivate(ClosestColorsArgs *args)
{
	calc(args, true, true);
	return 0;
}
static ColorObject* get_color_object(DragDrop* dd)
{
	ClosestColorsArgs *args = static_cast<ClosestColorsArgs*>(dd->userdata);
	ColorObject* color_object;
	if (source_get_color(args, &color_object) == 0){
		return color_object;
	}
	return 0;
}
static int set_color_object_at(DragDrop* dd, ColorObject* color_object, int x, int y, bool move)
{
	ClosestColorsArgs *args = static_cast<ClosestColorsArgs*>(dd->userdata);
	source_set_color(args, color_object);
	return 0;
}
ColorSource* source_implement(ColorSource *source, GlobalState *gs, dynvSystem *dynv_namespace)
{
	ClosestColorsArgs *args = new ClosestColorsArgs;
	args->params = dynv_system_ref(dynv_namespace);
	args->status_bar = gs->getStatusBar();
	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;
	GtkWidget *table, *vbox, *hbox, *widget, *hbox2;
	hbox = gtk_hbox_new(false, 0);
	vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, true, true, 5);
	args->color_previews = gtk_table_new(3, 3, false);
	gtk_box_pack_start(GTK_BOX(vbox), args->color_previews, true, true, 0);
	DragDrop dd;
	dragdrop_init(&dd, gs);
	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;
	widget = gtk_color_new();
	gtk_color_set_rounded(GTK_COLOR(widget), true);
	gtk_color_set_hcenter(GTK_COLOR(widget), true);
	gtk_color_set_roundness(GTK_COLOR(widget), 5);
	gtk_table_attach(GTK_TABLE(args->color_previews), widget, 0, 3, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
	args->color = widget;
	g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(on_color_button_press), args);
	g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(on_color_activate), args);
	g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(on_color_key_press), args);
	g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(on_color_popup_menu), args);
	g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(color_focus_in_cb), args);
	gtk_widget_set_size_request(widget, 30, 30);

	//setup drag&drop
	gtk_drag_dest_set(widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set( widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dd.handler_map = dynv_system_get_handler_map(gs->getColorList()->params);
	dd.userdata2 = (void*)-1;
	dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);

	for (int i = 0; i < 3; ++i){
		for (int j = 0; j < 3; ++j){
			widget = gtk_color_new();
			gtk_color_set_rounded(GTK_COLOR(widget), true);
			gtk_color_set_hcenter(GTK_COLOR(widget), true);
			gtk_color_set_roundness(GTK_COLOR(widget), 5);

			gtk_table_attach(GTK_TABLE(args->color_previews), widget, i, i + 1, j + 1, j + 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
			args->closest_colors[i + j * 3] = widget;

			g_signal_connect(G_OBJECT(widget), "button-press-event", G_CALLBACK(on_color_button_press), args);
			g_signal_connect(G_OBJECT(widget), "activated", G_CALLBACK(on_color_activate), args);
			g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(on_color_key_press), args);
			g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(on_color_popup_menu), args);
			g_signal_connect(G_OBJECT(widget), "focus-in-event", G_CALLBACK(color_focus_in_cb), args);

			gtk_widget_set_size_request(widget, 30, 30);
			gtk_drag_source_set(widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
			dd.handler_map = dynv_system_get_handler_map(gs->getColorList()->params);
			dd.userdata2 = reinterpret_cast<void*>(i + j * 3);
			dragdrop_widget_attach(widget, DragDropFlags(DRAGDROP_SOURCE), &dd);
		}
	}

	Color c;
	color_set(&c, 0.5);
	gtk_color_set_color(GTK_COLOR(args->color), dynv_get_color_wdc(args->params, "color", &c), "");

	hbox2 = gtk_hbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, false, false, 0);

	table = gtk_table_new(5, 2, false);
	gtk_box_pack_start(GTK_BOX(hbox2), table, true, true, 0);

	args->gs = gs;
	gtk_widget_show_all(hbox);
	update(0, args);
	args->main = hbox;
	args->source.widget = hbox;
	return (ColorSource*)args;
}
int closest_colors_source_register(ColorSourceManager *csm)
{
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "closest_colors", _("Closest colors"));
	color_source->implement = source_implement;
	color_source->default_accelerator = GDK_KEY_c;
	color_source_manager_add_source(csm, color_source);
	return 0;
}
