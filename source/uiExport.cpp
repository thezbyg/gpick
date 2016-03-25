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

#include "uiExport.h"
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

struct ExportFormat
{
	const char* name;
	const char* pattern;
	FileType type;
};

class ImportExportDialog
{
	public:
		GtkWidget *m_dialog;
		GtkWidget *m_converters;
		GlobalState *m_gs;
		ImportExportDialog(GtkWidget *dialog, GlobalState *gs)
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
			afterFilterChanged();
			int y = 0;
			GtkWidget *table = gtk_table_new(2, 3, false);
			gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new(_("Converter:"), 0, 0.5, 0, 0), 0, 1, y, y + 1, GTK_FILL, GTK_FILL, 3, 3);
			gtk_table_attach(GTK_TABLE(table), m_converters, 1, 2, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 3, 3);
			gtk_widget_show_all(table);
			gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), table);
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
		void saveState()
		{
			Converter *converter = getSelectedConverter();
			if (converter)
				dynv_set_string(m_gs->params, "gpick.import.converter", converter->function_name);
		}
		GtkWidget* newConverterList()
		{
			GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
			GtkWidget *list = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
			GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(list), renderer, true);
			gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(list), renderer, "text", 0, nullptr);
			if (store) g_object_unref (store);
			return list;
		}
		void afterFilterChanged()
		{
			GtkFileFilter *filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(m_dialog));
			ExportFormat *format = (ExportFormat*)g_object_get_data(G_OBJECT(filter), "format");
			gtk_widget_set_sensitive(m_converters, format->type == FileType::txt);
		}
		static void filterChanged(GtkFileChooserDialog*, GParamSpec*, ImportExportDialog *import_export_dialog)
		{
			import_export_dialog->afterFilterChanged();
		}
};

int dialog_export_show(GtkWindow* parent, ColorList *selected_color_list, bool selected, GlobalState *gs)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Export"), parent,
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_OK,
		NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	const ExportFormat formats[] = {
		{_("Gpick Palette (*.gpa)"), "*.gpa", FileType::gpa},
		{_("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", FileType::gpl},
		{_("Alias/WaveFront Material (*.mtl)"), "*.mtl", FileType::mtl},
		{_("Adobe Swatch Exchange (*.ase)"), "*.ase", FileType::ase},
		{_("Cascading Style Sheet (*.css)"), "*.css", FileType::css},
		{_("Hyper Text Markup Language (*.html, *.htm)"), "*.html,*.htm", FileType::html},
		{_("Text file (*.txt)"), "*.txt", FileType::txt},
	};
	const size_t n_formats = sizeof(formats) / sizeof(ExportFormat);
	const char* default_path = dynv_get_string_wd(gs->params, "gpick.export.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	string selected_filter = dynv_get_string_wd(gs->params, "gpick.export.filter", "*.gpl");
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
	ImportExportDialog import_export_dialog(dialog, gs);
	bool saved = false;
	while (!saved){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(gs->params, "gpick.import.path", path);
			g_free(path);
			import_export_dialog.saveState();
			string format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
			for (size_t i = 0; i != n_formats; ++i){
				if (formats[i].name == format_name){
					ColorList *color_list_arg;
					if (selected){
						color_list_arg = selected_color_list;
					}else{
						color_list_arg = gs->colors;
					}
					ImportExport import_export(color_list_arg, filename);
					import_export.setConverter(import_export_dialog.getSelectedConverter());
					if (import_export.exportType(formats[i].type)){
						saved = true;
					}else{
						GtkWidget* message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be exported"));
						gtk_window_set_title(GTK_WINDOW(dialog), _("Export"));
						gtk_dialog_run(GTK_DIALOG(message));
						gtk_widget_destroy(message);
					}
					dynv_set_string(gs->params, "gpick.export.filter", formats[i].pattern);
				}
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy(dialog);
	return 0;
}

int dialog_import_show(GtkWindow* parent, ColorList *selected_color_list, GlobalState *gs)
{
	GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import"), parent,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	struct ImportFormat
	{
		const char* name;
		const char* pattern;
		FileType type;
	};
	const ImportFormat formats[] = {
		{_("Gpick Palette (*.gpa)"), "*.gpa", FileType::gpa},
		{_("GIMP/Inkscape Palette (*.gpl)"), "*.gpl", FileType::gpl},
		{_("Adobe Swatch Exchange (*.ase)"), "*.ase", FileType::ase},
		{_("Text File (*.txt)"), "*.txt", FileType::txt},
	};
	const size_t n_formats = sizeof(formats) / sizeof(ImportFormat);
	const char* default_path = dynv_get_string_wd(gs->params, "gpick.import.path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path);
	const char* selected_filter = dynv_get_string_wd(gs->params, "gpick.import.filter", "all_supported");

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
	for (size_t i = 0; i != n_formats; ++i) {
		gtk_file_filter_add_pattern(filter, formats[i].pattern);
	}
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	if (g_strcmp0(selected_filter, "all_supported") == 0){
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	for (size_t i = 0; i != n_formats; ++i) {
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
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			dynv_set_string(gs->params, "gpick.import.path", path);
			g_free(path);
			FileType type = ImportExport::getFileType(filename);
			if (type == FileType::unknown){
				const gchar *format_name = gtk_file_filter_get_name(gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dialog)));
				for (size_t i = 0; i != n_formats; ++i) {
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
				Converters *converters = static_cast<Converters*>(dynv_get_pointer_wdc(gs->params, "Converters", 0));
				for (size_t i = 0; i != n_formats; ++i) {
					if (formats[i].type == type){
						struct ColorList *color_list_arg;
						color_list_arg = gs->colors;
						ImportExport import_export(color_list_arg, filename);
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
						dynv_set_string(gs->params, "gpick.import.filter", identification);
						break;
					}
				}
			}
			g_free(filename);
		}else break;
	}
	gtk_widget_destroy (dialog);
	return 0;
}

