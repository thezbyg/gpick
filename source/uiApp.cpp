/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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

#include "uiApp.h"
#include "ColorObject.h"
#include "ColorList.h"
#include "GlobalState.h"
#include "ColorSourceManager.h"
#include "IColorSource.h"
#include "Paths.h"
#include "AutoSave.h"
#include "Converter.h"
#include "Converters.h"
#include "StandardMenu.h"
#include "RegisterSources.h"
#include "GenerateScheme.h"
#include "ColorPicker.h"
#include "LayoutPreview.h"
#include "ImportExport.h"
#include "uiAbout.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "uiImportExport.h"
#include "uiDialogMix.h"
#include "uiDialogVariations.h"
#include "uiDialogGenerate.h"
#include "uiDialogEqualize.h"
#include "uiDialogEdit.h"
#include "uiDialogAutonumber.h"
#include "uiDialogSort.h"
#include "uiTemporaryPalette.h"
#include "uiColorDictionaries.h"
#include "uiTransformations.h"
#include "uiDialogOptions.h"
#include "uiConverter.h"
#include "uiStatusIcon.h"
#include "uiColorInput.h"
#include "tools/PaletteFromImage.h"
#include "tools/ColorSpaceSampler.h"
#include "tools/TextParser.h"
#include "tools/BackgroundColorPicker.h"
#include "dbus/Control.h"
#include "dynv/Map.h"
#include "common/Guard.h"
#include "FileFormat.h"
#include "Clipboard.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>
#include <filesystem>
#include <unordered_set>
using namespace std;

struct AppArgs
{
	GtkWidget *window;
	std::map<std::string, std::unique_ptr<IColorSource>, std::less<>> colorSourceMap;
	std::vector<IColorSource *> colorSourceIndex;
	list<string> recent_files;
	ColorSourceManager csm;
	IColorSource *current_color_source;
	std::unique_ptr<IColorSource> secondary_color_source;
	GtkWidget *secondary_source_widget;
	GtkWidget *secondary_source_scrolled_viewpoint;
	GtkWidget *secondary_source_container;
	GtkWidget *colorList;
	GtkWidget *notebook;
	GtkWidget *statusbar;
	GtkWidget *hpaned;
	GtkWidget *vpaned;
	GtkWidget *palette;
	uiStatusIcon* status_icon;
	FloatingPicker floatingPicker;
	StartupOptions startupOptions;
	dynv::Ref options;
	GlobalState *gs;
	string current_filename;
	bool current_filename_set;
	bool imported;
	GtkWidget *precision_loss_icon;
	gint x, y;
	gint width, height;
	bool initialization;
	dbus::Control dbus_control;
};

static void app_release(AppArgs *args);

static void update_recent_file_list(AppArgs *args, const char *filename, bool move_up)
{
	list<string>::iterator i = std::find(args->recent_files.begin(), args->recent_files.end(), string(filename));
	if (i == args->recent_files.end()){
		args->recent_files.push_front(string(filename));
		while (args->recent_files.size() > 10){
			args->recent_files.pop_back();
		}
	}else{
		if (move_up){
			args->recent_files.erase(i);
			args->recent_files.push_front(string(filename));
		}
	}
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, AppArgs *args)
{
	if (args->options->getBool("close_to_tray", false)){
		gtk_widget_hide(args->window);
		status_icon_set_visible(args->status_icon, true);
		return true;
	}
	return false;
}
static void detectLatinKeyGroup(GdkKeymap *, AppArgs *args) {
	GdkKeymapKey *keys;
	gint keyCount;
	args->gs->latinKeysGroup.reset();
	if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_for_display(gdk_display_get_default()), GDK_KEY_a, &keys, &keyCount)) {
		if (keyCount > 0) {
			for (size_t i = 0; i < static_cast<size_t>(keyCount); i++) {
				if (!args->gs->latinKeysGroup)
					args->gs->latinKeysGroup = keys[i].group;
				else if (keys[i].group >= 0 && static_cast<uint32_t>(keys[i].group) < *args->gs->latinKeysGroup)
					args->gs->latinKeysGroup = keys[i].group;
			}
		}
		g_free(keys);
	}
}

static gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, AppArgs *args)
{
	if (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED || event->new_window_state == GDK_WINDOW_STATE_WITHDRAWN){
		if (args->secondary_color_source) args->secondary_color_source->deactivate();
		if (args->current_color_source) args->current_color_source->deactivate();
	}else{
		if (args->secondary_color_source) args->secondary_color_source->activate();
		if (args->current_color_source) args->current_color_source->activate();
	}
	if (args->options->getBool("minimize_to_tray", false)){
		if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED){
			if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED){
				gtk_widget_hide(args->window);
				gtk_window_deiconify(GTK_WINDOW(args->window));
				status_icon_set_visible(args->status_icon, true);
				return true;
			}
		}
	}
	return false;
}

static gboolean on_window_key_press(GtkWidget *widget, GdkEventKey *event, AppArgs *args) {
	guint modifiers = gtk_accelerator_get_default_mod_mask();
	if (event->keyval == GDK_KEY_space && (event->state & modifiers) == GDK_CONTROL_MASK) {
		if (args->current_color_source != nullptr && IColorPicker::isColorPicker(*args->current_color_source)) {
			auto &colorPicker = *dynamic_cast<IColorPicker *>(args->current_color_source);
			colorPicker.focusSwatch();
			colorPicker.pick();
			return true;
		}
	}
	return false;
}

static gboolean on_window_configure(GtkWidget *widget, GdkEventConfigure *event, AppArgs *args)
{
	if (gtk_widget_get_visible(widget)){
		if (gdk_window_get_state(gtk_widget_get_window(widget)) & (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN | GDK_WINDOW_STATE_ICONIFIED)) {
			return false;
		}
		gint x, y;
		gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
		args->x = x;
		args->y = y;
		args->width = event->width;
		args->height = event->height;
		args->options->set("window.x", args->x);
		args->options->set("window.y", args->y);
		args->options->set("window.width", args->width);
		args->options->set("window.height", args->height);
	}
	return false;
}

static gboolean setFocusToColorPicker(IColorPicker *colorPicker) {
	colorPicker->focusSwatch();
	return false;
}
static void notebook_switch_cb(GtkNotebook *notebook, GtkWidget *page, guint page_num, AppArgs *args)
{
	if (args->current_color_source)
		args->current_color_source->deactivate();
	args->current_color_source = nullptr;
	if (page_num >= 0 && page_num <= args->colorSourceIndex.size() && args->colorSourceIndex[page_num]) {
		auto *colorSource = args->colorSourceIndex[page_num];
		if (!args->initialization) { // do not initialize color sources while initializing program
			colorSource->activate();
			if (IColorPicker::isColorPicker(*colorSource)) {
				g_idle_add((GSourceFunc)setFocusToColorPicker, colorSource);
			}
		}
		args->current_color_source = colorSource;
		args->gs->setCurrentColorSource(colorSource);
	}else{
		args->gs->setCurrentColorSource(nullptr);
	}
}

static void destroy_cb(GtkWidget *widget, AppArgs *args)
{
	g_signal_handlers_disconnect_matched(G_OBJECT(args->notebook), G_SIGNAL_MATCH_FUNC, 0, 0, nullptr, (void*)notebook_switch_cb, 0); //disconnect notebook switch callback, because destroying child widgets triggers it
	args->options->set("color_source", args->colorSourceIndex[gtk_notebook_get_current_page(GTK_NOTEBOOK(args->notebook))]->name());
	args->options->set("paned_position", gtk_paned_get_position(GTK_PANED(args->hpaned)));
	args->options->set("vertical_paned_position", gtk_paned_get_position(GTK_PANED(args->vpaned)));
	if (args->secondary_color_source){
		args->options->set("secondary_color_source", args->secondary_color_source->name());
	}else{
		args->options->set("secondary_color_source", "");
	}
	std::vector<int> columnWidths;
	for (size_t i = 0; ; ++i) {
		GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(args->colorList), i);
		if (!column)
			break;
		columnWidths.push_back(gtk_tree_view_column_get_width(column));
	}
	args->options->set("column_widths", columnWidths);
	args->options->set("window.x", args->x);
	args->options->set("window.y", args->y);
	args->options->set("window.width", args->width);
	args->options->set("window.height", args->height);
	app_release(args);
	gtk_main_quit();
}

