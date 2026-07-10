local scrollable_with_bar = require("mle.ui.comp.scrollable_with_bar")

local function validate(rows, options)
	if type(rows) ~= "table" then
		error("filterable_list rows must be a table")
	end
	if options ~= nil and type(options) ~= "table" then
		error("filterable_list options must be a table")
	end

	options = options or {}
	for _, key in ipairs({ "scrollbar", "input_text", "input_border" }) do
		if options[key] ~= nil and type(options[key]) ~= "table" then
			error("filterable_list " .. key .. " must be a table")
		end
	end

	local names = {}
	for index, row in ipairs(rows) do
		if type(row) ~= "table" then
			error("filterable_list row " .. index .. " must be a table")
		end
		if type(row.name) ~= "string" or row.name == "" then
			error("filterable_list row " .. index .. " name must be a nonempty string")
		end
		if names[row.name] then
			error("filterable_list duplicate row name: " .. row.name)
		end
		names[row.name] = true
	end
end

local function copy_rows(rows)
	local result = {}
	for _, row in ipairs(rows) do
		result[#result + 1] = table.deep_copy(row)
	end
	return result
end

local function merge_copy(base, overrides)
	local result = table.deep_copy(base)
	for key, value in pairs(overrides or {}) do
		result[key] = table.deep_copy(value)
	end
	return result
end

return function(rows, options)
	validate(rows, options)
	options = options or {}

	local source_rows = copy_rows(rows)
	local input_text = merge_copy({
		text = "",
		height = "16px",
		color = Colors.WHITE,
		input = {},
	}, options.input_text)
	local input_border = merge_copy({
		thickness = "1px",
		color = Colors.slate400,
		roundness = "5px",
	}, options.input_border)

	local results = scrollable_with_bar({
		list = {
			dir = "v",
			gap = options.list_gap or "6px",
		},
		c = copy_rows(source_rows),
	}, options.scrollbar)
	results.name = "results"
	results.size_y = "1f"

	return {
		table = {
			rows = source_rows,
			last_query = "",
		},
		fn = {
			applyFilter = function(root, query)
				local state = root:get("table")
				local results_ew = root:getChild("results")
				local scroll_driver = results_ew:getChild("viewport"):getChild("scroll_driver")

				scroll_driver:destroyAllChildren()
				for _, row in ipairs(state.rows) do
					if Utils.partial_word_match(row.name, query) then
						scroll_driver:addChild(table.deep_copy(row))
					end
				end
				state.last_query = query
				scroll_driver:requestInternalBoundsUpdate()
				results_ew:call("setOffset", 0)
			end,
		},
		on_update = function(root)
			local query = root:getChild("query_input"):get("text")
			if query ~= root:get("table").last_query then
				root:call("applyFilter", query)
			end
		end,
		list = {
			dir = "v",
			gap = options.list_gap or "6px",
		},
		c = {
			{
				name = "query_input",
				size_x = 1,
				size_y = options.input_height or "40px",
				padding = options.input_padding or { "10px", "8px" },
				background = options.input_background or Colors.slate800,
				border = input_border,
				text = input_text,
				on_keys = {
					lmb = function(input)
						input:apply("text_input_enable")
					end,
				},
			},
			results,
		},
	}
end
