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

#include "LayoutPreview.h"
#include "ColorSourceManager.h"
#include "ColorSource.h"
#include "DragDrop.h"
#include "uiColorInput.h"
#include "Clipboard.h"
#include "StandardMenu.h"
#include "Converters.h"
#include "Converter.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "MathUtil.h"
#include <gdk/gdkkeysyms.h>
#include "gtk/LayoutPreview.h"
#include "layout/Layout.h"
#include "layout/Layouts.h"
#include "layout/Style.h"
#include <math.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <list>
using namespace std;
using namespace layout;

typedef struct LayoutPreviewArgs{
	ColorSource source;
	GtkWidget *main;
	GtkWidget *statusbar;
	GtkWidget *layout;
	System *layout_system;
	string last_filename;
	dynv::Ref options;
	GlobalState *gs;
}LayoutPreviewArgs;

struct LayoutPreviewColorNameAssigner: public ToolColorNameAssigner
{
	protected:
		stringstream m_stream;
		const char *m_ident;
	public:
		LayoutPreviewColorNameAssigner(GlobalState *gs):ToolColorNameAssigner(gs){
		}
		void assign(ColorObject *color_object, const Color *color, const char *ident){
			m_ident = ident;
			ToolColorNameAssigner::assign(color_object, color);
		}
		virtual std::string getToolSpecificName(ColorObject *color_object, const Color *color){
			m_stream.str("");
			m_stream << _("layout preview") << " " << m_ident << " [" << color_names_get(m_gs->getColorNames(), color, false) << "]";
			return m_stream.str();
		}
};

typedef enum{
	LAYOUTLIST_LABEL = 0,
	LAYOUTLIST_PTR,
	LAYOUTLIST_N_COLUMNS
}LayoutListColumns;

typedef enum{
	STYLELIST_LABEL = 0,
	STYLELIST_CSS_SELECTOR,
	STYLELIST_PTR,
	STYLELIST_N_COLUMNS
}StyleListColumns;

static void style_cell_edited_cb(GtkCellRendererText *cell, gchar *path, gchar *new_text, GtkListStore *store)
{
	GtkTreeIter iter1;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path );
	gtk_list_store_set(store, &iter1, STYLELIST_CSS_SELECTOR, new_text, -1);
}
static void load_colors(LayoutPreviewArgs* args)
{
	if (args->layout_system == nullptr)
		return;
	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layout_system->styles)
		style->color = assignments->getColor(style->ident_name + ".color", style->color);
}
static void save_colors(LayoutPreviewArgs* args)
{
	if (args->layout_system == nullptr)
		return;
	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layout_system->styles)
		assignments->set(style->ident_name + ".color", style->color);
}

static GtkWidget* style_list_new(LayoutPreviewArgs *args)
{
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	GtkWidget *view;
	view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), 1);
	store = gtk_list_store_new(STYLELIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, true);
	gtk_tree_view_column_set_title(col, _("Style item"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "text", STYLELIST_LABEL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, true);
	gtk_tree_view_column_set_title(col, _("CSS selector"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "text", STYLELIST_CSS_SELECTOR);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_object_set(renderer, "editable", true, nullptr);
	g_signal_connect(renderer, "edited", (GCallback)style_cell_edited_cb, store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
	g_object_unref(GTK_TREE_MODEL(store));
	return view;
}
static void assign_css_selectors_cb(GtkWidget *widget, LayoutPreviewArgs* args)
{
	if (args->layout_system == nullptr)
		return;
	GtkWidget *table;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Assign CSS selectors"), GTK_WINDOW(gtk_widget_get_toplevel(args->main)), GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		nullptr);

	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("css_selectors.window.width", -1),
		args->options->getInt32("css_selectors.window.height", -1));

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gint table_y;
	table = gtk_table_new(1, 1, FALSE);
	table_y=0;

	GtkWidget* list_widget = style_list_new(args);
	gtk_widget_set_size_request(list_widget, 100, 100);
	gtk_table_attach(GTK_TABLE(table), list_widget, 0, 1, table_y, table_y+1, GtkAttachOptions(GTK_FILL|GTK_EXPAND), GtkAttachOptions(GTK_FILL|GTK_EXPAND), 5, 0);
	table_y++;

	GtkTreeIter iter1;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_widget));

	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layout_system->styles) {
		auto css_selector = assignments->getString(style->ident_name + ".selector", style->ident_name);
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter1,
			STYLELIST_LABEL, style->label.c_str(),
			STYLELIST_CSS_SELECTOR, css_selector.c_str(),
			STYLELIST_PTR, style,
		-1);
	}

	gtk_widget_show_all(table);
	setDialogContent(dialog, table);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){

		gboolean valid;
		Style *style;
		char* selector;

		valid = gtk_tree_model_get_iter_first(model, &iter1);
		while (valid){
			gtk_tree_model_get(model, &iter1, STYLELIST_PTR, &style, STYLELIST_CSS_SELECTOR, &selector, -1);
			assignments->set(style->ident_name + ".selector", selector);
			g_free(selector);
			valid = gtk_tree_model_iter_next(model, &iter1);
		}
	}
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(dialog), &width, &height);
	args->options->set("css_selectors.window.width", width);
	args->options->set("css_selectors.window.height", height);
	gtk_widget_destroy(dialog);
}