static void app_update_program_name(AppArgs *args)
{
	stringstream program_title;
	if (!args->current_filename_set){
		program_title << _("New palette");
		if (args->precision_loss_icon)
			gtk_widget_hide(args->precision_loss_icon);
	}else{
		gchar* filename = g_path_get_basename(args->current_filename.c_str());
		if (args->imported){
			program_title << filename << " " << _("(Imported)");
			if (args->precision_loss_icon)
				gtk_widget_show(args->precision_loss_icon);
		}else{
			program_title << filename;
			if (args->precision_loss_icon)
				gtk_widget_hide(args->precision_loss_icon);
		}
		g_free(filename);
	}
	program_title << " - " << program_name;
	string title = program_title.str();
	gtk_window_set_title(GTK_WINDOW(args->window), title.c_str());
}

static void show_dialog_converter(GtkWidget *widget, AppArgs *args)
{
	dialog_converter_show(GTK_WINDOW(args->window), args->gs);
	return;
}

static void show_dialog_transformations(GtkWidget *widget, AppArgs *args)
{
	dialog_transformations_show(GTK_WINDOW(args->window), args->gs);
	return;
}

static void show_dialog_options(GtkWidget *widget, AppArgs *args)
{
	dialog_options_show(GTK_WINDOW(args->window), args->gs);
	return;
}

static void show_dialog_color_dictionaries(GtkWidget *widget, AppArgs *args)
{
	dialog_color_dictionaries_show(GTK_WINDOW(args->window), args->gs);
	return;
}

static void menu_file_new(GtkWidget *widget, AppArgs *args)
{
	args->current_filename_set = false;
	args->gs->colorList().removeAll();
	app_update_program_name(args);
}

int app_save_file(AppArgs *args, const char *filename, const char *filter)
{
	using namespace std::filesystem;
	string current_filename;
	if (filename != nullptr){
		current_filename = filename;
	}else{
		if (!args->current_filename_set) return -1;
		current_filename = args->current_filename;
	}
	FileType filetype;
	ImportExport importExport(args->gs->colorList(), current_filename.c_str(), *args->gs);
	importExport.fixFileExtension(filter);
	current_filename = importExport.getFilename();
	bool returnValue = false;
	switch (filetype = ImportExport::getFileType(current_filename.c_str())){
		case FileType::gpl:
			returnValue = importExport.exportGPL();
			break;
		case FileType::ase:
			returnValue = importExport.exportASE();
			break;
		case FileType::gpa:
			returnValue = importExport.exportGPA();
			break;
		default:
			returnValue = importExport.exportGPA();
	}
	if (returnValue){
		if (filetype == FileType::gpa || filetype == FileType::unknown){
			args->imported = false;
		}else{
			args->imported = true;
		}
		args->current_filename = current_filename;
		args->current_filename_set = true;
		app_update_program_name(args);
		update_recent_file_list(args, current_filename.c_str(), true);
		return 0;
	}else{
		app_update_program_name(args);
		return -1;
	}
	return 0;
}

int app_load_file(AppArgs *args, const std::string &filename, ColorList &colorList, bool autoload)
{
	bool imported = false;
	bool returnValue = false;
	ImportExport importExport(colorList, filename.c_str(), *args->gs);
	switch (ImportExport::getFileType(filename.c_str())){
		case FileType::gpl:
			returnValue = importExport.importGPL();
			imported = true;
			break;
		case FileType::ase:
			returnValue = importExport.importASE();
			imported = true;
			break;
		case FileType::gpa:
			returnValue = importExport.importGPA();
			break;
		default:
			returnValue = importExport.importGPA();
	}
	args->current_filename_set = false;
	if (returnValue){
		if (imported){
			args->imported = true;
		}else{
			args->imported = false;
		}
		if (!autoload){
			args->current_filename = filename;
			args->current_filename_set = true;
			update_recent_file_list(args, filename.c_str(), false);
		}
		app_update_program_name(args);
	}else{
		app_update_program_name(args);
		return -1;
	}
	return 0;
}
int app_load_file(AppArgs *args, const std::string &filename, bool autoload)
{
	int result = 0;
	ColorList colorList;
	if ((result = app_load_file(args, filename, colorList, autoload)) == 0){
		auto &destination = args->gs->colorList();
		common::Guard colorListGuard = destination.changeGuard();
		destination.removeAll();
		destination.add(colorList);
	}
	return result;
}

int app_parse_geometry(AppArgs *args, const char *geometry)
{
	gtk_window_parse_geometry(GTK_WINDOW(args->window), geometry);
	return 0;
}

static void menu_file_revert(GtkWidget *widget, AppArgs *args)
{
	if (!args->current_filename_set) return;
	if (app_load_file(args, args->current_filename.c_str()) == 0){
	}else{
		GtkWidget* message;
		message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
		gtk_window_set_title(GTK_WINDOW(message), _("Open"));
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
	}
}

static void menu_file_open_last(GtkWidget *widget, AppArgs *args)
{
	const char *filename = args->recent_files.begin()->c_str();
	if (app_load_file(args, filename) == 0){
	}else{
		GtkWidget* message;
		message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
		gtk_window_set_title(GTK_WINDOW(message), _("Open"));
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
	}
}

static void menu_file_open_nth(GtkWidget *widget, AppArgs *args)
{
	list<string>::iterator i = args->recent_files.begin();
	uintptr_t index = (uintptr_t)g_object_get_data(G_OBJECT(widget), "index");
	std::advance(i, index);
	const char *filename = (*i).c_str();
	if (app_load_file(args, filename) == 0){
	}else{
		GtkWidget* message;
		message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
		gtk_window_set_title(GTK_WINDOW(message), _("Open"));
		gtk_dialog_run(GTK_DIALOG(message));
		gtk_widget_destroy(message);
	}
}

static void add_file_filters(GtkWidget *dialog, const char *selected_filter)
{
	const struct{
		FileType type;
		const char *label;
		const char *filter;
	}filters[] = {
		{FileType::gpa, _("Gpick Palette (*.gpa)"), "*.gpa"},
		{FileType::gpl, _("GIMP/Inkscape Palette (*.gpl)"), "*.gpl"},
		{FileType::ase, _("Adobe Swatch Exchange (*.ase)"), "*.ase"},
		{FileType::unknown, 0, 0},
	};
	GtkFileFilter *filter;
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	gtk_file_filter_add_pattern(filter, "*");
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer)"all");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (g_strcmp0(selected_filter, "all") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All supported formats"));
	for (gint i = 0; filters[i].type != FileType::unknown; ++i) {
		gtk_file_filter_add_pattern(filter, filters[i].filter);
	}
	if (g_strcmp0(selected_filter, "all_supported") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer)"all_supported");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	for (gint i = 0; filters[i].type != FileType::unknown; ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, filters[i].label);
		gtk_file_filter_add_pattern(filter, filters[i].filter);
		g_object_set_data(G_OBJECT(filter), "identification", (gpointer)filters[i].filter);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if (g_strcmp0(filters[i].filter, selected_filter) == 0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
}

