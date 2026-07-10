local scrollable = require("mle.ui.comp.scrollable")

local function assert_sequence(actual, expected)
	assert(#actual == #expected, "event count: expected " .. #expected .. ", got " .. #actual)
	for i, value in ipairs(expected) do
		assert(actual[i] == value, "event " .. i .. ": expected " .. value .. ", got " .. tostring(actual[i]))
	end
end

local function resize_with(overflow)
	local events = {}
	local parent = {}

	function parent:get(key)
		events[#events + 1] = "parent:get:" .. key
		error("legacy resize must not read native scroll state")
	end

	function parent:apply(op, state)
		events[#events + 1] = "parent:apply:" .. op
		events[#events + 1] = "state:current:" .. state.current_scroll_y
		events[#events + 1] = "state:max:" .. state.max_scroll_y
	end

	function parent:requestInternalBoundsUpdate()
		events[#events + 1] = "parent:bounds"
	end

	local driver = {}
	local ew = {}

	function ew:get(key)
		events[#events + 1] = "driver:get:" .. key
		return overflow
	end

	function ew:parent()
		events[#events + 1] = "driver:parent"
		return parent
	end

	scrollable(driver)
	driver.on_resized(ew)
	return events
end

local M = {}

function M.run()
	assert_sequence(resize_with(nil), {
		"driver:get:overflow",
	})

	assert_sequence(resize_with({ overflow_y = -17 }), {
		"driver:get:overflow",
		"driver:parent",
		"parent:apply:free",
		"state:current:0",
		"state:max:-17",
		"parent:bounds",
	})

	return true
end

return M
