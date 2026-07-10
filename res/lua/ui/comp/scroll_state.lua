local M = {}

function M.overflow_y(content)
	local overflow = content:get("overflow")
	if overflow == nil then
		return 0
	end
	return math.max(overflow.overflow_y or 0, 0)
end

function M.read(viewport)
	local state = viewport:get("scroll")
	if state == nil then
		return {
			current_scroll_y = 0,
			max_scroll_y = 0,
		}
	end
	return state
end

function M.apply_legacy(viewport, current_scroll_y, max_scroll_y)
	viewport:apply("free", {
		current_scroll_y = current_scroll_y,
		max_scroll_y = max_scroll_y,
	})
end

function M.set(viewport, current_scroll_y, max_scroll_y)
	M.apply_legacy(viewport, current_scroll_y, math.max(max_scroll_y, 0))
	return M.read(viewport)
end

function M.add_native_wheel(viewport, wheel_delta)
	viewport:apply("add_scroll_y", wheel_delta)
	viewport:requestInternalBoundsUpdate()
end

return M
