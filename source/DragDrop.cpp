/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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
#include "ColorObject.h"
#include "dynv/Map.h"
#include "GlobalState.h"
#include "gtk/ColorWidget.h"
#include "Converters.h"
#include "Converter.h"
#include "common/Scoped.h"
#include <string.h>
#include <iostream>
#include <sstream>
enum class Target : guint {
	string = 1,
	color,
	colorObjectList,
	serializedColorObjectList,
};
static GtkTargetEntry targets[] = {
	{ const_cast<gchar *>("color-object-list"), GTK_TARGET_SAME_APP, static_cast<guint>(Target::colorObjectList) },
	{ const_cast<gchar *>("application/x-color_object-list"), GTK_TARGET_OTHER_APP, static_cast<guint>(Target::serializedColorObjectList) },
	{ const_cast<gchar *>("application/x-color-object-list"), GTK_TARGET_OTHER_APP, static_cast<guint>(Target::serializedColorObjectList) },
	{ const_cast<gchar *>("application/x-color"), 0, static_cast<guint>(Target::color) },
	{ const_cast<gchar *>("text/plain"), 0, static_cast<guint>(Target::string) },
	{ const_cast<gchar *>("UTF8_STRING"), 0, static_cast<guint>(Target::string) },
	{ const_cast<gchar *>("STRING"), 0, static_cast<guint>(Target::string) },
};
static const size_t targetCount = sizeof(targets) / sizeof(GtkTargetEntry);
struct ColorObjectList {
	uintptr_t sourceWidgetId;
	uint64_t colorObjectCount;
	ColorObject *colorObject[1];
};
static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selectionData, Target targetType, guint time, DragDrop *dd);
static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint t, DragDrop *dd);
static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, DragDrop *dd);
static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, DragDrop *dd);
static void drag_data_delete(GtkWidget *widget, GdkDragContext *context, DragDrop *dd);
static void drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selectionData, Target targetType, guint time, DragDrop *dd);
static void drag_begin(GtkWidget *widget, GdkDragContext *context, DragDrop *dd);
static void drag_end(GtkWidget *widget, GdkDragContext *context, DragDrop *dd);
static void drag_destroy(GtkWidget *widget, DragDrop *dd);
int dragdrop_init(DragDrop *dd, GlobalState *gs) {
	dd->get_color_object = 0;
	dd->set_color_object_at = 0;
	dd->test_at = 0;
	dd->data_delete = 0;
	dd->drag_end = 0;
	dd->get_color_object_list = 0;
	dd->set_color_object_list_at = 0;
	dd->data_type = DragDrop::DATA_TYPE_NONE;
	memset(&dd->data, 0, sizeof(dd->data));
	dd->widget = 0;
	dd->gs = gs;
	dd->dragwidget = 0;
	return 0;
}
int dragdrop_widget_attach(GtkWidget *widget, DragDropFlags flags, DragDrop *user_dd) {
	DragDrop *dd = new DragDrop;
	memcpy(dd, user_dd, sizeof(DragDrop));
	dd->widget = widget;
	if (flags & DRAGDROP_SOURCE) {
		GtkTargetList *target_list = gtk_drag_source_get_target_list(widget);
		if (target_list) {
			gtk_target_list_add_table(target_list, targets, targetCount);
		} else {
			target_list = gtk_target_list_new(targets, targetCount);
			gtk_drag_source_set_target_list(widget, target_list);
		}
		g_signal_connect(widget, "drag-data-get", G_CALLBACK(drag_data_get), dd);
		g_signal_connect(widget, "drag-data-delete", G_CALLBACK(drag_data_delete), dd);
		g_signal_connect(widget, "drag-begin", G_CALLBACK(drag_begin), dd);
		g_signal_connect(widget, "drag-end", G_CALLBACK(drag_end), dd);
	}
	if (flags & DRAGDROP_DESTINATION) {
		GtkTargetList *target_list = gtk_drag_dest_get_target_list(widget);
		if (target_list) {
			gtk_target_list_add_table(target_list, targets, targetCount);
		} else {
			target_list = gtk_target_list_new(targets, targetCount);
			gtk_drag_dest_set_target_list(widget, target_list);
		}
		g_signal_connect(widget, "drag-data-received", G_CALLBACK(drag_data_received), dd);
		g_signal_connect(widget, "drag-leave", G_CALLBACK(drag_leave), dd);
		g_signal_connect(widget, "drag-motion", G_CALLBACK(drag_motion), dd);
		g_signal_connect(widget, "drag-drop", G_CALLBACK(drag_drop), dd);
	}
	g_signal_connect(widget, "destroy", G_CALLBACK(drag_destroy), dd);
	return 0;
}
static void drag_data_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selectionData, Target targetType, guint time, DragDrop *dd) {
	if ((selectionData == nullptr) || (gtk_selection_data_get_length(selectionData) == 0)) {
		gtk_drag_finish(context, false, gdk_drag_context_get_actions(context) == GDK_ACTION_MOVE, time);
		return;
	}
	bool success = false;
	switch (targetType) {
	case Target::colorObjectList: {
		ColorObjectList data;
		memcpy(&data, gtk_selection_data_get_data(selectionData), offsetof(ColorObjectList, colorObject));
		bool sameWidget = data.sourceWidgetId == reinterpret_cast<uintptr_t>(dd->widget);
		if (data.colorObjectCount > 1) {
			std::vector<ColorObject *> colorObjects(data.colorObjectCount);
			memcpy(&colorObjects.front(), gtk_selection_data_get_data(selectionData) + offsetof(ColorObjectList, colorObject), sizeof(ColorObject *) * data.colorObjectCount);
			if (dd->set_color_object_list_at)
				dd->set_color_object_list_at(dd, &colorObjects.front(), data.colorObjectCount, x, y, gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE, sameWidget);
			else if (dd->set_color_object_at) {
				memcpy(&data, gtk_selection_data_get_data(selectionData), sizeof(ColorObjectList));
				dd->set_color_object_at(dd, data.colorObject[0], x, y, gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE, sameWidget);
			}
		} else {
			if (dd->set_color_object_at) {
				memcpy(&data, gtk_selection_data_get_data(selectionData), sizeof(ColorObjectList));
				dd->set_color_object_at(dd, data.colorObject[0], x, y, gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE, sameWidget);
			}
		}
	} break;
	case Target::serializedColorObjectList: {
		auto data = gtk_selection_data_get_data(selectionData);
		auto text = std::string(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + gtk_selection_data_get_length(selectionData));
		std::stringstream textStream(text);
		dynv::Map values;
		if (!values.deserializeXml(textStream))
			break;
		auto colors = values.getMaps("colors");
		if (colors.size() == 0)
			break;
		static Color defaultColor = {};
		if (colors.size() == 1) {
			if (dd->set_color_object_at) {
				auto *colorObject = new ColorObject();
				colorObject->setName(colors[0]->getString("name", ""));
				colorObject->setColor(colors[0]->getColor("color", defaultColor));
				dd->set_color_object_at(dd, colorObject, x, y, false, false);
				colorObject->release();
				success = true;
				break;
			}
		}
		if (dd->set_color_object_list_at) {
			size_t colorCount = colors.size();
			std::vector<ColorObject *> colorObjects(colorCount);
			auto releaseColorObjects = common::makeScoped(std::function<void()>([&colorObjects]() {
				for (auto colorObject: colorObjects)
					if (colorObject)
						colorObject->release();
			}));
			for (size_t i = 0; i < colorCount; i++)
				colorObjects[i] = new ColorObject(colors[i]->getString("name", ""), colors[i]->getColor("color", defaultColor));
			dd->set_color_object_list_at(dd, &colorObjects.front(), colorCount, x, y, false, false);
			success = true;
		}
	} break;
	case Target::string: {
		gchar *data = (gchar *)gtk_selection_data_get_data(selectionData);
		if (data[gtk_selection_data_get_length(selectionData)] != 0) break; //not null terminated
		ColorObject *colorObject = nullptr;
		if (!dd->gs->converters().deserialize(data, &colorObject)) {
			gtk_drag_finish(context, false, false, time);
			return;
		}
		dd->set_color_object_at(dd, colorObject, x, y, gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE, false);
		colorObject->release();
		success = true;
	} break;
	case Target::color: {
		guint16 *data = (guint16 *)gtk_selection_data_get_data(selectionData);
		Color color;
		color.rgb.red = static_cast<float>(data[0] / static_cast<double>(0xFFFF));
		color.rgb.green = static_cast<float>(data[1] / static_cast<double>(0xFFFF));
		color.rgb.blue = static_cast<float>(data[2] / static_cast<double>(0xFFFF));
		color.ma[3] = 0;
		auto *colorObject = new ColorObject("", color);
		dd->set_color_object_at(dd, colorObject, x, y, gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE, false);
		colorObject->release();
		success = true;
	} break;
	}
	gtk_drag_finish(context, success, gdk_drag_context_get_actions(context) == GDK_ACTION_MOVE, time);
}
static gboolean drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, DragDrop *dd) {
	GdkDragAction suggested_action;
	bool suggested_action_set = true;
	bool dragging_moves = dd->gs->settings().getBool("gpick.main.dragging_moves", true);
	if (dragging_moves) {
		if ((gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE) == GDK_ACTION_MOVE)
			suggested_action = GDK_ACTION_MOVE;
		else if ((gdk_drag_context_get_actions(context) & GDK_ACTION_COPY) == GDK_ACTION_COPY)
			suggested_action = GDK_ACTION_COPY;
		else
			suggested_action_set = false;
	} else {
		if ((gdk_drag_context_get_actions(context) & GDK_ACTION_COPY) == GDK_ACTION_COPY)
			suggested_action = GDK_ACTION_COPY;
		else if ((gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE) == GDK_ACTION_MOVE)
			suggested_action = GDK_ACTION_MOVE;
		else
			suggested_action_set = false;
	}
	if (!dd->test_at) {
		GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
		if (target) {
			gdk_drag_status(context, suggested_action_set ? suggested_action : gdk_drag_context_get_selected_action(context), time);
		} else {
			gdk_drag_status(context, suggested_action_set ? suggested_action : GdkDragAction(0), time);
		}
		return true;
	}
	if (dd->test_at(dd, x, y)) {
		GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
		if (target) {
			gdk_drag_status(context, suggested_action_set ? suggested_action : gdk_drag_context_get_selected_action(context), time);
		} else {
			gdk_drag_status(context, suggested_action_set ? suggested_action : GdkDragAction(0), time);
		}
	} else {
		gdk_drag_status(context, suggested_action_set ? suggested_action : GdkDragAction(0), time);
	}
	return true;
}

