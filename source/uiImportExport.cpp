/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#include "uiImportExport.h"
#include "uiUtilities.h"
#include "uiListPalette.h"
#include "dynv/Map.h"
#include "ImportExport.h"
#include "StringUtils.h"
#include "ColorList.h"
#include "Converters.h"
#include "Converter.h"
#include "GlobalState.h"
#include "I18N.h"
#include "parser/TextFile.h"
#include "common/Guard.h"
#include <functional>
namespace {
struct ImportExportFormat {
	const char *name;
	const char *pattern;
	FileType type;
};
struct ListOption {
	const char *name;
	const char *label;
};
struct ImportExportDialogOptions {
	enum class Options {
		importFile,
		importTextFile,
	};
	Options m_options;
	GtkWidget *m_dialog;
	GtkWidget *m_converters, *m_itemSizes, *m_backgrounds, *m_includeColorNames;
	GtkWidget *m_singleLineCComments, *m_multiLineCComments, *m_singleLineHashComments, *m_cssRgb, *m_cssRgba, *m_shortHex, *m_fullHex, *m_floatValues, *m_intValues, *m_fullHexWithAlpha, *m_shortHexWithAlpha, *m_cssHsl, *m_cssHsla, *m_cssOklch;
	GlobalState &m_gs;
	ImportExportDialogOptions(GtkWidget *dialog, GlobalState &gs):
		m_dialog(dialog),
		m_gs(gs) {
	}
	void createImportOptions() {
		m_options = Options::importFile;
		m_converters = newConverterList();
		g_signal_connect(G_OBJECT(m_dialog), "notify::filter", G_CALLBACK(filterChanged), this);
		GtkTreeIter iter;
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(m_converters));
		bool converterFound = false;
		std::string converterName = m_gs.settings().getString("gpick.import.converter", "");
		for (auto &converter: m_gs.converters().all()) {
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, converter->label().c_str(), 1, converter, -1);
			if (converter->name() == converterName) {
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(m_converters), &iter);
				converterFound = true;
			}
		}
		if (!converterFound) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(m_converters), 0);
		}
		const ListOption itemSizeOptions[] = {
			{ "small", _("Small") },
			{ "medium", _("Medium") },
			{ "big", _("Big") },
			{ "controllable", _("User controllable") },
		};
		m_itemSizes = newOptionList(itemSizeOptions, sizeof(itemSizeOptions) / sizeof(ListOption), m_gs.settings().getString("gpick.import.item_size", "medium"));

		const ListOption backgroundOptions[] = {
			{ "none", _("None") },
			{ "white", _("White") },
			{ "gray", _("Gray") },
			{ "black", _("Black") },
			{ "first_color", _("First color") },
			{ "last_color", _("Last color") },
			{ "controllable", _("User controllable") },
		};
		m_backgrounds = newOptionList(backgroundOptions, sizeof(backgroundOptions) / sizeof(ListOption), m_gs.settings().getString("gpick.import.background", "none"));

		m_includeColorNames = newCheckbox(_("Include color names"), m_gs.settings().getBool("gpick.import.include_color_names", true));

		afterFilterChanged();
		int y = 0;
		GtkWidget *table = gtk_table_new(2, 3, false);
		addOption(_("Converter:"), m_converters, 0, y, table);
		addOption(_("Item size:"), m_itemSizes, 0, y, table);
		addOption(_("Background:"), m_backgrounds, 0, y, table);
		addOption(m_includeColorNames, 0, y, table);
		gtk_widget_show_all(table);
		gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(m_dialog), table);
	}
	void createImportTextFileOptions() {
		m_options = Options::importTextFile;
		int y = 0;
		GtkWidget *table = gtk_table_new(4, 12, false);
		auto settings = m_gs.settings().getOrCreateMap("gpick.import_text_file");
		addOption(m_singleLineCComments = newCheckbox(std::string(_("C style single-line comments")) + " (//abc)", settings->getBool("single_line_c_comments", true)), 0, y, table);
		addOption(m_multiLineCComments = newCheckbox(std::string(_("C style multi-line comments")) + " (/*abc*/)", settings->getBool("multi_line_c_comments", true)), 0, y, table);
		addOption(m_singleLineHashComments = newCheckbox(std::string(_("Hash single-line comments")) + " (#abc)", settings->getBool("single_line_hash_comments", true)), 0, y, table);
		y = 0;
		addOption(m_cssRgb = newCheckbox("CSS rgb()", settings->getBool("css_rgb", true)), 1, y, table);
		addOption(m_cssRgba = newCheckbox("CSS rgba()", settings->getBool("css_rgba", true)), 1, y, table);
		addOption(m_cssHsl = newCheckbox("CSS hsl()", settings->getBool("css_hsl", true)), 1, y, table);
		addOption(m_cssHsla = newCheckbox("CSS hsla()", settings->getBool("css_hsla", true)), 1, y, table);
		addOption(m_cssOklch = newCheckbox("CSS oklch()", settings->getBool("css_oklch", true)), 1, y, table);
		y = 0;
		addOption(m_fullHex = newCheckbox(_("Full hex"), settings->getBool("full_hex", true)), 2, y, table);
		addOption(m_fullHexWithAlpha = newCheckbox(_("Full hex with alpha"), settings->getBool("full_hex_with_alpha", true)), 2, y, table);
		addOption(m_shortHex = newCheckbox(_("Short hex"), settings->getBool("short_hex", true)), 2, y, table);
		addOption(m_shortHexWithAlpha = newCheckbox(_("Short hex with alpha"), settings->getBool("short_hex_with_alpha", true)), 2, y, table);
		y = 0;
		addOption(m_intValues = newCheckbox(_("Integer values"), settings->getBool("int_values", true)), 3, y, table);
		addOption(m_floatValues = newCheckbox(_("Real values"), settings->getBool("float_values", true)), 3, y, table);
		gtk_widget_show_all(table);
		gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(m_dialog), table);
	}
	GtkWidget *addOption(const char *label, GtkWidget *widget, int x, int &y, GtkWidget *table) {
		gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(label, 0, 0.5, 0, 0), x * 3, x * 3 + 1, y, y + 1, GTK_FILL, GTK_FILL, 3, 1);
		gtk_table_attach(GTK_TABLE(table), widget, 1, 2, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 3, 1);
		y++;
		return widget;
	}
	GtkWidget *addOption(GtkWidget *widget, int x, int &y, GtkWidget *table) {
		gtk_table_attach(GTK_TABLE(table), widget, x * 3 + 1, x * 3 + 2, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 3, 1);
		y++;
		return widget;
	}
	Converter *getSelectedConverter() {
		GtkTreeIter iter;
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(m_converters), &iter)) {
			GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(m_converters));
			Converter *converter;
			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &converter, -1);
			return converter;
		}
		return nullptr;
	}
	std::string getOptionValue(GtkWidget *widget) {
		GtkTreeIter iter;
		if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
			GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
			gchar *value;
			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &value, -1);
			std::string result(value);
			g_free(value);
			return result;
		}
		return std::string();
	}
	std::string getSelectedItemSize() {
		return getOptionValue(m_itemSizes);
	}
	std::string getSelectedBackground() {
		return getOptionValue(m_backgrounds);
	}
	bool isIncludeColorNamesEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_includeColorNames));
	}
	bool isSingleLineCCommentsEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_singleLineCComments));
	}
	bool isMultiLineCCommentsEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_multiLineCComments));
	}
	bool isSingleLineHashCommentsEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_singleLineHashComments));
	}
	bool isCssRgbEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_cssRgb));
	}
	bool isCssRgbaEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_cssRgba));
	}
	bool isCssHslEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_cssHsl));
	}
	bool isCssHslaEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_cssHsla));
	}
	bool isCssOklchEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_cssOklch));
	}
	bool isFullHexEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_fullHex));
	}
	bool isShortHexEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_shortHex));
	}
	bool isFullHexWithAlphaEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_fullHexWithAlpha));
	}
	bool isShortHexWithAlphaEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_shortHexWithAlpha));
	}
	bool isIntValuesEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_intValues));
	}
	bool isFloatValuesEnabled() {
		return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_floatValues));
	}
	void saveState() {
		auto &settings = m_gs.settings();
		if (m_options == Options::importFile) {
			Converter *converter = getSelectedConverter();
			if (converter)
				settings.set("gpick.import.converter", converter->name().c_str());
			std::string itemSize = getSelectedItemSize();
			settings.set("gpick.import.item_size", itemSize.c_str());
			std::string background = getSelectedBackground();
			settings.set("gpick.import.background", background.c_str());
			settings.set("gpick.import.include_color_names", isIncludeColorNamesEnabled());
		} else if (m_options == Options::importTextFile) {
			auto importTextFile = settings.getOrCreateMap("gpick.import_text_file");
			importTextFile->set("single_line_c_comments", isSingleLineCCommentsEnabled());
			importTextFile->set("multi_line_c_comments", isMultiLineCCommentsEnabled());
			importTextFile->set("single_line_hash_comments", isSingleLineHashCommentsEnabled());
			importTextFile->set("css_rgb", isCssRgbEnabled());
			importTextFile->set("css_rgba", isCssRgbaEnabled());
			importTextFile->set("css_hsl", isCssHslEnabled());
			importTextFile->set("css_hsla", isCssHslaEnabled());
			importTextFile->set("css_oklch", isCssOklchEnabled());
			importTextFile->set("full_hex", isFullHexEnabled());
			importTextFile->set("short_hex", isShortHexEnabled());
			importTextFile->set("full_hex_with_alpha", isFullHexWithAlphaEnabled());
			importTextFile->set("short_hex_with_alpha", isShortHexWithAlphaEnabled());
			importTextFile->set("int_values", isIntValuesEnabled());
			importTextFile->set("float_values", isFloatValuesEnabled());
		}
	}
	GtkWidget *newConverterList() {
		GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
		GtkWidget *list = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(list), renderer, true);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(list), renderer, "text", 0, nullptr);
		if (store) g_object_unref(store);
		return list;
	}
	GtkWidget *newOptionList(const ListOption *options, size_t n_options, const std::string &selected_value) {
		GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		GtkWidget *list = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(list), renderer, true);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(list), renderer, "text", 0, nullptr);
		bool found = false;
		GtkTreeIter iter;
		for (size_t i = 0; i != n_options; i++) {
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, options[i].label, 1, options[i].name, -1);
			if (selected_value == options[i].name) {
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(list), &iter);
				found = true;
			}
		}
		if (!found) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(list), 0);
		}
		g_object_unref(store);
		return list;
	}
	GtkWidget *newCheckbox(const char *label, bool value) {
		GtkWidget *widget = gtk_check_button_new_with_label(label);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
		return widget;
	}
	GtkWidget *newCheckbox(const std::string &label, bool value) {
		GtkWidget *widget = gtk_check_button_new_with_label(label.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
		return widget;
	}
	void afterFilterChanged() {
		GtkFileFilter *filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(m_dialog));
		ImportExportFormat *format = (ImportExportFormat *)g_object_get_data(G_OBJECT(filter), "format");
		gtk_widget_set_sensitive(m_converters, format->type == FileType::txt || format->type == FileType::html);
		gtk_widget_set_sensitive(m_itemSizes, format->type == FileType::html);
		gtk_widget_set_sensitive(m_backgrounds, format->type == FileType::html);
		gtk_widget_set_sensitive(m_includeColorNames, format->type == FileType::html || format->type == FileType::txt);
	}
	static void filterChanged(GtkFileChooserDialog *, GParamSpec *, ImportExportDialogOptions *import_export_dialog) {
		import_export_dialog->afterFilterChanged();
	}
};
}
ImportExportDialog::ImportExportDialog(GtkWindow *parent, ColorList &colorList, GlobalState &gs):
	m_parent(parent),
	m_colorList(colorList),
	m_gs(gs) {
}
ImportExportDialog::~ImportExportDialog() {
}
bool ImportExportDialog::showImport() {
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import"), m_parent,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	const ImportExportFormat formats[] = {
		{ _("Gpick Palette (*.gpa)"), "*.gpa", FileType::gpa },
		{ _("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", FileType::gpl },
		{ _("Adobe Swatch Exchange (*.ase)"), "*.ase", FileType::ase },
		{ _("Text File (*.txt)"), "*.txt", FileType::txt },
		{ _("rgb.txt File (rgb.txt)"), "rgb.txt", FileType::rgbtxt },
	};
	const size_t n_formats = sizeof(formats) / sizeof(ImportExportFormat);
	auto defaultPath = m_gs.settings().getString("gpick.import.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), defaultPath.c_str());
	auto selectedFilter = m_gs.settings().getString("gpick.import.filter", "all_supported");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer) "all");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (selectedFilter == "all") {
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All supported formats"));
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer) "all_supported");
	for (size_t i = 0; i != n_formats; ++i) {
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
	}
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (selectedFilter == "all_supported") {
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	for (size_t i = 0; i != n_formats; ++i) {
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		g_object_set_data(G_OBJECT(filter), "identification", (gpointer)formats[i].pattern);
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if (formats[i].pattern == selectedFilter) {
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
	bool finished = false;
	while (!finished) {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			m_gs.settings().set("gpick.import.path", path);
			g_free(path);
			FileType type = ImportExport::getFileType(filename);
			if (type == FileType::unknown) {
				const gchar *formatName = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
				for (size_t i = 0; i != n_formats; ++i) {
					if (g_strcmp0(formats[i].name, formatName) == 0) {
						type = formats[i].type;
						break;
					}
				}
			}
			GtkWidget *message;
			if (type == FileType::unknown) {
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File format is not supported"));
				gtk_window_set_title(GTK_WINDOW(message), _("Import"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			} else {
				for (size_t i = 0; i != n_formats; ++i) {
					if (formats[i].type == type) {
						ColorList colorList;
						ImportExport importExport(colorList, filename, m_gs);
						importExport.setConverters(&m_gs.converters());
						if (importExport.importType(formats[i].type)) {
							finished = true;
							m_colorList.add(colorList);
						} else {
							message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be imported"));
							gtk_window_set_title(GTK_WINDOW(message), _("Import"));
							gtk_dialog_run(GTK_DIALOG(message));
							gtk_widget_destroy(message);
						}
						const char *identification = (const char *)g_object_get_data(G_OBJECT(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog))), "identification");
						m_gs.settings().set("gpick.import.filter", identification);
						break;
					}
				}
			}
			g_free(filename);
		} else
			break;
	}
	gtk_widget_destroy(dialog);
	return finished;
}
bool ImportExportDialog::showImportTextFile() {
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import text file"), m_parent,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	auto defaultPath = m_gs.settings().getString("gpick.import_text_file.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), defaultPath.c_str());
	ImportExportDialogOptions importExportDialogOptions(dialog, m_gs);
	importExportDialogOptions.createImportTextFileOptions();
	bool finished = false;
	while (!finished) {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			m_gs.settings().set("gpick.import_text_file.path", path);
			g_free(path);
			importExportDialogOptions.saveState();
			GtkWidget *message;
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			ColorList colorList;
			ImportExport importExport(colorList, filename, m_gs);
			importExport.setConverters(&m_gs.converters());
			text_file_parser::Configuration configuration;
			configuration.singleLineCComments = importExportDialogOptions.isSingleLineCCommentsEnabled();
			configuration.multiLineCComments = importExportDialogOptions.isMultiLineCCommentsEnabled();
			configuration.singleLineHashComments = importExportDialogOptions.isSingleLineHashCommentsEnabled();
			configuration.cssRgb = importExportDialogOptions.isCssRgbEnabled();
			configuration.cssRgba = importExportDialogOptions.isCssRgbaEnabled();
			configuration.cssHsl = importExportDialogOptions.isCssHslEnabled();
			configuration.cssHsla = importExportDialogOptions.isCssHslaEnabled();
			configuration.cssOklch = importExportDialogOptions.isCssOklchEnabled();
			configuration.fullHex = importExportDialogOptions.isFullHexEnabled();
			configuration.shortHex = importExportDialogOptions.isShortHexEnabled();
			configuration.fullHexWithAlpha = importExportDialogOptions.isFullHexWithAlphaEnabled();
			configuration.shortHexWithAlpha = importExportDialogOptions.isShortHexWithAlphaEnabled();
			configuration.intValues = importExportDialogOptions.isIntValuesEnabled();
			configuration.floatValues = importExportDialogOptions.isFloatValuesEnabled();
			if (importExport.importTextFile(configuration)) {
				finished = true;
				m_colorList.add(colorList);
			} else {
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be imported"));
				gtk_window_set_title(GTK_WINDOW(message), _("Import text file"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
			g_free(filename);
		} else
			break;
	}
	gtk_widget_destroy(dialog);
	return finished;
}
bool ImportExportDialog::showExport() {
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Export"), m_parent,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	const ImportExportFormat formats[] = {
		{ _("Gpick Palette (*.gpa)"), "*.gpa", FileType::gpa },
		{ _("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", FileType::gpl },
		{ _("Alias/WaveFront Material (*.mtl)"), "*.mtl", FileType::mtl },
		{ _("Adobe Swatch Exchange (*.ase)"), "*.ase", FileType::ase },
		{ _("Cascading Style Sheet (*.css)"), "*.css", FileType::css },
		{ _("Hyper Text Markup Language (*.html, *.htm)"), "*.html,*.htm", FileType::html },
		{ _("Text file (*.txt)"), "*.txt", FileType::txt },
	};
	const size_t n_formats = sizeof(formats) / sizeof(ImportExportFormat);
	auto defaultPath = m_gs.settings().getString("gpick.export.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), defaultPath.c_str());
	std::string selectedFilter = m_gs.settings().getString("gpick.export.filter", "*.gpl");
	for (size_t i = 0; i != n_formats; ++i) {
		GtkFileFilter *filter = gtk_file_filter_new();
		g_object_set_data(G_OBJECT(filter), "format", (gpointer)&formats[i]);
		gtk_file_filter_set_name(filter, formats[i].name);
		split(formats[i].pattern, ',', true, [filter](const std::string &value) {
			gtk_file_filter_add_pattern(filter, value.c_str());
		});
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if (formats[i].pattern == selectedFilter) {
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
	ImportExportDialogOptions importExportDialogOptions(dialog, m_gs);
	importExportDialogOptions.createImportOptions();
	bool finished = false;
	while (!finished) {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			m_gs.settings().set("gpick.export.path", path);
			g_free(path);
			importExportDialogOptions.saveState();
			std::string formatName = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
			for (size_t i = 0; i != n_formats; ++i) {
				if (formats[i].name == formatName) {
					ImportExport importExport(m_colorList, filename, m_gs);
					importExport.fixFileExtension(formats[i].pattern);
					importExport.setConverter(importExportDialogOptions.getSelectedConverter());
					std::string itemSize = importExportDialogOptions.getSelectedItemSize();
					importExport.setItemSize(itemSize.c_str());
					std::string background = importExportDialogOptions.getSelectedBackground();
					importExport.setBackground(background.c_str());
					importExport.setIncludeColorNames(importExportDialogOptions.isIncludeColorNamesEnabled());
					if (importExport.exportType(formats[i].type)) {
						finished = true;
					} else {
						GtkWidget *message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be exported"));
						gtk_window_set_title(GTK_WINDOW(dialog), _("Export"));
						gtk_dialog_run(GTK_DIALOG(message));
						gtk_widget_destroy(message);
					}
					m_gs.settings().set("gpick.export.filter", formats[i].pattern);
				}
			}
			g_free(filename);
		} else
			break;
	}
	gtk_widget_destroy(dialog);
	return finished;
}