static void menu_file_open(GtkWidget *widget, AppArgs *args)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new(_("Open File"), GTK_WINDOW(gtk_widget_get_toplevel(widget)),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	auto default_path = args->options->getString("open.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path.c_str());
	auto selected_filter = args->options->getString("open.filter", "all_supported");
	add_file_filters(dialog, selected_filter.c_str());
	gboolean finished = FALSE;
	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			args->options->set("open.path", path);
			g_free(path);
			const char *identification = (const char*)g_object_get_data(G_OBJECT(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog))), "identification");
			args->options->set("open.filter", identification);
			if (app_load_file(args, filename) == 0){
				finished = TRUE;
			}else{
				GtkWidget* message;
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be opened"));
				gtk_window_set_title(GTK_WINDOW(dialog), _("Open"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy (dialog);
}

static void menu_file_save_as(GtkWidget *widget, AppArgs *args)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new(_("Save As"), GTK_WINDOW(gtk_widget_get_toplevel(widget)),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	auto default_path = args->options->getString("save.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path.c_str());
	auto selected_filter = args->options->getString("save.filter", "all_supported");
	add_file_filters(dialog, selected_filter.c_str());
	gboolean finished = FALSE;
	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path;
			path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			args->options->set("save.path", path);
			g_free(path);
			const char *identification = (const char*)g_object_get_data(G_OBJECT(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog))), "identification");
			args->options->set("save.filter", identification);
			if (app_save_file(args, filename, identification) == 0){
				finished = TRUE;
			}else{
				GtkWidget* message;
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
				gtk_window_set_title(GTK_WINDOW(message), _("Save As"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy (dialog);
}
static void menu_file_save(GtkWidget *widget, AppArgs *args) {
	if (!args->current_filename_set){
		menu_file_save_as(widget, args); // if file has no name, "Save As" dialog is shown instead.
	}else{
		if (app_save_file(args, nullptr, nullptr) == 0){
		}else{
			GtkWidget* message;
			message = gtk_message_dialog_new(GTK_WINDOW(args->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
			gtk_window_set_title(GTK_WINDOW(message), _("Save"));
			gtk_dialog_run(GTK_DIALOG(message));
			gtk_widget_destroy(message);
		}
	}
}
static void menu_file_export_all(GtkWidget *widget, AppArgs *args) {
	ImportExportDialog importExportDialog(GTK_WINDOW(args->window), args->gs->colorList(), args->gs);
	importExportDialog.showExport();
}
static void menu_file_import(GtkWidget *widget, AppArgs *args) {
	ImportExportDialog importExportDialog(GTK_WINDOW(args->window), args->gs->colorList(), args->gs);
	importExportDialog.showImport();
}
static void menu_file_import_text_file(GtkWidget *widget, AppArgs *args)
{
	ImportExportDialog importExportDialog(GTK_WINDOW(args->window), args->gs->colorList(), args->gs);
	importExportDialog.showImportTextFile();
}
static void menu_file_export(GtkWidget *widget, AppArgs *args) {
	ColorList colorList;
	palette_list_get_selected(args->colorList, colorList);
	ImportExportDialog importExportDialog(GTK_WINDOW(args->window), colorList, args->gs);
	importExportDialog.showExport();
}

struct FileMenuItems {
	GtkWidget *export_all;
	GtkWidget *export_selected;
	GtkWidget *recent_files;
	GtkWidget *revert;
};

static void menu_file_activate(GtkWidget *widget, gpointer data)
{
	AppArgs* args = (AppArgs*)data;
	gint32 selected_count = palette_list_get_selected_count(args->colorList);
	gint32 total_count = palette_list_get_count(args->colorList);
	FileMenuItems *items = (FileMenuItems*) g_object_get_data(G_OBJECT(widget), "items");
	gtk_widget_set_sensitive(items->export_all, (total_count >= 1));
	gtk_widget_set_sensitive(items->export_selected, (selected_count >= 1));
	gtk_widget_set_sensitive(items->revert, args->current_filename_set);
	if (args->recent_files.size() > 0){
		GtkMenu *menu2 = GTK_MENU(gtk_menu_new());
		GtkWidget *item;
		item = newMenuItem(_("Open Last File"), GTK_STOCK_OPEN);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu2), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(menu_file_open_last), args);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu2), gtk_separator_menu_item_new ());
		uintptr_t j = 0;
		for (list<string>::iterator i = args->recent_files.begin(); i != args->recent_files.end(); i++){
			item = gtk_menu_item_new_with_label ((*i).c_str());
			gtk_menu_shell_append (GTK_MENU_SHELL (menu2), item);
			g_object_set_data_full(G_OBJECT(item), "index", (void*)j, (GDestroyNotify)nullptr);
			g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(menu_file_open_nth), args);
			j++;
		}
		gtk_widget_show_all(GTK_WIDGET(menu2));
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (items->recent_files), GTK_WIDGET(menu2));
		gtk_widget_set_sensitive(items->recent_files, true);
	}else{
		gtk_widget_set_sensitive(items->recent_files, false);
	}
}

static void floating_picker_show_cb(GtkWidget *widget, AppArgs* args)
{
	floating_picker_activate(args->floatingPicker, false, false, nullptr);
}

static void show_about_box_cb(GtkWidget *widget, AppArgs* args)
{
	show_about_box(args->window);
}

static void repositionViews(AppArgs* args)
{
	bool palette = args->options->getBool("view.palette", true);
	bool primary_view = args->options->getBool("view.primary_view", true);
	bool secondary_view = gtk_widget_get_visible(args->secondary_source_container);
	string layout = args->options->getString("view.layout", "primary+secondary_palette");
	GtkWidget *widgets[4] = { args->palette, args->vpaned, args->secondary_source_container, args->notebook };
	for (size_t i = 0; i < sizeof(widgets) / sizeof(GtkWidget*); i++){
		g_object_ref(widgets[i]);
		GtkWidget *parent = gtk_widget_get_parent(widgets[i]);
		if (parent != nullptr)
			gtk_container_remove(GTK_CONTAINER(parent), widgets[i]);
	}
	struct NamedLayoutWidget{
		const char *name;
		GtkWidget *widget;
		bool visible;
	}named_widgets[] = {
		{"palette", args->palette, palette},
		{"primary", args->notebook, primary_view},
		{"secondary", args->secondary_source_container, secondary_view},
	};
	NamedLayoutWidget *left[2], *right[2];
	int left_index = 0, right_index = 0;
	bool left_side = true;
	size_t previous_position = 0;
	string name;
	for (;;){
		size_t split_position = layout.find_first_of("_+", previous_position);
		char type;
		if (split_position != string::npos){
			name = layout.substr(previous_position, split_position - previous_position);
			type = layout[split_position];
		}else{
			name = layout.substr(previous_position);
			type = 0;
		}
		for (size_t i = 0; i < sizeof(named_widgets) / sizeof(NamedLayoutWidget); i++){
			if (name == named_widgets[i].name){
				if (!named_widgets[i].visible)
					break;
				if (left_side)
					left[left_index++] = &named_widgets[i];
				else
					right[right_index++] = &named_widgets[i];
				break;
			}
		}
		previous_position = split_position + 1;
		if (type == '_')
			left_side = false;
		else if (type == 0)
			break;
	}
	bool vpaned_unused = false;
	if (left_index != 0 && right_index != 0){
		if (left_index > 1){
			gtk_paned_pack1(GTK_PANED(args->hpaned), args->vpaned, false, false);
			gtk_paned_pack2(GTK_PANED(args->hpaned), right[0]->widget, true, false);
			gtk_paned_pack1(GTK_PANED(args->vpaned), left[0]->widget, false, false);
			gtk_paned_pack2(GTK_PANED(args->vpaned), left[1]->widget, true, false);
		}else if (right_index > 1){
			gtk_paned_pack1(GTK_PANED(args->hpaned), left[0]->widget, false, false);
			gtk_paned_pack2(GTK_PANED(args->hpaned), args->vpaned, true, false);
			gtk_paned_pack1(GTK_PANED(args->vpaned), right[0]->widget, false, false);
			gtk_paned_pack2(GTK_PANED(args->vpaned), right[1]->widget, true, false);
		}else{
			gtk_paned_pack1(GTK_PANED(args->hpaned), left[0]->widget, false, false);
			gtk_paned_pack2(GTK_PANED(args->hpaned), right[0]->widget, true, false);
			vpaned_unused = true;
		}
		gtk_widget_show(GTK_WIDGET(args->hpaned));
	}else if (left_index != 0 || right_index != 0){
		gtk_widget_hide(GTK_WIDGET(args->hpaned));
		GtkWidget *widget = nullptr;
		if (left_index > 1){
			widget = args->vpaned;
			gtk_paned_pack1(GTK_PANED(args->vpaned), left[0]->widget, false, false);
			gtk_paned_pack2(GTK_PANED(args->vpaned), left[1]->widget, true, false);
		}else if (right_index > 1){
			widget = args->vpaned;
			gtk_paned_pack1(GTK_PANED(args->vpaned), right[0]->widget, false, false);
			gtk_paned_pack2(GTK_PANED(args->vpaned), right[1]->widget, true, false);
		}else if (left_index == 1){
			widget = left[0]->widget;
			vpaned_unused = true;
		}else if (right_index == 1){
			widget = right[0]->widget;
			vpaned_unused = true;
		}
		if (widget)
			gtk_box_pack_start(GTK_BOX(gtk_widget_get_parent(args->hpaned)), widget, true, true, 5);
	}else{
		gtk_widget_hide(GTK_WIDGET(args->hpaned));
		vpaned_unused = true;
	}
	if (vpaned_unused){
		gtk_widget_hide(args->vpaned);
		gtk_box_pack_start(GTK_BOX(gtk_widget_get_parent(args->hpaned)), args->vpaned, true, true, 5);
	}else{
		gtk_widget_show(args->vpaned);
	}
	for (size_t i = 0; i < sizeof(named_widgets) / sizeof(NamedLayoutWidget); i++){
		bool used = false;
		for (int j = 0; j < left_index; j++){
			if (left[j]->widget == named_widgets[i].widget){
				used = true;
				break;
			}
		}
		for (int j = 0; j < right_index; j++){
			if (right[j]->widget == named_widgets[i].widget){
				used = true;
				break;
			}
		}
		if (!used){
			gtk_widget_hide(named_widgets[i].widget);
			gtk_box_pack_start(GTK_BOX(gtk_widget_get_parent(args->hpaned)), named_widgets[i].widget, true, true, 5);
		}else{
			gtk_widget_show(named_widgets[i].widget);
		}
	}
	for (size_t i = 0; i < sizeof(widgets) / sizeof(GtkWidget*); i++){
		g_object_unref(widgets[i]);
	}
}
static void view_palette_cb(GtkWidget *widget, AppArgs* args)
{
	bool view = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
	args->options->set("view.palette", view);
	repositionViews(args);
}
static void view_primary_view_cb(GtkWidget *widget, AppArgs* args)
{
	bool view = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
	args->options->set("view.primary_view", view);
	repositionViews(args);
}
static void view_layout_cb(GtkWidget *widget, AppArgs* args)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))){
		args->options->set("view.layout", static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "source")));
		repositionViews(args);
	}
}
static void view_new_temporary_palette_cb(GtkWidget *widget, AppArgs* args)
{
	temporary_palette_show(GTK_WINDOW(args->window), *args->gs);
}

