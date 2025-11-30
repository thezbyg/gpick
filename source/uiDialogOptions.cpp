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

#include "uiDialogOptions.h"
#include "uiUtilities.h"
#include "ColorSpaces.h"
#include "ToolColorNaming.h"
#include "GlobalState.h"
#include "EventBus.h"
#include "I18N.h"
#include "dynv/Map.h"
#include "lua/Script.h"
#include "lua/DynvSystem.h"
#include "lua/Callbacks.h"
#include "lua/Lua.h"
#include <iostream>
#include <string>
#include <string_view>
using namespace std::string_literals;
struct DialogOptionsArgs {
	GtkWidget *minimize_to_tray;
	GtkWidget *close_to_tray;
	GtkWidget *start_in_tray;
	GtkWidget *refresh_rate;
	GtkWidget *single_instance;
	GtkWidget *default_drag_action[2];
	GtkWidget *hex_case[2];
	GtkWidget *save_restore_palette;
	GtkWidget *always_use_floating_picker;
	GtkWidget *hide_cursor;
	GtkWidget *add_on_release;
	GtkWidget *add_to_palette;
	GtkWidget *copy_to_clipboard;
	GtkWidget *rotate_swatch;
	GtkWidget *copy_on_release;
	GtkWidget *zoom_size;
	GtkWidget *imprecision_postfix;
	GtkWidget *tool_color_naming[3];
	struct {
		GtkWidget *colorSpaces[maxColorSpaces];
		GtkWidget *labIlluminant;
		GtkWidget *labObserver;
		GtkWidget *outOfGamutMask;
	} picker, editor;
	GtkWidget *add_to_swatch_on_release;
	GtkWidget *rotate_swatch_on_release;
	GtkWidget *css_percentages;
	GtkWidget *css_alpha_percentage;
	GtkWidget *css_comma_separators;
	dynv::Ref options;
	GlobalState* gs;
};

bool dialog_options_update(GlobalState *gs) {
	if (!gs->callbacks().optionChange().valid())
		return false;
	lua_State* L = gs->script();
	int stack_top = lua_gettop(L);
	gs->callbacks().optionChange().get();
	lua::pushDynvSystem(L, dynv::Ref(&gs->settings()));
	int status = lua_pcall(L, 1, 0, 0);
	if (status == 0){
		lua_settop(L, stack_top);
		return true;
	}else{
		std::cerr << "optionsUpdate: " << lua_tostring(L, -1) << std::endl;
	}
	lua_settop(L, stack_top);
	return false;
}

