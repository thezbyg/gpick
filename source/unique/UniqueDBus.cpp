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

#include "Unique.h"

#include <gtk/gtk.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <stdlib.h>
#include <stdio.h>

static DBusGProxy *proxy=NULL;
static int (*unique_callback)(void* user_data);
static void* unique_user_data;

#define SINGLE_INSTANCE_FACTORY_TYPE (single_instance_factory_get_type ())

typedef struct _SingleInstanceFactory {
	GObject object;
} SingleInstanceFactory;

typedef struct _SingleInstanceFactoryClass {
	GObjectClass object_class;
} SingleInstanceFactoryClass;

static gboolean single_instance_activate (SingleInstanceFactory *factory, GError **error);
					      
#include "single_instance_server.h"

static gboolean single_instance_activate (SingleInstanceFactory *factory, GError **error){
	unique_callback(unique_user_data);
	return TRUE;
}

static void
single_instance_factory_class_init (SingleInstanceFactoryClass *factory_class)
{
	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (factory_class),
					 &dbus_glib_single_instance_object_info);
}

static void
single_instance_factory_init (SingleInstanceFactory *factory)
{
}

G_DEFINE_TYPE(SingleInstanceFactory, single_instance_factory, G_TYPE_OBJECT);


#include "single_instance_client.h"

#define FACTORY_NAME "org.gpick.SingleInstanceFactory"
static int register_factory (void){
	DBusGConnection *connection=NULL;
	
	GError *error = NULL;
	SingleInstanceFactory *factory;
	guint32 request_name_ret;

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
        g_printerr ("Error: %s\n", error->message);
		g_error_free (error);    
		return -1;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);

	if (!org_freedesktop_DBus_request_name (proxy, FACTORY_NAME,
						0, &request_name_ret, &error)) {
        g_printerr ("Error: %s\n", error->message);
		g_error_free (error);    				
		return -1;
	}

	if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		//name already taken, 
		
		proxy = dbus_g_proxy_new_for_name (connection, FACTORY_NAME, "/Factory", 
				"com.google.code.gpick.SingleInstance"); 
		
		if (!com_google_code_gpick_SingleInstance_activate(proxy, &error)){
		    g_printerr ("Error: %s\n", error->message);
			g_error_free (error);    
		}
		
		return 1;
	}

	factory = (SingleInstanceFactory*)g_object_new (SINGLE_INSTANCE_FACTORY_TYPE, NULL);
	dbus_g_connection_register_g_object (connection, "/Factory", G_OBJECT (factory));
	return 0;
}


int unique_term(){
	g_object_unref(proxy);
	return 0;
}

int unique_init(int (*unique_callback)(void* user_data), void* user_data){

	unique_user_data = user_data;
	::unique_callback = unique_callback;

	int r;
	if ((r=register_factory()) == 0){
		return 0;
	}
	
	return 1;
}





