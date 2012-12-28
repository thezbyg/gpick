/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#include "DbusInterface.h"

#include <iostream>
using namespace std;

static GDBusObjectManagerServer *manager = NULL;

typedef struct DbusInterface {
	bool (*on_control_activate_floating_picker)(void *userdata);
	bool (*on_single_instance_activate)(void *userdata);
	void *userdata;
}DbusInterface;

static DbusInterface dbus_interface;

static gboolean on_control_activate_floating_picker(GpickControl *control, GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusInterface *dbus_interface = reinterpret_cast<DbusInterface*>(user_data);
	bool result = dbus_interface->on_control_activate_floating_picker(dbus_interface->userdata);
	gpick_control_complete_activate_floating_picker(control, invocation);
	return result;
}

static gboolean on_single_instance_activate(GpickSingleInstance *single_instance, GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusInterface *dbus_interface = reinterpret_cast<DbusInterface*>(user_data);
	bool result = dbus_interface->on_single_instance_activate(dbus_interface->userdata);
	gpick_single_instance_complete_activate(single_instance, invocation);
	return result;
}

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
	GpickObjectSkeleton *object;
	manager = g_dbus_object_manager_server_new ("/gpick");

	GpickControl *control;
	object = gpick_object_skeleton_new("/gpick/Control");
	control = gpick_control_skeleton_new();
	gpick_object_skeleton_set_control(object, control);
	g_object_unref(control);

	g_signal_connect(control, "handle-activate-floating-picker", G_CALLBACK(on_control_activate_floating_picker), user_data);
	g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
	g_object_unref(object);

	GpickSingleInstance *single_instance;
	object = gpick_object_skeleton_new("/gpick/SingleInstance");
	single_instance = gpick_single_instance_skeleton_new();
	gpick_object_skeleton_set_single_instance(object, single_instance);
	g_object_unref(single_instance);

	g_signal_connect(single_instance, "handle-activate", G_CALLBACK(on_single_instance_activate), user_data);
	g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
	g_object_unref(object);

	g_dbus_object_manager_server_set_connection(manager, connection);
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

guint gpick_own_name(bool (*on_control_activate_floating_picker)(void *userdata), bool (*on_single_instance_activate)(void *userdata), void *userdata)
{
	dbus_interface.on_control_activate_floating_picker = on_control_activate_floating_picker;
	dbus_interface.on_single_instance_activate = on_single_instance_activate;
	dbus_interface.userdata = userdata;
	return g_bus_own_name(G_BUS_TYPE_SESSION, "com.google.code.gpick", GBusNameOwnerFlags(G_BUS_NAME_OWNER_FLAGS_REPLACE), on_bus_acquired, on_name_acquired, on_name_lost, &dbus_interface, NULL);
}

void gpick_unown_name(guint bus_id)
{
	g_bus_unown_name(bus_id);
}

GDBusObjectManager* gpick_get_manager()
{
	GDBusObjectManager *manager;
	GError *error = NULL;
	manager = gpick_object_manager_client_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, "com.google.code.gpick", "/gpick", NULL, &error);
	if (manager == NULL) {
		cerr << "Error getting object manager client: " << error->message << endl;
		g_error_free (error);
		return 0;
	}
	return manager;
}

bool gpick_control_activate_floating_picker()
{
	GDBusObjectManager *manager = gpick_get_manager();
	if (!manager) return false;
	GError *error = NULL;
	GDBusInterface *interface = g_dbus_object_manager_get_interface(manager, "/gpick/Control", "com.google.code.gpick.Control");
	bool result = false;
	if (interface){
		if (!gpick_control_call_activate_floating_picker_sync(GPICK_CONTROL(interface), NULL, &error)){
			cerr << "Error calling \"Control.ActivateFloatingPicker\": " << error->message << endl;
			g_error_free (error);
		}else result = true;
		g_object_unref(interface);
	}
	g_object_unref(manager);
	return result;
}

bool gpick_single_instance_activate()
{
	GDBusObjectManager *manager = gpick_get_manager();
	if (!manager) return false;
	GError *error = NULL;
	GDBusInterface *interface = g_dbus_object_manager_get_interface(manager, "/gpick/SingleInstance", "com.google.code.gpick.SingleInstance");
	bool result = false;
	if (interface){
		if (!gpick_single_instance_call_activate_sync(GPICK_SINGLE_INSTANCE(interface), NULL, &error)){
			cerr << "Error calling \"SingleInstance.Activate\": " << error->message << endl;
			g_error_free (error);
		}else result = true;
		g_object_unref(interface);
	}
	g_object_unref(manager);
	return result;
}

