local function suggest(filename)
	local function p_require() require(filename) end
	local function supress_error(err) print(err) end
	return xpcall(p_require, supress_error)
end
local function round(number)
	if number - math.floor(number) >= 0.5 then
		return math.ceil(number)
	else
		return math.floor(number)
	end
end
return {
	suggest = suggest,
	round = round,
}
