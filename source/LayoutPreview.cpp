/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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
#include "IColorSource.h"
#include "Converters.h"
#include "Converter.h"
#include "dynv/Map.h"
#include "I18N.h"
#include "color_names/ColorNames.h"
#include "GlobalState.h"
#include "ToolColorNaming.h"
#include "uiUtilities.h"
#include "ColorList.h"
#include "gtk/LayoutPreview.h"
#include "layout/Layout.h"
#include "layout/Layouts.h"
#include "layout/Style.h"
#include "StandardEventHandler.h"
#include "StandardDragDropHandler.h"
#include "IDroppableColorUI.h"
#include "EventBus.h"
#include "common/Guard.h"
#include <gdk/gdkkeysyms.h>
#include <sstream>
#include <fstream>

struct LayoutPreviewColorNameAssigner: public ToolColorNameAssigner {
	LayoutPreviewColorNameAssigner(GlobalState &gs):
		ToolColorNameAssigner(gs) {
	}
	void assign(ColorObject &colorObject, std::string_view ident) {
		m_ident = ident;
		ToolColorNameAssigner::assign(colorObject);
	}
	virtual std::string getToolSpecificName(const ColorObject &colorObject) override {
		m_stream.str("");
		m_stream << _("layout preview") << " " << m_ident << " [" << color_names_get(m_gs.getColorNames(), &colorObject.getColor(), false) << "]";
		return m_stream.str();
	}
protected:
	std::stringstream m_stream;
	std::string_view m_ident;
};
struct LayoutPreviewArgs: public IColorSource, public IEventHandler {
	GtkWidget *main, *statusBar, *layoutView;
	common::Ref<layout::System> layoutSystem;
	std::string lastFilename;
	dynv::Ref options;
	GlobalState &gs;
	LayoutPreviewArgs(GlobalState &gs, const dynv::Ref &options):
		options(options),
		gs(gs),
		editable(*this) {
		statusBar = gs.getStatusBar();
		gs.eventBus().subscribe(EventType::displayFiltersUpdate, *this);
	}
	virtual ~LayoutPreviewArgs() {
		saveColors();
		gtk_widget_destroy(main);
		gs.eventBus().unsubscribe(*this);
	}
	virtual std::string_view name() const {
		return "layout_preview";
	}
	virtual void activate() override {
		gtk_statusbar_push(GTK_STATUSBAR(statusBar), gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "empty"), "");
	}
	virtual void deactivate() override {
	}
	virtual GtkWidget *getWidget() override {
		return main;
	}
	void setTransformationChain() {
		auto chain = gs.getTransformationChain();
		gtk_layout_preview_set_transformation_chain(GTK_LAYOUT_PREVIEW(layoutView), chain);
	}
	virtual void onEvent(EventType eventType) override {
		switch (eventType) {
		case EventType::displayFiltersUpdate:
			setTransformationChain();
			break;
		case EventType::optionsUpdate:
		case EventType::convertersUpdate:
		case EventType::colorDictionaryUpdate:
		case EventType::paletteChanged:
			break;
		}
	}
	void addToPalette() {
		if (!isSelected())
			return;
		color_list_add_color_object(gs.getColorList(), getColor(), true);
	}
	void addAllToPalette() {
		if (!layoutSystem)
			return;
		common::Guard colorListGuard(color_list_start_changes(gs.getColorList()), color_list_end_changes, gs.getColorList());
		LayoutPreviewColorNameAssigner nameAssigner(gs);
		for (auto &style: layoutSystem->styles()) {
			ColorObject colorObject(style->color());
			nameAssigner.assign(colorObject, style->label());
			color_list_add_color_object(gs.getColorList(), colorObject, true);
		}
	}
	virtual void setColor(const ColorObject &colorObject) override {
		Color color = colorObject.getColor();
		gtk_layout_preview_set_current_color(GTK_LAYOUT_PREVIEW(layoutView), color);
	}
	virtual void setNthColor(size_t index, const ColorObject &colorObject) override {
	}
	ColorObject colorObject;
	virtual const ColorObject &getColor() override {
		Color color;
		if (gtk_layout_preview_get_current_color(GTK_LAYOUT_PREVIEW(layoutView), color) != 0)
			return colorObject;
		common::Ref<layout::Style> style;
		if (gtk_layout_preview_get_current_style(GTK_LAYOUT_PREVIEW(layoutView), style) != 0)
			return colorObject;
		colorObject.setColor(color);
		LayoutPreviewColorNameAssigner nameAssigner(gs);
		nameAssigner.assign(colorObject, style->label());
		return colorObject;
	}
	virtual const ColorObject &getNthColor(size_t index) override {
		return colorObject;
	}
	std::vector<ColorObject> getColors(bool selected) {
		if (!layoutSystem)
			return std::vector<ColorObject>();
		LayoutPreviewColorNameAssigner nameAssigner(gs);
		std::vector<ColorObject> colors;
		if (selected) {
			auto colorObject = getColor();
			colors.push_back(colorObject);
		} else {
			for (auto &style: layoutSystem->styles()) {
				ColorObject colorObject(style->color());
				nameAssigner.assign(colorObject, style->label());
				colors.push_back(colorObject);
			}
		}
		return colors;
	}
	bool select(int x, int y) {
		gtk_layout_preview_set_focus_at(GTK_LAYOUT_PREVIEW(layoutView), x, y);
		return gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(layoutView));
	}
	bool isSelected() {
		return gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(layoutView));
	}
	bool isEditable() {
		if (!gtk_layout_preview_is_selected(GTK_LAYOUT_PREVIEW(layoutView)))
			return false;
		return gtk_layout_preview_is_editable(GTK_LAYOUT_PREVIEW(layoutView));
	}
	void saveColors() {
		if (!layoutSystem)
			return;
		auto assignments = options->getOrCreateMap("css_selectors.assignments");
		for (auto style: layoutSystem->styles())
			assignments->set(style->name() + ".color", style->color());
	}
	struct Editable: IEditableColorsUI, IDroppableColorUI {
		Editable(LayoutPreviewArgs &args):
			args(args) {
		}
		virtual ~Editable() = default;
		virtual void addToPalette(const ColorObject &) override {
			args.addToPalette();
		}
		virtual void addAllToPalette() override {
			args.addAllToPalette();
		}
		virtual void setColor(const ColorObject &colorObject) override {
			args.setColor(colorObject.getColor());
		}
		virtual void setColorAt(const ColorObject &colorObject, int x, int y) override {
			args.select(x, y);
			args.setColor(colorObject);
		}
		virtual void setColors(const std::vector<ColorObject> &colorObjects) override {
			args.setColor(colorObjects[0]);
		}
		virtual const ColorObject &getColor() override {
			return args.getColor();
		}
		virtual std::vector<ColorObject> getColors(bool selected) override {
			return args.getColors(selected);
		}
		virtual bool isEditable() override {
			return args.isEditable();
		}
		virtual bool hasColor() override {
			return true;
		}
		virtual bool hasSelectedColor() override {
			return args.isSelected();
		}
		virtual bool testDropAt(int x, int y) override {
			return args.select(x, y);
		}
		virtual void dropEnd(bool move) override {
		}
	private:
		LayoutPreviewArgs &args;
	};
	std::optional<Editable> editable;
};
enum LayoutListColumns {
	LAYOUTLIST_LABEL = 0,
	LAYOUTLIST_PTR,
	LAYOUTLIST_N_COLUMNS
};
enum StyleListColumns {
	STYLELIST_LABEL = 0,
	STYLELIST_CSS_SELECTOR,
	STYLELIST_PTR,
	STYLELIST_N_COLUMNS
};
static void onSelectorEdit(GtkCellRendererText *cell, gchar *path, gchar *new_text, GtkListStore *store) {
	GtkTreeIter iter1;
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter1, path);
	gtk_list_store_set(store, &iter1, STYLELIST_CSS_SELECTOR, new_text, -1);
}
static void loadColors(LayoutPreviewArgs *args) {
	if (!args->layoutSystem)
		return;
	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layoutSystem->styles())
		style->setColor(assignments->getColor(style->name() + ".color", style->color()));
}
static GtkWidget *newSelectorList(LayoutPreviewArgs *args) {
	auto view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), true);
	auto store = gtk_list_store_new(STYLELIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	auto col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, true);
	gtk_tree_view_column_set_title(col, _("Style item"));
	auto renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "text", STYLELIST_LABEL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_resizable(col, true);
	gtk_tree_view_column_set_title(col, _("CSS selector"));
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, true);
	gtk_tree_view_column_add_attribute(col, renderer, "text", STYLELIST_CSS_SELECTOR);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	g_object_set(renderer, "editable", true, nullptr);
	g_signal_connect(renderer, "edited", (GCallback)onSelectorEdit, store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
	g_object_unref(GTK_TREE_MODEL(store));
	return view;
}
static void onAssignSelectors(GtkWidget *widget, LayoutPreviewArgs *args) {
	if (!args->layoutSystem)
		return;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Assign CSS selectors"), GTK_WINDOW(gtk_widget_get_toplevel(args->main)), GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, nullptr);
	gtk_window_set_default_size(GTK_WINDOW(dialog), args->options->getInt32("css_selectors.window.width", -1), args->options->getInt32("css_selectors.window.height", -1));
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	auto table = gtk_table_new(1, 1, false);
	int table_y = 0;
	auto selectorList = newSelectorList(args);
	gtk_widget_set_size_request(selectorList, 100, 100);
	gtk_table_attach(GTK_TABLE(table), selectorList, 0, 1, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 5, 0);
	table_y++;
	GtkTreeIter iter1;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(selectorList));
	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layoutSystem->styles()) {
		auto css_selector = assignments->getString(style->name() + ".selector", style->name());
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter1,
			STYLELIST_LABEL, style->label().c_str(),
			STYLELIST_CSS_SELECTOR, css_selector.c_str(),
			STYLELIST_PTR, style,
			-1);
	}
	gtk_widget_show_all(table);
	setDialogContent(dialog, table);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		auto valid = gtk_tree_model_get_iter_first(model, &iter1);
		while (valid) {
			layout::Style *style;
			char *selector;
			gtk_tree_model_get(model, &iter1, STYLELIST_PTR, &style, STYLELIST_CSS_SELECTOR, &selector, -1);
			assignments->set(style->name() + ".selector", selector);
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
static GtkWidget *newLayoutDropdown() {
	auto store = gtk_list_store_new(LAYOUTLIST_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
	auto combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	auto renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, true);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", LAYOUTLIST_LABEL, nullptr);
	if (store)
		g_object_unref(store);
	return combo;
}
static void onLayoutChange(GtkWidget *widget, LayoutPreviewArgs *args) {
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)) {
		GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
		layout::Layout *layout;
		gtk_tree_model_get(model, &iter, LAYOUTLIST_PTR, &layout, -1);
		args->saveColors();
		args->layoutSystem = layout->build();
		gtk_layout_preview_set_system(GTK_LAYOUT_PREVIEW(args->layoutView), args->layoutSystem);
		args->options->set("layout_name", layout->name());
		loadColors(args);
	}
}
static int saveCssFile(const char *filename, LayoutPreviewArgs *args) {
	if (!args->layoutSystem)
		return -1;
	std::ofstream file(filename, std::ios::out);
	if (!file.is_open())
		return -1;
	auto converter = args->gs.converters().firstCopy();
	auto assignments = args->options->getOrCreateMap("css_selectors.assignments");
	for (auto style: args->layoutSystem->styles()) {
		auto cssSelector = assignments->getString(style->name() + ".selector", style->name());
		if (cssSelector.empty())
			continue;
		ColorObject colorObject(style->color());
		std::string text = converter->serialize(colorObject);
		file << cssSelector << " {\n";
		if (style->type() == layout::Style::Type::background) {
			file << "\tbackground-color: " << text << ";\n";
		} else if (style->type() == layout::Style::Type::color) {
			file << "\tcolor: " << text << ";\n";
		} else if (style->type() == layout::Style::Type::border) {
			file << "\tborder-color: " << text << ";\n";
		}
		file << "}\n\n";
	}
	file.close();
	return 0;
}
static void onExportCss(GtkWidget *widget, LayoutPreviewArgs *args) {
	if (!args->lastFilename.empty()) {
		if (saveCssFile(args->lastFilename.c_str(), args) == 0) {
		} else {
			GtkWidget *message = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
			gtk_dialog_run(GTK_DIALOG(message));
			gtk_widget_destroy(message);
		}
		return;
	}
	auto dialog = gtk_file_chooser_dialog_new(_("Export"), GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_OK, nullptr);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	auto default_path = args->options->getString("export_path", "");
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), default_path.c_str());
	auto filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Cascading Style Sheets *.css"));
	gtk_file_filter_add_pattern(filter, "*.css");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	gboolean finished = false;
	while (!finished) {
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
			gchar *filename;
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			gchar *path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
			args->options->set("export_path", path);
			g_free(path);
			if (saveCssFile(filename, args) == 0) {
				args->lastFilename = filename;
				finished = true;
			} else {
				GtkWidget *message;
				message = gtk_message_dialog_new(GTK_WINDOW(dialog), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("File could not be saved"));
				//gtk_window_set_title(GTK_WINDOW(dialog), "Open");
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
			g_free(filename);
		} else
			break;
	}
	gtk_widget_destroy(dialog);
}
static void onExportCssAs(GtkWidget *widget, LayoutPreviewArgs *args) {
	args->lastFilename = "";
	onExportCss(widget, args);
}
static GtkWidget *withLabel(GtkWidget *widget, const char *label) {
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_aligned_new(label, 0, 0.5, 0, 0), false, true, 0);
	gtk_box_pack_start(GTK_BOX(hbox), widget, true, true, 0);
	return hbox;
}
static gboolean onButtonPress(GtkWidget *widget, GdkEventButton *event, LayoutPreviewArgs *args) {
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		args->addToPalette();
		return true;
	}
	return false;
}
static std::unique_ptr<IColorSource> build(GlobalState &gs, const dynv::Ref &options) {
	auto args = std::make_unique<LayoutPreviewArgs>(gs, options);
	GtkWidget *table, *vbox, *hbox;
	vbox = gtk_vbox_new(false, 10);
	hbox = gtk_hbox_new(false, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, true, true, 5);
	gint table_y;
	table = gtk_table_new(4, 4, false);
	gtk_box_pack_start(GTK_BOX(hbox), table, true, true, 5);
	table_y = 0;
	auto toolbar = gtk_toolbar_new();
	gtk_table_attach(GTK_TABLE(table), toolbar, 0, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL), GTK_FILL, 0, 0);
	table_y++;
	auto tool = gtk_tool_item_new();
	gtk_tool_item_set_expand(tool, true);
	GtkWidget *layoutDropdown = newLayoutDropdown();
	gtk_container_add(GTK_CONTAINER(tool), withLabel(layoutDropdown, _("Layout:")));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool, -1);
	g_signal_connect(G_OBJECT(layoutDropdown), "changed", G_CALLBACK(onLayoutChange), args.get());
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	tool = gtk_menu_tool_button_new(newIcon(GTK_STOCK_SAVE, IconSize::toolbar), _("Export CSS File"));
	gtk_tool_item_set_tooltip_text(tool, _("Export CSS file"));
	g_signal_connect(G_OBJECT(tool), "clicked", G_CALLBACK(onExportCss), args.get());
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool, -1);
	auto menu = gtk_menu_new();
	auto item = newMenuItem(_("_Export CSS File As..."), GTK_STOCK_SAVE_AS);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(onExportCssAs), args.get());
	item = gtk_menu_item_new_with_mnemonic(_("_Assign CSS Selectors..."));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(onAssignSelectors), args.get());
	gtk_widget_show_all(menu);
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(tool), menu);
	GtkWidget *scrolled = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_table_attach(GTK_TABLE(table), scrolled, 0, 3, table_y, table_y + 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), GtkAttachOptions(GTK_FILL | GTK_EXPAND), 0, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), args->layoutView = gtk_layout_preview_new());
	g_signal_connect_after(G_OBJECT(args->layoutView), "button-press-event", G_CALLBACK(onButtonPress), args.get());
	StandardEventHandler::forWidget(args->layoutView, &args->gs, &*args->editable);
	StandardDragDropHandler::forWidget(args->layoutView, &args->gs, &*args->editable);
	table_y++;
	// Restore settings and fill list
	auto layoutName = args->options->getString("layout_name", "std_layout_menu_1");
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(layoutDropdown));
	GtkTreeIter iter1;
	bool layoutFound = false;
	for (auto &layout: gs.layouts().all()) {
		if (layout->mask() != 0)
			continue;
		gtk_list_store_append(GTK_LIST_STORE(model), &iter1);
		gtk_list_store_set(GTK_LIST_STORE(model), &iter1,
			LAYOUTLIST_LABEL, layout->label().c_str(),
			LAYOUTLIST_PTR, layout,
			-1);
		if (layout->name() == layoutName) {
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(layoutDropdown), &iter1);
			layoutFound = true;
		}
	}
	if (!layoutFound) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(layoutDropdown), 0);
	}
	gtk_widget_show_all(vbox);
	args->main = vbox;
	args->setTransformationChain();
	return args;
}
void registerLayoutPreview(ColorSourceManager &csm) {
	csm.add("layout_preview", _("Layout preview"), RegistrationFlags::none, GDK_KEY_l, build);
}
