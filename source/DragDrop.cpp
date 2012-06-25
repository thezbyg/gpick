/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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
#include "dynv/DynvXml.h"

#include <string.h>
#include <iostream>
#include <sstream>

using namespace std;

enum {
	TARGET_STRING = 1,
	TARGET_ROOTWIN,
	TARGET_COLOR,
	TARGET_COLOR_OBJECT_LIST,
	TARGET_COLOR_OBJECT_LIST_SERIALIZED,
};

static GtkTargetEntry targets[] = {
	{ (char*)"colorobject-list", GTK_TARGET_SAME_APP, TARGET_COLOR_OBJECT_LIST },
	{ (char*)"application/x-colorobject-list", GTK_TARGET_OTHER_APP, TARGET_COLOR_OBJECT_LIST_SERIALIZED },
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
	dd->drag_end = 0;
	dd->get_color_object_list = 0;
	dd->set_color_object_list_at = 0;

	dd->handler_map = 0;
	dd->data_type = DragDrop::DATA_TYPE_NONE;
	memset(&dd->data, 0, sizeof(dd->data));
	dd->widget = 0;
	dd->gs = gs;
	dd->dragwidget = 0;

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
		case TARGET_COLOR_OBJECT_LIST:
			{
				struct ColorList{
					uint64_t color_object_n;
					struct ColorObject* color_object;
				}data;

				memcpy(&data, selection_data->data, sizeof(data));

				if (data.color_object_n > 1){
					struct ColorObject **color_objects = new struct ColorObject*[data.color_object_n];
					memcpy(color_objects, selection_data->data + offsetof(ColorList, color_object), sizeof(struct ColorObject*) * data.color_object_n);

					if (dd->set_color_object_list_at)
						dd->set_color_object_list_at(dd, color_objects, data.color_object_n, x, y, context->action & GDK_ACTION_MOVE );
					else if (dd->set_color_object_at)
						dd->set_color_object_at(dd, data.color_object, x, y, context->action & GDK_ACTION_MOVE );

				}else{
					if (dd->set_color_object_at)
						dd->set_color_object_at(dd, data.color_object, x, y, context->action & GDK_ACTION_MOVE );
				}

			}
			success = true;
			break;

		case TARGET_COLOR_OBJECT_LIST_SERIALIZED:
			{
				char *buffer = new char [selection_data->length + 1];
				buffer[selection_data->length] = 0;
				memcpy(buffer, selection_data->data, selection_data->length);
				stringstream str(buffer);
				delete [] buffer;
				struct dynvSystem *params = dynv_system_create(dd->handler_map);
				dynv_xml_deserialize(params, str);

				uint32_t color_n = 0;
				struct dynvSystem **colors = (struct dynvSystem**)dynv_get_dynv_array_wd(params, "colors", 0, 0, &color_n);
				if (color_n > 0 && colors){
					if (color_n > 1){
						if (dd->set_color_object_list_at){
							struct ColorObject **color_objects = new struct ColorObject*[color_n];
							for (uint32_t i = 0; i < color_n; i++){
								color_objects[i] = color_object_new(NULL);
								color_objects[i]->params = dynv_system_ref(colors[i]);
								color_objects[i] = color_object_copy(color_objects[i]);
							}
							dd->set_color_object_list_at(dd, color_objects, color_n, x, y, false);
							for (uint32_t i = 0; i < color_n; i++){
								color_object_release(color_objects[i]);
							}
							delete [] color_objects;
						}else	if (dd->set_color_object_at){
							struct ColorObject* color_object = color_object_new(NULL);
							color_object->params = dynv_system_ref(colors[0]);
							dd->set_color_object_at(dd, color_object, x, y, false);
							color_object_release(color_object);
						}
					}else{
						if (dd->set_color_object_at){
							struct ColorObject* color_object = color_object_new(NULL);
							color_object->params = dynv_system_ref(colors[0]);
							dd->set_color_object_at(dd, color_object, x, y, false);
							color_object_release(color_object);
						}
					}
				}
				if (colors){
					for (uint32_t i = 0; i < color_n; i++){
						dynv_system_release(colors[i]);
					}
					delete [] colors;
				}

				/*
				if (data.color_object_n > 1){
					memcpy(color_objects, selection_data->data + offsetof(ColorList, color_object), sizeof(struct ColorObject*) * data.color_object_n);

					if (dd->set_color_object_list_at)
					else if (dd->set_color_object_at)
						dd->set_color_object_at(dd, data.color_object, x, y, context->action & GDK_ACTION_MOVE );

				}else{
				} */

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
	struct DragDrop *dd = (struct DragDrop*)user_data;

	GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);

	if (target != GDK_NONE){
		gtk_drag_get_data(widget, context, target, time);
		if (dd->drag_end)
			dd->drag_end(dd, widget, context);
		return TRUE;
	}
	if (dd->drag_end)
		dd->drag_end(dd, widget, context);
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

		if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECT){

			struct ColorObject* color_object = dd->data.color_object.color_object;
			if (!color_object) return;
			Color color;

			switch (target_type){
			case TARGET_COLOR_OBJECT_LIST:
				{
					struct{
						uint64_t color_object_n;
						struct ColorObject* color_object;
					}data;
					data.color_object_n = 1;
					data.color_object = color_object;
					gtk_selection_data_set(selection_data, gdk_atom_intern("colorobject", TRUE), 8, (guchar *)&data, sizeof(data));
				}
				break;

			case TARGET_COLOR_OBJECT_LIST_SERIALIZED:
				{
					struct dynvSystem *params = dynv_system_create(dd->handler_map);
					struct dynvSystem **colors = new struct dynvSystem*[1];
					colors[0] = color_object->params;
					dynv_set_dynv_array(params, "colors", (const dynvSystem**)colors, 1);
					delete [] colors;

					stringstream str;
					str << "<?xml version=\"1.0\" encoding='UTF-8'?><root>" << endl;
					dynv_xml_serialize(params, str);
					str << "</root>" << endl;
					string xml_data = str.str();

					gtk_selection_data_set(selection_data, gdk_atom_intern("application/x-colorobject-list", TRUE), 8, (guchar *)xml_data.c_str(), xml_data.length());
				}
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
				break;

			case TARGET_ROOTWIN:
				g_print ("Dropped on the root window!\n");
				break;

			default:
				g_assert_not_reached ();
			}
		}else if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECTS){

			struct ColorObject** color_objects = dd->data.color_objects.color_objects;
			uint32_t color_object_n = dd->data.color_objects.color_object_n;
			if (!color_objects) return;
			Color color;

			switch (target_type){
			case TARGET_COLOR_OBJECT_LIST:
				{
					struct ColorList{
						uint64_t color_object_n;
						struct ColorObject* color_object[1];
					};
					uint32_t data_length = sizeof(uint64_t) + sizeof(struct ColorObject*) * color_object_n;
          struct ColorList *data = (struct ColorList*)new char [data_length];

					data->color_object_n = color_object_n;
					memcpy(&data->color_object[0], color_objects, sizeof(struct ColorObject*) * color_object_n);

					gtk_selection_data_set(selection_data, gdk_atom_intern("colorobject", TRUE), 8, (guchar *)data, data_length);
					delete [] (char*)data;
				}
				break;

			case TARGET_COLOR_OBJECT_LIST_SERIALIZED:
				{
					struct dynvSystem *params = dynv_system_create(dd->handler_map);

					if (color_object_n > 0){
						struct dynvSystem **colors = new struct dynvSystem*[color_object_n];
						for (uint32_t i = 0; i < color_object_n; i++){
							colors[i] = color_objects[i]->params;
						}
						dynv_set_dynv_array(params, "colors", (const dynvSystem**)colors, color_object_n);
						delete [] colors;
					}

					stringstream str;
					str << "<?xml version=\"1.0\" encoding='UTF-8'?><root>" << endl;
					dynv_xml_serialize(params, str);
					str << "</root>" << endl;
					string xml_data = str.str();

					gtk_selection_data_set(selection_data, gdk_atom_intern("application/x-colorobject-list", TRUE), 8, (guchar *)xml_data.c_str(), xml_data.length());
				}
				break;

			case TARGET_STRING:
				{
					stringstream ss;
					for (uint32_t i = 0; i != color_object_n; i++){
						struct ColorObject *color_object = color_objects[i];

						color_object_get_color(color_object, &color);
						char* text = main_get_color_text(dd->gs, &color, COLOR_TEXT_TYPE_COPY);
						if (text){
							ss << text << endl;
							g_free(text);
						}
					}
					gtk_selection_data_set_text(selection_data, ss.str().c_str(), ss.str().length() + 1);
				}
				break;

			case TARGET_COLOR:
				{
					struct ColorObject *color_object = color_objects[0];

					color_object_get_color(color_object, &color);
					guint16 data_color[4];

					data_color[0] = int(color.rgb.red * 0xFFFF);
					data_color[1] = int(color.rgb.green * 0xFFFF);
					data_color[2] = int(color.rgb.blue * 0xFFFF);
					data_color[3] = 0xffff;

					gtk_selection_data_set (selection_data, gdk_atom_intern ("application/x-color", TRUE), 16, (guchar *)data_color, 8);
				}
				break;

			case TARGET_ROOTWIN:
				g_print ("Dropped on the root window!\n");
				break;

			default:
				g_assert_not_reached ();
			}



		}

	}
}

