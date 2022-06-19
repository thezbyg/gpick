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

#include "PaletteFromImage.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "uiUtilities.h"
#include "uiListPalette.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "I18N.h"
#include "dynv/Map.h"
#include "math/OctreeColorQuantization.h"
#include "common/Guard.h"
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>

struct PaletteColorNameAssigner: public ToolColorNameAssigner {
	PaletteColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
		m_index = 0;
	}
	void assign(ColorObject &colorObject, std::string_view fileName, const int index) {
		m_fileName = fileName;
		m_index = index;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << m_fileName << " #" << m_index;
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_fileName;
	int m_index;
};
struct PaletteFromImageArgs {
	GtkWidget *fileBrowser, *rangeColors, *previewExpander;
	std::string filename, previousFilename;
	uint32_t numberOfColors;
	math::OctreeColorQuantization octree;
	ColorList *previewColorList;
	dynv::Ref options;
	GlobalState *gs;
	static uint8_t toUint8(float value) {
		return static_cast<uint8_t>(std::max(std::min(static_cast<int>(value * 256), 255), 0));
	}
	void processImage() {
		previousFilename = filename;
		octree.clear();
		GError *error = nullptr;
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename.c_str(), &error);
		if (error) {
			std::cout << error->message << '\n';
			g_error_free(error);
			return;
		}
		int channels = gdk_pixbuf_get_n_channels(pixbuf);
		int width = gdk_pixbuf_get_width(pixbuf);
		int height = gdk_pixbuf_get_height(pixbuf);
		int stride = gdk_pixbuf_get_rowstride(pixbuf);
		guchar *imageData = gdk_pixbuf_get_pixels(pixbuf);
		if (width * height < (1 << 16) || std::max(1u, std::thread::hardware_concurrency()) == 1) {
			guchar *dataPointer = imageData;
			Color color;
			for (int y = 0; y < height; y++) {
				dataPointer = imageData + stride * y;
				for (int x = 0; x < width; x++) {
					if (channels == 1) {
						color.xyz.x = color.xyz.y = color.xyz.z = dataPointer[0] / 255.0f;
					} else {
						color.xyz.x = dataPointer[0] / 255.0f;
						color.xyz.y = dataPointer[1] / 255.0f;
						color.xyz.z = dataPointer[2] / 255.0f;
					}
					color.alpha = 1.0f;
					color.linearRgbInplace();
					std::array<uint8_t, 3> position = { dataPointer[0], dataPointer[1], dataPointer[2] };
					octree.add(color, position);
					dataPointer += channels;
				}
			}
		} else {
			size_t threadCount = std::min(8u, std::thread::hardware_concurrency());
			std::mutex octreeMutex;
			std::vector<std::thread> threads(threadCount);
			size_t index = 0;
			for (auto &thread: threads) {
				thread = std::thread([this, &octreeMutex, index, threadCount, imageData, channels, width, height, stride]() {
					math::OctreeColorQuantization threadOctree;
					guchar *dataPointer = imageData + stride * index;
					Color color;
					for (int y = 0; y < height; y += threadCount) {
						dataPointer = imageData + stride * y;
						for (int x = 0; x < width; x++) {
							if (channels == 1) {
								color.xyz.x = color.xyz.y = color.xyz.z = dataPointer[0] / 255.0f;
							} else {
								color.xyz.x = dataPointer[0] / 255.0f;
								color.xyz.y = dataPointer[1] / 255.0f;
								color.xyz.z = dataPointer[2] / 255.0f;
							}
							color.alpha = 1.0f;
							color.linearRgbInplace();
							std::array<uint8_t, 3> position = { dataPointer[0], dataPointer[1], dataPointer[2] };
							threadOctree.add(color, position);
							dataPointer += channels;
						}
					}
					threadOctree.reduce(1000);
					std::scoped_lock<std::mutex> lock(octreeMutex);
					threadOctree.visit([this](const float sum[3], size_t pixels) {
						Color color(sum[0] / pixels, sum[1] / pixels, sum[2] / pixels, 1.0f);
						Color nonLinearColor = color.nonLinearRgb();
						std::array<uint8_t, 3> position = { toUint8(nonLinearColor.red), toUint8(nonLinearColor.green), toUint8(nonLinearColor.blue) };
						octree.add(color, pixels, position);
					});
				});
				++index;
			}
			for (auto &thread: threads) {
				thread.join();
			}
		}
		g_object_unref(pixbuf);
		octree.reduce(1000);
	}
	void update(bool preview) {
		int index = 0;
		gchar *name = g_path_get_basename(filename.c_str());
		PaletteColorNameAssigner nameAssigner(*gs);
		if (!filename.empty() && previousFilename != filename)
			processImage();
		ColorList *colorList;
		if (preview)
			colorList = previewColorList;
		else
			colorList = gs->getColorList();
		math::OctreeColorQuantization reducedOctree(octree);
		reducedOctree.reduce(numberOfColors);
		common::Guard colorListGuard(color_list_start_changes(colorList), color_list_end_changes, colorList);
		reducedOctree.visit([&](const float sum[3], size_t pixels) {
			Color color(sum[0] / pixels, sum[1] / pixels, sum[2] / pixels, 1.0f);
			color.nonLinearRgbInplace();
			ColorObject *colorObject = color_list_new_color_object(colorList, &color);
			nameAssigner.assign(*colorObject, name, index);
			color_list_add_color_object(colorList, colorObject, 1);
			colorObject->release();
			index++;
		});
	}
	void getSettings() {
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileBrowser));
		if (filename) {
			this->filename = filename;
			g_free(filename);
		} else {
			this->filename.clear();
		}
		numberOfColors = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(rangeColors)));
	}
	void saveSettings() {
		options->set("colors", static_cast<int32_t>(numberOfColors));
		gchar *currentFolder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fileBrowser));
		if (currentFolder) {
			options->set("current_folder", currentFolder);
			g_free(currentFolder);
		}
		GtkFileFilter *filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(fileBrowser));
		if (filter) {
			const char *filterName = static_cast<const char *>(g_object_get_data(G_OBJECT(filter), "name"));
			options->set("filter", filterName);
		}
	}
	static void onUpdate(GtkWidget *widget, PaletteFromImageArgs *args) {
		color_list_remove_all(args->previewColorList);
		args->getSettings();
		args->update(true);
	}
	static void onDestroy(GtkWidget *widget, PaletteFromImageArgs *args) {
		color_list_destroy(args->previewColorList);
		delete args;
	}
	static void onResponse(GtkWidget *widget, gint responseId, PaletteFromImageArgs *args) {
		args->getSettings();
		args->saveSettings();
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(widget), &width, &height);
		args->options->set("window.width", width);
		args->options->set("window.height", height);
		args->options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(args->previewExpander)));
		switch (responseId) {
		case GTK_RESPONSE_APPLY:
			args->update(false);
			break;
		case GTK_RESPONSE_DELETE_EVENT:
			break;
		case GTK_RESPONSE_CLOSE:
			gtk_widget_destroy(widget);
			break;
		}
	}
};
void tools_palette_from_image_show(GtkWindow *parent, GlobalState *gs) {
	PaletteFromImageArgs *args = new PaletteFromImageArgs;
	args->previousFilename = "";
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick.tools.palette_from_image");
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Palette from image"), parent, GtkDialogFlags(GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_ADD, GTK_RESPONSE_APPLY, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("window.width", -1),
		args->options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY, GTK_RESPONSE_CLOSE, -1);

	Grid grid(2, 3);
	grid.addLabel(_("Image:"));
	GtkWidget *widget;
	args->fileBrowser = widget = grid.add(gtk_file_chooser_button_new(_("Image file"), GTK_FILE_CHOOSER_ACTION_OPEN), true);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), args->options->getString("current_folder", "").c_str());
	g_signal_connect(G_OBJECT(widget), "file-set", G_CALLBACK(PaletteFromImageArgs::onUpdate), args);
	auto selectedFilter = args->options->getString("filter", "all_images");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	g_object_set_data_full(G_OBJECT(filter), "name", (void *)"all_files", GDestroyNotify(nullptr));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
	if ("all_files" == selectedFilter)
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widget), filter);
	GtkFileFilter *allImageFilter = gtk_file_filter_new();
	gtk_file_filter_set_name(allImageFilter, _("All images"));
	g_object_set_data_full(G_OBJECT(allImageFilter), "name", (void *)"all_images", GDestroyNotify(nullptr));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), allImageFilter);
	if ("all_images" == selectedFilter)
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widget), allImageFilter);
	std::stringstream ss;
	GSList *formats = gdk_pixbuf_get_formats();
	GSList *i = formats;
	while (i) {
		GdkPixbufFormat *format = static_cast<GdkPixbufFormat *>(g_slist_nth_data(i, 0));
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, gdk_pixbuf_format_get_description(format));
		gchar **extensions = gdk_pixbuf_format_get_extensions(format);
		if (extensions) {
			for (int j = 0; extensions[j]; j++) {
				ss.str("");
				ss << "*." << extensions[j];
				auto pattern = ss.str();
				gtk_file_filter_add_pattern(filter, pattern.c_str());
				gtk_file_filter_add_pattern(allImageFilter, pattern.c_str());
			}
			g_strfreev(extensions);
		}
		g_object_set_data_full(G_OBJECT(filter), "name", gdk_pixbuf_format_get_name(format), GDestroyNotify(nullptr));
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(widget), filter);
		if (gdk_pixbuf_format_get_name(format) == selectedFilter)
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(widget), filter);
		i = g_slist_next(i);
	}
	if (formats)
		g_slist_free(formats);
	grid.addLabel(_("Colors:"));
	args->rangeColors = widget = grid.add(gtk_spin_button_new_with_range(1, 1000, 1), true);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), args->options->getInt32("colors", 3));
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(PaletteFromImageArgs::onUpdate), args);
	ColorList *previewColorList = nullptr;
	args->previewExpander = grid.add(palette_list_preview_new(gs, true, args->options->getBool("show_preview", true), gs->getColorList(), &previewColorList), true, 2, true);
	args->previewColorList = previewColorList;
	gtk_widget_show_all(grid);
	setDialogContent(dialog, grid);
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(PaletteFromImageArgs::onDestroy), args);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(PaletteFromImageArgs::onResponse), args);
	gtk_widget_show(dialog);
}