static void drag_leave(GtkWidget *widget, GdkDragContext *context, guint time, DragDrop *dd) {
}
static gboolean drag_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, DragDrop *dd) {
	GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
	if (target != GDK_NONE) {
		gtk_drag_get_data(widget, context, target, time);
		if (dd->drag_end)
			dd->drag_end(dd, widget, context);
		return true;
	}
	if (dd->drag_end)
		dd->drag_end(dd, widget, context);
	return false;
}
static void drag_data_delete(GtkWidget *widget, GdkDragContext *context, DragDrop *dd) {
	if (dd->data_delete) {
		dd->data_delete(dd, widget, context);
	}
}
static void drag_data_get(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selectionData, Target targetType, guint time, DragDrop *dd) {
	if (!selectionData)
		return;
	if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECT) {
		ColorObject *colorObject = dd->data.colorObject.colorObject;
		if (!colorObject) return;
		Color color;
		switch (targetType) {
		case Target::colorObjectList: {
			ColorObjectList data;
			data.sourceWidgetId = reinterpret_cast<uintptr_t>(dd->widget);
			data.colorObjectCount = 1;
			data.colorObject[0] = colorObject;
			gtk_selection_data_set(selectionData, gdk_atom_intern("colorObject", false), 8, (guchar *)&data, sizeof(data));
		} break;
		case Target::serializedColorObjectList: {
			std::vector<dynv::Ref> colors;
			auto color = dynv::Map::create();
			color->set("name", colorObject->getName());
			color->set("color", colorObject->getColor());
			colors.push_back(color);
			dynv::Map values;
			values.set("colors", colors);
			std::stringstream str;
			values.serializeXml(str);
			auto data = str.str();
			gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color-object-list", false), 8, reinterpret_cast<guchar *>(&data.front()), data.length());
		} break;
		case Target::string: {
			auto text = dd->gs->converters().serialize(colorObject, Converters::Type::copy);
			gtk_selection_data_set_text(selectionData, text.c_str(), text.length() + 1);
		} break;
		case Target::color: {
			color = colorObject->getColor();
			guint16 data_color[4];
			data_color[0] = int(color.rgb.red * 0xFFFF);
			data_color[1] = int(color.rgb.green * 0xFFFF);
			data_color[2] = int(color.rgb.blue * 0xFFFF);
			data_color[3] = 0xffff;
			gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color", false), 16, (guchar *)data_color, 8);
		} break;
		}
	} else if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECTS) {
		ColorObject **colorObjects = dd->data.colorObjects.colorObjects;
		uint32_t colorObjectCount = dd->data.colorObjects.colorObjectCount;
		if (!colorObjects) return;
		Color color;
		switch (targetType) {
		case Target::colorObjectList: {
			size_t dataLength = offsetof(ColorObjectList, colorObject) + sizeof(ColorObject *) * colorObjectCount;
			std::vector<uint8_t> dataBytes(dataLength);
			auto data = reinterpret_cast<ColorObjectList *>(&dataBytes.front());
			data->sourceWidgetId = reinterpret_cast<uintptr_t>(dd->widget);
			data->colorObjectCount = colorObjectCount;
			memcpy(&data->colorObject[0], colorObjects, sizeof(ColorObject *) * colorObjectCount);
			gtk_selection_data_set(selectionData, gdk_atom_intern("color-object", false), 8, reinterpret_cast<const guchar *>(&dataBytes.front()), dataLength);
		} break;
		case Target::serializedColorObjectList: {
			std::vector<dynv::Ref> colors;
			colors.reserve(colorObjectCount);
			for (uint32_t i = 0; i < colorObjectCount; i++) {
				auto color = dynv::Map::create();
				color->set("name", colorObjects[i]->getName());
				color->set("color", colorObjects[i]->getColor());
				colors.push_back(color);
			}
			dynv::Map values;
			values.set("colors", colors);
			std::stringstream str;
			values.serializeXml(str);
			auto data = str.str();
			gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color-object-list", false), 8, (guchar *)&data.front(), data.length());
		} break;
		case Target::string: {
			std::stringstream ss;
			auto converter = dd->gs->converters().firstCopy();
			if (converter) {
				for (uint32_t i = 0; i != colorObjectCount; i++) {
					ss << converter->serialize(colorObjects[i]) << "\n";
				}
			}
			std::string text = ss.str();
			gtk_selection_data_set_text(selectionData, text.c_str(), text.length() + 1);
		} break;
		case Target::color: {
			ColorObject *colorObject = colorObjects[0];
			color = colorObject->getColor();
			guint16 data_color[4];
			data_color[0] = int(color.rgb.red * 0xFFFF);
			data_color[1] = int(color.rgb.green * 0xFFFF);
			data_color[2] = int(color.rgb.blue * 0xFFFF);
			data_color[3] = 0xffff;
			gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color", false), 16, (guchar *)data_color, 8);
		} break;
		}
	}
}

