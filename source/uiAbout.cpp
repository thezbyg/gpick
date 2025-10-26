/*
 * Copyright (c) 2009-2025, Albertas Vyšniauskas
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

#include "uiAbout.h"
#include "uiUtilities.h"
#include "version/Version.h"
#include "I18N.h"
#include "Paths.h"

const gchar* program_name = "Gpick";

static GtkWidget* new_page(const char *text)
{
	GtkWidget *text_view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(buffer, text, -1);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), false);
	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_container_add(GTK_CONTAINER(scrolled), text_view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	return scrolled;
}
void show_about_box(GtkWidget *widget)
{
	const char *license = {
"Copyright \xc2\xa9 2009-2025, Albertas Vyšniauskas\n"
"\n"
"All rights reserved.\n"
"\n"
"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:\n"
"\n"
"   * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.\n"
"   * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.\n"
"   * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
	};
	const char *expat_license = {
"Copyright \xc2\xa9 1998, 1999, 2000 Thai Open Source Software Center Ltd and Clark Cooper\n"
"Copyright \xc2\xa9 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers.\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
	};
	const char *lua_license = {
"Copyright \xc2\xa9 1994-2017 Lua.org, PUC-Rio.\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
	};
	const char *program_authors = {
"Albertas Vyšniauskas <albertas.vysniauskas@gpick.org>\n"
/* Add yourself here if you helped Gpick project in any way (patch, translation, etc).
 * Everything is optional, if you do not want, you do not have to disclose your e-mail
 * address, real name or any other information.
 * Please keep this list sorted alphabetically.
 */
	};
	GtkWidget* dialog = gtk_dialog_new_with_buttons(_("About Gpick"), GTK_WINDOW(gtk_widget_get_toplevel(widget)), GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, nullptr);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	GtkWidget *align_box = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), align_box, false, false, 0);
	gtk_box_pack_start(GTK_BOX(align_box), gtk_vbox_new(false, 0), true, true, 0);
	gtk_box_pack_end(GTK_BOX(align_box), gtk_vbox_new(false, 0), true, true, 0);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(align_box), hbox, false, false, 0);
	auto logoFilename = buildFilename("logo.png");
	GtkWidget *image = gtk_image_new_from_file(logoFilename.c_str());
	gtk_box_pack_start(GTK_BOX(hbox), image, false, false, 0);
	GtkWidget *vbox2 = gtk_vbox_new(false, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, false, false, 0);
	gchar *tmp_string = g_markup_printf_escaped("<span size=\"xx-large\" weight=\"bold\">%s %s</span>\n<span size=\"small\">%s: %s</span>", program_name, version::versionFull, _("Hash"), version::hash);
	GtkWidget *name = gtk_label_new(0);
	gtk_label_set_selectable(GTK_LABEL(name), true);
	gtk_label_set_justify(GTK_LABEL(name), GTK_JUSTIFY_CENTER);
	gtk_label_set_markup(GTK_LABEL(name), tmp_string);
	gtk_box_pack_start(GTK_BOX(vbox2), name, false, false, 0);
	g_free(tmp_string);
	tmp_string = g_markup_printf_escaped("<span size=\"small\">%s © 2009-2025, Albertas Vyšniauskas %s</span>", _("Copyright"), _("and Gpick development team"));
	GtkWidget *copyright = gtk_label_new(0);
	gtk_label_set_selectable(GTK_LABEL(copyright), true);
	gtk_label_set_justify(GTK_LABEL(copyright), GTK_JUSTIFY_CENTER);
	gtk_label_set_line_wrap(GTK_LABEL(copyright), true);
	gtk_label_set_markup(GTK_LABEL(copyright), tmp_string);
	gtk_box_pack_start(GTK_BOX(vbox2), copyright, false, false, 0);
	g_free(tmp_string);
	GtkWidget *website = gtk_link_button_new("https://www.gpick.org/");
	gtk_box_pack_start(GTK_BOX(vbox2), website, false, false, 0);
	GtkWidget *notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), true);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), new_page(license), gtk_label_new(_("License")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), new_page(program_authors), gtk_label_new(_("Credits")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), new_page(expat_license), gtk_label_new(_("Expat License")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), new_page(lua_license), gtk_label_new(_("Lua License")));
	gtk_box_pack_start(GTK_BOX(vbox), notebook, true, true, 0);
	setDialogContent(dialog, vbox);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return;
}
