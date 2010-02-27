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

#include "Paths.h"

#include <glib/gstdio.h>

static gchar* get_data_dir(){
	static gchar* data_dir=NULL;
	if (data_dir) return data_dir;

	GList *paths=NULL, *i=NULL;
	gchar *tmp;

	i=g_list_append(i, (gchar*)"share");
	paths=i;

	i=g_list_append(i, (gchar*)g_get_user_data_dir());

	const gchar* const *datadirs = g_get_system_data_dirs();
	for (gint datadirs_i=0; datadirs[datadirs_i];++datadirs_i){
		i=g_list_append(i, (gchar*)datadirs[datadirs_i]);
	}

	i=paths;
	struct stat sb;
	while (i){
		tmp = g_build_filename((gchar*)i->data, "gpick", NULL);

		if (g_stat( tmp, &sb )==0){
			data_dir=g_strdup(tmp);
			g_free(tmp);
			break;
		}
		g_free(tmp);
		i=g_list_next(i);
	}

	g_list_free(paths);

	if (data_dir==NULL){
		data_dir=g_strdup("");
		return data_dir;
	}

	return data_dir;
}

gchar* build_filename(const gchar* filename){
	if (filename)
		return g_build_filename(get_data_dir(), filename, NULL);
	else
		return g_build_filename(get_data_dir(), NULL);
}

gchar* build_config_path(const gchar *filename){

	if (filename)
		return g_build_filename(g_get_user_config_dir(), "gpick", filename, NULL);
	else
		return g_build_filename(g_get_user_config_dir(), "gpick", NULL);
}