static void palette_from_image_cb(GtkWidget *widget, AppArgs* args)
{
	tools_palette_from_image_show(GTK_WINDOW(args->window), args->gs);
}
static void color_space_sampler_cb(GtkWidget *widget, AppArgs* args)
{
	tools_color_space_sampler_show(GTK_WINDOW(args->window), args->gs);
}
static void text_parser_cb(GtkWidget *widget, AppArgs* args)
{
	tools_text_parser_show(GTK_WINDOW(args->window), args->gs);
}
static void background_color_picker_cb(GtkWidget *widget, AppArgs* args)
{
	tools_background_color_picker(GTK_WINDOW(args->window), *args->gs);
}
static void destroy_file_menu_items(FileMenuItems *items)
{
	delete items;
}

static void activate_secondary_source(AppArgs *args, const ColorSourceManager::Registration *registration)
{
	if (args->secondary_color_source){
		args->secondary_color_source->deactivate();
		args->secondary_color_source.reset();
		args->secondary_source_widget = nullptr;
		if (args->secondary_source_scrolled_viewpoint)
			gtk_container_remove(GTK_CONTAINER(args->secondary_source_container), args->secondary_source_scrolled_viewpoint);
		args->secondary_source_scrolled_viewpoint = 0;
		if (!registration)
			gtk_widget_hide(args->secondary_source_container);
	}
	if (!registration)
		return;
	std::string namespace_str = "gpick.secondary_view." + registration->name;
	auto options = args->gs->settings().getOrCreateMap(namespace_str);
	auto source = registration->build(*args->gs, options);
	GtkWidget *new_widget = source->getWidget();
	if (registration->needsViewport) {
		GtkWidget *scrolled_window = gtk_scrolled_window_new(0,0);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_NONE);
		gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), new_widget);
		args->secondary_source_scrolled_viewpoint = scrolled_window;
		gtk_box_pack_start(GTK_BOX(args->secondary_source_container), args->secondary_source_scrolled_viewpoint, true, true, 0);
		gtk_widget_show(args->secondary_source_scrolled_viewpoint);
	}else{
		gtk_box_pack_start(GTK_BOX(args->secondary_source_container), new_widget, true, true, 0);
		gtk_widget_show(new_widget);
	}
	gtk_widget_show(args->secondary_source_container);
	source->activate();
	args->secondary_color_source = std::move(source);
	args->secondary_source_widget = new_widget;
}

static void secondary_view_cb(GtkWidget *widget, AppArgs *args)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))){
		auto *source = static_cast<ColorSourceManager::Registration *>(g_object_get_data(G_OBJECT(widget), "source"));
		activate_secondary_source(args, source);
		repositionViews(args);
	}
}

static void addLayoutMenuItem(const string &layout, GtkMenu *menu, GSList *&group, AppArgs *args)
{
	const char *components[] = {
		 _("Primary"),
		 _("Palette"),
		 _("Secondary"),
	};
	string label;
	size_t previous_position = 0;
	for (;;){
		size_t split_position = layout.find_first_of("_+", previous_position);
		char type;
		string name;
		if (split_position != string::npos){
			name = layout.substr(previous_position, split_position - previous_position);
			type = layout[split_position];
		}else{
			name = layout.substr(previous_position);
			type = 0;
		}
		if (name == "primary"){
			label += components[0];
		}else if (name == "palette"){
			label += components[1];
		}else if (name == "secondary"){
			label += components[2];
		}
		if (type == '_'){
			label += ", ";
		}else if (type == '+'){
			label += "/";
		}else if (type == 0){
			break;
		}
		previous_position = split_position + 1;
	}
	GtkWidget *item = gtk_radio_menu_item_new_with_label(group, label.c_str());
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	setWidgetData(item, "source", layout);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(view_layout_cb), args);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	string current_layout = args->options->getString("view.layout", "primary+secondary_palette");
	if (current_layout == layout)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
}

