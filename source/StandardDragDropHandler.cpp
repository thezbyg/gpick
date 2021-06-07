/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#include "StandardDragDropHandler.h"
#include "GlobalState.h"
#include "ColorObject.h"
#include "Converter.h"
#include "dynv/Map.h"
#include "IDroppableColorUI.h"
#include "gtk/ColorWidget.h"
#include <sstream>

struct State {
	GlobalState &gs;
	Converters::Type converterType;
	std::vector<ColorObject> colorObjects;
	GtkWidget *dragWidget;
	bool supportsMove;
	State(GlobalState &gs, Converters::Type converterType, bool supportsMove):
		gs(gs),
		converterType(converterType),
		dragWidget(nullptr),
		supportsMove(supportsMove) {
	}
};
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
	std::vector<ColorObject> *colorObjects;
};
static bool setColor(ColorObject &colorObject, IReadonlyColorUI *readonlyColorUI, int x, int y) {
	auto *droppableColorUI = dynamic_cast<IDroppableColorUI*>(readonlyColorUI);
	if (droppableColorUI) {
		droppableColorUI->setColorAt(colorObject, x, y);
		return true;
	}
	auto *editableColorUI = dynamic_cast<IEditableColorUI*>(readonlyColorUI);
	if (editableColorUI) {
		editableColorUI->setColor(colorObject);
		return true;
	}
	return false;
}
static bool setColors(std::vector<ColorObject> &colorObjects, IReadonlyColorUI *readonlyColorUI, int x, int y) {
	if (colorObjects.size() == 0)
		return false;
	if (colorObjects.size() == 1) {
		auto *droppableColorUI = dynamic_cast<IDroppableColorUI*>(readonlyColorUI);
		if (droppableColorUI) {
			droppableColorUI->setColorAt(colorObjects[0], x, y);
			return true;
		}
		auto *editableColorUI = dynamic_cast<IEditableColorUI*>(readonlyColorUI);
		if (editableColorUI) {
			editableColorUI->setColor(colorObjects[0]);
			return true;
		}
		return false;
	}
	auto *droppableColorsUI = dynamic_cast<IDroppableColorsUI*>(readonlyColorUI);
	if (droppableColorsUI) {
		droppableColorsUI->setColorsAt(colorObjects, x, y);
		return true;
	}
	auto *droppableColorUI = dynamic_cast<IDroppableColorUI*>(readonlyColorUI);
	if (droppableColorUI) {
		droppableColorUI->setColorAt(colorObjects[0], x, y);
		return true;
	}
	auto *editableColorsUI = dynamic_cast<IEditableColorsUI*>(readonlyColorUI);
	if (editableColorsUI) {
		editableColorsUI->setColors(colorObjects);
		return true;
	}
	auto *editableColorUI = dynamic_cast<IEditableColorUI*>(readonlyColorUI);
	if (editableColorUI) {
		editableColorUI->setColor(colorObjects[0]);
		return true;
	}
	return false;
}
static void onDragDataReceived(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selectionData, Target targetType, guint time, IReadonlyColorUI *readonlyColorUI) {
	if ((selectionData == nullptr) || (gtk_selection_data_get_length(selectionData) == 0)) {
		gtk_drag_finish(context, false, gdk_drag_context_get_actions(context) == GDK_ACTION_MOVE, time);
		return;
	}
	auto &state = *reinterpret_cast<State *>(g_object_get_data(G_OBJECT(widget), "dragDropState"));
	bool success = false;
	switch (targetType) {
	case Target::colorObjectList: {
		auto *dataPointer = gtk_selection_data_get_data(selectionData);
		if (!dataPointer)
			break;
		const auto &data = *reinterpret_cast<const ColorObjectList *>(dataPointer);
		success = setColors(*data.colorObjects, readonlyColorUI, x, y);
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
		std::vector<ColorObject> colorObjects;
		colorObjects.reserve(colors.size());
		for (size_t i = 0; i < colors.size(); i++) {
			colorObjects.emplace_back(colors[i]->getString("name", ""), colors[i]->getColor("color", defaultColor));
		}
		success = setColors(colorObjects, readonlyColorUI, x, y);
	} break;
	case Target::string: {
		gchar *data = (gchar *)gtk_selection_data_get_data(selectionData);
		ColorObject colorObject;
		if (!state.gs.converters().deserialize(std::string(data, data + gtk_selection_data_get_length(selectionData)), colorObject)) {
			gtk_drag_finish(context, false, false, time);
			return;
		}
		success = setColor(colorObject, readonlyColorUI, x, y);
	} break;
	case Target::color: {
		guint16 *data = (guint16 *)gtk_selection_data_get_data(selectionData);
		Color color;
		color.rgb.red = static_cast<float>(data[0] / static_cast<double>(0xFFFF));
		color.rgb.green = static_cast<float>(data[1] / static_cast<double>(0xFFFF));
		color.rgb.blue = static_cast<float>(data[2] / static_cast<double>(0xFFFF));
		color[3] = 0;
		ColorObject colorObject("", color);
		success = setColor(colorObject, readonlyColorUI, x, y);
	} break;
	}
	gtk_drag_finish(context, success, gdk_drag_context_get_actions(context) == GDK_ACTION_MOVE, time);
}
static gboolean onDragMotion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, IReadonlyColorUI *readonlyColorUI) {
	GdkDragAction suggestedAction;
	bool suggestedActionSet = true;
	auto &state = *reinterpret_cast<State *>(g_object_get_data(G_OBJECT(widget), "dragDropState"));
	if (state.supportsMove) {
		bool draggingMoves = state.gs.settings().getBool("gpick.main.dragging_moves", true);
		if (draggingMoves) {
			if ((gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE) == GDK_ACTION_MOVE)
				suggestedAction = GDK_ACTION_MOVE;
			else if ((gdk_drag_context_get_actions(context) & GDK_ACTION_COPY) == GDK_ACTION_COPY)
				suggestedAction = GDK_ACTION_COPY;
			else
				suggestedActionSet = false;
		} else {
			if ((gdk_drag_context_get_actions(context) & GDK_ACTION_COPY) == GDK_ACTION_COPY)
				suggestedAction = GDK_ACTION_COPY;
			else if ((gdk_drag_context_get_actions(context) & GDK_ACTION_MOVE) == GDK_ACTION_MOVE)
				suggestedAction = GDK_ACTION_MOVE;
			else
				suggestedActionSet = false;
		}
	} else {
		suggestedActionSet = false;
	}
	auto *droppableColorUI = dynamic_cast<IDroppableColorUI*>(readonlyColorUI);
	if (!droppableColorUI) {
		GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
		if (target) {
			gdk_drag_status(context, suggestedActionSet ? suggestedAction : gdk_drag_context_get_selected_action(context), time);
		} else {
			gdk_drag_status(context, suggestedActionSet ? suggestedAction : GdkDragAction(0), time);
		}
		return true;
	}
	if (droppableColorUI->testDropAt(x, y)) {
		GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
		if (target) {
			gdk_drag_status(context, suggestedActionSet ? suggestedAction : gdk_drag_context_get_selected_action(context), time);
		} else {
			gdk_drag_status(context, suggestedActionSet ? suggestedAction : GdkDragAction(0), time);
		}
	} else {
		gdk_drag_status(context, suggestedActionSet ? suggestedAction : GdkDragAction(0), time);
	}
	return true;
}
static gboolean onDragDrop(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, IReadonlyColorUI *readonlyColorUI) {
	GdkAtom target = gtk_drag_dest_find_target(widget, context, 0);
	auto *droppableColorUI = dynamic_cast<IDroppableColorUI*>(readonlyColorUI);
	if (target != GDK_NONE) {
		gtk_drag_get_data(widget, context, target, time);
		if (droppableColorUI)
			droppableColorUI->dropEnd(gdk_drag_context_get_selected_action(context) == GDK_ACTION_MOVE);
		return true;
	}
	if (droppableColorUI)
		droppableColorUI->dropEnd(gdk_drag_context_get_selected_action(context) == GDK_ACTION_MOVE);
	return false;
}
static void onDragDataGet(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selectionData, Target targetType, guint time, IReadonlyColorUI *readonlyColorUI) {
	if (!selectionData)
		return;
	auto &state = *reinterpret_cast<State *>(g_object_get_data(G_OBJECT(widget), "dragDropState"));
	if (state.colorObjects.size() == 0)
		return;
	Color color;
	switch (targetType) {
	case Target::colorObjectList: {
		ColorObjectList data;
		data.colorObjects = &state.colorObjects;
		gtk_selection_data_set(selectionData, gdk_atom_intern("color-object", false), 8, reinterpret_cast<const guchar *>(&data), sizeof(ColorObjectList));
	} break;
	case Target::serializedColorObjectList: {
		std::vector<dynv::Ref> colors;
		colors.reserve(state.colorObjects.size());
		for (uint32_t i = 0; i < state.colorObjects.size(); i++) {
			auto color = dynv::Map::create();
			color->set("name", state.colorObjects[i].getName());
			color->set("color", state.colorObjects[i].getColor());
			colors.push_back(color);
		}
		dynv::Map values;
		values.set("colors", colors);
		std::stringstream str;
		values.serializeXml(str);
		auto data = str.str();
		if (!data.empty())
			gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color-object-list", false), 8, (const guchar *)&data.front(), data.length());
		else
			gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color-object-list", false), 8, (const guchar *)"", 0);
	} break;
	case Target::string: {
		std::stringstream ss;
		ConverterSerializePosition position(state.colorObjects.size());
		auto converter = state.gs.converters().firstCopy();
		if (converter) {
			for (size_t i = 0, count = state.colorObjects.size(); i < count; i++) {
				if (position.index() + 1 == position.count())
					position.last(true);
				if (position.first()) {
					ss << converter->serialize(state.colorObjects[i], position);
					position.first(false);
				} else {
					ss << "\n" << converter->serialize(state.colorObjects[i], position);
				}
				position.incrementIndex();
			}
		}
		std::string text = ss.str();
		gtk_selection_data_set_text(selectionData, text.c_str(), text.length());
	} break;
	case Target::color: {
		const auto &colorObject = state.colorObjects[0];
		color = colorObject.getColor();
		guint16 dataColor[4];
		dataColor[0] = int(color.rgb.red * 0xFFFF);
		dataColor[1] = int(color.rgb.green * 0xFFFF);
		dataColor[2] = int(color.rgb.blue * 0xFFFF);
		dataColor[3] = 0xffff;
		gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color", false), 16, (guchar *)dataColor, 8);
	} break;
	}
}
static void onDragBegin(GtkWidget *widget, GdkDragContext *context, IReadonlyColorUI *readonlyColorUI) {
	auto &state = *reinterpret_cast<State *>(g_object_get_data(G_OBJECT(widget), "dragDropState"));
	auto *readonlyColorsUI = dynamic_cast<IReadonlyColorsUI *>(readonlyColorUI);
	if (readonlyColorsUI) {
		if (!readonlyColorsUI->hasSelectedColor())
			return;
		auto colorObjects = readonlyColorsUI->getColors(true);
		if (colorObjects.size() == 0)
			return;
		state.colorObjects = colorObjects;
		auto dragWindow = gtk_window_new(GTK_WINDOW_POPUP);
		auto hbox = gtk_vbox_new(true, 0);
		gtk_container_add(GTK_CONTAINER(dragWindow), hbox);
		auto showColors = std::min<size_t>(colorObjects.size(), 5);
		gtk_widget_set_size_request(dragWindow, 164, 24 * showColors);
		auto converter = state.gs.converters().forType(state.converterType);
		if (converter) {
			for (size_t i = 0; i < showColors; i++) {
				auto colorWidget = gtk_color_new();
				auto text = converter ? converter->serialize(colorObjects[i]) : "";
				Color color = colorObjects[i].getColor();
				gtk_color_set_color(GTK_COLOR(colorWidget), &color, text.c_str());
				gtk_box_pack_start(GTK_BOX(hbox), colorWidget, true, true, 0);
			}
		}
		gtk_drag_set_icon_widget(context, dragWindow, 0, 0);
		gtk_widget_show_all(dragWindow);
		state.dragWidget = dragWindow;
		return;
	}
	if (!readonlyColorUI->hasSelectedColor())
		return;
	auto colorObject = readonlyColorUI->getColor();
	state.colorObjects.push_back(colorObject);
	auto dragWindow = gtk_window_new(GTK_WINDOW_POPUP);
	auto colorWidget = gtk_color_new();
	gtk_container_add(GTK_CONTAINER(dragWindow), colorWidget);
	gtk_widget_set_size_request(dragWindow, 164, 24);
	auto text = state.gs.converters().serialize(colorObject, state.converterType);
	Color color = colorObject.getColor();
	gtk_color_set_color(GTK_COLOR(colorWidget), &color, text.c_str());
	gtk_drag_set_icon_widget(context, dragWindow, 0, 0);
	gtk_widget_show_all(dragWindow);
	state.dragWidget = dragWindow;
	return;
}
static void onDragEnd(GtkWidget *widget, GdkDragContext *context, IReadonlyColorUI *readonlyColorUI) {
	auto &state = *reinterpret_cast<State *>(g_object_get_data(G_OBJECT(widget), "dragDropState"));
	state.colorObjects.resize(0);
	state.colorObjects.shrink_to_fit();
	if (state.dragWidget) {
		gtk_widget_destroy(state.dragWidget);
		state.dragWidget = nullptr;
	}
	auto *draggableColorUI= dynamic_cast<IDraggableColorUI*>(readonlyColorUI);
	if (draggableColorUI)
		draggableColorUI->dragEnd(gdk_drag_context_get_selected_action(context) == GDK_ACTION_MOVE);
}
static void onStateDestroy(State *state){
	delete state;
}
StandardDragDropHandler::Options::Options():
	m_afterEvents(true),
	m_supportsMove(false),
	m_allowDrop(true),
	m_allowDrag(true),
	m_converterType(Converters::Type::display) {
}
StandardDragDropHandler::Options &StandardDragDropHandler::Options::afterEvents(bool enable) {
	m_afterEvents = enable;
	return *this;
}
StandardDragDropHandler::Options &StandardDragDropHandler::Options::supportsMove(bool enable) {
	m_supportsMove = enable;
	return *this;
}
StandardDragDropHandler::Options &StandardDragDropHandler::Options::allowDrop(bool enable) {
	m_allowDrop = enable;
	return *this;
}
StandardDragDropHandler::Options &StandardDragDropHandler::Options::allowDrag(bool enable) {
	m_allowDrag = enable;
	return *this;
}
StandardDragDropHandler::Options &StandardDragDropHandler::Options::converterType(Converters::Type type) {
	m_converterType = type;
	return *this;
}
void StandardDragDropHandler::forWidget(GtkWidget *widget, GlobalState *gs, Interface interface, Options options) {
	void *data = std::visit([](auto *interface) -> void * {
		return interface;
	}, interface);
	auto flags = options.m_afterEvents ? G_CONNECT_AFTER : static_cast<GConnectFlags>(0);
	if (options.m_allowDrag) {
		if (options.m_supportsMove)
			gtk_drag_source_set(widget, GDK_BUTTON1_MASK, 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));
		else
			gtk_drag_source_set(widget, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
		GtkTargetList *targetList = gtk_drag_source_get_target_list(widget);
		if (targetList) {
			gtk_target_list_add_table(targetList, targets, targetCount);
		} else {
			targetList = gtk_target_list_new(targets, targetCount);
			gtk_drag_source_set_target_list(widget, targetList);
		}
		g_signal_connect_data(widget, "drag-data-get", G_CALLBACK(onDragDataGet), data, nullptr, flags);
		g_signal_connect_data(widget, "drag-begin", G_CALLBACK(onDragBegin), data, nullptr, flags);
		g_signal_connect_data(widget, "drag-end", G_CALLBACK(onDragEnd), data, nullptr, flags);
	}
	auto *editableColorUI = dynamic_cast<IEditableColorUI *>(static_cast<IReadonlyColorUI *>(data));
	if (options.m_allowDrop && editableColorUI) {
		if (options.m_supportsMove)
			gtk_drag_dest_set(widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GdkDragAction(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_ASK));
		else
			gtk_drag_dest_set(widget, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
		GtkTargetList *targetList = gtk_drag_dest_get_target_list(widget);
		if (targetList) {
			gtk_target_list_add_table(targetList, targets, targetCount);
		} else {
			targetList = gtk_target_list_new(targets, targetCount);
			gtk_drag_dest_set_target_list(widget, targetList);
		}
		g_signal_connect_data(widget, "drag-data-received", G_CALLBACK(onDragDataReceived), data, nullptr, flags);
		g_signal_connect_data(widget, "drag-motion", G_CALLBACK(onDragMotion), data, nullptr, flags);
		g_signal_connect_data(widget, "drag-drop", G_CALLBACK(onDragDrop), data, nullptr, flags);
	}
	auto state = new State(*gs, options.m_converterType, options.m_supportsMove);
	g_object_set_data_full(G_OBJECT(widget), "dragDropState", state, GDestroyNotify(onStateDestroy));
}
