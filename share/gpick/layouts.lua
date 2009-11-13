
gpick.layouts = {};

layouts = {};

layouts.add_buttons = function (container, buttons, percentage, button_style, spacing)
	local root = layout:new_box('', spacing, spacing, 1-spacing*2, percentage-spacing*2);
	container:add(root);

	local size = 1/buttons;
	local padding = size/8;		

	for i=0,buttons-1 do
		local button = layout:new_fill("button", padding, i*size+padding, 1-padding*2, size-padding, button_style);
		button:add(layout:new_text("button_text", 0.25, 0.25, 0.5, 0.5, button_style, 'Button'));
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

gpick.layouts['std_layout_webpage_1'] = {
	human_readable = 'Webpage',
	build = function (layout_system)
		local styles = {
			header = layout_style:new("header", color:new(0.2, 0.2, 0.2), color:new(1.0, 1.0, 1.0), 1.0),
			sidebar = layout_style:new("sidebar", color:new(0.7, 0.7, 0.7), color:new(1.0, 1.0, 1.0), 0.5),
			content = layout_style:new("content", color:new(0.6, 0.6, 0.6), color:new(0.0, 0.0, 0.0), 0.6),
			button = layout_style:new("button", color:new(0.3, 0.3, 0.3), color:new(0.8, 0.8, 0.8), 1.0),
			footer = layout_style:new("footer", color:new(0.1, 0.1, 0.1), color:new(0.7, 0.7, 0.7), 0.8),
		};
		for i,v in pairs(styles) do
			layout_system:addstyle(v);
		end;
		
		local root = layout:new_box("root", 0, 0, 640, 480);
		layout_system:setbox(root);
		
		root:add(layout:new_fill("header", 0, 0, 1, 0.15, styles['header']):add(layout:new_text("header_text", 0.25, 0.25, 0.5, 0.5, styles['header'], 'Header')));
		root:add(layouts.add_buttons(layout:new_fill("sidebar", 0, 0.15, 0.25, 0.8, styles['sidebar']), 5, 0.5, styles['button'], 0));
		root:add(layouts.add_text(layout:new_fill("content", 0.25, 0.15, 0.75, 0.8, styles['content']), 10, styles['content']));
		root:add(layout:new_fill("footer", 0, 0.95, 1, 0.05, styles['footer']));
		
		return 1;
	end; };
	
gpick.layouts['std_layout_menu_1'] = {
	human_readable = 'Menu',
	build = function (layout_system)
		local styles = {
			menu = layout_style:new("menu", color:new(0.7, 0.7, 0.7), color:new(1.0, 1.0, 1.0), 0.5),
			button = layout_style:new("button", color:new(0.3, 0.3, 0.3), color:new(0.8, 0.8, 0.8), 1.0),
		};
		for i,v in pairs(styles) do
			layout_system:addstyle(v);
		end;
		
		local root = layout:new_box("root", 0, 0, 300, 400);
		layout_system:setbox(root);
		
		root:add(layouts.add_buttons(layout:new_fill("menu", 0, 0, 1, 1, styles['menu']), 8, 1, styles['button'], 0.1));
		
		return 1;
	end; };

gpick.layouts_get = function()
	local layouts = {};
	for k,v in pairs(gpick.layouts) do table.insert(layouts, k) end
	return layouts;
end;