static void create_menu(GtkMenuBar *menu_bar, AppArgs *args, GtkAccelGroup *accel_group)
{
	GtkMenu *menu;
	GtkWidget *item;
	GtkWidget* file_item;
	GtkStockItem stock_item;
	menu = GTK_MENU(gtk_menu_new());
	FileMenuItems *items = new FileMenuItems();
	if (gtk_stock_lookup(GTK_STOCK_NEW, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append(GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator(item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(menu_file_new), args);
	}
	if (gtk_stock_lookup(GTK_STOCK_OPEN, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator(item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_file_open), args);
	}
	item = gtk_menu_item_new_with_mnemonic(_("Recent _Files"));
	items->recent_files = item;
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	if (gtk_stock_lookup(GTK_STOCK_REVERT_TO_SAVED, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_r, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_F5, GdkModifierType(0), GTK_ACCEL_VISIBLE);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_file_revert), args);
		items->revert = item;
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new ());
	if (gtk_stock_lookup(GTK_STOCK_SAVE, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(menu_file_save), args);
	}
	if (gtk_stock_lookup(GTK_STOCK_SAVE_AS, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(menu_file_save_as), args);
	}
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
	item = gtk_image_menu_item_new_with_mnemonic(_("Ex_port..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_e, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(menu_file_export_all), args);
	items->export_all = item;
	item = gtk_image_menu_item_new_with_mnemonic(_("Expo_rt Selected..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_e, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(menu_file_export), args);
	items->export_selected = item;
	item = gtk_image_menu_item_new_with_mnemonic(_("_Import..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_i, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(menu_file_import), args);

	item = gtk_image_menu_item_new_with_mnemonic(_("Import _Text File..."));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_i, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(menu_file_import_text_file), args);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
	if (gtk_stock_lookup(GTK_STOCK_QUIT, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(destroy_cb), args);
	}
	file_item = gtk_menu_item_new_with_mnemonic(_("_File"));
	g_signal_connect (G_OBJECT (file_item), "activate", G_CALLBACK(menu_file_activate), args);
	g_object_set_data_full(G_OBJECT(file_item), "items", items, (GDestroyNotify)destroy_file_menu_items);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
	menu = GTK_MENU(gtk_menu_new());
	item = newMenuItem(_("Edit _Converters..."), GTK_STOCK_PROPERTIES);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(show_dialog_converter), args);
	item = newMenuItem(_("Display _Filters..."), GTK_STOCK_PROPERTIES);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(show_dialog_transformations), args);
	item = newMenuItem(_("Color _Dictionaries..."), GTK_STOCK_PROPERTIES);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(show_dialog_color_dictionaries), args);
	if (gtk_stock_lookup(GTK_STOCK_PREFERENCES, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(show_dialog_options), args);
	}
	file_item = gtk_menu_item_new_with_mnemonic(_("_Edit"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
	menu = GTK_MENU(gtk_menu_new());

	GtkMenu *menu2 = GTK_MENU(gtk_menu_new());
	GSList *group = nullptr;
	addLayoutMenuItem("primary+secondary_palette", menu2, group, args);
	addLayoutMenuItem("primary+palette_secondary", menu2, group, args);
	addLayoutMenuItem("primary_palette+secondary", menu2, group, args);
	addLayoutMenuItem("palette+secondary_primary", menu2, group, args);
	addLayoutMenuItem("palette+primary_secondary", menu2, group, args);
	addLayoutMenuItem("palette_primary+secondary", menu2, group, args);
	addLayoutMenuItem("secondary+primary_palette", menu2, group, args);
	addLayoutMenuItem("secondary+palette_primary", menu2, group, args);
	addLayoutMenuItem("secondary_primary+palette", menu2, group, args);
	item = gtk_menu_item_new_with_mnemonic(_("_Layout"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), GTK_WIDGET(menu2));

	menu2 = GTK_MENU(gtk_menu_new());
	group = nullptr;
	auto colorSourceName = args->options->getString("secondary_color_source", "");
	item = gtk_radio_menu_item_new_with_label(group, _("None"));
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
	if (colorSourceName.empty() || !args->csm.has(colorSourceName))
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
	g_object_set_data_full(G_OBJECT(item), "source", nullptr, (GDestroyNotify)nullptr);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(secondary_view_cb), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_n, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu2), item);
	args->csm.visit([args, accel_group, menu2, &colorSourceName, &group](const ColorSourceManager::Registration &registration) {
		if (registration.singleInstanceOnly)
			return;
		auto item = gtk_radio_menu_item_new_with_label(group, registration.label.c_str());
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
		if (registration.name == colorSourceName)
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), true);
		g_object_set_data_full(G_OBJECT(item), "source", const_cast<ColorSourceManager::Registration *>(&registration), (GDestroyNotify)nullptr);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(secondary_view_cb), args);
		if (registration.defaultAccelerator)
			gtk_widget_add_accelerator(item, "activate", accel_group, registration.defaultAccelerator, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu2), item);
	});
	item = gtk_menu_item_new_with_mnemonic(_("_Secondary View"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), GTK_WIDGET(menu2));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	item = gtk_check_menu_item_new_with_mnemonic(_("_Primary View"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), args->options->getBool("view.primary_view", true));
	g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(view_primary_view_cb), args);
	item = gtk_check_menu_item_new_with_mnemonic(_("Palette"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), args->options->getBool("view.palette", true));
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_p, GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(item), "toggled", G_CALLBACK(view_palette_cb), args);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_mnemonic(_("New Temporary Palette"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(view_new_temporary_palette_cb), args);

	file_item = gtk_menu_item_new_with_mnemonic (_("_View"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
	menu = GTK_MENU(gtk_menu_new());
	item = gtk_menu_item_new_with_mnemonic(_("_Pick colors..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_p, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(floating_picker_show_cb), args);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_mnemonic(_("Palette From _Image..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_from_image_cb), args);
	item = gtk_menu_item_new_with_mnemonic(_("Color Space _Sampler..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(color_space_sampler_cb), args);
	item = gtk_menu_item_new_with_mnemonic(_("_Text Parser..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(text_parser_cb), args);
	item = gtk_menu_item_new_with_mnemonic(_("_Background Color Picker..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(background_color_picker_cb), args);
	file_item = gtk_menu_item_new_with_mnemonic(_("_Tools"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), GTK_WIDGET(menu));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);
	menu = GTK_MENU(gtk_menu_new());
	if (gtk_stock_lookup(GTK_STOCK_ABOUT, &stock_item)){
		item = newMenuItem(stock_item.label, stock_item.stock_id);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		if (stock_item.keyval) gtk_widget_add_accelerator (item, "activate", accel_group, stock_item.keyval, stock_item.modifier, GTK_ACCEL_VISIBLE);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK(show_about_box_cb), args);
	}
	file_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item),GTK_WIDGET( menu));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);
}

static void palette_popup_menu_remove_all(GtkWidget *widget, AppArgs *args) {
	args->gs->colorList().removeAll();
}
static void palette_popup_menu_remove_selected(GtkWidget *widget, AppArgs *args) {
	auto selected = palette_list_get_selected(args->colorList);
	args->gs->colorList().remove([&](ColorObject *colorObject) {
		return selected.count(colorObject) != 0;
	}, true, true);
}
static void palette_popup_menu_clear_names(GtkWidget *widget, AppArgs *args) {
	palette_list_foreach(args->colorList, true, [](ColorObject *colorObject) {
		colorObject->setName("");
		return Update::name;
	});
}
static void palette_popup_menu_auto_name(GtkWidget *widget, AppArgs *args) {
	auto *colorNames = args->gs->getColorNames();
	bool imprecisionPostfix = args->gs->settings().getBool("gpick.color_names.imprecision_postfix", false);
	palette_list_foreach(args->colorList, true, [imprecisionPostfix, colorNames](ColorObject *colorObject) {
		Color color = colorObject->getColor();
		colorObject->setName(color_names_get(colorNames, &color, imprecisionPostfix));
		return Update::name;
	});
}
static void palette_popup_menu_auto_number(GtkWidget *, AppArgs *args) {
	dialog_autonumber_show(GTK_WINDOW(args->window), args->colorList, *args->gs);
}
static void palette_popup_menu_reverse(GtkWidget *widget, AppArgs *args) {
	ColorList colorList;
	palette_list_get_selected(args->colorList, colorList);
	std::reverse(colorList.begin(), colorList.end());
	auto i = colorList.begin();
	palette_list_foreach(args->colorList, true, [&i](ColorObject **colorObject) {
		*colorObject = *i;
		++i;
		return Update::none;
	});
	auto selected = palette_list_get_selected(args->colorList);
	i = colorList.begin();
	for (auto *&colorObject: args->gs->colorList()) {
		if (selected.count(colorObject) != 0) {
			colorObject = *i;
			++i;
		}
	}
}
static void palette_popup_menu_group_and_sort(GtkWidget *widget, AppArgs *args) {
	ColorList colorList, sortedColorList;
	palette_list_get_selected(args->colorList, colorList);
	if (dialog_sort_show(GTK_WINDOW(args->window), &colorList, &sortedColorList, args->gs)) {
		auto i = sortedColorList.begin();
		palette_list_foreach(args->colorList, true, [&i](ColorObject **colorObject) {
			*colorObject = *i;
			++i;
			return Update::none;
		});
		auto selected = palette_list_get_selected(args->colorList);
		i = sortedColorList.begin();
		for (auto *&colorObject: args->gs->colorList()) {
			if (selected.count(colorObject) != 0) {
				colorObject = *i;
				++i;
			}
		}
	}
}
static void palette_popup_menu_add(GtkWidget *widget, AppArgs *args) {
	common::Ref<ColorObject> newColorObject;
	if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), *args->gs, std::nullopt, true, newColorObject) == 0){
		args->gs->colorList().add(newColorObject.pointer());
	}
}
static void palette_popup_menu_edit(GtkWidget *widget, AppArgs *args) {
	auto selectedCount = palette_list_get_selected_count(args->colorList);
	if (selectedCount == 0)
		return;
	if (selectedCount == 1) {
		ColorObject *colorObject = palette_list_get_first_selected(args->colorList)->reference();
		common::Ref<ColorObject> newColorObject;
		if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(widget)), *args->gs, *colorObject, true, newColorObject) == 0){
			colorObject->setColor(newColorObject->getColor());
			colorObject->setName(newColorObject->getName());
			palette_list_update_first_selected(args->colorList, false, true);
		}
		colorObject->release();
	} else {
		ColorList colorList;
		palette_list_get_selected(args->colorList, colorList);
		dialog_edit_show(GTK_WINDOW(args->window), colorList, *args->gs);
		palette_list_update_selected(args->colorList, false, true);
	}
}
static void palette_popup_menu_paste(GtkWidget *, AppArgs *args) {
	auto newColorList = clipboard::getColors(*args->gs);
	if (!newColorList)
		return;
	args->gs->colorList().add(*newColorList);
}
static void palette_popup_menu_mix(GtkWidget *widget, AppArgs *args) {
	ColorList colorList;
	palette_list_get_selected(args->colorList, colorList);
	dialog_mix_show(GTK_WINDOW(args->window), colorList, *args->gs);
}

