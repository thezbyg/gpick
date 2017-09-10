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

#include "Control.h"
#ifndef WIN32
#include "DbusInterface.h"
#include <iostream>
using namespace std;

namespace dbus
{
	struct Control::Impl
	{
		public:
			Control *m_decl;
			GDBusObjectManagerServer *m_manager;
			guint m_bus_id;
			Impl(Control *decl):
				m_decl(decl),
				m_manager(nullptr),
				m_bus_id(0)
			{
			}
			void ownName()
			{
				m_bus_id = g_bus_own_name(G_BUS_TYPE_SESSION, "org.gpick", GBusNameOwnerFlags(G_BUS_NAME_OWNER_FLAGS_REPLACE), (GBusAcquiredCallback)on_bus_acquired, (GBusNameAcquiredCallback)on_name_acquired, (GBusNameLostCallback)on_name_lost, this, nullptr);
			}
			void unownName()
			{
				g_bus_unown_name(m_bus_id);
				m_bus_id = 0;
			}
			GDBusObjectManager* getManager()
			{
				GDBusObjectManager *manager;
				GError *error = nullptr;
				manager = gpick_object_manager_client_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, "org.gpick", "/org/gpick", nullptr, &error);
				if (manager == nullptr) {
					cerr << "Error getting object manager client: " << error->message << endl;
					g_error_free (error);
					return 0;
				}
				return manager;
			}
			bool activateFloatingPicker(const std::string &converter_name)
			{
				GDBusObjectManager *manager = getManager();
				if (!manager) return false;
				GError *error = nullptr;
				GDBusInterface *interface = g_dbus_object_manager_get_interface(manager, "/org/gpick/Control", "org.gpick.Control");
				bool result = false;
				if (interface){
					if (!gpick_control_call_activate_floating_picker_sync(GPICK_CONTROL(interface), converter_name.c_str(), nullptr, &error)){
						cerr << "Error calling \"Control.ActivateFloatingPicker\": " << error->message << endl;
						g_error_free (error);
					}else result = true;
					g_object_unref(interface);
				}
				g_object_unref(manager);
				return result;
			}
			bool checkIfRunning()
			{
				GDBusObjectManager *manager = getManager();
				if (!manager) return false;
				GError *error = nullptr;
				GDBusInterface *interface = g_dbus_object_manager_get_interface(manager, "/org/gpick/Control", "org.gpick.Control");
				bool result = false;
				if (interface){
					if (!gpick_control_call_check_if_running_sync(GPICK_CONTROL(interface), nullptr, &error)){
						cerr << "Error calling \"Control.CheckIfRunning\": " << error->message << endl;
						g_error_free (error);
					}else result = true;
					g_object_unref(interface);
				}
				g_object_unref(manager);
				return result;
			}
			bool singleInstanceActivate()
			{
				GDBusObjectManager *manager = getManager();
				if (!manager) return false;
				GError *error = nullptr;
				GDBusInterface *interface = g_dbus_object_manager_get_interface(manager, "/org/gpick/SingleInstance", "org.gpick.SingleInstance");
				bool result = false;
				if (interface){
					if (!gpick_single_instance_call_activate_sync(GPICK_SINGLE_INSTANCE(interface), nullptr, &error)){
						cerr << "Error calling \"SingleInstance.Activate\": " << error->message << endl;
						g_error_free (error);
					}else result = true;
					g_object_unref(interface);
				}
				g_object_unref(manager);
				return result;
			}
			static gboolean on_control_activate_floating_picker(GpickControl *control, GDBusMethodInvocation *invocation, const char *converter_name, Impl *impl)
			{
				bool result = impl->m_decl->onActivateFloatingPicker ? impl->m_decl->onActivateFloatingPicker(converter_name) : false;
				gpick_control_complete_activate_floating_picker(control, invocation);
				return result;
			}
			static gboolean on_control_check_if_running(GpickControl *control, GDBusMethodInvocation *invocation, Impl*)
			{
				gpick_control_complete_check_if_running(control, invocation);
				return true;
			}
			static gboolean on_single_instance_activate(GpickSingleInstance *single_instance, GDBusMethodInvocation *invocation, Impl *impl)
			{
				bool result = impl->m_decl->onSingleInstanceActivate();
				gpick_single_instance_complete_activate(single_instance, invocation);
				return result;
			}
			static void on_bus_acquired(GDBusConnection *connection, const gchar *name, Impl *impl)
			{
				auto manager = g_dbus_object_manager_server_new("/org/gpick");
				impl->m_manager = manager;

				GpickObjectSkeleton *object;
				object = gpick_object_skeleton_new("/org/gpick/Control");
				GpickControl *control;
				control = gpick_control_skeleton_new();
				gpick_object_skeleton_set_control(object, control);
				g_object_unref(control);

				g_signal_connect(control, "handle-activate-floating-picker", G_CALLBACK(on_control_activate_floating_picker), impl);
				g_signal_connect(control, "handle-check-if-running", G_CALLBACK(on_control_check_if_running), impl);
				g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
				g_object_unref(object);

				GpickSingleInstance *single_instance;
				object = gpick_object_skeleton_new("/org/gpick/SingleInstance");
				single_instance = gpick_single_instance_skeleton_new();
				gpick_object_skeleton_set_single_instance(object, single_instance);
				g_object_unref(single_instance);

				g_signal_connect(single_instance, "handle-activate", G_CALLBACK(on_single_instance_activate), impl);
				g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
				g_object_unref(object);

				g_dbus_object_manager_server_set_connection(manager, connection);
			}
			static void on_name_acquired(GDBusConnection *connection, const gchar *name, Impl*)
			{
			}
			static void on_name_lost(GDBusConnection *connection, const gchar *name, Impl*)
			{
			};
	};
	Control::Control()
	{
		m_impl = make_unique<Impl>(this);
	}
	Control::~Control()
	{
	}
	void Control::ownName()
	{
		m_impl->ownName();
	}
	void Control::unownName()
	{
		m_impl->unownName();
	}
	bool Control::singleInstanceActivate()
	{
		return m_impl->singleInstanceActivate();
	}
	bool Control::activateFloatingPicker(const std::string &converter_name)
	{
		return m_impl->activateFloatingPicker(converter_name);
	}
	bool Control::checkIfRunning()
	{
		return m_impl->checkIfRunning();
	}
}
#else
namespace dbus
{
	struct Control::Impl
	{
	};
	Control::Control()
	{
	}
	Control::~Control()
	{
	}
	void Control::ownName()
	{
	}
	void Control::unownName()
	{
	}
	bool Control::singleInstanceActivate()
	{
		return false;
	}
	bool Control::activateFloatingPicker(const std::string &converter_name)
	{
		return false;
	}
	bool Control::checkIfRunning()
	{
		return false;
	}
}
#endif