static int source_destroy(LayoutPreviewArgs *args)
{
	save_colors(args);
	if (args->layout_system) System::unref(args->layout_system);
	args->layout_system = nullptr;
	gtk_widget_destroy(args->main);
	delete args;
	return 0;
}

static int source_get_color(LayoutPreviewArgs *args, ColorObject** color)
{
	Style *style = 0;
	if (gtk_layout_preview_get_current_style(GTK_LAYOUT_PREVIEW(args->layout), &style) == 0){
		ColorObject *color_object = color_list_new_color_object(args->gs->getColorList(), &style->color);
		LayoutPreviewColorNameAssigner name_assigner(args->gs);
		name_assigner.assign(color_object, &style->color, style->ident_name.c_str());
		*color = color_object;
		return 0;
	}
	return -1;
}
static int source_set_color(LayoutPreviewArgs *args, ColorObject* color_object)
{
	Color color = color_object->getColor();
	gtk_layout_preview_set_current_color(GTK_LAYOUT_PREVIEW(args->layout), &color);
	return -1;
}
static int source_deactivate(LayoutPreviewArgs *args)
{
	return 0;
}
static ColorObject* get_color_object(DragDrop* dd)
{
	LayoutPreviewArgs *args = (LayoutPreviewArgs*)dd->userdata;
	ColorObject *color_object;
	if (source_get_color(args, &color_object) == 0){
		return color_object;
	}
	return 0;
}

static int set_color_object_at(struct DragDrop* dd, ColorObject* color_object, int x, int y, bool, bool)
{
	LayoutPreviewArgs* args=(LayoutPreviewArgs*)dd->userdata;
	Color color = color_object->getColor();
	gtk_layout_preview_set_color_at(GTK_LAYOUT_PREVIEW(args->layout), &color, x, y);
	return 0;
}

static bool test_at(struct DragDrop* dd, int x, int y)
{
	LayoutPreviewArgs* args=(LayoutPreviewArgs*)dd->userdata;
	gtk_layout_preview_set_focus_at(GTK_LAYOUT_PREVIEW(args->layout), x, y);
	return gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(args->layout));
}

static GtkWidget* layout_preview_dropdown_new(LayoutPreviewArgs *args, GtkTreeModel *model)
{
	GtkListStore *store = 0;
	GtkCellRenderer *renderer;
	GtkWidget *combo;
	if (model){
		combo = gtk_combo_box_new_with_model(model);
	}else{
		store = gtk_list_store_new (LAYOUTLIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
		combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	}
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, true);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer, "text", LAYOUTLIST_LABEL, nullptr);
	if (store) g_object_unref (store);
	return combo;
}

