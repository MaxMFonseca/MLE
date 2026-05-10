LuaUtils = {
	printR = function(t)
		local print_r_cache = {}
		local function subPrintR(t, indent)
			if print_r_cache[tostring(t)] then
				print(indent .. "*" .. tostring(t))
			else
				print_r_cache[tostring(t)] = true
				if type(t) == "table" then
					for pos, val in pairs(t) do
						if type(val) == "table" then
							print(indent .. "[" .. pos .. "] => " .. tostring(t) .. " {")
							subPrintR(val, indent .. string.rep(" ", string.len(pos) + 2))
							print(indent .. string.rep(" ", string.len(pos)) .. "}")
						else
							print(indent .. "[" .. pos .. "] => " .. tostring(val))
						end
					end
				else
					print(indent .. tostring(t))
				end
			end
		end
		print("*-- --*")
		subPrintR(t, "^")
	end,

	split_multiline_string = function(multiline_string)
		local lines = {}
		for line in multiline_string:gmatch("([^\n]*)\n?") do
			if line ~= "" then
				table.insert(lines, line)
			end
		end
		return lines
	end,

	shallow_copy = function(t)
		if type(t) ~= "table" then
			return t
		end
		local new_table = {}
		for k, v in pairs(t) do
			new_table[k] = v
		end
		return new_table
	end,

	shallow_copy_meta = function(t)
		if type(t) ~= "table" then
			return t
		end
		local new_table = {}
		for k, v in pairs(t) do
			new_table[k] = v
		end
		setmetatable(new_table, getmetatable(t))
		return new_table
	end,

	deep_copy = function(obj)
		if type(obj) ~= "table" then
			return obj
		end
		local new_table = {}
		for k, v in pairs(obj) do
			if type(v) == "table" then
				new_table[k] = LuaUtils.deep_copy(v)
			else
				new_table[k] = v
			end
		end
		return new_table
	end,

	deep_copy_meta = function(obj)
		if type(obj) ~= "table" then
			return obj
		end
		local new_table = {}
		for k, v in pairs(obj) do
			if type(v) == "table" then
				new_table[k] = LuaUtils.deep_copy(v)
			else
				new_table[k] = v
			end
		end
		return new_table
	end,

	clamp = function(value, min, max)
		if value < min then
			return min
		elseif value > max then
			return max
		else
			return value
		end
	end,

	deep_merge = function(dest, src)
		for k, v in pairs(src) do
			if type(v) == "table" then
				if type(dest[k] or false) == "table" then
					LuaUtils.deep_merge(dest[k], v)
				else
					dest[k] = LuaUtils.deep_copy(v)
				end
			else
				dest[k] = v
			end
		end
		return dest
	end,
}

table.printR = LuaUtils.printR
table.shallow_copy = LuaUtils.shallow_copy
table.shallow_copy_meta = LuaUtils.shallow_copy_meta
table.deep_copy = LuaUtils.deep_copy
table.deep_copy_meta = LuaUtils.deep_copy_meta
table.deep_merge = LuaUtils.deep_merge
math.clamp = LuaUtils.clamp

return LuaUtils
