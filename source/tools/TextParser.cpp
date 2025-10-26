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

#include "TextParser.h"
#include "ColorList.h"
#include "ColorObject.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "I18N.h"
#include "dynv/Map.h"
#include "uiListPalette.h"
#include "uiUtilities.h"
#include "parser/TextFile.h"
#include "common/Guard.h"
#include <sstream>
#include <vector>
using namespace std::string_literals;

struct TextParserDialog: public ToolColorNameAssigner {
	TextParserDialog(GtkWindow *parent, GlobalState *gs);
	~TextParserDialog();
	bool show();
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override;
	struct TextParser: public text_file_parser::TextFile {
		TextParser(const std::string &text):
			m_text(text),
			m_failed(false) {
		}
		virtual ~TextParser() {
		}
		virtual void outOfMemory() {
			m_failed = true;
		}
		virtual void syntaxError(size_t, size_t, size_t, size_t) {
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
		const std::vector<Color> &colors() const {
			return m_colors;
		}
	private:
		std::stringstream m_text;
		std::vector<Color> m_colors;
		bool m_failed;
	};
private:
	GtkWindow *m_parent;
	GtkWidget *m_dialog, *m_textView;
	GtkWidget *m_single_line_c_comments, *m_multi_line_c_comments, *m_single_line_hash_comments, *m_css_rgb, *m_css_rgba, *m_short_hex, *m_full_hex, *m_short_hex_with_alpha, *m_full_hex_with_alpha, *m_float_values, *m_int_values, *m_css_hsl, *m_css_hsla, *m_css_oklch, *m_css_oklab;
	GtkWidget *m_preview_expander;
	common::Ref<ColorList> m_previewColorList;
	GlobalState *m_gs;
	size_t m_index;
	dynv::Ref m_options;
	bool isSingleLineCCommentsEnabled();
	bool isMultiLineCCommentsEnabled();
	bool isSingleLineHashCommentsEnabled();
	bool isCssRgbEnabled();
	bool isCssRgbaEnabled();
	bool isCssHslEnabled();
	bool isCssHslaEnabled();
	bool isCssOklchEnabled();
	bool isCssOklabEnabled();
	bool isFullHexEnabled();
	bool isShortHexEnabled();
	bool isFullHexWithAlphaEnabled();
	bool isShortHexWithAlphaEnabled();
	bool isIntValuesEnabled();
	bool isFloatValuesEnabled();
	void saveSettings();
	void preview();
	void apply();
	bool parse(ColorList &colorList);
	static void onDestroy(GtkWidget *widget, TextParserDialog *dialog);
	static void onResponse(GtkWidget *widget, gint response_id, TextParserDialog *dialog);
	static void onChange(GtkWidget *widget, TextParserDialog *dialog);
};
TextParserDialog::TextParserDialog(GtkWindow *parent, GlobalState *gs):
	ToolColorNameAssigner(*gs),
	m_parent(parent),
	m_gs(gs) {
	m_options = m_gs->settings().getOrCreateMap("gpick.tools.text_parser");
	GtkWidget *dialog = m_dialog = gtk_dialog_new_with_buttons(_("Text parser"), m_parent, GtkDialogFlags(GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_ADD, GTK_RESPONSE_APPLY, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), m_options->getInt32("window.width", -1), m_options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_APPLY, GTK_RESPONSE_CLOSE, -1);
	GtkWidget *table = gtk_table_new(6, 15, false);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), m_textView = newTextView(m_options->getString("text", "").c_str()));
	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_textView))), "changed", G_CALLBACK(onChange), this);
	GtkWidget *vbox = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), newLabel(_("Text") + ":"s), false, false, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
	gtk_table_attach(GTK_TABLE(table), vbox, 0, 14, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 5);
	int y = 1;
	addOption(m_single_line_c_comments = newCheckbox(_("C style single-line comments") + " (//abc)"s, m_options->getBool("single_line_c_comments", true)), 0, y, table);
	addOption(m_multi_line_c_comments = newCheckbox(_("C style multi-line comments") + " (/*abc*/)"s, m_options->getBool("multi_line_c_comments", true)), 0, y, table);
	addOption(m_single_line_hash_comments = newCheckbox(_("Hash single-line comments") + " (#abc)"s, m_options->getBool("single_line_hash_comments", true)), 0, y, table);
	y = 1;
	addOption(m_css_rgb = newCheckbox("CSS rgb()", m_options->getBool("css_rgb", true)), 1, y, table);
	addOption(m_css_rgba = newCheckbox("CSS rgba()", m_options->getBool("css_rgba", true)), 1, y, table);
	addOption(m_css_hsl = newCheckbox("CSS hsl()", m_options->getBool("css_hsl", true)), 1, y, table);
	addOption(m_css_hsla = newCheckbox("CSS hsla()", m_options->getBool("css_hsla", true)), 1, y, table);
	y = 1;
	addOption(m_css_oklch = newCheckbox("CSS oklch()", m_options->getBool("css_oklch", true)), 2, y, table);
	addOption(m_css_oklab = newCheckbox("CSS oklab()", m_options->getBool("css_oklab", true)), 2, y, table);
	y = 1;
	addOption(m_full_hex = newCheckbox(_("Full hex"), m_options->getBool("full_hex", true)), 3, y, table);
	addOption(m_full_hex_with_alpha = newCheckbox(_("Full hex with alpha"), m_options->getBool("full_hex_with_alpha", true)), 3, y, table);
	addOption(m_short_hex = newCheckbox(_("Short hex"), m_options->getBool("short_hex", true)), 3, y, table);
	addOption(m_short_hex_with_alpha = newCheckbox(_("Short hex with alpha"), m_options->getBool("short_hex_with_alpha", true)), 3, y, table);
	y = 1;
	addOption(m_int_values = newCheckbox(_("Integer values"), m_options->getBool("int_values", true)), 4, y, table);
	addOption(m_float_values = newCheckbox(_("Real values"), m_options->getBool("float_values", true)), 4, y, table);
	y = 5;
	g_signal_connect(G_OBJECT(m_single_line_c_comments), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_multi_line_c_comments), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_single_line_hash_comments), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_rgb), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_rgba), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_hsl), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_hsla), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_oklch), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_css_oklab), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_full_hex), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_short_hex), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_full_hex_with_alpha), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_short_hex_with_alpha), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_int_values), "toggled", G_CALLBACK(onChange), this);
	g_signal_connect(G_OBJECT(m_float_values), "toggled", G_CALLBACK(onChange), this);
	gtk_widget_show_all(table);
	gtk_table_attach(GTK_TABLE(table), m_preview_expander = palette_list_preview_new(*m_gs, true, m_options->getBool("show_preview", true), m_previewColorList), 0, 14, y, y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	preview();
	gtk_widget_show_all(table);
	setDialogContent(dialog, table);
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(onDestroy), this);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(onResponse), this);
}
TextParserDialog::~TextParserDialog() {
}
bool TextParserDialog::show() {
	gtk_widget_show(m_dialog);
	return true;
}
void TextParserDialog::onDestroy(GtkWidget *widget, TextParserDialog *dialog) {
	delete dialog;
}
bool TextParserDialog::parse(ColorList &colorList) {
	text_file_parser::Configuration configuration;
	configuration.singleLineCComments = isSingleLineCCommentsEnabled();
	configuration.multiLineCComments = isMultiLineCCommentsEnabled();
	configuration.singleLineHashComments = isSingleLineHashCommentsEnabled();
	configuration.cssRgb = isCssRgbEnabled();
	configuration.cssRgba = isCssRgbaEnabled();
	configuration.cssHsl = isCssHslEnabled();
	configuration.cssHsla = isCssHslaEnabled();
	configuration.cssOklch = isCssOklchEnabled();
	configuration.cssOklab = isCssOklabEnabled();
	configuration.fullHex = isFullHexEnabled();
	configuration.shortHex = isShortHexEnabled();
	configuration.fullHexWithAlpha = isFullHexWithAlphaEnabled();
	configuration.shortHexWithAlpha = isShortHexWithAlphaEnabled();
	configuration.intValues = isIntValuesEnabled();
	configuration.floatValues = isFloatValuesEnabled();
	auto text = getTextViewText(m_textView);
	TextParser textParser(text);
	if (!textParser.parse(configuration)) {
		return false;
	}
	if (textParser.failed()) {
		return false;
	}
	if (textParser.colors().size() == 0) {
		return false;
	}
	m_index = 0;
	common::Guard colorListGuard = colorList.changeGuard();
	for (auto color: textParser.colors()) {
		ColorObject colorObject(color);
		ToolColorNameAssigner::assign(colorObject);
		colorList.add(colorObject);
		m_index++;
	}
	return true;
}
void TextParserDialog::onChange(GtkWidget *widget, TextParserDialog *dialog) {
	dialog->preview();
}
void TextParserDialog::preview() {
	m_previewColorList->removeAll();
	parse(*m_previewColorList);
}
void TextParserDialog::apply() {
	parse(m_gs->colorList());
}
std::string TextParserDialog::getToolSpecificName(const ColorObject &colorObject) {
	return _("Parsed text color") + " #"s + std::to_string(m_index);
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
	auto text = getTextViewText(m_textView);
	m_options->set("text", text.c_str());
	m_options->set("single_line_c_comments", isSingleLineCCommentsEnabled());
	m_options->set("multi_line_c_comments", isMultiLineCCommentsEnabled());
	m_options->set("single_line_hash_comments", isSingleLineHashCommentsEnabled());
	m_options->set("css_rgb", isCssRgbEnabled());
	m_options->set("css_rgba", isCssRgbaEnabled());
	m_options->set("css_hsl", isCssHslEnabled());
	m_options->set("css_hsla", isCssHslaEnabled());
	m_options->set("css_oklch", isCssOklchEnabled());
	m_options->set("css_oklab", isCssOklabEnabled());
	m_options->set("full_hex", isFullHexEnabled());
	m_options->set("short_hex", isShortHexEnabled());
	m_options->set("full_hex_with_alpha", isFullHexWithAlphaEnabled());
	m_options->set("short_hex_with_alpha", isShortHexWithAlphaEnabled());
	m_options->set("int_values", isIntValuesEnabled());
	m_options->set("float_values", isFloatValuesEnabled());
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(m_dialog), &width, &height);
	m_options->set("window.width", width);
	m_options->set("window.height", height);
	m_options->set<bool>("show_preview", gtk_expander_get_expanded(GTK_EXPANDER(m_preview_expander)));
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
bool TextParserDialog::isCssHslEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_css_hsl));
}
bool TextParserDialog::isCssHslaEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_css_hsla));
}
bool TextParserDialog::isCssOklchEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_css_oklch));
}
bool TextParserDialog::isCssOklabEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_css_oklab));
}
bool TextParserDialog::isFullHexEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_full_hex));
}
bool TextParserDialog::isShortHexEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_short_hex));
}
bool TextParserDialog::isFullHexWithAlphaEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_full_hex_with_alpha));
}
bool TextParserDialog::isShortHexWithAlphaEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_short_hex_with_alpha));
}
bool TextParserDialog::isIntValuesEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_int_values));
}
bool TextParserDialog::isFloatValuesEnabled() {
	return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_float_values));
}
void tools_text_parser_show(GtkWindow *parent, GlobalState *gs) {
	auto *dialog = new TextParserDialog(parent, gs);
	dialog->show();
}