static void edit_cb(GtkWidget *widget, gpointer item)
{
	LayoutPreviewArgs* args=(LayoutPreviewArgs*)item;
	ColorObject *color_object;
	ColorObject* new_color_object = 0;
	if (source_get_color(args, &color_object) == 0){
		if (dialog_color_input_show(GTK_WINDOW(gtk_widget_get_toplevel(args->main)), args->gs, color_object, &new_color_object ) == 0){
			source_set_color(args, new_color_object);
			new_color_object->release();
		}
		color_object->release();
	}
}

static void paste_cb(GtkWidget *, LayoutPreviewArgs *args) {
	auto colorObject = clipboard::getFirst(args->gs);
	if (colorObject) {
		source_set_color(args, colorObject);
		colorObject->release();
	}
}

static void add_color_to_palette(Style *style, LayoutPreviewColorNameAssigner &name_assigner, LayoutPreviewArgs *args)
{
	ColorObject *color_object;
	color_object = color_list_new_color_object(args->gs->getColorList(), &style->color);
	name_assigner.assign(color_object, &style->color, style->ident_name.c_str());
	color_list_add_color_object(args->gs->getColorList(), color_object, 1);
	color_object->release();
}

static void add_to_palette_cb(GtkWidget *widget, gpointer item)
{
	LayoutPreviewArgs* args = (LayoutPreviewArgs*)item;
	LayoutPreviewColorNameAssigner name_assigner(args->gs);
	Style* style = 0;
	if (gtk_layout_preview_get_current_style(GTK_LAYOUT_PREVIEW(args->layout), &style) == 0){
		add_color_to_palette(style, name_assigner, args);
	}
}

static void add_all_to_palette_cb(GtkWidget *widget, LayoutPreviewArgs *args)
{
	if (args->layout_system == nullptr)
		return;
	LayoutPreviewColorNameAssigner name_assigner(args->gs);
	for (list<Style*>::iterator i = args->layout_system->styles.begin(); i != args->layout_system->styles.end(); i++){
		add_color_to_palette(*i, name_assigner, args);
	}
}

static gboolean button_press_cb (GtkWidget *widget, GdkEventButton *event, LayoutPreviewArgs* args)
{
	GtkWidget *menu;
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS){
		add_to_palette_cb(widget, args);
		return true;
	}else if (event->button == 3 && event->type == GDK_BUTTON_PRESS){
		GtkWidget* item ;
		gint32 button, event_time;
		menu = gtk_menu_new ();
		bool selection_avail = gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(args->layout));

		item = newMenuItem(_("_Add to palette"), GTK_STOCK_ADD);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_to_palette_cb), args);
		if (!selection_avail) gtk_widget_set_sensitive(item, false);

		item = newMenuItem(_("A_dd all to palette"), GTK_STOCK_ADD);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (add_all_to_palette_cb), args);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		if (selection_avail){
			ColorObject *color_object;
			source_get_color(args, &color_object);
			StandardMenu::appendMenu(menu, *color_object, args->gs);
			color_object->release();
		}else{
			StandardMenu::appendMenu(menu);
		}
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		item = newMenuItem(_("_Edit..."), GTK_STOCK_EDIT);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (edit_cb), args);
		if (!selection_avail) gtk_widget_set_sensitive(item, false);

		item = newMenuItem(_("_Paste"), GTK_STOCK_PASTE);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (paste_cb), args);
		if (!selection_avail) gtk_widget_set_sensitive(item, false);
		gtk_widget_set_sensitive(item, clipboard::colorObjectAvailable());

		gtk_widget_show_all (GTK_WIDGET(menu));
		button = event->button;
		event_time = event->time;
		gtk_menu_popup(GTK_MENU(menu), nullptr, nullptr, nullptr, nullptr, button, event_time);
		g_object_ref_sink(menu);
		g_object_unref(menu);
		return TRUE;
	}
	return FALSE;
}
static void layout_changed_cb(GtkWidget *widget, LayoutPreviewArgs* args)
{
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
		GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		Layout* layout;
		gtk_tree_model_get(model, &iter, LAYOUTLIST_PTR, &layout, -1);
		save_colors(args);
		System* layout_system = layout->build();
		gtk_layout_preview_set_system(GTK_LAYOUT_PREVIEW(args->layout), layout_system);
		if (args->layout_system) System::unref(args->layout_system);
		args->layout_system = layout_system;
		args->options->set("layout_name", layout->name());
		load_colors(args);
	}
}
static int save_css_file(const char* filename, LayoutPreviewArgs* args)
{
	if (args->layout_system == nullptr)
		return -1;
	ofstream file(filename, ios::out);
	if (!file.is_open())
		return -1;
	auto converter = args->gs->converters().firstCopy();
	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layout_system->styles) {
		auto cssSelector = assignments->getString(style->ident_name + ".selector", style->ident_name);
		if (cssSelector.empty())
			continue;
		ColorObject colorObject(style->color);
		string text = converter->serialize(colorObject);
		file << cssSelector << " {" << endl;
		if (style->style_type == Style::TYPE_BACKGROUND){
			file << "\tbackground-color: " << text << ";" << endl;
		}else if (style->style_type == Style::TYPE_COLOR){
			file << "\tcolor: " << text << ";" << endl;
		}else if (style->style_type == Style::TYPE_BORDER){
			file << "\tborder-color: " << text << ";" << endl;
		}
		file << "}" << endl << endl;
	}
	file.close();
	return 0;
}

