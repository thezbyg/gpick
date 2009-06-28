
function round(number)
	if number-math.floor(number)>=0.5 then
		return math.ceil(number);
	else
		return math.floor(number);
	end;
end;

function color_web_hex(color_object)
	local c = color_object:get_color();
	return '#' .. string.format('%02X%02X%02X', round(c:red()*255), round(c:green()*255), round(c:blue()*255));
end;

function color_web_hex_3_digit(color_object)
	local c = color_object:get_color();
	return '#' .. string.format('%01X%01X%01X', round(c:red()*15), round(c:green()*15), round(c:blue()*15));
end;

function color_css_hsl(color_object)
	local c = color_object:get_color();
	c = c:rgb_to_hsl();
	return 'hsl(' .. string.format('%d, %d%%, %d%%', round(c:hue()*360), round(c:saturation()*100), round(c:lightness()*100)) .. ')';
end;

function color_css_rgb(color_object)
	local c = color_object:get_color();
	return 'rgb(' .. string.format('%d, %d, %d', round(c:red()*255), round(c:green()*255), round(c:blue()*255)) .. ')';
end;