static void drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;

	if (dd->get_color_object_list){
		uint32_t color_object_n;
		struct ColorObject** color_objects = dd->get_color_object_list(dd, &color_object_n);
		if (color_objects){
			dd->data_type = DragDrop::DATA_TYPE_COLOR_OBJECTS;
			dd->data.color_objects.color_objects = color_objects;
			dd->data.color_objects.color_object_n = color_object_n;

			GtkWidget* dragwindow = gtk_window_new(GTK_WINDOW_POPUP);
			GtkWidget* hbox = gtk_vbox_new(true, 0);
			gtk_container_add(GTK_CONTAINER(dragwindow), hbox);
			gtk_window_resize(GTK_WINDOW(dragwindow), 164, 24 * std::min(color_object_n, (uint32_t)5));

			for (int i = 0; i < std::min(color_object_n, (uint32_t)5); i++){
				Color color;
				color_object_get_color(color_objects[i], &color);

				GtkWidget* colorwidget = gtk_color_new();
				char* text = main_get_color_text(dd->gs, &color, COLOR_TEXT_TYPE_DISPLAY);
				gtk_color_set_color(GTK_COLOR(colorwidget), &color, text);
				g_free(text);

				gtk_box_pack_start(GTK_BOX(hbox), colorwidget, true, true, 0);
			}

			gtk_drag_set_icon_widget(context, dragwindow, 0, 0);
			gtk_widget_show_all(dragwindow);

			dd->dragwidget = dragwindow;
			return;
		}
	}

	if (dd->get_color_object){
		struct ColorObject* color_object = dd->get_color_object(dd);
		if (color_object){
			dd->data_type = DragDrop::DATA_TYPE_COLOR_OBJECT;
			dd->data.color_object.color_object = color_object;

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
			return;
		}
	}
}

static void drag_end(GtkWidget *widget, GdkDragContext *context, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;

	if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECT){
		if (dd->data.color_object.color_object){
			color_object_release(dd->data.color_object.color_object);
			memset(&dd->data, 0, sizeof(dd->data));
		}
		dd->data_type = DragDrop::DATA_TYPE_NONE;
	}
	if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECTS){
		if (dd->data.color_objects.color_objects){
			for (uint32_t i = 0; i < dd->data.color_objects.color_object_n; i++){
				color_object_release(dd->data.color_objects.color_objects[i]);
			}
			delete [] dd->data.color_objects.color_objects;
			memset(&dd->data, 0, sizeof(dd->data));
		}
		dd->data_type = DragDrop::DATA_TYPE_NONE;
	}

	if (dd->dragwidget){
		gtk_widget_destroy(dd->dragwidget);
		dd->dragwidget = 0;
	}

	if (dd->drag_end)
		dd->drag_end(dd, widget, context);

}

static void drag_destroy(GtkWidget *widget, gpointer user_data){
	struct DragDrop *dd = (struct DragDrop*)user_data;
	dynv_handler_map_release(dd->handler_map);
	delete dd;
}