static void palette_popup_menu_variations(GtkWidget *widget, AppArgs *args) {
	ColorList colorList;
	palette_list_get_selected(args->colorList, colorList);
	dialog_variations_show(GTK_WINDOW(args->window), colorList, *args->gs);
}
static void palette_popup_menu_generate(GtkWidget *widget, AppArgs *args) {
	ColorList colorList;
	palette_list_get_selected(args->colorList, colorList);
	dialog_generate_show(GTK_WINDOW(args->window), colorList, *args->gs);
}
static void palette_popup_menu_equalize(GtkWidget *widget, AppArgs *args) {
	ColorList colorList;
	palette_list_get_selected(args->colorList, colorList);
	dialog_equalize_show(GTK_WINDOW(args->window), colorList, *args->gs);
	palette_list_update_selected(args->colorList, false, true);
}
static gboolean palette_popup_menu_show(GtkWidget *widget, GdkEventButton *event, AppArgs *args) {
	auto menu = gtk_menu_new();
	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);
	gint32 selected_count = palette_list_get_selected_count(args->colorList);
	gint32 total_count = palette_list_get_count(args->colorList);
	if (total_count > 0 && selected_count >= 1){
		palette_list_append_copy_menu(args->colorList, menu);
	}else{
		StandardMenu::appendMenu(menu);
	}
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	auto item = newMenuItem(_("_Add..."), GTK_STOCK_NEW);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_popup_menu_add), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_n, GdkModifierType(), GTK_ACCEL_VISIBLE);

	item = newMenuItem(_("_Edit..."), GTK_STOCK_EDIT);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_popup_menu_edit), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_e, GdkModifierType(), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, selected_count > 0);

	item = newMenuItem(_("_Paste"), GTK_STOCK_PASTE);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_v, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_popup_menu_paste), args);
	gtk_widget_set_sensitive(item, clipboard::colorObjectAvailable());

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	item = newMenuItem (_("_Mix Colors..."), GTK_STOCK_CONVERT);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_mix), args);
	gtk_widget_set_sensitive(item, (selected_count >= 2));
	item = newMenuItem (_("_Variations..."), GTK_STOCK_CONVERT);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_variations), args);
	gtk_widget_set_sensitive(item, (selected_count >= 1));
	item = newMenuItem (_("_Generate..."), GTK_STOCK_CONVERT);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_generate), args);
	gtk_widget_set_sensitive(item, (selected_count >= 1));
	item = newMenuItem(_("E_qualize..."), GTK_STOCK_CONVERT);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_popup_menu_equalize), args);
	gtk_widget_set_sensitive(item, selected_count > 1);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
	item = gtk_menu_item_new_with_mnemonic (_("C_lear names"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_clear_names), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_c, GdkModifierType(GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 1));
	item = gtk_menu_item_new_with_mnemonic (_("Autona_me"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_auto_name), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_a, GdkModifierType(GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 1));
	item = gtk_menu_item_new_with_mnemonic (_("Auto_number..."));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_auto_number), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_n, GdkModifierType(GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 1));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
	item = gtk_menu_item_new_with_mnemonic(_("R_everse"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_reverse), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_r, GdkModifierType(GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 2));
	item = gtk_menu_item_new_with_mnemonic(_("Group and _sort..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(palette_popup_menu_group_and_sort), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_g, GdkModifierType(GDK_SHIFT_MASK), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 2));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
	item = newMenuItem(_("_Remove"), GTK_STOCK_REMOVE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_remove_selected), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_Delete, GdkModifierType(0), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (selected_count >= 1));
	item = newMenuItem(_("Remove All"), GTK_STOCK_REMOVE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(palette_popup_menu_remove_all), args);
	gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_Delete, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_widget_set_sensitive(item, (total_count >= 1));
	showContextMenu(menu, event);
	g_object_ref_sink(G_OBJECT(accel_group));
	g_object_unref(G_OBJECT(accel_group));
	return TRUE;
}

static void on_palette_popup_menu(GtkWidget *widget, AppArgs *args)
{
	palette_popup_menu_show(widget, 0, args);
}

static gboolean on_palette_button_press(GtkWidget *widget, GdkEventButton *event, AppArgs *args)
{
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
		return palette_popup_menu_show(widget, event, args);
	}
	return FALSE;
}

static gboolean on_palette_list_key_press(GtkWidget *widget, GdkEventKey *event, AppArgs *args) {
	guint state = event->state & gtk_accelerator_get_default_mod_mask();
	guint keyval;
	switch ((keyval = getKeyval(*event, args->gs->latinKeysGroup))) {
		case GDK_KEY_1:
		case GDK_KEY_KP_1:
		case GDK_KEY_2:
		case GDK_KEY_KP_2:
		case GDK_KEY_3:
		case GDK_KEY_KP_3:
		case GDK_KEY_4:
		case GDK_KEY_KP_4:
		case GDK_KEY_5:
		case GDK_KEY_KP_5:
		case GDK_KEY_6:
		case GDK_KEY_KP_6:
			{
				ColorObject *selected = palette_list_get_first_selected(args->colorList);
				if (selected) {
					IColorSource *colorSource = args->gs->getCurrentColorSource();
					uint32_t color_index = 0;
					switch(keyval)
					{
						case GDK_KEY_KP_1:
						case GDK_KEY_1: color_index = 0; break;
						case GDK_KEY_KP_2:
						case GDK_KEY_2: color_index = 1; break;
						case GDK_KEY_KP_3:
						case GDK_KEY_3: color_index = 2; break;
						case GDK_KEY_KP_4:
						case GDK_KEY_4: color_index = 3; break;
						case GDK_KEY_KP_5:
						case GDK_KEY_5: color_index = 4; break;
						case GDK_KEY_KP_6:
						case GDK_KEY_6: color_index = 5; break;
					}
					if (state == GDK_CONTROL_MASK) {
						auto colorObject = colorSource->getNthColor(color_index);
						selected->setColor(colorObject.getColor());
						palette_list_update_first_selected(args->colorList, false, true);
					} else {
						colorSource->setNthColor(color_index, *selected);
					}
				}
				return true;
			}
			return false;
		case GDK_KEY_c:
			if (state == GDK_CONTROL_MASK) {
				clipboard::set(args->colorList, *args->gs, Converters::Type::copy);
				return true;
			} else if (state == GDK_SHIFT_MASK) {
				palette_popup_menu_clear_names(widget, args);
				return true;
			}
			return false;
		case GDK_KEY_v:
			if (state == GDK_CONTROL_MASK) {
				palette_popup_menu_paste(nullptr, args);
				return true;
			}
			return false;
		case GDK_KEY_r:
			if (state == GDK_SHIFT_MASK) {
				palette_popup_menu_reverse(widget, args);
				return true;
			}
			return false;
		case GDK_KEY_g:
			if (state == GDK_SHIFT_MASK) {
				palette_popup_menu_group_and_sort(widget, args);
				return true;
			}
			break;
		case GDK_KEY_Delete:
			if (state == GDK_CONTROL_MASK) {
				palette_popup_menu_remove_all(widget, args);
				return true;
			} else if (state == 0) {
				palette_popup_menu_remove_selected(widget, args);
				return true;
			}
			break;
		case GDK_KEY_n:
			if (state == GDK_SHIFT_MASK) {
				palette_popup_menu_auto_number(widget, args);
				return true;
			} else if (state == 0) {
				palette_popup_menu_add(widget, args);
				return true;
			}
			break;
		case GDK_KEY_e:
			if (state == 0) {
				palette_popup_menu_edit(widget, args);
				return true;
			}
			break;
		case GDK_KEY_a:
			if (state == GDK_SHIFT_MASK) {
				palette_popup_menu_auto_name(widget, args);
				return true;
			}
			break;
		default:
			return false;
	}
	return false;
}

