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

#include "StandardMenu.h"
#include "CopyMenu.h"
#include "NearestColorsMenu.h"
#include "I18N.h"
#include <gdk/gdkkeysyms.h>

static void buildMenu(GtkWidget *menu, GtkWidget **copy_to_clipboard, GtkWidget **nearest_from_palette)
{
	GtkAccelGroup *accel_group = gtk_menu_get_accel_group(GTK_MENU(menu));
	GtkWidget *item = *copy_to_clipboard = gtk_menu_item_new_with_mnemonic(_("_Copy to clipboard"));
	if (accel_group)
		gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_c, GdkModifierType(GDK_CONTROL_MASK), GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	item = *nearest_from_palette = gtk_menu_item_new_with_mnemonic(_("_Nearest from palette"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}
void StandardMenu::appendMenu(GtkWidget *menu, ColorObject* color_object, GtkWidget *palette_widget, GlobalState *gs)
{
	GtkWidget *copy_to_clipboard, *nearest_from_palette;
	buildMenu(menu, &copy_to_clipboard, &nearest_from_palette);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copy_to_clipboard), CopyMenu::newMenu(color_object, palette_widget, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearest_from_palette), NearestColorsMenu::newMenu(color_object, gs));
}
void StandardMenu::appendMenu(GtkWidget *menu, ColorObject* color_object, GlobalState *gs)
{
	GtkWidget *copy_to_clipboard, *nearest_from_palette;
	buildMenu(menu, &copy_to_clipboard, &nearest_from_palette);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(copy_to_clipboard), CopyMenu::newMenu(color_object, gs));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(nearest_from_palette), NearestColorsMenu::newMenu(color_object, gs));
}
void StandardMenu::appendMenu(GtkWidget *menu)
{
	GtkWidget *copy_to_clipboard, *nearest_from_palette;
	buildMenu(menu, &copy_to_clipboard, &nearest_from_palette);
	gtk_widget_set_sensitive(copy_to_clipboard, false);
	gtk_widget_set_sensitive(nearest_from_palette, false);
}
