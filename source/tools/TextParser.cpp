/*
 * Copyright (c) 2009-2019, Albertas Vyšniauskas
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

#include "TextParser.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "GlobalState.h"
#include "I18N.h"
#include "DynvHelpers.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "parser/TextFile.h"
#include <sstream>
using namespace std;

struct TextParserDialog {
	TextParserDialog(GtkWindow* parent, GlobalState *gs);
	~TextParserDialog();
	bool show();
	struct TextParser: public text_file_parser::TextFile {
		TextParser(const string &text):
			m_text(text),
			m_failed(false) {
		}
		virtual ~TextParser() {
		}
		virtual void outOfMemory() {
			m_failed = true;
		}
		virtual void syntaxError(size_t start_line, size_t start_column, size_t end_line, size_t end_colunn) {
			m_failed = true;
		}
		virtual size_t read(char *buffer, size_t length) {
			m_text.read(buffer, length);
			size_t bytes = m_text.gcount();
			if (bytes > 0)
				return bytes;
			if (m_text.eof())
				return 0;
			if (!m_text.good()) {
				m_failed = true;
			}
			return 0;
		}
		virtual void addColor(const Color &color) {
			m_colors.push_back(color);
		}
		bool failed() const {
			return m_failed;
		}
		const list<Color> &colors() const {
			return m_colors;
		}
	private:
		stringstream m_text;
		list<Color> m_colors;
		bool m_failed;
	};
private:
	GtkWindow *m_parent;
	GtkWidget *m_dialog, *m_text_view;
	GtkWidget *m_single_line_c_comments, *m_multi_line_c_comments, *m_single_line_hash_comments, *m_css_rgb, *m_css_rgba, *m_short_hex, *m_full_hex, *m_float_values, *m_int_values;
	GtkWidget *m_preview_expander;
	ColorList *m_preview_color_list;
	GlobalState *m_gs;
	struct dynvSystem *m_params;
	bool isSingleLineCCommentsEnabled();
	bool isMultiLineCCommentsEnabled();
	bool isSingleLineHashCommentsEnabled();
	bool isCssRgbEnabled();
	bool isCssRgbaEnabled();
	bool isFullHexEnabled();
	bool isShortHexEnabled();
	bool isIntValuesEnabled();
	bool isFloatValuesEnabled();
	void saveSettings();
	void preview();
	void apply();
	bool parse(ColorList *color_list);
	static void onDestroy(GtkWidget *widget, TextParserDialog *dialog);
	static void onResponse(GtkWidget *widget, gint response_id, TextParserDialog *dialog);
	static void onChange(GtkWidget *widget, TextParserDialog *dialog);
};
TextParserDialog::TextParserDialog(GtkWindow* parent, GlobalState *gs):
	m_parent(parent),
	m_gs(gs) {
	m_params = dynv_get_dynv(m_gs->getSettings(), "gpick.tools.text_parser");
	GtkWidget *dialog = m_dialog = gtk_dialog_new_with_buttons(_("Text parser"), m_parent, GtkDialogFlags(GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_ADD, GTK_RESPONSE_APPLY, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), dynv_get_int32_wd(m_params, "window.width", -1), dynv_get_int32_wd(m_params, "window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY, GTK_RESPONSE_CLOSE, -1);
	GtkWidget *table = gtk_table_new(5, 9, false);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), m_text_view = newTextView(dynv_get_string_wd(m_params, "text", "")));
	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_text_view))), "changed", G_CALLBACK(onChange), this);
	GtkWidget *vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), newLabel(string(_("Text")) + ":"), false, false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
	gtk_table_attach(GTK_TABLE(table), vbox, 0, 8, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	int y = 1;
	addOption(m_single_line_c_comments = newCheckbox(string(_("C style single-line comments")) + " (//abc)", dynv_get_bool_wd(m_params, "single_line_c_comments", true)), 0, y, table);
	addOption(m_multi_line_c_comments = newCheckbox(string(_("C style multi-line comments")) + " (/*abc*/)", dynv_get_bool_wd(m_params, "multi_line_c_comments", true)), 0, y, table);
	addOption(m_single_line_hash_comments = newCheckbox(string(_("Hash single-line comments")) + " (#abc)", dynv_get_bool_wd(m_params, "single_line_hash_comments", true)), 0, y, table);
	y = 1;
	addOption(m_css_rgb = newCheckbox("CSS rgb()", dynv_get_bool_wd(m_params, "css_rgb", true)), 1, y, table);
	addOption(m_css_rgba = newCheckbox("CSS rgba()", dynv_get_bool_wd(m_params, "css_rgba", true)), 1, y, table);
	addOption(m_full_hex = newCheckbox(_("Full hex"), dynv_get_bool_wd(m_params, "full_hex", true)), 1, y, table);
	y = 1;
	addOption(m_short_hex = newCheckbox(_("Short hex"), dynv_get_bool_wd(m_params, "short_hex", true)), 2, y, table);
	addOption(m_int_values = newCheckbox(_("Integer values"), dynv_get_bool_wd(m_params, "int_values", true)), 2, y, table);
	addOption(m_float_values = newCheckbox(_("Real values"), dynv_get_bool_wd(m_params, "float_values", true)), 2, y, table);
	g_signal_connect(G_OBJECT(m_single_line_c_comments), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_multi_line_c_comments), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_single_line_hash_comments), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_rgb), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_rgba), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_full_hex), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_short_hex), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_int_values), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_float_values), "toggled", G_CALLBACK(onChange), this);
	gtk_widget_show_all(table);
	gtk_table_attach(GTK_TABLE(table), m_preview_expander = palette_list_preview_new(m_gs, true, dynv_get_bool_wd(m_params, "show_preview", true), m_gs->getColorList(), &m_preview_color_list), 0, 8, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	preview();
	gtk_widget_show_all(table);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(onDestroy), this);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(onResponse), this);
}
TextParserDialog::~TextParserDialog() {
	color_list_destroy(m_preview_color_list);
	dynv_system_release(m_params);
}
bool TextParserDialog::show() {
	gtk_widget_show(m_dialog);
	return true;
}
void TextParserDialog::onDestroy(GtkWidget *widget, TextParserDialog *dialog) {
	delete dialog;
}
bool TextParserDialog::parse(ColorList *color_list) {
	text_file_parser::Configuration configuration;
	configuration.single_line_c_comments = isSingleLineCCommentsEnabled();
	configuration.multi_line_c_comments = isMultiLineCCommentsEnabled();
	configuration.single_line_hash_comments = isSingleLineHashCommentsEnabled();
	configuration.css_rgb = isCssRgbEnabled();
	configuration.css_rgba = isCssRgbaEnabled();
	configuration.full_hex = isFullHexEnabled();
	configuration.short_hex = isShortHexEnabled();
	configuration.int_values = isIntValuesEnabled();
	configuration.float_values = isFloatValuesEnabled();
	string text = getTextViewText(m_text_view);
	TextParser text_parser(text);
	if (!text_parser.parse(configuration)) {
		return false;
	}
	if (text_parser.failed()) {
		return false;
	}
	if (text_parser.colors().size() == 0) {
		return false;
	}
	for (auto color: text_parser.colors()) {
		auto color_object = new ColorObject("", color);
		color_list_add_color_object(color_list, color_object, true);
		color_object->release();
	}
	return true;
}
void TextParserDialog::onChange(GtkWidget *widget, TextParserDialog *dialog) {
	dialog->preview();
}
void TextParserDialog::preview() {
	color_list_remove_all(m_preview_color_list);
	parse(m_preview_color_list);
}
void TextParserDialog::apply() {
	parse(m_gs->getColorList());
}
void TextParserDialog::onResponse(GtkWidget *widget, gint response_id, TextParserDialog *dialog) {
	dialog->saveSettings();
	switch (response_id) {
	case GTK_RESPONSE_APPLY:
		dialog->apply();
		break;
	case GTK_RESPONSE_DELETE_EVENT:
		break;
	case GTK_RESPONSE_CLOSE:
		gtk_widget_destroy(widget);
		break;
	}
}
void TextParserDialog::saveSettings() {
	string text = getTextViewText(m_text_view);
	dynv_set_string(m_params, "text", text.c_str());
	dynv_set_bool(m_params, "single_line_c_comments", isSingleLineCCommentsEnabled());
	dynv_set_bool(m_params, "multi_line_c_comments", isMultiLineCCommentsEnabled());
	dynv_set_bool(m_params, "single_line_hash_comments", isSingleLineHashCommentsEnabled());
	dynv_set_bool(m_params, "css_rgb", isCssRgbEnabled());
	dynv_set_bool(m_params, "css_rgba", isCssRgbaEnabled());
	dynv_set_bool(m_params, "full_hex", isFullHexEnabled());
	dynv_set_bool(m_params, "short_hex", isShortHexEnabled());
	dynv_set_bool(m_params, "int_values", isIntValuesEnabled());
	dynv_set_bool(m_params, "float_values", isFloatValuesEnabled());
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(m_dialog), &width, &height);
	dynv_set_int32(m_params, "window.width", width);
	dynv_set_int32(m_params, "window.height", height);
	dynv_set_bool(m_params, "show_preview", gtk_expander_get_expanded(GTK_EXPANDER(m_preview_expander)));
}
bool TextParserDialog::isSingleLineCCommentsEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_single_line_c_comments));
}
bool TextParserDialog::isMultiLineCCommentsEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_multi_line_c_comments));
}
bool TextParserDialog::isSingleLineHashCommentsEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_single_line_hash_comments));
}
bool TextParserDialog::isCssRgbEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_css_rgb));
}
bool TextParserDialog::isCssRgbaEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_css_rgba));
}
bool TextParserDialog::isFullHexEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_full_hex));
}
bool TextParserDialog::isShortHexEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_short_hex));
}
bool TextParserDialog::isIntValuesEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_int_values));
}
bool TextParserDialog::isFloatValuesEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_float_values));
}
void tools_text_parser_show(GtkWindow* parent, GlobalState* gs) {
	auto *dialog = new TextParserDialog(parent, gs);
	dialog->show();
}
