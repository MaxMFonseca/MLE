return function(scroll_driver)
	if scroll_driver.name then
		error("scroll_driver element must not have a name")
	end
	scroll_driver.name = "scroll_driver"
	scroll_driver.on_resized = function(ew)
		local overflow = ew:get("overflow")
		if overflow == nil then
			return
		end

		local parent = ew:parent()

		parent:apply("free", {
			current_scroll_y = 0,
			max_scroll_y = overflow.overflow_y,
		})
		parent:requestInternalBoundsUpdate()
	end

	local ret = {
		on_scroll = function(ew, scroll_y)
			ew:apply("add_scroll_y", scroll_y)
			ew:requestInternalBoundsUpdate()
		end,

		free = {},

		c = {
			scroll_driver,
		},
	}
	return ret
end