static void calc( DialogOptionsArgs *args, bool preview, int limit)
{
	if (preview) return;
	auto &options = args->options;
	options->set<bool>("main.minimize_to_tray", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->minimize_to_tray)));
	options->set<bool>("main.close_to_tray", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->close_to_tray)));
	options->set<bool>("main.start_in_tray", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->start_in_tray)));
	options->set<bool>("main.single_instance", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->single_instance)));
	options->set<bool>("main.save_restore_palette", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->save_restore_palette)));
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->default_drag_action[0])))
		options->set("main.dragging_moves", true);
	else
		options->set("main.dragging_moves", false);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->hex_case[0])))
		options->set("options.hex_case", "lower");
	else
		options->set("options.hex_case", "upper");
	options->set<bool>("options.css_percentages", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->css_percentages)));
	options->set<bool>("options.css_alpha_percentage", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->css_alpha_percentage)));
	options->set<bool>("options.css_comma_separators", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->css_comma_separators)));
	options->set<int32_t>("picker.refresh_rate", static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->refresh_rate))));
	options->set<int32_t>("picker.zoom_size", static_cast<int32_t>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(args->zoom_size))));
	options->set<bool>("picker.always_use_floating_picker", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->always_use_floating_picker)));
	options->set<bool>("picker.hide_cursor", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->hide_cursor)));
	options->set<bool>("picker.sampler.add_on_release", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->add_on_release)));
	options->set<bool>("picker.sampler.copy_on_release", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->copy_on_release)));
	options->set<bool>("picker.sampler.add_to_swatch_on_release", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->add_to_swatch_on_release)));
	options->set<bool>("picker.sampler.rotate_swatch_on_release", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->rotate_swatch_on_release)));
	options->set<bool>("picker.sampler.add_to_palette", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->add_to_palette)));
	options->set<bool>("picker.sampler.copy_to_clipboard", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->copy_to_clipboard)));
	options->set<bool>("picker.sampler.rotate_swatch_after_sample", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->rotate_swatch)));
	options->set<bool>("picker.out_of_gamut_mask", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->picker.outOfGamutMask)));
	options->set<bool>("editor.out_of_gamut_mask", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->editor.outOfGamutMask)));
	options->set<bool>("color_names.imprecision_postfix", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->imprecision_postfix)));
	const ToolColorNamingOption *color_naming_options = tool_color_naming_get_options();
	int i = 0;
	while (color_naming_options[i].name){
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->tool_color_naming[i]))){
			options->set("color_names.tool_color_naming", color_naming_options[i].name);
			break;
		}
		i++;
	}
	i = 0;
	for (const auto &colorSpace: colorSpaces()) {
		options->set<bool>("picker.color_space."s + colorSpace.id, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->picker.colorSpaces[i])));
		options->set<bool>("editor.color_space."s + colorSpace.id, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(args->editor.colorSpaces[i])));
		i++;
	}
	options->set("picker.lab.illuminant", gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(args->picker.labIlluminant)));
	options->set("picker.lab.observer", gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(args->picker.labObserver)));
	options->set("editor.lab.illuminant", gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(args->editor.labIlluminant)));
	options->set("editor.lab.observer", gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(args->editor.labObserver)));
}
template<typename Type>
GtkWidget *buildColorSpaceSettings(Type &type, dynv::Map &options, std::string_view optionsPrefix) {
	auto prefix = std::string(optionsPrefix) + ".color_space.";
	GtkWidget *frame = gtk_frame_new(_("Enabled color spaces"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	GtkWidget *table = gtk_table_new(maxColorSpaces, 1, false);
	gtk_container_add(GTK_CONTAINER(frame), table);
	int i = 0;
	for (const auto &colorSpace: colorSpaces()) {
		GtkWidget *widget = type.colorSpaces[i++] = gtk_check_button_new_with_label(colorSpace.name);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), options.getBool(prefix + colorSpace.id, true));
		gtk_table_attach(GTK_TABLE(table), widget, 0, 1, i, i + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 3, 3);
	}
	return frame;
}
template<typename Type>
GtkWidget *buildLabSettings(Type &type, dynv::Map &options, std::string_view optionsPrefix) {
	auto prefix = std::string(optionsPrefix) + ".lab.";
	GtkWidget *frame = gtk_frame_new(_("Lab settings"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	GtkWidget *table = gtk_table_new(2, 2, false);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new(_("_Illuminant:"), 0, 0.5, 0, 0), 0, 1, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 3, 3);
	GtkWidget *widget;
	type.labIlluminant = widget = gtk_combo_box_text_new();
	const char *illuminants[] = {
		"A",
		"C",
		"D50",
		"D55",
		"D65",
		"D75",
		"F2",
		"F7",
		"F11",
		nullptr,
	};
	int selected = 0;
	auto option = options.getString(prefix + "illuminant", "D50");
	for (int i = 0; illuminants[i]; i++) {
		if (illuminants[i] == option)
			selected = i;
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), illuminants[i]);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), selected);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new(_("_Observer:"), 0, 0.5, 0, 0), 0, 1, 1, 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 3, 3);
	type.labObserver = widget = gtk_combo_box_text_new();
	const char *observers[] = {
		"2",
		"10",
		nullptr,
	};
	selected = 0;
	option = options.getString(prefix + "observer", "2");
	for (int i = 0; observers[i]; i++) {
		if (observers[i] == option)
			selected = i;
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), observers[i]);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), selected);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, 1, 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 5, 5);
	return frame;
}
template<typename Type>
GtkWidget *buildOtherSettings(Type &type, dynv::Map &options, std::string_view optionsPrefix) {
	auto prefix = std::string(optionsPrefix) + '.';
	GtkWidget *frame = gtk_frame_new(_("Other settings"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	GtkWidget *table = gtk_table_new(1, 2, false);
	gtk_container_add(GTK_CONTAINER(frame), table);
	GtkWidget *widget = type.outOfGamutMask = gtk_check_button_new_with_mnemonic(_("_Mask out of gamut colors"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), options.getBool(prefix + "out_of_gamut_mask", true));
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GTK_FILL, 3, 3);
	return frame;
}
void dialog_options_show(GtkWindow* parent, GlobalState* gs)
{
	DialogOptionsArgs *args = new DialogOptionsArgs;
	args->gs = gs;
	args->options = args->gs->settings().getOrCreateMap("gpick");
	GtkWidget *table, *table_m, *widget;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Options"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("options.window.width", -1), args->options->getInt32("options.window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	GtkWidget *frame;
	GtkWidget* notebook = gtk_notebook_new();
	gint table_y, table_m_y;
	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new(_("System"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->single_instance = widget = gtk_check_button_new_with_mnemonic (_("_Single instance"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("main.single_instance", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->save_restore_palette = widget = gtk_check_button_new_with_mnemonic (_("Save/_Restore palette"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("main.save_restore_palette", true));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	frame = gtk_frame_new(_("System tray"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->minimize_to_tray = widget = gtk_check_button_new_with_mnemonic (_("_Minimize to system tray"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("main.minimize_to_tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->close_to_tray = widget = gtk_check_button_new_with_mnemonic (_("_Close to system tray"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("main.close_to_tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->start_in_tray = widget = gtk_check_button_new_with_mnemonic (_("_Start in system tray"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("main.start_in_tray", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	frame = gtk_frame_new(_("Default drag action"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	GSList *group = nullptr;
	bool dragging_moves = args->options->getBool("main.dragging_moves", true);
	args->default_drag_action[0] = widget = gtk_radio_button_new_with_mnemonic(group, _("M_ove"));
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(widget));
	if (dragging_moves)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), true);
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->default_drag_action[1] = widget = gtk_radio_button_new_with_mnemonic(group, _("Cop_y"));
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(widget));
	if (dragging_moves == false)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), true);
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	table_m_y = 0;
	frame = gtk_frame_new(_("Hex format"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 1, 2, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(1, 1, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	group = nullptr;
	std::string hexFormat = args->options->getString("options.hex_case", "upper");
	args->hex_case[0] = widget = gtk_radio_button_new_with_mnemonic(group, _("Lower case"));
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(widget));
	if (hexFormat == "lower")
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), true);
	gtk_table_attach(GTK_TABLE(table), widget,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->hex_case[1] = widget = gtk_radio_button_new_with_mnemonic(group, _("Upper case"));
	group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(widget));
	if (hexFormat == "upper")
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), true);
	gtk_table_attach(GTK_TABLE(table), widget,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	frame = gtk_frame_new(_("CSS format"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 1, 2, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(1, 1, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->css_percentages = widget = gtk_check_button_new_with_mnemonic(_("Use percentages"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("options.css_percentages", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->css_alpha_percentage = widget = gtk_check_button_new_with_mnemonic(_("Use percentage for alpha"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("options.css_alpha_percentage", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->css_comma_separators = widget = gtk_check_button_new_with_mnemonic(_("Use comma separators"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("options.css_comma_separators", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic(_("_Main")));
	table_m = gtk_table_new(3, 2, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new(_("Display"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new(_("_Refresh rate:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	args->refresh_rate = widget = gtk_spin_button_new_with_range(1, 60, 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->refresh_rate), args->options->getInt32("picker.refresh_rate", 30));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,5);
	gtk_table_attach(GTK_TABLE(table), gtk_label_aligned_new("Hz",0,0.5,0,0),2,3,table_y,table_y+1,GTK_FILL,GTK_FILL,5,5);
	table_y++;
	gtk_table_attach(GTK_TABLE(table), gtk_label_mnemonic_aligned_new(_("_Magnified area size:"),0,0.5,0,0),0,1,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	args->zoom_size = widget = gtk_spin_button_new_with_range(75, 300, 15);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(args->zoom_size), args->options->getInt32("picker.zoom_size", 150));
	gtk_table_attach(GTK_TABLE(table), widget,1,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,5,5);
	table_y++;

	frame = gtk_frame_new(_("Picker"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->always_use_floating_picker = widget = gtk_check_button_new_with_mnemonic(_("_Always use floating picker"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.always_use_floating_picker", true));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->hide_cursor = widget = gtk_check_button_new_with_mnemonic(_("_Hide cursor"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.hide_cursor", false));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;

	frame = gtk_frame_new(_("Floating picker click behaviour"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->add_on_release = widget = gtk_check_button_new_with_mnemonic(_("_Add to palette"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.add_on_release", true));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->copy_on_release = widget = gtk_check_button_new_with_mnemonic(_("_Copy to clipboard"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.copy_on_release", true));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->add_to_swatch_on_release = widget = gtk_check_button_new_with_mnemonic(_("A_dd to swatch"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.add_to_swatch_on_release", true));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->rotate_swatch_on_release = widget = gtk_check_button_new_with_mnemonic(_("R_otate swatch"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.rotate_swatch_on_release", true));
	gtk_table_attach(GTK_TABLE(table), widget,0,3,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	frame = gtk_frame_new(_("'Spacebar' button behaviour"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->add_to_palette = widget = gtk_check_button_new_with_mnemonic(_("_Add to palette"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.add_to_palette", true));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->copy_to_clipboard = widget = gtk_check_button_new_with_mnemonic(_("_Copy to clipboard"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.copy_to_clipboard", true));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	args->rotate_swatch = widget = gtk_check_button_new_with_mnemonic(_("_Rotate swatch"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("picker.sampler.rotate_swatch_after_sample", true));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	table_m_y = 0;
	frame = buildColorSpaceSettings(args->picker, *args->options, "picker");
	gtk_table_attach(GTK_TABLE(table_m), frame, 1, 2, table_m_y, table_m_y + 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y += 2;
	frame = buildLabSettings(args->picker, *args->options, "picker");
	gtk_table_attach(GTK_TABLE(table_m), frame, 1, 2, table_m_y, table_m_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	frame = buildOtherSettings(args->picker, *args->options, "picker");
	gtk_table_attach(GTK_TABLE(table_m), frame, 1, 2, table_m_y, table_m_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic(_("_Picker")));
	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = buildColorSpaceSettings(args->editor, *args->options, "editor");
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y + 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y += 2;
	frame = buildLabSettings(args->editor, *args->options, "editor");
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	frame = buildOtherSettings(args->editor, *args->options, "editor");
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic(_("_Editor")));
	table_m = gtk_table_new(3, 1, FALSE);
	table_m_y = 0;
	frame = gtk_frame_new(_("Color name generation"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	args->imprecision_postfix = widget = gtk_check_button_new_with_mnemonic(_("_Imprecision postfix"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), args->options->getBool("color_names.imprecision_postfix", false));
	gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
	table_y++;
	frame = gtk_frame_new(_("Tool color naming"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_table_attach(GTK_TABLE(table_m), frame, 0, 1, table_m_y, table_m_y+1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL), 5, 5);
	table_m_y++;
	table = gtk_table_new(5, 3, FALSE);
	table_y=0;
	gtk_container_add(GTK_CONTAINER(frame), table);
	group = nullptr;
	ToolColorNamingType color_naming_type = tool_color_naming_name_to_type(args->options->getString("color_names.tool_color_naming", "automatic_name"));
	const ToolColorNamingOption *color_naming_options = tool_color_naming_get_options();
	int i = 0;
	while (color_naming_options[i].name){
		args->tool_color_naming[i] = widget = gtk_radio_button_new_with_mnemonic(group, _(color_naming_options[i].label));
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(widget));
		if (color_naming_type == color_naming_options[i].type)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), true);
		gtk_table_attach(GTK_TABLE(table), widget,1,2,table_y,table_y+1,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GTK_FILL,3,3);
		table_y++;
		i++;
	}
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table_m, gtk_label_new_with_mnemonic(_("_Color names")));
	gtk_widget_show_all(notebook);
	setDialogContent(dialog, notebook);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		calc(args, false, 0);
		dialog_options_update(args->gs);
		args->gs->eventBus().trigger(EventType::optionsUpdate);
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("options.window.width", width);
	args->options->set("options.window.height", height);
	gtk_widget_destroy(dialog);
	delete args;
}
