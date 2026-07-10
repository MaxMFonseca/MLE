local scrollable_with_bar = require("mle.ui.comp.scrollable_with_bar")

local function assert_sequence(actual, expected)
	assert(#actual == #expected, "event count: expected " .. #expected .. ", got " .. #actual)
	for i, value in ipairs(expected) do
		assert(actual[i] == value, "event " .. i .. ": expected " .. value .. ", got " .. tostring(actual[i]))
	end
end

local function assert_contains(actual, expected)
	for _, value in ipairs(actual) do
		if value == expected then
			return
		end
	end
	error("missing event: " .. expected)
end

local function color()
	return {
		withA = function(self, alpha)
			return { alpha = alpha }
		end,
	}
end

local function make_bounds(height)
	return {
		size = function(self)
			return { y = height }
		end,
	}
end

local function run_overflow_transition()
	Colors = {
		slate400 = color(),
		slate800 = color(),
	}

	local root_def = scrollable_with_bar({}, { min_thumb_px = 10 })
	local events = {}
	local state = root_def.table

	local thumb = {}
	function thumb:apply(op, value)
		events[#events + 1] = "thumb:" .. op .. ":" .. tostring(value)
	end
	function thumb:requestExternalBoundsUpdate()
		events[#events + 1] = "thumb:external_bounds"
	end

	local scrollbar = {}
	function scrollbar:apply(op, value)
		events[#events + 1] = "scrollbar:" .. op .. ":" .. tostring(value)
	end
	function scrollbar:getChild(name)
		assert(name == "thumb", "unexpected scrollbar child " .. tostring(name))
		return thumb
	end
	function scrollbar:getBoundsOnRoot()
		return make_bounds(100)
	end

	local viewport = {
		current_scroll_y = 0,
		max_scroll_y = 0,
	}
	function viewport:getBoundsOnRoot()
		return make_bounds(100)
	end
	function viewport:apply(op, value)
		assert(op == "free", "unexpected viewport apply " .. tostring(op))
		self.max_scroll_y = math.max(value.max_scroll_y, 0)
		self.current_scroll_y = math.max(0, math.min(value.current_scroll_y, self.max_scroll_y))
		events[#events + 1] = "viewport:free:" .. self.current_scroll_y .. ":" .. self.max_scroll_y
	end
	function viewport:get(key)
		assert(key == "scroll", "unexpected viewport get " .. tostring(key))
		return {
			current_scroll_y = self.current_scroll_y,
			max_scroll_y = self.max_scroll_y,
		}
	end
	function viewport:requestInternalBoundsUpdate()
		events[#events + 1] = "viewport:bounds"
	end

	local root = {}
	function root:get(key)
		assert(key == "table", "unexpected root get " .. tostring(key))
		return state
	end
	function root:getChild(name)
		if name == "viewport" then
			return viewport
		end
		if name == "scrollbar" then
			return scrollbar
		end
		error("unexpected root child " .. tostring(name))
	end
	function root:requestExternalBoundsUpdate()
		events[#events + 1] = "root:external_bounds"
	end
	function root:call(name, ...)
		return root_def.fn[name](self, ...)
	end

	root:call("syncGeometry", 100)

	assert(state.bar_visible == true, "bar should become visible")
	assert(state.max_scroll_y == 100, "max scroll should track overflow")
	assert(state.thumb_ratio == 0.5, "thumb should be half viewport when content is 2x")
	assert_sequence(events, {
		"scrollbar:disabled:false",
		"root:external_bounds",
		"viewport:free:0:100",
		"thumb:size_y:0.5",
		"thumb:pos_y:0",
		"thumb:external_bounds",
	})
end

local function run_visible_shrink_clamps_with_surviving_layout_request()
	Colors = {
		slate400 = color(),
		slate800 = color(),
	}

	local root_def = scrollable_with_bar({}, { min_thumb_px = 10 })
	local events = {}
	local state = root_def.table
	state.current_scroll_y = 80
	state.max_scroll_y = 200
	state.thumb_ratio = 100 / 300
	state.bar_visible = true

	local thumb = {}
	function thumb:apply(op, value)
		events[#events + 1] = "thumb:" .. op
	end
	function thumb:requestExternalBoundsUpdate()
		events[#events + 1] = "thumb:external_bounds"
	end

	local scrollbar = {}
	function scrollbar:apply(op, value)
		events[#events + 1] = "scrollbar:" .. op .. ":" .. tostring(value)
	end
	function scrollbar:getChild(name)
		assert(name == "thumb", "unexpected scrollbar child " .. tostring(name))
		return thumb
	end
	function scrollbar:getBoundsOnRoot()
		return make_bounds(100)
	end

	local viewport = {
		current_scroll_y = 80,
		max_scroll_y = 200,
	}
	function viewport:getBoundsOnRoot()
		return make_bounds(100)
	end
	function viewport:apply(op, value)
		assert(op == "free", "unexpected viewport apply " .. tostring(op))
		self.max_scroll_y = math.max(value.max_scroll_y, 0)
		self.current_scroll_y = math.max(0, math.min(value.current_scroll_y, self.max_scroll_y))
		events[#events + 1] = "viewport:free:" .. self.current_scroll_y .. ":" .. self.max_scroll_y
	end
	function viewport:get(key)
		assert(key == "scroll", "unexpected viewport get " .. tostring(key))
		return {
			current_scroll_y = self.current_scroll_y,
			max_scroll_y = self.max_scroll_y,
		}
	end
	function viewport:requestInternalBoundsUpdate()
		events[#events + 1] = "viewport:bounds"
	end

	local root = {}
	function root:get(key)
		assert(key == "table", "unexpected root get " .. tostring(key))
		return state
	end
	function root:getChild(name)
		if name == "viewport" then
			return viewport
		end
		if name == "scrollbar" then
			return scrollbar
		end
		error("unexpected root child " .. tostring(name))
	end
	function root:requestExternalBoundsUpdate()
		events[#events + 1] = "root:external_bounds"
	end
	function root:call(name, ...)
		return root_def.fn[name](self, ...)
	end

	root:call("syncGeometry", 50)

	assert(state.current_scroll_y == 50, "scroll state should clamp to shrunken overflow")
	assert(state.max_scroll_y == 50, "max scroll should track shrunken overflow")
	assert(state.bar_visible == true, "bar should stay visible")
	assert_contains(events, "viewport:free:50:50")
	assert_contains(events, "viewport:bounds")
	assert_contains(events, "root:external_bounds")
end

local function run_visible_ratio_change_requests_layout()
	Colors = {
		slate400 = color(),
		slate800 = color(),
	}

	local root_def = scrollable_with_bar({}, { min_thumb_px = 10 })
	local events = {}
	local state = root_def.table
	state.current_scroll_y = 0
	state.max_scroll_y = 100
	state.thumb_ratio = 0.5
	state.bar_visible = true

	local thumb = {}
	function thumb:apply(op, value)
		events[#events + 1] = "thumb:" .. op .. ":" .. tostring(value)
	end
	function thumb:requestExternalBoundsUpdate()
		events[#events + 1] = "thumb:external_bounds"
	end

	local scrollbar = {}
	function scrollbar:apply(op, value)
		events[#events + 1] = "scrollbar:" .. op .. ":" .. tostring(value)
	end
	function scrollbar:getChild(name)
		assert(name == "thumb", "unexpected scrollbar child " .. tostring(name))
		return thumb
	end
	function scrollbar:getBoundsOnRoot()
		return make_bounds(100)
	end

	local viewport = {
		current_scroll_y = 0,
		max_scroll_y = 100,
	}
	function viewport:getBoundsOnRoot()
		return make_bounds(400)
	end
	function viewport:apply(op, value)
		assert(op == "free", "unexpected viewport apply " .. tostring(op))
		self.max_scroll_y = math.max(value.max_scroll_y, 0)
		self.current_scroll_y = math.max(0, math.min(value.current_scroll_y, self.max_scroll_y))
		events[#events + 1] = "viewport:free:" .. self.current_scroll_y .. ":" .. self.max_scroll_y
	end
	function viewport:get(key)
		assert(key == "scroll", "unexpected viewport get " .. tostring(key))
		return {
			current_scroll_y = self.current_scroll_y,
			max_scroll_y = self.max_scroll_y,
		}
	end
	function viewport:requestInternalBoundsUpdate()
		events[#events + 1] = "viewport:bounds"
	end

	local root = {}
	function root:get(key)
		assert(key == "table", "unexpected root get " .. tostring(key))
		return state
	end
	function root:getChild(name)
		if name == "viewport" then
			return viewport
		end
		if name == "scrollbar" then
			return scrollbar
		end
		error("unexpected root child " .. tostring(name))
	end
	function root:requestExternalBoundsUpdate()
		events[#events + 1] = "root:external_bounds"
	end
	function root:call(name, ...)
		return root_def.fn[name](self, ...)
	end

	root:call("syncGeometry", 300)

	assert(state.current_scroll_y == 0, "scroll state should stay at top")
	assert(state.max_scroll_y == 300, "max scroll should track grown overflow")
	assert(state.thumb_ratio == 4 / 7, "thumb should match viewport-to-content ratio")
	assert_contains(events, "viewport:free:0:300")
	assert_contains(events, "thumb:size_y:" .. tostring(4 / 7))
	assert_contains(events, "thumb:external_bounds")
	assert_contains(events, "root:external_bounds")
end

local function run_visible_thumb_position_change_requests_layout()
	Colors = {
		slate400 = color(),
		slate800 = color(),
	}

	local root_def = scrollable_with_bar({}, { min_thumb_px = 50 })
	local events = {}
	local state = root_def.table
	state.current_scroll_y = 50
	state.max_scroll_y = 100
	state.thumb_ratio = 0.5
	state.thumb_size_y = 0.5
	state.thumb_pos_y = 0.25
	state.bar_visible = true

	local thumb = {}
	function thumb:apply(op, value)
		events[#events + 1] = "thumb:" .. op .. ":" .. tostring(value)
	end
	function thumb:requestExternalBoundsUpdate()
		events[#events + 1] = "thumb:external_bounds"
	end

	local scrollbar = {}
	function scrollbar:apply(op, value)
		events[#events + 1] = "scrollbar:" .. op .. ":" .. tostring(value)
	end
	function scrollbar:getChild(name)
		assert(name == "thumb", "unexpected scrollbar child " .. tostring(name))
		return thumb
	end
	function scrollbar:getBoundsOnRoot()
		return make_bounds(100)
	end

	local viewport = {
		current_scroll_y = 50,
		max_scroll_y = 100,
	}
	function viewport:getBoundsOnRoot()
		return make_bounds(100)
	end
	function viewport:apply(op, value)
		assert(op == "free", "unexpected viewport apply " .. tostring(op))
		self.max_scroll_y = math.max(value.max_scroll_y, 0)
		self.current_scroll_y = math.max(0, math.min(value.current_scroll_y, self.max_scroll_y))
		events[#events + 1] = "viewport:free:" .. self.current_scroll_y .. ":" .. self.max_scroll_y
	end
	function viewport:get(key)
		assert(key == "scroll", "unexpected viewport get " .. tostring(key))
		return {
			current_scroll_y = self.current_scroll_y,
			max_scroll_y = self.max_scroll_y,
		}
	end
	function viewport:requestInternalBoundsUpdate()
		events[#events + 1] = "viewport:bounds"
	end

	local root = {}
	function root:get(key)
		assert(key == "table", "unexpected root get " .. tostring(key))
		return state
	end
	function root:getChild(name)
		if name == "viewport" then
			return viewport
		end
		if name == "scrollbar" then
			return scrollbar
		end
		error("unexpected root child " .. tostring(name))
	end
	function root:requestExternalBoundsUpdate()
		events[#events + 1] = "root:external_bounds"
	end
	function root:call(name, ...)
		return root_def.fn[name](self, ...)
	end

	root:call("syncGeometry", 300)

	assert(state.current_scroll_y == 50, "scroll offset should survive")
	assert(state.max_scroll_y == 300, "max scroll should track grown overflow")
	assert(state.thumb_ratio == 0.5, "thumb ratio should stay at min size")
	assert(state.thumb_pos_y ~= 0.25, "thumb position should change when progress changes")
	assert_contains(events, "viewport:free:50:300")
	assert_contains(events, "thumb:external_bounds")
	assert_contains(events, "root:external_bounds")
end

local function run_thumb_press_begins_track_drag()
	Colors = {
		slate400 = color(),
		slate800 = color(),
	}

	local root_def = scrollable_with_bar({}, { min_thumb_px = 10 })
	local state = root_def.table
	state.thumb_ratio = 0.25
	state.max_scroll_y = 300
	local drag_started = false
	local dragged_offset = nil
	local viewport = {
		current_scroll_y = 0,
		max_scroll_y = 300,
	}
	function viewport:getBoundsOnRoot()
		return make_bounds(100)
	end
	function viewport:apply(op, value)
		assert(op == "free", "unexpected viewport apply " .. tostring(op))
		dragged_offset = value.current_scroll_y
		self.current_scroll_y = value.current_scroll_y
		self.max_scroll_y = value.max_scroll_y
	end
	function viewport:get(key)
		assert(key == "scroll", "unexpected viewport get " .. tostring(key))
		return {
			current_scroll_y = self.current_scroll_y,
			max_scroll_y = self.max_scroll_y,
		}
	end
	function viewport:requestInternalBoundsUpdate() end

	local scrollbar
	local thumb
	local root = {}
	function root:get(key)
		assert(key == "table", "unexpected root get " .. tostring(key))
		return state
	end
	function root:getChild(name)
		if name == "viewport" then
			return viewport
		end
		if name == "scrollbar" then
			return scrollbar
		end
		error("unexpected root child " .. tostring(name))
	end
	function root:call(name, ...)
		return root_def.fn[name](self, ...)
	end

	scrollbar = {}
	function scrollbar:parent()
		return root
	end
	function scrollbar:beginCursorDrag()
		drag_started = true
	end
	function scrollbar:get(key)
		assert(key == "hovered", "unexpected scrollbar get " .. tostring(key))
		return { pos_self_norm = { y = 0.1 } }
	end
	function scrollbar:getChild(name)
		assert(name == "thumb", "unexpected scrollbar child " .. tostring(name))
		return thumb
	end

	thumb = {}
	function thumb:parent()
		return scrollbar
	end
	function thumb:get(key)
		assert(key == "hovered", "unexpected thumb get " .. tostring(key))
		return { pos_self_norm = { y = 0.4 } }
	end
	function thumb:apply(op, value) end
	function thumb:requestExternalBoundsUpdate() end

	local scrollbar_def = root_def.c[2]
	assert(scrollbar_def.name == "scrollbar", "scrollbar should retain name without keyed list entry")
	scrollbar_def.on_keys.lmb(scrollbar)
	scrollbar_def.fn.onCursorDrag(scrollbar, { y = 0.602 })

	assert(drag_started, "thumb press should begin drag on track")
	assert(state.drag_grab_offset == 0.1, "grab offset should map thumb-local position into track space")
	assert(dragged_offset == 201, "drag position should round to native integer scroll units")
end

local M = {}

function M.run()
	run_overflow_transition()
	run_visible_shrink_clamps_with_surviving_layout_request()
	run_visible_ratio_change_requests_layout()
	run_visible_thumb_position_change_requests_layout()
	run_thumb_press_begins_track_drag()
	return true
end

return M
