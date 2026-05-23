local function copy_options(options)
	local ret = {}
	if options ~= nil then
		for _, option in ipairs(options) do
			table.insert(ret, option)
		end
	end
	return ret
end

local function contains(options, value)
	for _, option in ipairs(options) do
		if option == value then
			return true
		end
	end
	return false
end

local function unpack_options_payload(new_options, selected)
	if new_options ~= nil and new_options.options ~= nil then
		return new_options.options, new_options.selected
	end
	return new_options, selected
end

local function popup_anchor(ew, popup_origin)
	local bounds = ew:getBoundsOnRootNormalized()
	local pos = bounds:pos()
	local size = bounds:size()

	if popup_origin == "top_right" then
		return {
			pos = {
				x = pos.x + size.x,
				y = pos.y,
			},
			origin = "lt",
		}
	end

	return {
		pos = {
			x = pos.x,
			y = pos.y + size.y,
		},
		origin = "lt",
	}
end

return function(options, callback, popup_origin)
	local initial_options = copy_options(options)
	local initial_selected = initial_options[1] or ""

	local ret = {
		fn = {
			set_option = function(ew, option)
				if option ~= nil then
					ew:apply("text", option)
					ew:get("table").selected = option
					if callback ~= nil then
						callback(ew, option)
					end
				end
			end,

			set_options = function(ew, new_options, selected)
				new_options, selected = unpack_options_payload(new_options, selected)
				local ew_table = ew:get("table")
				ew_table.options = copy_options(new_options)

				local next_selected = selected
				if next_selected == nil or not contains(ew_table.options, next_selected) then
					next_selected = ew_table.options[1] or ""
				end

				ew:call("set_option", next_selected)
			end,

			setOptions = function(ew, payload)
				ew:call("set_options", payload)
			end,
		},

		text = initial_selected,

		table = {
			options = initial_options,
			selected = initial_selected,
			popup_origin = popup_origin or "bottom_left",
		},

		on_keys = {
			lmb = function(ew)
				local ew_table = ew:get("table")
				local anchor = popup_anchor(ew, ew_table.popup_origin)
				local popup_children = {}

				for _, option in ipairs(ew_table.options) do
					table.insert(popup_children, {
						size_x = "1f",
						size_y = "32px",
						padding = { "10px", "4px" },
						background = option == ew_table.selected and Colors.slate600 or Colors.slate800,
						text = {
							text = option,
							height = "16px",
							color = Colors.WHITE,
						},
						on_keys = {
							lmb = function(option_ew)
								local popup_panel = option_ew:parent()
								local popup_root = popup_panel:parent()
								local owner = popup_root:get("table").owner
								owner:call("set_option", option)
								popup_root:destroy()
							end,
						},
					})
				end

				ew:createPopup({
					child = {
						pos = anchor.pos,
						origin = anchor.origin,
						size_x = ew:getBoundsOnRootNormalized():size().x,
						padding = "4px",
						background = Colors.slate900:withA(0.95),
						border = {
							thickness = "1px",
							color = Colors.slate500,
							roundness = "4px",
						},
						list = {
							dir = "v",
							gap = "2px",
						},
						on_keys = {
							lmb = function() end,
						},
						c = popup_children,
					},
				})
			end,
		},
	}

	return ret
end
