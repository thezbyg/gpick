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

#include "DragDrop.h"
#include "gtk/ColorWidget.h"
#include "uiApp.h"

#include <string.h>

enum {
	TARGET_STRING = 1,
	TARGET_ROOTWIN,
	TARGET_COLOR,
	TARGET_COLOR_OBJECT,
};

static GtkTargetEntry targets[] = {
	{ (char*)"colorobject", GTK_TARGET_SAME_APP, TARGET_COLOR_OBJECT },
	{ (char*)"application/x-color", 0, TARGET_COLOR },
	{ (char*)"text/plain", 0, TARGET_STRING },
	{ (char*)"STRING",     0, TARGET_STRING },
	{ (char*)"application/x-rootwin-drop", 0, TARGET_ROOTWIN }
};

static guint n_targets = G_N_ELEMENTS (targets);

static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint target_type, guint time, gpointer data);
static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t, gpointer user_data);
static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data);
static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data);

static void drag_data_delete(GtkWidget *widget, GdkDragContext *context, gpointer user_data);
static void drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data);
static void drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data);
static void drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data);

static void drag_destroy(GtkWidget *widget, gpointer user_data);

int dragdrop_init(struct DragDrop* dd, GlobalState *gs){
	dd->get_color_object = 0;
	dd->set_color_object_at = 0;
	dd->test_at = 0;
	dd->data_received = 0;
	dd->data_get = 0;
	dd->data_delete = 0;
	
	dd->handler_map = 0;
	dd->color_object = 0;
	dd->widget = 0;
	dd->gs = gs;
	dd->dragwidget = 0;
	
	GtkWidget* widget;
	void* userdata;
	return 0;
}

int dragdrop_widget_attach(GtkWidget* widget, DragDropFlags flags, struct DragDrop *user_dd){

	struct DragDrop* dd=new struct DragDrop;
	memcpy(dd, user_dd, sizeof(struct DragDrop));
	dd->widget = widget;
	
	if (flags & DRAGDROP_SOURCE){
		GtkTargetList *target_list = gtk_drag_source_get_target_list(widget);
		if (target_list){
			gtk_target_list_add_table(target_list, targets, n_targets);
		}else{
			target_list = gtk_target_list_new(targets, n_targets);
			gtk_drag_source_set_target_list(widget, target_list);
		}
		g_signal_connect (widget, "drag-data-get", G_CALLBACK (drag_data_get), dd);
		g_signal_connect (widget, "drag-data-delete", G_CALLBACK (drag_data_delete), dd);
		g_signal_connect (widget, "drag-begin", G_CALLBACK (drag_begin), dd);
		g_signal_connect (widget, "drag-end", G_CALLBACK (drag_end), dd);
	}
	
	if (flags & DRAGDROP_DESTINATION){
		GtkTargetList *target_list = gtk_drag_dest_get_target_list(widget);
		if (target_list){
			gtk_target_list_add_table(target_list, targets, n_targets);
		}else{
			target_list = gtk_target_list_new(targets, n_targets);
			gtk_drag_dest_set_target_list(widget, target_list);
		}
		g_signal_connect (widget, "drag-data-received", G_CALLBACK(drag_data_received), dd);
		g_signal_connect (widget, "drag-leave", G_CALLBACK (drag_leave), dd);
		g_signal_connect (widget, "drag-motion", G_CALLBACK (drag_motion), dd);
		g_signal_connect (widget, "drag-drop", G_CALLBACK (drag_drop), dd);	
	}
	
	g_signal_connect (widget, "destroy", G_CALLBACK (drag_destroy), dd);
	
	return 0;
}

static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data){
	bool success = false;

	if ((selection_data != NULL) && (selection_data->length >= 0)){
		
		struct DragDrop *dd = (struct DragDrop*)user_data;

		if (dd->data_received){
			success = dd->data_received(dd, widget, context, x, y, selection_data, target_type, time);				
		}

		if (!success)
		switch (target_type){
		case TARGET_COLOR_OBJECT:
			{
				struct ColorObject* color_object;
				memcpy(&color_object, selection_data->data, sizeof(struct ColorObject*));
				dd->set_color_object_at(dd, color_object, x, y, context->action & GDK_ACTION_MOVE );
				color_object_release(color_object);
			}
			success = true;
			break;
	
		case TARGET_STRING:
			{
				gchar* data = (gchar*)selection_data->data;
				if (data[selection_data->length]!=0) break;	//not null terminated
				
				Color color;
				
				if (main_get_color_from_text(dd->gs, data, &color)!=0){
					gtk_drag_finish (context, false, false, time);
					return;
				}
				
				struct ColorObject* color_object = color_object_new(dd->handler_map);
				color_object_set_color(color_object, &color);
				dd->set_color_object_at(dd, color_object, x, y, context->action & GDK_ACTION_MOVE );
				color_object_release(color_object);
			}
			success = true;
			break;
			
		case TARGET_COLOR:
			{
				guint16* data = (guint16*)selection_data->data;

				Color color;

				color.rgb.red = data[0] / (double)0xFFFF;
				color.rgb.green = data[1] / (double)0xFFFF;				
				color.rgb.blue = data[2] / (double)0xFFFF;

				struct ColorObject* color_object = color_object_new(dd->handler_map);
				color_object_set_color(color_object, &color);
				dd->set_color_object_at(dd, color_object, x, y, context->action & GDK_ACTION_MOVE );
				color_object_release(color_object);
			}
			success = true;
			break;
			
		default:
			g_assert_not_reached ();
		}

	}
	
	gtk_drag_finish (context, success, (context->action==GDK_ACTION_MOVE), time);
}


