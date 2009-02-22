/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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
	GtkWidget* menu_item = gtk_image_menu_item_new_with_mnemonic(label);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
	return menu_item;
}

GtkWidget* gtk_label_aligned_new(const gchar* text, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale) {
	GtkWidget* align = gtk_alignment_new(xalign, yalign, xscale, yscale);
	GtkWidget* label = gtk_label_new(text);
	gtk_container_add(GTK_CONTAINER(align), label);
	return align;
}

GtkWidget* gtk_widget_aligned_new(GtkWidget* widget, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale){
	GtkWidget* align = gtk_alignment_new(xalign, yalign, xscale, yscale);
	gtk_container_add(GTK_CONTAINER(align), widget);
	return align;
}

gint g_key_file_get_integer_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, gint default_value) {
	GError *error=NULL;
	gint r=g_key_file_get_integer(key_file, group_name, key, &error);
	if (error){
		g_error_free(error);
		r=default_value;
	}
	return r;
}

gdouble g_key_file_get_double_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, gdouble default_value) {
	GError *error=NULL;
	gdouble r=g_key_file_get_double(key_file, group_name, key, &error);
	if (error){
		g_error_free(error);
		r=default_value;
	}
	return r;
}

gboolean g_key_file_get_boolean_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, gboolean default_value) {
	GError *error = NULL;
	gboolean r = g_key_file_get_boolean(key_file, group_name, key, &error);
	if (error) {
		g_error_free(error);
		r = default_value;
	}
	return r;
}

gchar* g_key_file_get_string_with_default(GKeyFile *key_file, const gchar *group_name, const gchar *key, const gchar *default_value) {
	GError *error = NULL;
	gchar* r = g_key_file_get_string(key_file, group_name, key, &error);
	if (error) {
		g_error_free(error);
		r = g_strdup(default_value);
	}
	return r;
}
