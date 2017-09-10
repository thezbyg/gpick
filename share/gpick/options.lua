local gpick = require('gpick')
local options = {}
local optionsUpdate = function(params)
	options.upperCase = params:getString('gpick.options.hex_case', 'upper') == 'upper'
end
gpick:setOptionChangeCallback(optionsUpdate)
return options
