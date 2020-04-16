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
GtkWidget* gtk_menu_item_new_with_image(const gchar* label, GtkWidget *image) {
	GtkWidget *menu_item = gtk_image_menu_item_new_with_mnemonic(label);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
	return menu_item;
}
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
guint getKeyval(const GdkEventKey &key, boost::optional<uint32_t> latinKeysGroup) {
	guint keyval;
	gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(gdk_display_get_default()), key.hardware_keycode, static_cast<GdkModifierType>(0), latinKeysGroup ? *latinKeysGroup : 0, &keyval, nullptr, nullptr, nullptr);
	return keyval;
}