static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data){
	
	struct DragDrop *dd = (struct DragDrop*)user_data;
	
	if (!dd->test_at){
		GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
		if (target){
			gdk_drag_status(context, context->action, time);
		}else{
			gdk_drag_status(context, GdkDragAction(0), time);
		}
		return TRUE;
	}

	if (dd->test_at(dd, x, y)){
		GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
		if (target){
			gdk_drag_status(context, context->action, time);
		}else{
			gdk_drag_status(context, GdkDragAction(0), time);
		}
	}else{
		gdk_drag_status(context, GdkDragAction(0), time);
	}
	return TRUE;
}


static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, gpointer user_data){
}


static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, gpointer user_data){
	
	GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
	
	if (target != GDK_NONE){
		gtk_drag_get_data(widget, context, target, time);
		return TRUE;
	}
	return FALSE;
}




static void drag_data_delete(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;
	
	if (dd->data_delete){
		dd->data_delete(dd, widget, context);				
	}
}

static void drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint target_type, guint time, gpointer user_data){

	g_assert (selection_data != NULL);
	
	struct DragDrop *dd = (struct DragDrop*)user_data;
	
	bool success = false;
	
	if (dd->data_get){
		success = dd->data_get(dd, widget, context, selection_data, target_type, time);				
	}
	
	if (!success){
		
		struct ColorObject* color_object = dd->color_object;
		if (!color_object) return;
		Color color;
	
		switch (target_type){
		case TARGET_COLOR_OBJECT:
			gtk_selection_data_set (selection_data, gdk_atom_intern ("colorobject", TRUE), 8, (guchar *)&color_object, sizeof(struct ColorObject*));
			break;
			
		case TARGET_STRING:
			{
				color_object_get_color(color_object, &color);
				char* text = main_get_color_text(dd->gs, &color, COLOR_TEXT_TYPE_COPY);
				if (text){
					gtk_selection_data_set_text(selection_data, text, strlen(text)+1);
					g_free(text);
				}
			}
			color_object_release(color_object);
			break;
			
		case TARGET_COLOR:
			{
				color_object_get_color(color_object, &color);
				guint16 data_color[4];

				data_color[0] = int(color.rgb.red * 0xFFFF);
				data_color[1] = int(color.rgb.green * 0xFFFF);
				data_color[2] = int(color.rgb.blue * 0xFFFF);
				data_color[3] = 0xffff;
				
				gtk_selection_data_set (selection_data, gdk_atom_intern ("application/x-color", TRUE), 16, (guchar *)data_color, 8);
			}
			color_object_release(color_object);
			break;

		case TARGET_ROOTWIN:
			g_print ("Dropped on the root window!\n");
			color_object_release(color_object);
			break;

		default:
			color_object_release(color_object);
			g_assert_not_reached ();	
		}
	}
}

static void drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;
	
	struct ColorObject* color_object = dd->get_color_object(dd);
	if (color_object){
		dd->color_object = color_object_ref(color_object);
		
		GtkWidget* dragwindow = gtk_window_new(GTK_WINDOW_POPUP);
		GtkWidget* colorwidget = gtk_color_new();
		gtk_container_add(GTK_CONTAINER(dragwindow), colorwidget);
		gtk_window_resize(GTK_WINDOW(dragwindow), 164, 24); 
		
		Color color;
		color_object_get_color(color_object, &color);
		
		char* text = main_get_color_text(dd->gs, &color, COLOR_TEXT_TYPE_DISPLAY);
		gtk_color_set_color(GTK_COLOR(colorwidget), &color, text);
		g_free(text);
		
		gtk_drag_set_icon_widget(context, dragwindow, 0, 0);
		gtk_widget_show_all(dragwindow);
		
		dd->dragwidget = dragwindow;
	}
}

static void drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;
	
	if (dd->color_object){
		color_object_release(dd->color_object);
		dd->color_object = 0;
	}
	
	if (dd->dragwidget){
		gtk_widget_destroy(dd->dragwidget);
		dd->dragwidget = 0;
	}
	
}

static void drag_destroy(GtkWidget *widget, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;
	dynv_handler_map_release(dd->handler_map);
	delete dd;
}

