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

#ifndef GPICK_UI_UTILITIES_H_
#define GPICK_UI_UTILITIES_H_
#include <gtk/gtk.h>
#include <string>
#include <boost/optional.hpp>
GtkWidget* gtk_label_aligned_new(const gchar* text, gfloat xalign = 0, gfloat yalign = 0, gfloat xscale = 0, gfloat yscale = 0);
GtkWidget* gtk_label_mnemonic_aligned_new(const gchar* text, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale);
GtkWidget* gtk_widget_aligned_new(GtkWidget* widget, gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale);
GtkWidget *addOption(const char *label, GtkWidget *widget, int x, int &y, GtkWidget *table);
GtkWidget *addOption(GtkWidget *widget, int x, int &y, GtkWidget *table);
GtkWidget *newCheckbox(const char *label, bool value);
GtkWidget *newCheckbox(const std::string &label, bool value);
GtkWidget *newTextView(const std::string &text);
std::string getTextViewText(GtkWidget *widget);
GtkWidget *newLabel(const std::string &text);
guint getKeyval(const GdkEventKey &key, boost::optional<uint32_t> latinKeysGroup);
void setDialogContent(GtkWidget *dialog, GtkWidget *content);
enum class IconSize {
	toolbar,
	menu,
};
GtkWidget *newIcon(const char *name, IconSize size);
GtkWidget *newIcon(const char *name, int size);
GtkWidget *newMenuItem(const char *label, const char *iconName);
void showContextMenu(GtkWidget *menu, GdkEventButton *event);
#endif /* GPICK_UI_UTILITIES_H_ */