static void drag_begin(GtkWidget *widget, GdkDragContext *context, DragDrop *dd) {
	if (dd->get_color_object_list) {
		size_t colorObjectCount;
		auto colorObjects = dd->get_color_object_list(dd, &colorObjectCount);
		if (colorObjects) {
			dd->data_type = DragDrop::DATA_TYPE_COLOR_OBJECTS;
			dd->data.colorObjects.colorObjects = colorObjects;
			dd->data.colorObjects.colorObjectCount = colorObjectCount;
			auto dragWindow = gtk_window_new(GTK_WINDOW_POPUP);
			auto hbox = gtk_vbox_new(true, 0);
			gtk_container_add(GTK_CONTAINER(dragWindow), hbox);
			auto showColors = std::min<size_t>(colorObjectCount, 5);
			gtk_widget_set_size_request(dragWindow, 164, 24 * showColors);
			auto converter = dd->gs->converters().forType(dd->converterType);
			if (converter) {
				for (size_t i = 0; i < showColors; i++) {
					auto colorWidget = gtk_color_new();
					auto text = converter ? converter->serialize(colorObjects[i]) : "";
					Color color = colorObjects[i]->getColor();
					gtk_color_set_color(GTK_COLOR(colorWidget), &color, text.c_str());
					gtk_box_pack_start(GTK_BOX(hbox), colorWidget, true, true, 0);
				}
			}
			gtk_drag_set_icon_widget(context, dragWindow, 0, 0);
			gtk_widget_show_all(dragWindow);
			dd->dragwidget = dragWindow;
			return;
		}
	}
	if (dd->get_color_object) {
		ColorObject *colorObject = dd->get_color_object(dd);
		if (colorObject) {
			dd->data_type = DragDrop::DATA_TYPE_COLOR_OBJECT;
			dd->data.colorObject.colorObject = colorObject;
			auto dragWindow = gtk_window_new(GTK_WINDOW_POPUP);
			auto colorWidget = gtk_color_new();
			gtk_container_add(GTK_CONTAINER(dragWindow), colorWidget);
			gtk_widget_set_size_request(dragWindow, 164, 24);
			auto text = dd->gs->converters().serialize(colorObject, dd->converterType);
			Color color = colorObject->getColor();
			gtk_color_set_color(GTK_COLOR(colorWidget), &color, text.c_str());
			gtk_drag_set_icon_widget(context, dragWindow, 0, 0);
			gtk_widget_show_all(dragWindow);
			dd->dragwidget = dragWindow;
			return;
		}
	}
}
static void drag_end(GtkWidget *widget, GdkDragContext *context, DragDrop *dd) {
	if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECT) {
		if (dd->data.colorObject.colorObject) {
			dd->data.colorObject.colorObject->release();
			memset(&dd->data, 0, sizeof(dd->data));
		}
		dd->data_type = DragDrop::DATA_TYPE_NONE;
	}
	if (dd->data_type == DragDrop::DATA_TYPE_COLOR_OBJECTS) {
		if (dd->data.colorObjects.colorObjects) {
			for (uint32_t i = 0; i < dd->data.colorObjects.colorObjectCount; i++) {
				dd->data.colorObjects.colorObjects[i]->release();
			}
			delete[] dd->data.colorObjects.colorObjects;
			memset(&dd->data, 0, sizeof(dd->data));
		}
		dd->data_type = DragDrop::DATA_TYPE_NONE;
	}
	if (dd->dragwidget) {
		gtk_widget_destroy(dd->dragwidget);
		dd->dragwidget = 0;
	}
	if (dd->drag_end)
		dd->drag_end(dd, widget, context);
}
static void drag_destroy(GtkWidget *widget, DragDrop *dd) {
	delete dd;
}
