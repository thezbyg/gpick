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

#include "uiUtilities.h"
GtkWidget* gtk_label_aligned_new(const gchar* text, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale) {
	GtkWidget *align = gtk_alignment_new(xalign, yalign, xscale, yscale);
	GtkWidget *label = gtk_label_new(text);
	gtk_container_add(GTK_CONTAINER(align), label);
	return align;
}
GtkWidget* gtk_label_mnemonic_aligned_new(const gchar* text, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale) {
	GtkWidget *align = gtk_alignment_new(xalign, yalign, xscale, yscale);
	GtkWidget *label = gtk_label_new("");
	gtk_label_set_text_with_mnemonic(GTK_LABEL(label), text);
	gtk_container_add(GTK_CONTAINER(align), label);
	return align;
}
GtkWidget* gtk_widget_aligned_new(GtkWidget* widget, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale) {
	GtkWidget *align = gtk_alignment_new(xalign, yalign, xscale, yscale);
	gtk_container_add(GTK_CONTAINER(align), widget);
	return align;
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
GtkWidget *newTextView(const std::string &text) {
	GtkWidget *widget = gtk_text_view_new();
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), text.c_str(), text.length());
	return widget;
}
std::string getTextViewText(GtkWidget *widget) {
	auto buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, false);
	std::string result(text);
	g_free(text);
	return result;
}
GtkWidget *newLabel(const std::string &text) {
	return gtk_label_aligned_new(text.c_str(), 0, 0.5, 0, 0);
}
guint getKeyval(const GdkEventKey &key, std::optional<uint32_t> latinKeysGroup) {
	guint keyval;
	gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(gdk_display_get_default()), key.hardware_keycode, static_cast<GdkModifierType>(0), latinKeysGroup ? *latinKeysGroup : 0, &keyval, nullptr, nullptr, nullptr);
	return keyval;
}
void setDialogContent(GtkWidget *dialog, GtkWidget *content) {
	auto contentArea = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(contentArea), content, true, true, 5);
	gtk_widget_show_all(content);
}
GtkWidget *newIcon(const char *name, IconSize size) {
#if GTK_MAJOR_VERSION >= 3
	return gtk_image_new_from_icon_name(name, size == IconSize::menu ? GTK_ICON_SIZE_MENU : GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
	return gtk_image_new_from_stock(name, size == IconSize::menu ? GTK_ICON_SIZE_MENU : GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif
}
GtkWidget *newIcon(const char *name, int size) {
#if GTK_MAJOR_VERSION >= 3
	auto image = gtk_image_new_from_icon_name(name, GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
	auto image = gtk_image_new_from_stock(name, GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif
	gtk_image_set_pixel_size(GTK_IMAGE(image), size);
	return image;
}
GtkWidget *newMenuItem(const char *label, const char *iconName) {
	auto menuItem = gtk_image_menu_item_new_with_mnemonic(label);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuItem), newIcon(iconName, IconSize::menu));
	return menuItem;
}
void showContextMenu(GtkWidget *menu, GdkEventButton *event) {
	gtk_widget_show_all(GTK_WIDGET(menu));
	gint32 button, eventTime;
	if (event) {
		button = event->button;
		eventTime = event->time;
	} else {
		button = 0;
		eventTime = gtk_get_current_event_time();
	}
	gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, nullptr, nullptr, button, eventTime);
	g_object_ref_sink(menu);
	g_object_unref(menu);
}
void setWidgetData(GtkWidget *widget, const char *name, const std::string &value) {
	g_object_set_data_full(G_OBJECT(widget), name, g_strdup(value.c_str()), (GDestroyNotify)g_free);
}
Grid::Grid(int columns, int rows, int columnSpacing, int rowSpacing):
	m_columns(columns),
	m_column(0),
	m_row(0),
	m_columnSpacing(columnSpacing),
	m_rowSpacing(rowSpacing) {
#if GTK_MAJOR_VERSION >= 3
	m_grid = gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(m_grid), columnSpacing);
	gtk_grid_set_row_spacing(GTK_GRID(m_grid), rowSpacing);
#else
	m_grid = gtk_table_new(rows, columns, false);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(m_grid), 5);
}
GtkWidget *Grid::add(GtkWidget *widget, bool expand, int width, bool verticalExpand) {
#if GTK_MAJOR_VERSION >= 3
	if (expand)
		gtk_widget_set_hexpand(widget, true);
	if (verticalExpand)
		gtk_widget_set_vexpand(widget, true);
	gtk_grid_attach(GTK_GRID(m_grid), widget, m_column, m_row, width, 1);
#else
	gtk_table_attach(GTK_TABLE(m_grid), widget, m_column, m_column + width, m_row, m_row + 1, GtkAttachOptions(GTK_FILL | (expand ? GTK_EXPAND : 0)), GtkAttachOptions(GTK_FILL | (verticalExpand ? GTK_EXPAND : 0)), m_columnSpacing / 2, m_rowSpacing / 2);
#endif
	m_column += width;
	if (m_column >= m_columns) {
		m_column = 0;
		m_row++;
	}
	return widget;
}
void Grid::nextColumn(int columns) {
	m_column += columns;
	if (m_column >= m_columns) {
		m_column = 0;
		m_row++;
	}
}
void Grid::nextRow() {
	m_row++;
}
Grid::operator GtkWidget *() {
	return m_grid;
}
