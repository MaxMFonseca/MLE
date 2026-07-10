local scroll_state = require("mle.ui.comp.scroll_state")

local function clamp(value, low, high)
	return math.max(low, math.min(high, value))
end

local function copy_table(source)
	local result = {}
	for key, value in pairs(source) do
		result[key] = value
	end
	return result
end

local function validate(content, options)
	if type(content) ~= "table" then
		error("scrollable_with_bar content must be a table")
	end
	if options ~= nil and type(options) ~= "table" then
		error("scrollable_with_bar options must be a table")
	end
	if options.wheel_speed ~= nil and options.wheel_speed < 0 then
		error("scrollable_with_bar wheel_speed must be nonnegative")
	end
	if options.min_thumb_px ~= nil and options.min_thumb_px < 1 then
		error("scrollable_with_bar min_thumb_px must be at least 1")
	end
end

return function(content, options)
	options = options or {}
	validate(content, options)

	local bar_width = options.bar_width or "10px"
	local min_thumb_px = options.min_thumb_px or 24
	local wheel_speed = options.wheel_speed or 40
	local track_color = options.track_color or Colors.slate800:withA(0.45)
	local thumb_color = options.thumb_color or Colors.slate400:withA(0.85)

	local scroll_driver = copy_table(content)
	local user_on_resized = scroll_driver.on_resized
	scroll_driver.name = "scroll_driver"
	scroll_driver.on_resized = function(ew)
		if user_on_resized then
			user_on_resized(ew)
		end
		local root = ew:parent():parent()
		root:call("syncGeometry", scroll_state.overflow_y(ew))
	end

	local function update_thumb(root)
		local state = root:get("table")
		local scrollbar = root:getChild("scrollbar")
		local thumb = scrollbar:getChild("thumb")
		local progress = 0
		if state.max_scroll_y > 0 then
			progress = state.current_scroll_y / state.max_scroll_y
		end
		local thumb_pos_y = progress * (1 - state.thumb_ratio)
		local changed = state.thumb_size_y ~= state.thumb_ratio or state.thumb_pos_y ~= thumb_pos_y
		state.thumb_size_y = state.thumb_ratio
		state.thumb_pos_y = thumb_pos_y
		thumb:apply("size_y", state.thumb_ratio)
		thumb:apply("pos_y", thumb_pos_y)
		if changed and thumb.requestExternalBoundsUpdate then
			thumb:requestExternalBoundsUpdate()
		end
		return changed
	end

	local function set_offset(root, desired_offset)
		local state = root:get("table")
		local viewport = root:getChild("viewport")
		local integer_offset = math.floor(clamp(desired_offset, 0, state.max_scroll_y) + 0.5)
		local native = scroll_state.set(viewport, integer_offset, state.max_scroll_y)
		local changed = state.current_scroll_y ~= native.current_scroll_y
		state.current_scroll_y = native.current_scroll_y
		state.max_scroll_y = native.max_scroll_y
		local thumb_changed = update_thumb(root)
		if changed then
			viewport:requestInternalBoundsUpdate()
		end
		return changed, thumb_changed
	end

	local scrollbar_element = {
		name = "scrollbar",
		size_x = bar_width,
		size_y = 1,
		disabled = true,
		background = track_color,
		border = {
			roundness = "5px",
		},
		free = {
			scrollable = false,
		},

		fn = {
			onCursorDrag = function(scrollbar, cursor_pos_relative)
				local root = scrollbar:parent()
				local state = root:get("table")
				local travel = 1 - state.thumb_ratio
				if travel <= 0 then
					root:call("setOffset", 0)
					return
				end
				local thumb_top = clamp(cursor_pos_relative.y - state.drag_grab_offset, 0, travel)
				root:call("setOffset", (thumb_top / travel) * state.max_scroll_y)
			end,
			onCursorDragEnd = function(scrollbar) end,
		},

		on_keys = {
			lmb = function(scrollbar)
				local hovered = scrollbar:get("hovered")
				if hovered == nil then
					return
				end
				local root = scrollbar:parent()
				local state = root:get("table")
				local progress = 0
				if state.max_scroll_y > 0 then
					progress = state.current_scroll_y / state.max_scroll_y
				end
				local thumb_top = progress * (1 - state.thumb_ratio)
				local thumb_bottom = thumb_top + state.thumb_ratio
				local pointer_y = hovered.pos_self_norm.y
				local viewport_height = root:getChild("viewport"):getBoundsOnRoot():size().y

				if pointer_y < thumb_top then
					root:call("scrollBy", -viewport_height)
				elseif pointer_y > thumb_bottom then
					root:call("scrollBy", viewport_height)
				else
					state.drag_grab_offset = pointer_y - thumb_top
					scrollbar:beginCursorDrag()
				end
			end,
		},

		c = {
			thumb = {
				size_x = 1,
				size_y = 1,
				pos_y = 0,
				background = thumb_color,
				border = {
					roundness = "5px",
				},
			},
		},
	}

	local viewport_element = {
		name = "viewport",
		size_x = "1f",
		free = {},
		c = {
			scroll_driver,
		},
	}

	local retc = {}

	if options.scrollbar_left then
		retc = {
			scrollbar_element,
			viewport_element,
		}
	else
		retc = {
			viewport_element,
			scrollbar_element,
		}
	end

	return {
		table = {
			current_scroll_y = 0,
			max_scroll_y = 0,
			thumb_ratio = 1,
			thumb_size_y = 1,
			thumb_pos_y = 0,
			drag_grab_offset = 0,
			bar_visible = false,
		},

		fn = {
			setOffset = function(root, offset)
				set_offset(root, offset)
			end,

			scrollBy = function(root, delta)
				local state = root:get("table")
				set_offset(root, state.current_scroll_y + delta)
			end,

			syncGeometry = function(root, overflow_y)
				local state = root:get("table")
				local viewport = root:getChild("viewport")
				local scrollbar = root:getChild("scrollbar")
				local viewport_height = viewport:getBoundsOnRoot():size().y
				local max_scroll_y = math.max(overflow_y or 0, 0)
				local visible = max_scroll_y > 0
				local relayout_requested = false

				if state.bar_visible ~= visible then
					state.bar_visible = visible
					scrollbar:apply("disabled", not visible)
					root:requestExternalBoundsUpdate()
					relayout_requested = true
				end

				state.max_scroll_y = max_scroll_y
				if not visible or viewport_height <= 0 then
					state.thumb_ratio = 1
					local scroll_changed, thumb_changed = set_offset(root, 0)
					if (scroll_changed or thumb_changed) and not relayout_requested then
						root:requestExternalBoundsUpdate()
					end
					return
				end

				local content_height = viewport_height + max_scroll_y
				local natural_ratio = viewport_height / content_height
				local minimum_ratio = min_thumb_px / viewport_height
				state.thumb_ratio = clamp(math.max(natural_ratio, minimum_ratio), 0, 1)
				local scroll_changed, thumb_changed = set_offset(root, state.current_scroll_y)
				if (scroll_changed or thumb_changed) and not relayout_requested then
					root:requestExternalBoundsUpdate()
				end
			end,
		},

		on_scroll = function(root, wheel_delta)
			root:call("scrollBy", -wheel_delta * wheel_speed)
		end,

		list = {
			dir = "h",
			-- cross_align = "stretch",
		},

		c = retc,
	}
end
