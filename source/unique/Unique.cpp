/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#include "Unique.h"

#include <gtk/gtk.h>
#include <unique/unique.h>

static UniqueApp *unique_app=NULL;
static void* unique_user_data=NULL;

static UniqueResponse user_callback (UniqueApp *app, gint command, UniqueMessageData *message_data, guint time_, int (*unique_callback)(void* user_data)){

	if (command == UNIQUE_ACTIVATE){
		unique_callback(unique_user_data);
		//gtk_window_set_startup_id(GTK_WINDOW (our_window), startup_id);
	}

	return UNIQUE_RESPONSE_OK;
}


int unique_init(int (*unique_callback)(void* user_data), void* user_data){

	unique_app = unique_app_new("org.gnome.gpick", NULL);
	unique_user_data = user_data;

	if (unique_app_is_running(unique_app)){

		unique_app_send_message(unique_app, UNIQUE_ACTIVATE, 0);

		g_object_unref(unique_app);
		return 1;
	}else{
		g_signal_connect(G_OBJECT (unique_app), "message-received", (GCallback) user_callback, (void*)unique_callback);
	}

	return 0;
}

int unique_term(){
	if (unique_app) g_object_unref(unique_app);
	return 0;
}


