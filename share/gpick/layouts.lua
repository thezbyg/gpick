
gpick.layouts = {};

layouts = {};

layouts.add_buttons = function (container, buttons, percentage, button_style, text_style, spacing)
	local root = layout:new_box('', spacing , spacing, 1-spacing*2, percentage-spacing*2);
	container:add(root);

	local size = 1/buttons;
	local padding = size/8;
	local styles_n = # button_style;
	local styles_text_n = # text_style;

	local texts = {_("Homepage"), _("About us"), _("Links to us"), _("Privacy"), _("Terms"), _("Contact us"), _("RSS")};

	for i=0,buttons-1 do
		local button = layout:new_fill("button", padding, i*size+padding, 1-padding*2, size-padding, button_style[i % styles_n + 1] );
		button:add(layouts.make_helper(layout:new_text("button_text", 0.25, 0.65, 0.5, 0.3, text_style[i % styles_text_n + 1], button_style[i % styles_n + 1]:humanname())));
		button:add(layout:new_text("button_text", 0.25, 0.25, 0.5, 0.5, text_style[i % styles_text_n + 1], texts[i % (# texts) + 1]));
		root:add(button);
	end;
	return container;
end;

layouts.add_text = function (container, lines, style)
	local text = color:new();

	for i=0,lines-1 do
		container:add(layout:new_text("content_text", 0.02, i*0.06+0.01, 0.96, 0.06, style, _("The quick brown fox jumps over the lazy dog")));
	end;
	return container;
end;

layouts.make_helper = function (container)
	container:helper_only(true);
	return container;
end;

layouts.make_locked = function (container)
	container:locked(true);
	return container;
end;

gpick.layouts['std_layout_webpage_1'] = {
	human_readable = _("Webpage"),
	mask = 0,
	build = function (layout_system)

		local styles = {
			header = layout_style:new("header_b:" .. _("Header"), color:new(0.2, 0.2, 0.2)),
			header_text = layout_style:new("header_t:" .. _("Header text"), color:new(1.0, 1.0, 1.0), 1.0),

			content = layout_style:new("content_b:" .. _("Content"), color:new(0.6, 0.6, 0.6)),
			content_text = layout_style:new("content_t:" .. _("Content text"), color:new(0.1, 0.1, 0.1), 0.6),

			sidebar = layout_style:new("sidebar_b:" .. _("Sidebar"), color:new(0.7, 0.7, 0.7)),

			button = layout_style:new("navitem_b:" .. _("Button"), color:new(0.3, 0.3, 0.3)),
			button_hover = layout_style:new("navitem_bh:" .. _("Button (hover)"), color:new(0.35, 0.35, 0.35)),
			button_text = layout_style:new("navitem_t:" .. _("Button text"), color:new(0.8, 0.8, 0.8), 0.95),
			button_text_hover = layout_style:new("navitem_th:" .. _("Button text (hover)"), color:new(0.9, 0.9, 0.9), 0.95),

			footer = layout_style:new("footer_b:" .. _("Footer"), color:new(0.1, 0.1, 0.1)),
		};
		for i,v in pairs(styles) do
			layout_system:addstyle(v);
		end;

		local root = layout:new_box("root", 0, 0, 640, 480);
		layout_system:setbox(root);

		root:add(layout:new_fill("header", 0, 0, 1, 0.15, styles['header']):add(layout:new_text("header_text", 0.25, 0.25, 0.5, 0.5, styles['header_text'], _("Header"))));
		root:add(layouts.add_buttons(layout:new_fill("sidebar", 0, 0.15, 0.25, 0.8, styles['sidebar']), 6, 0.7, { styles['button'], styles['button'], styles['button_hover'] }, { styles['button_text'], styles['button_text'], styles['button_text_hover'] }, 0, 0));
		root:add(layouts.add_text(layout:new_fill("content", 0.25, 0.15, 0.75, 0.8, styles['content']), 10, styles['content_text']));
		root:add(layout:new_fill("footer", 0, 0.95, 1, 0.05, styles['footer']):add(layouts.make_helper(layout:new_text("footer_text", 0.25, 0.5, 0.5, 0.45, nil, styles['footer']:humanname()))));

		return 1;
	end; };

gpick.layouts['std_layout_menu_1'] = {
	human_readable = _("Menu"),
	mask = 0,
	build = function (layout_system)
		local styles = {
			menu = layout_style:new("menu_b:" .. _("Menu"), color:new(0.7, 0.7, 0.7)),
			button = layout_style:new("navitem_b:" .. _("Button"), color:new(0.3, 0.3, 0.3)),
			button_hover = layout_style:new("navitem_bh:" .. _("Button (hover)"), color:new(0.3, 0.3, 0.3)),
			button_text = layout_style:new("navitem_t:" .. _("Button text"), color:new(0.8, 0.8, 0.8), 1.0),
			button_text_hover = layout_style:new("navitem_th:" .. _("Button text (hover)"), color:new(0.8, 0.8, 0.8), 1.0),
		};
		for i,v in pairs(styles) do
			layout_system:addstyle(v);
		end;

		local root = layout:new_box("root", 0, 0, 300, 400);
		layout_system:setbox(root);

		root:add(layouts.add_buttons(layout:new_fill("menu", 0, 0, 1, 1, styles['menu']), 7, 1, { styles['button'], styles['button'], styles['button_hover'] }, { styles['button_text'], styles['button_text'], styles['button_text_hover'] }, 0.1));

		return 1;
	end; };


gpick.layouts['std_layout_brightness_darkness'] = {
	human_readable = _("Brightness-Darkness"),
	mask = 1,
	build = function (layout_system)
		local styles = {
			main = layout_style:new("main:" .. _("main"), color:new(0.7, 0.7, 0.7)),
			bvar1 = layout_style:new("b1:b1", color:new(0.3, 0.3, 0.3)),
			bvar2 = layout_style:new("b2:b2", color:new(0.3, 0.3, 0.3)),
			bvar3 = layout_style:new("b3:b3", color:new(0.3, 0.3, 0.3)),
			bvar4 = layout_style:new("b4:b4", color:new(0.3, 0.3, 0.3)),
			cvar1 = layout_style:new("c1:c1", color:new(0.3, 0.3, 0.3)),
			cvar2 = layout_style:new("c2:c2", color:new(0.3, 0.3, 0.3)),
			cvar3 = layout_style:new("c3:c3", color:new(0.3, 0.3, 0.3)),
			cvar4 = layout_style:new("c4:c4", color:new(0.3, 0.3, 0.3)),
		};
		local styles_order = {'cvar4', 'cvar3',	'cvar2','cvar1', 'main', 'bvar1', 'bvar2', 'bvar3', 'bvar4'};
		for i,v in ipairs(styles_order) do
			layout_system:addstyle(styles[v]);
		end;

		local root = layout:new_box("root", 0, 0, 320, 128);
		layout_system:setbox(root);

		root:add(layouts.make_locked(layout:new_fill("b1", 0.00, 0.00, 0.30, 0.25, styles['bvar1'])));
		root:add(layouts.make_locked(layout:new_fill("b2", 0.00, 0.25, 0.30, 0.25, styles['bvar2'])));
		root:add(layouts.make_locked(layout:new_fill("b3", 0.00, 0.50, 0.30, 0.25, styles['bvar3'])));
		root:add(layouts.make_locked(layout:new_fill("b4", 0.00, 0.75, 0.30, 0.25, styles['bvar4'])));

		root:add(layouts.make_locked(layout:new_fill("c1", 0.70, 0.00, 0.30, 0.25, styles['cvar1'])));
		root:add(layouts.make_locked(layout:new_fill("c2", 0.70, 0.25, 0.30, 0.25, styles['cvar2'])));
		root:add(layouts.make_locked(layout:new_fill("c3", 0.70, 0.50, 0.30, 0.25, styles['cvar3'])));
		root:add(layouts.make_locked(layout:new_fill("c4", 0.70, 0.75, 0.30, 0.25, styles['cvar4'])));

		root:add(layout:new_fill("main", 0.30, 0.00, 0.40, 1.00, styles['main']));

		return 1;
	end; };

gpick.layouts['std_layout_grid_1'] = {
	human_readable = _("Grid (4x3)"),
	mask = 0,
	build = function (layout_system)
		local root = layout:new_box("root", 0, 0, 400, 300);
		layout_system:setbox(root);

		for j=0,2 do
			for i=0,3 do
				local item_i = 1 + (i + j * 4);
				local style = layout_style:new("item" .. item_i .. ":" .. _("Item") .. item_i, color:new(0.8, 0.8, 0.8), 1.0);
				local style_text = layout_style:new("item" .. item_i .. "_text:" .. _("Item text") .. item_i, color:new(0.2, 0.2, 0.2), 0.5);
				layout_system:addstyle(style);
				layout_system:addstyle(style_text);

				local fill = layout:new_fill("b" .. item_i, (1 / 4) * i, (1 / 3) * j, (1 / 4) * 0.95, (1 / 3) * 0.95, style);
				fill:add(layout:new_text("item_text".. item_i, 0, 0.25, 1, 0.5, style_text, _("Item") .. item_i));


				root:add(fill);
			end;
		end;

		return 1;
	end; };


gpick.layouts['std_layout_grid_2'] = {
	human_readable = _("Grid (5x4)"),
	mask = 0,
	build = function (layout_system)
		local root = layout:new_box("root", 0, 0, 500, 400);
		layout_system:setbox(root);

		for j=0,3 do
			for i=0,4 do
				local item_i = 1 + (i + j * 5);
				local style = layout_style:new("item" .. item_i .. ":" .. _("Item") .. item_i, color:new(0.8, 0.8, 0.8), 1.0);
				local style_text = layout_style:new("item" .. item_i .. "_text:" .. _("Item text") .. item_i, color:new(0.2, 0.2, 0.2), 0.5);
				layout_system:addstyle(style);
				layout_system:addstyle(style_text);

				local fill = layout:new_fill("b" .. item_i, (1 / 5) * i, (1 / 4) * j, (1 / 5) * 0.95, (1 / 4) * 0.95, style);
				fill:add(layout:new_text("item_text".. item_i, 0, 0.25, 1, 0.5, style_text, _("Item") .. item_i));


				root:add(fill);
			end;
		end;

		return 1;
	end; };

gpick.layouts_get = function()
	local layouts = {};
	for k,v in pairs(gpick.layouts) do table.insert(layouts, k) end;
	table.sort(layouts);
	return layouts;
end;
