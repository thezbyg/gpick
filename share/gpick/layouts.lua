
gpick.layouts = {};

layouts = {};

layouts.add_buttons = function (container, buttons, percentage, button_style, text_style, spacing)
	local root = layout:new_box('', spacing , spacing, 1-spacing*2, percentage-spacing*2);
	container:add(root);

	local size = 1/buttons;
	local padding = size/8;
	local styles_n = # button_style;
	local styles_text_n = # text_style;
	
	local texts = {'Homepage', 'About us', 'Links to us', 'Privacy', 'Terms', 'Contact us', 'RSS'};

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
		container:add(layout:new_text("content_text", 0.02, i*0.06+0.01, 0.96, 0.06, style, 'The quick brown fox jumps over the lazy dog'));
	end;
	return container;
end;

layouts.make_helper = function (container)
	container:helper_only(true);
	return container;
end;

gpick.layouts['std_layout_webpage_1'] = {
	human_readable = 'Webpage',
	build = function (layout_system)
	
		local styles = {
			header = layout_style:new("header_b:Header", color:new(0.2, 0.2, 0.2)),
			header_text = layout_style:new("header_t:Header text", color:new(1.0, 1.0, 1.0), 1.0),
			
			content = layout_style:new("content_b:Content", color:new(0.6, 0.6, 0.6)),
			content_text = layout_style:new("content_t:Content text", color:new(0.1, 0.1, 0.1), 0.6),
			
			sidebar = layout_style:new("sidebar_b:Sidebar", color:new(0.7, 0.7, 0.7)),
		
			button = layout_style:new("navitem_b:Button", color:new(0.3, 0.3, 0.3)),
			button_hover = layout_style:new("navitem_bh:Button (hover)", color:new(0.35, 0.35, 0.35)),
			button_text = layout_style:new("navitem_t:Button text", color:new(0.8, 0.8, 0.8), 0.95),
			button_text_hover = layout_style:new("navitem_th:Button text (hover)", color:new(0.9, 0.9, 0.9), 0.95),
			
			footer = layout_style:new("footer_b:Footer", color:new(0.1, 0.1, 0.1)),
		};
		for i,v in pairs(styles) do
			layout_system:addstyle(v);
		end;
		
		local root = layout:new_box("root", 0, 0, 640, 480);
		layout_system:setbox(root);
		
		root:add(layout:new_fill("header", 0, 0, 1, 0.15, styles['header']):add(layout:new_text("header_text", 0.25, 0.25, 0.5, 0.5, styles['header_text'], 'Header')));
		root:add(layouts.add_buttons(layout:new_fill("sidebar", 0, 0.15, 0.25, 0.8, styles['sidebar']), 6, 0.7, { styles['button'], styles['button'], styles['button_hover'] }, { styles['button_text'], styles['button_text'], styles['button_text_hover'] }, 0, 0));
		root:add(layouts.add_text(layout:new_fill("content", 0.25, 0.15, 0.75, 0.8, styles['content']), 10, styles['content_text']));
		root:add(layout:new_fill("footer", 0, 0.95, 1, 0.05, styles['footer']):add(layouts.make_helper(layout:new_text("footer_text", 0.25, 0.5, 0.5, 0.45, nil, styles['footer']:humanname()))));
		
		return 1;
	end; };
	
gpick.layouts['std_layout_menu_1'] = {
	human_readable = 'Menu',
	build = function (layout_system)
		local styles = {
			menu = layout_style:new("menu_b:Menu", color:new(0.7, 0.7, 0.7)),
			button = layout_style:new("navitem_b:Button", color:new(0.3, 0.3, 0.3)),
			button_hover = layout_style:new("navitem_bh:Button (hover)", color:new(0.3, 0.3, 0.3)),
			button_text = layout_style:new("navitem_t:Button Text", color:new(0.8, 0.8, 0.8), 1.0),
			button_text_hover = layout_style:new("navitem_th:Button Text (hover)", color:new(0.8, 0.8, 0.8), 1.0),
		};
		for i,v in pairs(styles) do
			layout_system:addstyle(v);
		end;
		
		local root = layout:new_box("root", 0, 0, 300, 400);
		layout_system:setbox(root);
		
		root:add(layouts.add_buttons(layout:new_fill("menu", 0, 0, 1, 1, styles['menu']), 7, 1, { styles['button'], styles['button'], styles['button_hover'] }, { styles['button_text'], styles['button_text'], styles['button_text_hover'] }, 0.1));
		
		return 1;
	end; };

gpick.layouts_get = function()
	local layouts = {};
	for k,v in pairs(gpick.layouts) do table.insert(layouts, k) end
	return layouts;
end;
