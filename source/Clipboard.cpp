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

#include "Clipboard.h"
#include "Converters.h"
#include "Converter.h"
#include "GlobalState.h"
#include "ColorObject.h"
#include "Color.h"
#include "uiListPalette.h"
#include "ColorList.h"
#include "DynvHelpers.h"
#include "dynv/DynvXml.h"
#include <gtk/gtk.h>
#include <sstream>
namespace clipboard {
enum class Target : guint {
	string = 1,
	color,
	serializedColorObjectList,
};
static const GtkTargetEntry targets[] = {
	{ const_cast<gchar *>("application/x-color_object-list"), 0, static_cast<guint>(Target::serializedColorObjectList) },
	{ const_cast<gchar *>("application/x-color-object-list"), 0, static_cast<guint>(Target::serializedColorObjectList) },
	{ const_cast<gchar *>("application/x-color"), 0, static_cast<guint>(Target::color) },
	{ const_cast<gchar *>("text/plain"), 0, static_cast<guint>(Target::string) },
	{ const_cast<gchar *>("UTF8_STRING"), 0, static_cast<guint>(Target::string) },
	{ const_cast<gchar *>("STRING"), 0, static_cast<guint>(Target::string) },
};
static const size_t targetCount = sizeof(targets) / sizeof(GtkTargetEntry);
struct CopyPasteArgs {
	Converter *converter;
	ColorList *colors;
	GlobalState *gs;
	CopyPasteArgs(ColorList *colors, Converter *converter, GlobalState *gs):
		converter(converter),
		colors(colors),
		gs(gs) {
	}
	~CopyPasteArgs() {
		if (colors != nullptr)
			color_list_destroy(colors);
	}
};
static void setData(GtkClipboard *, GtkSelectionData *selectionData, Target targetType, CopyPasteArgs *args) {
	if (!selectionData)
		return;
	switch (targetType) {
	case Target::string: {
		const auto &colors = args->colors->colors;
		std::stringstream text;
		std::string textLine;
		ConverterSerializePosition position(args->colors->colors.size());
		if (position.count() > 0) {
			for (auto &color: colors) {
				if (position.index() + 1 == position.count())
					position.last(true);
				textLine = args->converter->serialize(color, position);
				if (position.first()) {
					text << textLine;
					position.first(false);
				} else {
					text << "\n" << textLine;
				}
				position.incrementIndex();
			}
		}
		textLine = text.str();
		if (textLine.length() > 0)
			gtk_selection_data_set_text(selectionData, textLine.c_str(), -1);
	} break;
	case Target::color: {
		auto &colorObject = args->colors->colors.front();
		auto &color = colorObject->getColor();
		guint16 dataColor[4];
		dataColor[0] = static_cast<guint16>(color.rgb.red * 0xFFFF);
		dataColor[1] = static_cast<guint16>(color.rgb.green * 0xFFFF);
		dataColor[2] = static_cast<guint16>(color.rgb.blue * 0xFFFF);
		dataColor[3] = 0xffff;
		gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color", false), 16, reinterpret_cast<guchar *>(dataColor), 8);
	} break;
	case Target::serializedColorObjectList: {
		auto handlerMap = dynv_system_get_handler_map(args->gs->getSettings());
		auto params = dynv_system_create(handlerMap);
		std::vector<dynvSystem *> colors;
		colors.reserve(args->colors->colors.size());
		for (auto &colorObject : args->colors->colors) {
			auto color = dynv_system_create(handlerMap);
			dynv_set_string(color, "name", colorObject->getName().c_str());
			dynv_set_color(color, "color", &colorObject->getColor());
			colors.push_back(color);
		}
		dynv_handler_map_release(handlerMap);
		dynv_set_dynv_array(params, "colors", const_cast<const dynvSystem **>(&colors.front()), colors.size());
		std::stringstream str;
		str << "<?xml version=\"1.0\" encoding='UTF-8'?><root>";
		dynv_xml_serialize(params, str);
		str << "</root>";
		auto data = str.str();
		for (auto &color : colors) {
			dynv_system_release(color);
		}
		dynv_system_release(params);
		gtk_selection_data_set(selectionData, gdk_atom_intern("application/x-color-object-list", false), 16, reinterpret_cast<guchar *>(&data.front()), data.length());
	} break;
	}
}
static void deleteState(GtkClipboard *, CopyPasteArgs *args) {
	delete args;
}
static bool setupClipboard(const ColorObject &colorObject, Converter *converter, GlobalState *gs) {
	auto colorList = color_list_new();
	color_list_add_color_object(colorList, colorObject.copy(), false);
	auto args = new CopyPasteArgs(colorList, converter, gs);
	if (gtk_clipboard_set_with_data(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), targets, targetCount,
			reinterpret_cast<GtkClipboardGetFunc>(setData),
			reinterpret_cast<GtkClipboardClearFunc>(deleteState), args)) {
		gtk_clipboard_set_can_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), nullptr, 0);
		return true;
	}
	deleteState(nullptr, args);
	return false;
}
static bool setupClipboard(ColorList *colorList, Converter *converter, GlobalState *gs) {
	auto args = new CopyPasteArgs(colorList, converter, gs);
	if (gtk_clipboard_set_with_data(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), targets, targetCount,
			reinterpret_cast<GtkClipboardGetFunc>(setData),
			reinterpret_cast<GtkClipboardClearFunc>(deleteState), args)) {
		gtk_clipboard_set_can_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), nullptr, 0);
		return true;
	}
	deleteState(nullptr, args);
	return false;
}
static Converter *getConverter(ConverterSelection converterSelection, GlobalState *gs) {
	struct GetConverter: public boost::static_visitor<Converter *> {
		GetConverter(GlobalState *gs):
			gs(gs) {
		}
		Converter *operator()(const char *&name) const {
			auto converter = gs->converters().byNameOrFirstCopy(name);
			if (converter == nullptr)
				converter = gs->converters().firstCopyOrAny();
			return converter;
		}
		Converter *operator()(Converter *converter) const {
			auto result = converter;
			if (result == nullptr)
				result = gs->converters().firstCopyOrAny();
			return result;
		}
		Converter *operator()(Converters::Type converterType) const {
			auto converter = gs->converters().forType(converterType);
			if (converter == nullptr)
				converter = gs->converters().firstCopyOrAny();
			return converter;
		}
		GlobalState *gs;
	};
	return boost::apply_visitor(GetConverter(gs), converterSelection);
}
void set(const std::string &value) {
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), value.c_str(), -1);
	gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), value.c_str(), -1);
}
void set(const ColorObject *colorObject, GlobalState *gs, ConverterSelection converterSelection) {
	auto converter = getConverter(converterSelection, gs);
	if (converter == nullptr)
		return;
	setupClipboard(*colorObject, converter, gs);
}
static PaletteListCallbackReturn addToColorList(ColorObject *colorObject, ColorList *colorList) {
	color_list_add_color_object(colorList, colorObject, 1);
	return PALETTE_LIST_CALLBACK_NO_UPDATE;
}
void set(GtkWidget *palette_widget, GlobalState *gs, ConverterSelection converterSelection) {
	auto converter = getConverter(converterSelection, gs);
	if (converter == nullptr)
		return;
	std::stringstream text;
	auto colorList = color_list_new();
	palette_list_foreach_selected(palette_widget, (PaletteListCallback)addToColorList, colorList);
	if (colorList->colors.empty())
		return;
	setupClipboard(colorList, converter, gs);
}
void set(const Color &color, GlobalState *gs, ConverterSelection converterSelection) {
	auto converter = getConverter(converterSelection, gs);
	if (converter == nullptr)
		return;
	setupClipboard(ColorObject("", color), converter, gs);
}
enum class VisitResult {
	stop,
	advance,
};
static void perMatchedTarget(GdkAtom *availableTargets, size_t count, std::function<VisitResult(size_t, Target)> callback) {
	for (size_t j = 0; j < targetCount; ++j) {
		for (size_t i = 0; i < count; ++i) {
			auto atomName = gdk_atom_name(availableTargets[i]);
			if (g_strcmp0(targets[j].target, atomName) == 0) {
				g_free(atomName);
				if (callback(i, static_cast<Target>(targets[j].info)) == VisitResult::stop)
					return;
			} else {
				g_free(atomName);
			}
		}
	}
}
bool colorObjectAvailable() {
	GdkAtom *availableTargets;
	gint availableTargetCount;
	if (!gtk_clipboard_wait_for_targets(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), &availableTargets, &availableTargetCount))
		return false;
	if (availableTargetCount <= 0) {
		g_free(availableTargets);
		return false;
	}
	bool found = false;
	perMatchedTarget(availableTargets, static_cast<size_t>(availableTargetCount), [&found](size_t, Target) {
		found = true;
		return VisitResult::stop;
	});
	g_free(availableTargets);
	return found;
}
ColorObject *getFirst(GlobalState *gs) {
	auto clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	GdkAtom *availableTargets;
	gint availableTargetCount;
	if (!gtk_clipboard_wait_for_targets(clipboard, &availableTargets, &availableTargetCount))
		return nullptr;
	if (availableTargetCount <= 0) {
		g_free(availableTargets);
		return nullptr;
	}
	ColorObject *result = nullptr;
	perMatchedTarget(availableTargets, static_cast<size_t>(availableTargetCount), [&result, clipboard, availableTargets, gs](size_t i, Target target) {
		auto selectionData = gtk_clipboard_wait_for_contents(clipboard, availableTargets[i]);
		if (!selectionData)
			return VisitResult::advance;
		switch (target) {
		case Target::string: {
			auto data = gtk_selection_data_get_data(selectionData);
			auto text = std::string(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + gtk_selection_data_get_length(selectionData));
			if (gs->converters().deserialize(text.c_str(), &result))
				return VisitResult::stop;
		} break;
		case Target::color: {
			if (gtk_selection_data_get_length(selectionData) != 8)
				return VisitResult::advance;
			auto data = gtk_selection_data_get_data(selectionData);
			guint16 values[4];
			::memcpy(values, data, 8);
			Color color;
			color.rgb.red = static_cast<float>(data[0] / static_cast<double>(0xFFFF));
			color.rgb.green = static_cast<float>(data[1] / static_cast<double>(0xFFFF));
			color.rgb.blue = static_cast<float>(data[2] / static_cast<double>(0xFFFF));
			color.ma[3] = 0;
			result = new ColorObject("", color);
			return VisitResult::stop;
		} break;
		case Target::serializedColorObjectList: {
			auto data = gtk_selection_data_get_data(selectionData);
			auto text = std::string(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + gtk_selection_data_get_length(selectionData));
			std::stringstream textStream(text);
			auto handlerMap = dynv_system_get_handler_map(gs->getSettings());
			auto params = dynv_system_create(handlerMap);
			dynv_handler_map_release(handlerMap);
			dynv_xml_deserialize(params, textStream);
			uint32_t colorCount = 0;
			auto colors = reinterpret_cast<dynvSystem**>(dynv_get_dynv_array_wd(params, "colors", nullptr, 0, &colorCount));
			if (!colors || colorCount == 0) {
				dynv_system_release(params);
				if (colors) {
					delete [] colors;
				}
				return VisitResult::advance;
			}
			static Color defaultColor = { .ma { 0, 0, 0, 0 } };
			result = new ColorObject(dynv_get_string_wd(colors[0], "name", ""), *dynv_get_color_wdc(colors[0], "color", &defaultColor));
			return VisitResult::stop;
		} break;
		}
		return VisitResult::advance;
	});
	return result;
}
ColorList *getColors(GlobalState *gs) {
	auto clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	GdkAtom *availableTargets;
	gint availableTargetCount;
	if (!gtk_clipboard_wait_for_targets(clipboard, &availableTargets, &availableTargetCount))
		return nullptr;
	if (availableTargetCount <= 0) {
		g_free(availableTargets);
		return nullptr;
	}
	bool success = false;
	auto colorList = color_list_new();
	perMatchedTarget(availableTargets, static_cast<size_t>(availableTargetCount), [colorList, &success, clipboard, availableTargets, gs](size_t i, Target target) {
		auto selectionData = gtk_clipboard_wait_for_contents(clipboard, availableTargets[i]);
		if (!selectionData)
			return VisitResult::advance;
		switch (target) {
		case Target::string: {
			auto data = gtk_selection_data_get_data(selectionData);
			auto text = std::string(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + gtk_selection_data_get_length(selectionData));
			ColorObject *colorObject = nullptr;
			//TODO: multiple colors should be extracted from string, but converters do not support this right now
			if (gs->converters().deserialize(text.c_str(), &colorObject)) {
				color_list_add_color_object(colorList, colorObject, false);
				success = true;
				return VisitResult::stop;
			}
		} break;
		case Target::color: {
			if (gtk_selection_data_get_length(selectionData) != 8)
				return VisitResult::advance;
			auto data = gtk_selection_data_get_data(selectionData);
			guint16 values[4];
			::memcpy(values, data, 8);
			Color color;
			color.rgb.red = static_cast<float>(data[0] / static_cast<double>(0xFFFF));
			color.rgb.green = static_cast<float>(data[1] / static_cast<double>(0xFFFF));
			color.rgb.blue = static_cast<float>(data[2] / static_cast<double>(0xFFFF));
			color.ma[3] = 0;
			color_list_add_color_object(colorList, new ColorObject("", color), false);
			success = true;
			return VisitResult::stop;
		} break;
		case Target::serializedColorObjectList: {
			auto data = gtk_selection_data_get_data(selectionData);
			auto text = std::string(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + gtk_selection_data_get_length(selectionData));
			std::stringstream textStream(text);
			auto handlerMap = dynv_system_get_handler_map(gs->getSettings());
			auto params = dynv_system_create(handlerMap);
			dynv_handler_map_release(handlerMap);
			dynv_xml_deserialize(params, textStream);
			uint32_t colorCount = 0;
			auto colors = reinterpret_cast<dynvSystem**>(dynv_get_dynv_array_wd(params, "colors", nullptr, 0, &colorCount));
			if (!colors || colorCount == 0) {
				dynv_system_release(params);
				if (colors) {
					delete [] colors;
				}
				return VisitResult::advance;
			}
			static Color defaultColor = { .ma { 0, 0, 0, 0 } };
			for (size_t i = 0; i < colorCount; i++) {
				color_list_add_color_object(colorList, new ColorObject(dynv_get_string_wd(colors[i], "name", ""), *dynv_get_color_wdc(colors[i], "color", &defaultColor)), false);
			}
			success = true;
			return VisitResult::stop;
		} break;
		}
		return VisitResult::advance;
	});
	if (!success) {
		color_list_destroy(colorList);
		return nullptr;
	}
	return colorList;
}
}
