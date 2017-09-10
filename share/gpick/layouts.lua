local gpick = require('gpick')
local color = require('gpick/color')
local layout = require('gpick/layout')
local _ = gpick._
local makeHelper = function(container)
	container:helperOnly(true)
	return container
end
local makeLocked = function(container)
	container:locked(true)
	return container
end
local addText = function(container, lines, style)
	local text = color:new()
	for i = 0, lines - 1 do
		container:add(layout:newText("content_text", 0.02, i * 0.06 + 0.01, 0.96, 0.06, style, _("The quick brown fox jumps over the lazy dog")))
	end
	return container
end
local addButtons = function(container, buttons, percentage, buttonStyle, textStyle, spacing)
	local root = layout:newBox('', spacing , spacing, 1 - spacing * 2, percentage - spacing * 2)
	container:add(root)
	local size = 1 / buttons
	local padding = size / 8
	local styleCount = #buttonStyle
	local styleTextCount = #textStyle
	local texts = {_("Homepage"), _("About us"), _("Links to us"), _("Privacy"), _("Terms"), _("Contact us"), _("RSS")}
	for i = 0, buttons - 1 do
		local button = layout:newFill("button", padding, i * size + padding, 1 - padding * 2, size - padding, buttonStyle[i % styleCount + 1])
		button:add(makeHelper(layout:newText("button_text", 0.25, 0.65, 0.5, 0.3, textStyle[i % styleTextCount + 1], buttonStyle[i % styleCount + 1]:label())))
		button:add(layout:newText("button_text", 0.25, 0.25, 0.5, 0.5, textStyle[i % styleTextCount + 1], texts[i % #texts + 1]))
		root:add(button)
	end
	return container
end
gpick:addLayout('std_layout_brightness_darkness', _("Brightness-Darkness"), function(system)
	local styles = {
		main = layout:newStyle("main:" .. _("main"), color:new(0.7, 0.7, 0.7)),
		bvar1 = layout:newStyle("b1:b1", color:new(0.3, 0.3, 0.3)),
		bvar2 = layout:newStyle("b2:b2", color:new(0.3, 0.3, 0.3)),
		bvar3 = layout:newStyle("b3:b3", color:new(0.3, 0.3, 0.3)),
		bvar4 = layout:newStyle("b4:b4", color:new(0.3, 0.3, 0.3)),
		cvar1 = layout:newStyle("c1:c1", color:new(0.3, 0.3, 0.3)),
		cvar2 = layout:newStyle("c2:c2", color:new(0.3, 0.3, 0.3)),
		cvar3 = layout:newStyle("c3:c3", color:new(0.3, 0.3, 0.3)),
		cvar4 = layout:newStyle("c4:c4", color:new(0.3, 0.3, 0.3)),
	}
	local stylesOrder = {'cvar4', 'cvar3', 'cvar2','cvar1', 'main', 'bvar1', 'bvar2', 'bvar3', 'bvar4'}
	for i, v in ipairs(stylesOrder) do
		system:addStyle(styles[v])
	end
	local root = layout:newBox("root", 0, 0, 320, 128)
	system:setBox(root)
	root:add(makeLocked(layout:newFill("b1", 0.00, 0.00, 0.30, 0.25, styles['bvar1'])))
	root:add(makeLocked(layout:newFill("b2", 0.00, 0.25, 0.30, 0.25, styles['bvar2'])))
	root:add(makeLocked(layout:newFill("b3", 0.00, 0.50, 0.30, 0.25, styles['bvar3'])))
	root:add(makeLocked(layout:newFill("b4", 0.00, 0.75, 0.30, 0.25, styles['bvar4'])))
	root:add(makeLocked(layout:newFill("c1", 0.70, 0.00, 0.30, 0.25, styles['cvar1'])))
	root:add(makeLocked(layout:newFill("c2", 0.70, 0.25, 0.30, 0.25, styles['cvar2'])))
	root:add(makeLocked(layout:newFill("c3", 0.70, 0.50, 0.30, 0.25, styles['cvar3'])))
	root:add(makeLocked(layout:newFill("c4", 0.70, 0.75, 0.30, 0.25, styles['cvar4'])))
	root:add(layout:newFill("main", 0.30, 0.00, 0.40, 1.00, styles['main']))
end, 1)
gpick:addLayout('std_layout_webpage_1', _("Webpage"), function(system)
	local styles = {
		header = layout:newStyle("header_b:" .. _("Header"), color:new(0.2, 0.2, 0.2)),
		header_text = layout:newStyle("header_t:" .. _("Header text"), color:new(1.0, 1.0, 1.0), 1.0),
		content = layout:newStyle("content_b:" .. _("Content"), color:new(0.6, 0.6, 0.6)),
		content_text = layout:newStyle("content_t:" .. _("Content text"), color:new(0.1, 0.1, 0.1), 0.6),
		sidebar = layout:newStyle("sidebar_b:" .. _("Sidebar"), color:new(0.7, 0.7, 0.7)),
		button = layout:newStyle("navitem_b:" .. _("Button"), color:new(0.3, 0.3, 0.3)),
		button_hover = layout:newStyle("navitem_bh:" .. _("Button (hover)"), color:new(0.35, 0.35, 0.35)),
		button_text = layout:newStyle("navitem_t:" .. _("Button text"), color:new(0.8, 0.8, 0.8), 0.95),
		button_text_hover = layout:newStyle("navitem_th:" .. _("Button text (hover)"), color:new(0.9, 0.9, 0.9), 0.95),
		footer = layout:newStyle("footer_b:" .. _("Footer"), color:new(0.1, 0.1, 0.1)),
	}
	for i, v in pairs(styles) do
		system:addStyle(v)
	end
	local root = layout:newBox("root", 0, 0, 640, 480)
	system:setBox(root)
	root:add(layout:newFill("header", 0, 0, 1, 0.15, styles['header']):add(layout:newText("header_text", 0.25, 0.25, 0.5, 0.5, styles['header_text'], _("Header"))))
	root:add(addButtons(layout:newFill("sidebar", 0, 0.15, 0.25, 0.8, styles['sidebar']), 6, 0.7, { styles['button'], styles['button'], styles['button_hover'] }, { styles['button_text'], styles['button_text'], styles['button_text_hover'] }, 0, 0))
	root:add(addText(layout:newFill("content", 0.25, 0.15, 0.75, 0.8, styles['content']), 10, styles['content_text']))
	root:add(layout:newFill("footer", 0, 0.95, 1, 0.05, styles['footer']):add(makeHelper(layout:newText("footer_text", 0.25, 0.5, 0.5, 0.45, nil, styles['footer']:label()))))
end)
gpick:addLayout('std_layout_menu_1', _("Menu"), function(system)
	local styles = {
		menu = layout:newStyle("menu_b:" .. _("Menu"), color:new(0.7, 0.7, 0.7)),
		button = layout:newStyle("navitem_b:" .. _("Button"), color:new(0.3, 0.3, 0.3)),
		button_hover = layout:newStyle("navitem_bh:" .. _("Button (hover)"), color:new(0.3, 0.3, 0.3)),
		button_text = layout:newStyle("navitem_t:" .. _("Button text"), color:new(0.8, 0.8, 0.8), 1.0),
		button_text_hover = layout:newStyle("navitem_th:" .. _("Button text (hover)"), color:new(0.8, 0.8, 0.8), 1.0),
	}
	for i, v in pairs(styles) do
		system:addStyle(v)
	end
	local root = layout:newBox("root", 0, 0, 300, 400)
	system:setBox(root)
	root:add(addButtons(layout:newFill("menu", 0, 0, 1, 1, styles['menu']), 7, 1, { styles['button'], styles['button'], styles['button_hover'] }, { styles['button_text'], styles['button_text'], styles['button_text_hover'] }, 0.1))
end)
gpick:addLayout('std_layout_grid_1', _("Grid (4x3)"), function(system)
	local root = layout:newBox("root", 0, 0, 400, 300)
	system:setBox(root)
	for j = 0, 2 do
		for i = 0, 3 do
			local itemIndex = 1 + (i + j * 4)
			local style = layout:newStyle("item" .. itemIndex .. ":" .. _("Item") .. itemIndex, color:new(0.8, 0.8, 0.8), 1.0)
			local styleText = layout:newStyle("item" .. itemIndex .. "_text:" .. _("Item text") .. itemIndex, color:new(0.2, 0.2, 0.2), 0.5)
			system:addStyle(style)
			system:addStyle(styleText)
			local fill = layout:newFill("b" .. itemIndex, (1 / 4) * i, (1 / 3) * j, (1 / 4) * 0.95, (1 / 3) * 0.95, style)
			fill:add(layout:newText("item_text".. itemIndex, 0, 0.25, 1, 0.5, styleText, _("Item") .. itemIndex))
			root:add(fill)
		end
	end
end)
gpick:addLayout('std_layout_grid_2', _("Grid (5x4)"), function(system)
	local root = layout:newBox("root", 0, 0, 500, 400)
	system:setBox(root)
	for j = 0, 3 do
		for i = 0, 4 do
			local itemIndex = 1 + (i + j * 5)
			local style = layout:newStyle("item" .. itemIndex .. ":" .. _("Item") .. itemIndex, color:new(0.8, 0.8, 0.8), 1.0)
			local styleText = layout:newStyle("item" .. itemIndex .. "_text:" .. _("Item text") .. itemIndex, color:new(0.2, 0.2, 0.2), 0.5)
			system:addStyle(style)
			system:addStyle(styleText)
			local fill = layout:newFill("b" .. itemIndex, (1 / 5) * i, (1 / 4) * j, (1 / 5) * 0.95, (1 / 4) * 0.95, style)
			fill:add(layout:newText("item_text".. itemIndex, 0, 0.25, 1, 0.5, styleText, _("Item") .. itemIndex))
			root:add(fill)
		end
	end
end)
return {}
