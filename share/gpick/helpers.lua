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
local function format(format, ...)
	local previousLocale = os.setlocale(nil, "numeric")
	os.setlocale("C", "numeric")
	local result = string.format(format, ...)
	os.setlocale(previousLocale, "numeric")
	return result
end
local function split(text, valueStart, separator, count)
	local findStart, findEnd = string.find(text, valueStart)
	if findStart == nil then
		return nil, nil, nil
	end
	local resultStart = findStart
	local values = {}
	table.insert(values, string.sub(text, findStart, findEnd))
	local searchPosition = findEnd
	local i
	for i = 2, count do
		position = string.find(text, separator, searchPosition)
		if position == nil then
			return nil, nil, nil
		end
		findStart, findEnd = string.find(text, valueStart, position + 1)
		if findStart == nil then
			return nil, nil, nil
		end
		table.insert(values, string.sub(text, findStart, findEnd))
		searchPosition = findEnd + 1
	end
	return resultStart, findEnd, values
end
local function clamp(value, min, max)
	if type(value) == 'table' then
		for i = 1, #value do
			local v = tonumber(value[i])
			if v < min then
				v = min
			elseif v > max then
				v = max
			end
			value[i] = v
		end
		return value
	else
		local v = tonumber(value)
		if v < min then
			v = min
		elseif v > max then
			v = max
		end
		return v
	end
end
return {
	suggest = suggest,
	round = round,
	clamp = clamp,
	format = format,
	split = split,
}
