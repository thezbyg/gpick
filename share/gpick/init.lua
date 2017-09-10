local gpick = require('gpick')
local color = require('gpick/color')
local helpers = require('helpers')
local _ = gpick._
local round = helpers.round
local componentToText = function(componentType, color)
	if componentType == 'rgb' then
		return {round(color:red() * 255) .. '', round(color:green() * 255) .. '', round(color:blue() * 255) .. ''}
	end
	if componentType == 'hsl' then
		return {round(color:hue() * 360) .. '', round(color:saturation() * 100) .. '', round(color:lightness() * 100) .. ''}
	end
	if componentType == 'hsv' then
		return {round(color:hue() * 360) .. '', round(color:saturation() * 100) .. '', round(color:value() * 100) .. ''}
	end
	if componentType == 'cmyk' then
		return {round(color:cyan() * 255) .. '', round(color:magenta() * 255) .. '', round(color:yellow() * 255) .. '', round(color:key_black() * 255) .. ''}
	end
	if componentType == 'lab' then
		return {round(color:labLightness()) .. '', round(color:labA()) .. '', round(color:labB()) .. ''}
	end
	if componentType == 'lch' then
		return {round(color:lchLightness()) .. '', round(color:lchChroma()) .. '', round(color:lchHue()) .. ''}
	end
	return {}
end
gpick:setComponentToTextCallback(componentToText)
require('options')
require('layouts')
require('converters')
helpers.suggest('user_init')
