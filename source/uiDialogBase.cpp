/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#include "uiDialogBase.h"
#include "uiUtilities.h"
#include "GlobalState.h"
#include "ColorList.h"
#include "dynv/Map.h"
DialogBase::DialogBase(GlobalState &gs, const char *optionsKey, const char *title, GtkWindow *parent):
	gs(gs),
	dialog(nullptr) {
	options = gs.settings().getOrCreateMap(optionsKey);
	dialog = gtk_dialog_new_with_buttons(title, parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), options->getInt32("window.width", -1), options->getInt32("window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
}
DialogBase::~DialogBase() {
	if (dialog != nullptr) {
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
		options->set("window.width", width);
		options->set("window.height", height);
		gtk_widget_destroy(dialog);
	}
}
void DialogBase::run() {
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		apply(false);
	}
}
void DialogBase::setContent(GtkWidget *widget) {
	setDialogContent(dialog, widget);
}
void DialogBase::onUpdate(GtkWidget *, DialogBase *dialogBase) {
	dialogBase->apply(true);
}
