
gpick.layouts = {};

gpick.layouts['std_template_simple'] = {
	human_readable = 'Simple template',
	build = function ()
		local bg = color:new();
		local text = color:new();
		
		local root = layout:new_box("root", 0, 0, 1, 1);
		
		bg:rgb(0.2, 0.2, 0.2);
		text:rgb(1, 1, 1);
		local header = layout:new_fill("header", 0, 0, 1, 0.15, bg);
		header:add(layout:new_text("header_text", 0.25, 0.25, 0.5, 0.5, text, 1, 'Header'));
		root:add(header);
		
		bg:rgb(0.3, 0.3, 0.3);
		root:add(layout:new_fill("header", 0, 0.15, 1, 0.05, bg));
		
		bg:rgb(0.7, 0.7, 0.7);
		local sidebar = layout:new_fill("sidebar", 0, 0.20, 0.25, 0.85, bg);
		root:add(sidebar);
		
		function add_buttons(container, buttons, percentage)
			local root = layout:new_box('', 0, 0, 1, percentage);
			container:add(root);
			
			local bg = color:new();
			local text = color:new();
			bg:rgb(0.3, 0.3, 0.3);
			text:rgb(0.8, 0.8, 0.8);
			
			local size = 1/buttons;
			local padding = size/8;		
			
			for i=0,buttons-1 do
				local button = layout:new_fill("button", padding, i*size+padding, 1-padding*2, size-padding, bg);
				button:add(layout:new_text("button_text", 0.25, 0.25, 0.5, 0.5, text, 1, 'Button'));
				root:add(button);
			end;
		end;
		
		add_buttons(sidebar, 5, 0.5);
		
		bg:rgb(0.5, 0.5, 0.5);
		local content = layout:new_fill("content", 0.25, 0.20, 0.75, 0.85, bg);
		root:add(content);
		
		function add_text(container)
			local text = color:new();
			text:rgb(0, 0, 0);
			for i=0,10 do
				container:add(layout:new_text("content_text", 0.01, i*0.04+0.01, 1.0, 0.04, text, 1, 'The quick brown fox jumps over the lazy dog'));
			end;
		end;
		
		add_text(content);
		
		return root;
	end; };

gpick.layouts_get = function()
	local layouts = {};
	for k,v in pairs(gpick.layouts) do table.insert(layouts, k) end
	return layouts;
end;