int main_show_window(GtkWidget* window, const dynv::Ref &options) {
	if (gtk_widget_get_visible(window)){
		gtk_window_deiconify(GTK_WINDOW(window));
		return -1; //already visible
	}
	gint x, y;
	gint width, height;
	x = options->getInt32("window.x", -1);
	y = options->getInt32("window.y", -1);
	width = options->getInt32("window.width", 640);
	height = options->getInt32("window.height", 400);
	if (x < 0 || y < 0 || x > gdk_screen_width() || y > gdk_screen_height()){
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	}else{
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
		gtk_window_move(GTK_WINDOW(window), x, y);
	}
	if (width > 0 && height > 0)
		gtk_window_resize(GTK_WINDOW(window), width, height);
	gtk_widget_show(window);
	return 0;
}

static gboolean on_window_focus_change(GtkWidget *widget, GdkEventFocus *event, AppArgs* args)
{
	if (event->in){
		if (args->secondary_color_source) args->secondary_color_source->activate();
		if (args->current_color_source) args->current_color_source->activate();
	}else{
		if (args->secondary_color_source) args->secondary_color_source->deactivate();
		if (args->current_color_source) args->current_color_source->deactivate();
	}
	return FALSE;
}

static void set_main_window_icon()
{
	GList *icons = 0;
	GtkIconTheme *icon_theme;
	GError *error = nullptr;
	icon_theme = gtk_icon_theme_get_default();
	gint sizes[] = {16, 32, 48, 128};
	for (guint32 i = 0; i < sizeof(sizes) / sizeof(gint); ++i) {
		GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(icon_theme, "gpick", sizes[i], GtkIconLookupFlags(0), &error);
		if (pixbuf && (error == nullptr)){
			icons = g_list_append(icons, pixbuf);
		}
		if (error){
			cerr << error->message << endl;
			g_error_free(error);
			error = nullptr;
		}
	}
	if (icons){
		gtk_window_set_default_icon_list(icons);
		g_list_foreach(icons, (GFunc)g_object_unref, nullptr);
		g_list_free(icons);
	}
}

bool app_is_autoload_enabled(AppArgs *args)
{
	return args->options->getBool("main.save_restore_palette", true);
}

static void app_initialize_variables(AppArgs *args)
{
	args->current_filename_set = false;
	args->imported = false;
	args->precision_loss_icon = 0;
	args->current_color_source = nullptr;
	args->secondary_source_widget = 0;
	args->secondary_source_scrolled_viewpoint = 0;
	args->gs->loadAll();
	dialog_options_update(args->gs);
	args->options = args->gs->settings().getOrCreateMap("gpick.main");
	registerSources(args->csm);
}
static void app_initialize_floating_picker(AppArgs *args)
{
	args->floatingPicker = floating_picker_new(args->gs);
}

static void app_initialize_picker(AppArgs *args, GtkWidget *notebook) {
	const auto &registration = args->csm["color_picker"];
	auto options = args->gs->settings().getOrCreateMap("gpick.picker");
	auto colorPicker = registration.make<IColorPicker>(*args->gs, options);
	GtkWidget *widget = colorPicker->getWidget();
	floating_picker_set_picker_source(args->floatingPicker, colorPicker.get());
	colorPicker->setFloatingPicker(args->floatingPicker);
	args->colorSourceIndex.push_back(colorPicker.get());
	args->colorSourceMap.emplace(registration.name, std::move(colorPicker));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new_with_mnemonic(_("Color pic_ker")));
	gtk_widget_show(widget);
}
void app_initialize()
{
	GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
	auto dataPath = buildFilename();
	gtk_icon_theme_append_search_path(icon_theme, dataPath.c_str());
}
AppArgs* app_create_main(const StartupOptions &startupOptions, int &returnValue) {
	AppArgs* args = new AppArgs;
	args->initialization = true;
	args->startupOptions = startupOptions;
	Color::initialize();
	args->gs = new GlobalState();
	args->gs->loadSettings();
	if (args->startupOptions.single_color_pick_mode){
		app_initialize_variables(args);
		app_initialize_floating_picker(args);
		args->initialization = false;
		return args; // No main UI initialization needed
	}else{
		args->dbus_control.onActivateFloatingPicker = [args](const char *converter_name){
			if (converter_name != nullptr && string(converter_name).length() > 0){
				floating_picker_activate(args->floatingPicker, false, false, converter_name);
			}else{
				floating_picker_activate(args->floatingPicker, false, false, nullptr);
			}
			return true;
		};
		args->dbus_control.onSingleInstanceActivate = [args]{
			status_icon_set_visible(args->status_icon, false);
			main_show_window(args->window, args->options);
			return true;
		};
		args->dbus_control.ownName();
		bool cancel_startup = false;
		if (!cancel_startup && startupOptions.floating_picker_mode){
			if (args->dbus_control.activateFloatingPicker(startupOptions.converter_name)){
				cancel_startup = true;
			}else if (startupOptions.do_not_start){
				returnValue = 1;
				cancel_startup = true;
			}
		}
		if (startupOptions.do_not_start){
			if (args->dbus_control.checkIfRunning()){
				returnValue = 1;
				cancel_startup = true;
			}
		}
		if (!cancel_startup && args->gs->settings().getBool("gpick.main.single_instance", false)){
			if (args->dbus_control.singleInstanceActivate()){
				cancel_startup = true;
			}
		}
		if (cancel_startup){
			delete args;
			return 0;
		}
		app_initialize_variables(args);
	}
	g_signal_connect(G_OBJECT(gdk_keymap_get_for_display(gdk_display_get_default())), "keys-changed", G_CALLBACK(detectLatinKeyGroup), args);
	detectLatinKeyGroup(nullptr, args);
#if GTK_MAJOR_VERSION >= 3
	auto cssProvider = gtk_css_provider_new();
	GError *error = nullptr;
	auto style = "button.small-statusbar-button{padding:1px;margin:0;min-height:0;min-width:0;}"s;
	gtk_css_provider_load_from_data(cssProvider, style.c_str(), style.length(), &error);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);
