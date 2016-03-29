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

#include "uiImportExport.h"
#include "uiUtilities.h"
#include "uiListPalette.h"
#include "DynvHelpers.h"
#include "ImportExport.h"
#include "StringUtils.h"
#include "Converter.h"
#include "GlobalStateStruct.h"
#include "Internationalisation.h"
#include <functional>
#include <iostream>
using namespace std;

struct ImportExportFormat
{
	const char* name;
	const char* pattern;
	FileType type;
};
struct ListOption
{
	const char* name;
	const char* label;
};

class ImportExportDialogOptions
{
	public:
		GtkWidget *m_dialog;
		GtkWidget *m_converters, *m_item_sizes, *m_backgrounds, *m_include_color_names;
		GlobalState *m_gs;
		ImportExportDialogOptions(GtkWidget *dialog, GlobalState *gs)
		{
			m_dialog = dialog;
			m_gs = gs;
			m_converters = newConverterList();
			g_signal_connect(G_OBJECT(dialog), "notify::filter", G_CALLBACK(filterChanged), this);
			Converters *converters = static_cast<Converters*>(dynv_get_pointer_wdc(gs->params, "Converters", 0));
			Converter **converter_table;
			uint32_t total_converters;
			converter_table = converters_get_all(converters, &total_converters);
			GtkTreeIter iter;
			GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(m_converters));
			bool converter_found = false;
			string converter_name = dynv_get_string_wd(gs->params, "gpick.import.converter", "");
			for (size_t i = 0; i < total_converters; i++){
				gtk_list_store_append(GTK_LIST_STORE(model), &iter);
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, converter_table[i]->human_readable, 1, converter_table[i], -1);
				if (converter_name == converter_table[i]->function_name){
					gtk_combo_box_set_active_iter(GTK_COMBO_BOX(m_converters), &iter);
					converter_found = true;
				}
			}
			if (!converter_found){
				gtk_combo_box_set_active(GTK_COMBO_BOX(m_converters), 0);
			}
			delete [] converter_table;

			const ListOption item_size_options[] = {
				{"small", _("Small")},
				{"medium", _("Medium")},
				{"big", _("Big")},
				{"controllable", _("User controllable")},
			};
			m_item_sizes = newOptionList(item_size_options, sizeof(item_size_options) / sizeof(ListOption), dynv_get_string_wd(gs->params, "gpick.import.item_size", "medium"));

			const ListOption background_options[] = {
				{"none", _("None")},
				{"white", _("White")},
				{"gray", _("Gray")},
				{"black", _("Black")},
				{"first_color", _("First color")},
				{"last_color", _("Last color")},
				{"controllable", _("User controllable")},
			};
			m_backgrounds = newOptionList(background_options, sizeof(background_options) / sizeof(ListOption), dynv_get_string_wd(gs->params, "gpick.import.background", "none"));

			m_include_color_names = newCheckbox(_("Include color names"), dynv_get_bool_wd(m_gs->params, "gpick.import.include_color_names", true));

			afterFilterChanged();
			int y = 0;
			GtkWidget *table = gtk_table_new(2, 3, false);
			addOption(_("Converter:"), m_converters, y, table);
			addOption(_("Item size:"), m_item_sizes, y, table);
			addOption(_("Background:"), m_backgrounds, y, table);
			addOption(m_include_color_names, y, table);
			gtk_widget_show_all(table);
			gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), table);
		}
		GtkWidget *addOption(const char *label, GtkWidget *widget, int &y, GtkWidget *table)
		{
			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(label, 0, 0.5, 0, 0), 0, 1, y, y + 1, GTK_FILL, GTK_FILL, 3, 1);
			gtk_table_attach(GTK_TABLE(table), widget, 1, 2, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 3, 1);
			y++;
			return widget;
		}
		GtkWidget *addOption(GtkWidget *widget, int &y, GtkWidget *table)
		{
			gtk_table_attach(GTK_TABLE(table), widget, 1, 2, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 3, 1);
			y++;
			return widget;
		}
		Converter *getSelectedConverter()
		{
			GtkTreeIter iter;
			if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(m_converters), &iter)){
				GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(m_converters));
				Converter *converter;
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &converter, -1);
				return converter;
			}
			return nullptr;
		}
		string getOptionValue(GtkWidget *widget)
		{
			GtkTreeIter iter;
			if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
				GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
				gchar *value;
				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 1, &value, -1);
				string result(value);
				g_free(value);
				return result;
			}
			return string();
		}
		string getSelectedItemSize()
		{
			return getOptionValue(m_item_sizes);
		}
		string getSelectedBackground()
		{
			return getOptionValue(m_backgrounds);
		}
		bool isIncludeColorNamesEnabled()
		{
			return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_include_color_names));
		}
		void saveState()
		{
			Converter *converter = getSelectedConverter();
			if (converter)
				dynv_set_string(m_gs->params, "gpick.import.converter", converter->function_name);
			string item_size = getSelectedItemSize();
			dynv_set_string(m_gs->params, "gpick.import.item_size", item_size.c_str());
			string background = getSelectedBackground();
			dynv_set_string(m_gs->params, "gpick.import.background", background.c_str());
			dynv_set_bool(m_gs->params, "gpick.import.include_color_names", isIncludeColorNamesEnabled());
		}
		GtkWidget* newConverterList()
		{
			GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
			GtkWidget *list = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(list), renderer, true);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(list), renderer, "text", 0, nullptr);
			if (store) g_object_unref(store);
			return list;
		}
		GtkWidget* newOptionList(const ListOption *options, size_t n_options, const string &selected_value)
		{
			GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
			GtkWidget *list = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(list), renderer, true);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(list), renderer, "text", 0, nullptr);
			bool found = false;
			GtkTreeIter iter;
			for (size_t i = 0; i != n_options; i++){
				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, 0, options[i].label, 1, options[i].name, -1);
				if (selected_value == options[i].name){
					gtk_combo_box_set_active_iter(GTK_COMBO_BOX(list), &iter);
					found = true;
				}
			}
			if (!found){
				gtk_combo_box_set_active(GTK_COMBO_BOX(list), 0);
			}
			g_object_unref(store);
			return list;
		}
		GtkWidget *newCheckbox(const char *label, bool value)
		{
			GtkWidget *widget = gtk_check_button_new_with_label(label);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
			return widget;
		}
		void afterFilterChanged()
		{
			GtkFileFilter *filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(m_dialog));
			ImportExportFormat *format = (ImportExportFormat*)g_object_get_data(G_OBJECT(filter), "format");
			gtk_widget_set_sensitive(m_converters, format->type == FileType::txt);
			gtk_widget_set_sensitive(m_item_sizes, format->type == FileType::html);
		}
		static void filterChanged(GtkFileChooserDialog*, GParamSpec*, ImportExportDialogOptions *import_export_dialog)
		{
			import_export_dialog->afterFilterChanged();
		}
};