static void export_css_cb(GtkWidget *widget, LayoutPreviewArgs* args){

	if (!args->last_filename.empty()){

		if (save_css_file(args->last_filename.c_str(), args) == 0){

		}else{
			GtkWidget* message;
			message=gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
			gtk_dialog_run(GTK_DIALOG(message));
			gtk_widget_destroy(message);
		}

		return;
	}

	GtkWidget *dialog;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new(_("Export"), GTK_WINDOW(gtk_widget_get_toplevel(widget)),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_OK,
		nullptr);

	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	auto default_path = args->options->getString("export_path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path.c_str());

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Cascading Style Sheets *.css"));
	gtk_file_filter_add_pattern(filter, "*.css");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	gboolean finished = false;

	while (!finished){
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK){
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			args->options->set("export_path", path);
			g_free(path);

			if (save_css_file(filename, args) == 0){

				args->last_filename = filename;
				finished = true;
			}else{
				GtkWidget* message;
				message=gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
				//gtk_window_set_title(GTK_WINDOW(dialog), "Open");
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}

			g_free(filename);
		}else break;
	}

	gtk_widget_destroy (dialog);
}

static void export_css_as_cb(GtkWidget *widget, LayoutPreviewArgs* args){
	args->last_filename="";
	export_css_cb(widget, args);
}

static GtkWidget* attach_label(GtkWidget *widget, const char *label){
	GtkWidget* hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_aligned_new(label, 0, 0.5, 0, 0), false, true, 0);
	gtk_box_pack_start(GTK_BOX(hbox), widget, true, true, 0);
	return hbox;
}