#endif
	args->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	set_main_window_icon();
	app_update_program_name(args);
	g_signal_connect(G_OBJECT(args->window), "delete_event", G_CALLBACK(delete_event), args);
	g_signal_connect(G_OBJECT(args->window), "destroy", G_CALLBACK(destroy_cb), args);
	g_signal_connect(G_OBJECT(args->window), "configure-event", G_CALLBACK(on_window_configure), args);
	g_signal_connect(G_OBJECT(args->window), "window-state-event", G_CALLBACK(on_window_state_event), args);
	g_signal_connect(G_OBJECT(args->window), "focus-in-event", G_CALLBACK(on_window_focus_change), args);
	g_signal_connect(G_OBJECT(args->window), "focus-out-event", G_CALLBACK(on_window_focus_change), args);
	g_signal_connect(G_OBJECT(args->window), "key-press-event", G_CALLBACK(on_window_key_press), args);
	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(args->window), accel_group);
	g_object_unref(G_OBJECT (accel_group));
	GtkWidget *widget, *statusbar, *notebook, *vpaned, *hpaned;
	GtkWidget* vbox_main = gtk_vbox_new(false, 0);
	gtk_container_add (GTK_CONTAINER(args->window), vbox_main);
	GtkWidget* menu_bar;
	menu_bar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX(vbox_main), menu_bar, FALSE, FALSE, 0);
	hpaned = gtk_hpaned_new();
	vpaned = gtk_vpaned_new();
	args->vpaned = vpaned;
	notebook = gtk_notebook_new();
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(notebook_switch_cb), args);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), true);
	statusbar = gtk_statusbar_new();
	args->gs->setStatusBar(statusbar);
	gtk_widget_show_all(notebook);
	gtk_widget_show_all(vpaned);
	gtk_box_pack_start(GTK_BOX(vbox_main), hpaned, true, true, 5);
	gtk_widget_show_all(vbox_main);
	app_initialize_floating_picker(args);
	app_initialize_picker(args, notebook);
	{
		auto generateSchemeOptions = args->gs->settings().getOrCreateMap("gpick.generate_scheme");
		const auto &registration = args->csm["generate_scheme"];
		auto source = registration.build(*args->gs, generateSchemeOptions);
		widget = source->getWidget();
		args->colorSourceIndex.push_back(source.get());
		args->colorSourceMap.emplace(source->name(), std::move(source));
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new_with_mnemonic(_("Scheme _generation")));
		gtk_widget_show(widget);
	}
	{
		auto name = args->options->getString("secondary_color_source", "");
		widget = gtk_vbox_new(false, 0);
		args->secondary_source_container = widget;
		if (args->csm.has(name)) {
			const auto &registration = args->csm[name];
			activate_secondary_source(args, &registration);
		}
	}
	{
		auto layoutPreviewOptions = args->gs->settings().getOrCreateMap("gpick.layout_preview");
		const auto &registration = args->csm["layout_preview"];
		auto source = registration.build(*args->gs, layoutPreviewOptions);
		widget = source->getWidget();
		args->colorSourceIndex.push_back(source.get());
		args->colorSourceMap.emplace(source->name(), std::move(source));
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new_with_mnemonic(_("Lay_out preview")));
		gtk_widget_show(widget);
	}
	GtkWidget *count_label = gtk_label_new("");
	widget = palette_list_new(*args->gs, count_label);
	args->colorList = widget;
	auto columnWidths = args->options->getInt32s("column_widths");
	for (size_t i = 0; ; ++i) {
		GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(widget), i);
		if (!column)
			break;
		if (i < columnWidths.size())
			gtk_tree_view_column_set_fixed_width(column, columnWidths[i]);
	}
	gtk_widget_show(widget);
	g_signal_connect(G_OBJECT(widget), "popup-menu", G_CALLBACK(on_palette_popup_menu), args);
	g_signal_connect(G_OBJECT(widget), "button-press-event",G_CALLBACK(on_palette_button_press), args);
	g_signal_connect(G_OBJECT(widget), "key_press_event", G_CALLBACK(on_palette_list_key_press), args);
	GtkWidget *scrolled_window;
	scrolled_window = gtk_scrolled_window_new (0,0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(scrolled_window), args->colorList );
	gtk_widget_show(scrolled_window);
	args->palette = scrolled_window;
	args->hpaned = hpaned;
	args->notebook = notebook;
	repositionViews(args);
	{
		auto tab = args->options->getString("color_source", "");
		auto i = args->colorSourceMap.find(tab);
		if (i != args->colorSourceMap.end()) {
			size_t tabIndex = 0;
			for (auto colorSource: args->colorSourceIndex) {
				if (colorSource == (*i).second.get()) {
					gtk_notebook_set_current_page(GTK_NOTEBOOK(args->notebook), tabIndex);
					if (IColorPicker::isColorPicker(*colorSource))
						dynamic_cast<IColorPicker *>(colorSource)->focusSwatch();
					break;
				}
				tabIndex++;
			}
		}
	}
	gtk_box_pack_end (GTK_BOX(vbox_main), statusbar, 0, 0, 0);
	args->statusbar = statusbar;
	GtkWidget *button = gtk_button_new();
#if GTK_MAJOR_VERSION >= 3
	auto context = gtk_widget_get_style_context(button);
	gtk_style_context_add_class(context, "small-statusbar-button");
#endif
	gtk_button_set_focus_on_click(GTK_BUTTON(button), false);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(floating_picker_show_cb), args);
	gtk_widget_add_accelerator(button, "clicked", accel_group, GDK_KEY_p, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_set_tooltip_text(button, _("Pick colors (Ctrl+P)"));
	gtk_container_add(GTK_CONTAINER(button), newIcon("gpick", 16));
	gtk_box_pack_end(GTK_BOX(statusbar), button, false, false, 0);
	gtk_widget_show_all(button);
	gtk_box_pack_end(GTK_BOX(statusbar), count_label, false, false, 0);
	gtk_widget_show_all(count_label);
	widget = newIcon(GTK_STOCK_DIALOG_WARNING, 16);
	gtk_widget_set_tooltip_text(widget, _("File is currently in a non-native format, possible loss of precision and/or metadata."));
	args->precision_loss_icon = widget;
	if (args->imported)
		gtk_widget_show(widget);
	gtk_box_pack_end(GTK_BOX(statusbar), widget, false, false, 0);
	gtk_widget_show(statusbar);
	{
		//Load recent file list
		auto recentFiles = args->gs->settings().getStrings("gpick.recent.files");
		args->recent_files = std::list<std::string>(recentFiles.begin(), recentFiles.end());
	}
	create_menu(GTK_MENU_BAR(menu_bar), args, accel_group);
	gtk_widget_show_all(menu_bar);
	args->status_icon = status_icon_new(args->window, args->gs, args->floatingPicker);
	args->initialization = false;
	return args;
}

static void app_release(AppArgs *args) {
	if (args->current_color_source) {
		args->current_color_source->deactivate();
		args->current_color_source = nullptr;
	}
	if (args->secondary_color_source) {
		args->secondary_color_source->deactivate();
		args->secondary_color_source.reset();
		args->secondary_source_widget = nullptr;
	}
	args->colorSourceMap.clear();
	args->colorSourceIndex.clear();
	floating_picker_free(args->floatingPicker);
	if (!args->startupOptions.single_color_pick_mode){
		if (app_is_autoload_enabled(args)){
			autoSave(args->gs->colorList());
		}
		args->gs->colorList().removeAll();
	}
}

static void app_save_recent_file_list(AppArgs *args) {
	std::vector<std::string> recentFiles(args->recent_files.begin(), args->recent_files.end());
	args->gs->settings().set("gpick.recent.files", recentFiles);
}

struct FloatingPickerAction
{
	public:
		AppArgs *args;
		bool clipboard_touched;
		FloatingPickerAction(AppArgs *args)
		{
			this->args = args;
			clipboard_touched = false;
		}
		void colorPicked(FloatingPicker fp, const Color &color)
		{
			string text;
			if (args->startupOptions.converter_name.length() > 0){
				auto converter = args->gs->converters().byName(args->startupOptions.converter_name);
				if (converter != nullptr){
					text = converter->serialize(color);
				}
			}else{
				auto converter = args->gs->converters().firstCopy();
				if (converter != nullptr){
					text = converter->serialize(color);
				}
			}
			if (text.length() > 0) {
				if (args->startupOptions.output_picked_color) {
					if (args->startupOptions.output_without_newline) {
						cout << text;
					}else{
						cout << text << endl;
					}
				}else{
					clipboard::set(text);
					clipboard_touched = true;
				}
			}
		}
		static gboolean close_application(gpointer user_data)
		{
			gtk_main_quit();
			return FALSE;
		}
		void done(FloatingPicker fp)
		{
			if (clipboard_touched)
				g_timeout_add(100, close_application, args); // quiting main gtk loop early will not give enough time for clipboard manager to retrieve the clipboard contents
			else
				gtk_main_quit();
		}
};

int app_run(AppArgs *args)
{
	if (args->startupOptions.single_color_pick_mode) {
		FloatingPickerAction pick_action(args);
		floating_picker_set_custom_pick_action(args->floatingPicker, [&pick_action](FloatingPicker fp, const Color &color){
			pick_action.colorPicked(fp, color);
		});
		floating_picker_set_custom_done_action(args->floatingPicker, [&pick_action](FloatingPicker fp){
			pick_action.done(fp);
		});
		floating_picker_enable_custom_pick_action(args->floatingPicker);
		floating_picker_activate(args->floatingPicker, false, true, args->startupOptions.converter_name.c_str());
		gtk_main();
		app_release(args);
	}else{
		gtk_widget_realize(args->window);
		if (args->startupOptions.floating_picker_mode || args->options->getBool("start_in_tray", false)){
			status_icon_set_visible(args->status_icon, true);
		}else{
			gtk_paned_set_position(GTK_PANED(args->hpaned), args->options->getInt32("paned_position", 0));
			gtk_paned_set_position(GTK_PANED(args->vpaned), args->options->getInt32("vertical_paned_position", 0));
			main_show_window(args->window, args->options);
		}
		if (args->startupOptions.floating_picker_mode)
			floating_picker_activate(args->floatingPicker, false, false, args->startupOptions.converter_name.c_str());
		gtk_main();
		app_save_recent_file_list(args);
		args->dbus_control.unownName();
		status_icon_destroy(args->status_icon);
	}
	args->gs->writeSettings();
	delete args->gs;
	delete args;
	return 0;
}