ImportExportDialog::ImportExportDialog(GtkWindow* parent, ColorList *color_list, GlobalState *gs):
	m_parent(parent),
	m_color_list(color_list),
	m_gs(gs)
{
}
ImportExportDialog::~ImportExportDialog()
{
}
bool ImportExportDialog::showImport()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import"), m_parent,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	const ImportExportFormat formats[] = {
		{_("Gpick Palette (*.gpa)"), "*.gpa", FileType::gpa},
		{_("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", FileType::gpl},
		{_("Adobe Swatch Exchange (*.ase)"), "*.ase", FileType::ase},
		{_("Text File (*.txt)"), "*.txt", FileType::txt},
	};
	const size_t n_formats = sizeof(formats) / sizeof(ImportExportFormat);
	const char* default_path = dynv_get_string_wd(m_gs->params, "gpick.import.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	const char* selected_filter = dynv_get_string_wd(m_gs->params, "gpick.import.filter", "all_supported");
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All files"));
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer)"all");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (g_strcmp0(selected_filter, "all") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("All supported formats"));
	g_object_set_data(G_OBJECT(filter), "identification", (gpointer)"all_supported");
	for (size_t i = 0; i != n_formats; ++i){
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
	}
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (g_strcmp0(selected_filter, "all_supported") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	for (size_t i = 0; i != n_formats; ++i){
		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, formats[i].name);
		g_object_set_data(G_OBJECT(filter), "identification", (gpointer)formats[i].pattern);
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if (g_strcmp0(formats[i].pattern, selected_filter) == 0){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
	bool finished = false;
	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(m_gs->params, "gpick.import.path", path);
			g_free(path);
			FileType type = ImportExport::getFileType(filename);
			if (type == FileType::unknown){
				const gchar *format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
				for (size_t i = 0; i != n_formats; ++i){
					if (g_strcmp0(formats[i].name, format_name) == 0){
						type = formats[i].type;
						break;
					}
				}
			}
			GtkWidget* message;
			if (type == FileType::unknown){
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File format is not supported"));
				gtk_window_set_title(GTK_WINDOW(message), _("Import"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}else{
				Converters *converters = static_cast<Converters*>(dynv_get_pointer_wdc(m_gs->params, "Converters", 0));
				for (size_t i = 0; i != n_formats; ++i){
					if (formats[i].type == type){
						ImportExport import_export(m_color_list, filename, m_gs);
						import_export.setConverters(converters);
						if (import_export.importType(formats[i].type)){
							finished = true;
						}else{
							message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be imported"));
							gtk_window_set_title(GTK_WINDOW(message), _("Import"));
							gtk_dialog_run(GTK_DIALOG(message));
							gtk_widget_destroy(message);
						}
						const char *identification = (const char*)g_object_get_data(G_OBJECT(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog))), "identification");
						dynv_set_string(m_gs->params, "gpick.import.filter", identification);
						break;
					}
				}
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy(dialog);
	return finished;
}
bool ImportExportDialog::showImportTextFile()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import text file"), m_parent,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	const char* default_path = dynv_get_string_wd(m_gs->params, "gpick.import_text_file.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	bool finished = false;
	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(m_gs->params, "gpick.import_text_file.path", path);
			g_free(path);
			GtkWidget* message;
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			ImportExport import_export(m_color_list, filename, m_gs);
			Converters *converters = static_cast<Converters*>(dynv_get_pointer_wdc(m_gs->params, "Converters", 0));
			import_export.setConverters(converters);
			if (import_export.importTXT()){
				finished = true;
			}else{
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be imported"));
				gtk_window_set_title(GTK_WINDOW(message), _("Import text file"));
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy(dialog);
	return finished;
}
bool ImportExportDialog::showExport()
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Export"), m_parent,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_OK,
		nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	const ImportExportFormat formats[] = {
		{_("Gpick Palette (*.gpa)"), "*.gpa", FileType::gpa},
		{_("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", FileType::gpl},
		{_("Alias/WaveFront Material (*.mtl)"), "*.mtl", FileType::mtl},
		{_("Adobe Swatch Exchange (*.ase)"), "*.ase", FileType::ase},
		{_("Cascading Style Sheet (*.css)"), "*.css", FileType::css},
		{_("Hyper Text Markup Language (*.html, *.htm)"), "*.html,*.htm", FileType::html},
		{_("Text file (*.txt)"), "*.txt", FileType::txt},
	};
	const size_t n_formats = sizeof(formats) / sizeof(ImportExportFormat);
	const char* default_path = dynv_get_string_wd(m_gs->params, "gpick.export.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	string selected_filter = dynv_get_string_wd(m_gs->params, "gpick.export.filter", "*.gpl");
	for (size_t i = 0; i != n_formats; ++i){
		GtkFileFilter *filter = gtk_file_filter_new();
		g_object_set_data(G_OBJECT(filter), "format", (gpointer)&formats[i]);
		gtk_file_filter_set_name(filter, formats[i].name);
		split(formats[i].pattern, ',', true, [filter](const string &value){
			gtk_file_filter_add_pattern(filter, value.c_str());
		});
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
		if (formats[i].pattern == selected_filter){
			gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
		}
	}
	ImportExportDialogOptions import_export_dialog_options(dialog, m_gs);
	bool finished = false;
	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(m_gs->params, "gpick.import.path", path);
			g_free(path);
			import_export_dialog_options.saveState();
			string format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
			for (size_t i = 0; i != n_formats; ++i){
				if (formats[i].name == format_name){
					ImportExport import_export(m_color_list, filename, m_gs);
					import_export.setConverter(import_export_dialog_options.getSelectedConverter());
					string item_size = import_export_dialog_options.getSelectedItemSize();
					import_export.setItemSize(item_size.c_str());
					string background = import_export_dialog_options.getSelectedBackground();
					import_export.setBackground(background.c_str());
					import_export.setIncludeColorNames(import_export_dialog_options.isIncludeColorNamesEnabled());
					if (import_export.exportType(formats[i].type)){
						finished = true;
					}else{
						GtkWidget* message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be exported"));
						gtk_window_set_title(GTK_WINDOW(dialog), _("Export"));
						gtk_dialog_run(GTK_DIALOG(message));
						gtk_widget_destroy(message);
					}
					dynv_set_string(m_gs->params, "gpick.export.filter", formats[i].pattern);
				}
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy(dialog);
	return finished;
}