static int source_activate(LayoutPreviewArgs *args)
{
	auto chain = args->gs->getTransformationChain();
	gtk_layout_preview_set_transformation_chain(GTK_LAYOUT_PREVIEW(args->layout), chain);
	gtk_statusbar_push(GTK_STATUSBAR(args->statusbar), gtk_statusbar_get_context_id(GTK_STATUSBAR(args->statusbar), "empty"), "");
	return 0;
}
static ColorSource* source_implement(ColorSource *source, GlobalState* gs, const dynv::Ref &options)
{
	LayoutPreviewArgs* args = new LayoutPreviewArgs;
	args->options = options;
	args->statusbar = gs->getStatusBar();
	args->layout_system = nullptr;
	color_source_init(&args->source, source->identificator, source->hr_name);
	args->source.destroy = (int (*)(ColorSource *source))source_destroy;
	args->source.get_color = (int (*)(ColorSource *source, ColorObject** color))source_get_color;
	args->source.set_color = (int (*)(ColorSource *source, ColorObject* color))source_set_color;
	args->source.deactivate = (int (*)(ColorSource *source))source_deactivate;
	args->source.activate = (int (*)(ColorSource *source))source_activate;
	GtkWidget *table, *vbox, *hbox;
	vbox = gtk_vbox_new(false, 10);
	hbox = gtk_hbox_new(false, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, true, true, 5);
	gint table_y;
	table = gtk_table_new(4, 4, false);
	gtk_box_pack_start(GTK_BOX(hbox), table, true, true, 5);
	table_y = 0;
	GtkToolItem *tool;
	GtkWidget *toolbar = gtk_toolbar_new();
	gtk_table_attach(GTK_TABLE(table), toolbar, 0, 3, table_y, table_y+1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	table_y++;
	tool = gtk_tool_item_new();
	gtk_tool_item_set_expand(tool, true);
	GtkWidget *layout_dropdown = layout_preview_dropdown_new(args, 0);
	gtk_container_add(GTK_CONTAINER(tool), attach_label(layout_dropdown, _("Layout:")));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool, -1);
	g_signal_connect (G_OBJECT(layout_dropdown), "changed", G_CALLBACK(layout_changed_cb), args);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	tool = gtk_menu_tool_button_new(newIcon(GTK_STOCK_SAVE, IconSize::toolbar), _("Export CSS File"));
	gtk_tool_item_set_tooltip_text(tool, _("Export CSS file"));
	g_signal_connect(G_OBJECT(tool), "clicked", G_CALLBACK(export_css_cb), args);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool, -1);
	GtkWidget *menu;
	GtkWidget* item;
	menu = gtk_menu_new();
	item = newMenuItem(_("_Export CSS File As..."), GTK_STOCK_SAVE_AS);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (export_css_as_cb), args);
	item = gtk_menu_item_new_with_mnemonic(_("_Assign CSS Selectors..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (assign_css_selectors_cb), args);
	gtk_widget_show_all(menu);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool), menu);
	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_table_attach(GTK_TABLE(table), scrolled, 0, 3, table_y, table_y+1 ,GtkAttachOptions(GTK_FILL | GTK_EXPAND),GtkAttachOptions(GTK_FILL | GTK_EXPAND),0,0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), args->layout = gtk_layout_preview_new());
	g_signal_connect_after(G_OBJECT(args->layout), "button-press-event", G_CALLBACK(button_press_cb), args);
	table_y++;
	struct DragDrop dd;
	dragdrop_init(&dd, gs);
	dd.converterType = Converters::Type::display;
	dd.userdata = args;
	dd.get_color_object = get_color_object;
	dd.set_color_object_at = set_color_object_at;
	dd.test_at = test_at;
	gtk_drag_dest_set(args->layout, GtkDestDefaults(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT), 0, 0, GDK_ACTION_COPY);
	gtk_drag_source_set(args->layout, GDK_BUTTON1_MASK, 0, 0, GDK_ACTION_COPY);
	dragdrop_widget_attach(args->layout, DragDropFlags(DRAGDROP_SOURCE | DRAGDROP_DESTINATION), &dd);
	args->gs = gs;
	// Restore settings and fill list
	auto layout_name = args->options->getString("layout_name", "std_layout_menu_1");
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(layout_dropdown));
	GtkTreeIter iter1;
	bool layout_found = false;
	for (auto &layout: gs->layouts().all()){
		if (layout->mask() != 0)
			continue;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter1,
			LAYOUTLIST_LABEL, layout->label().c_str(),
			LAYOUTLIST_PTR, layout,
		-1);
		if (layout->name() == layout_name){
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(layout_dropdown), &iter1);
			layout_found = true;
		}
	}
	if (!layout_found){
		gtk_combo_box_set_active(GTK_COMBO_BOX(layout_dropdown), 0);
	}
	gtk_widget_show_all(vbox);
	args->main = vbox;
	args->source.widget =vbox;
	return (ColorSource*)args;
}
int layout_preview_source_register(ColorSourceManager *csm)
{
	ColorSource *color_source = new ColorSource;
	color_source_init(color_source, "layout_preview", _("Layout preview"));
	color_source->needs_viewport = false;
	color_source->implement = (ColorSource* (*)(ColorSource *source, GlobalState *gs, const dynv::Ref &options))source_implement;
	color_source->default_accelerator = GDK_KEY_l;
	color_source_manager_add_source(csm, color_source);
	return 0;
}
