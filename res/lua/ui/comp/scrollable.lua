return function(scroll_driver)
	if scroll_driver.name then
		error("scroll_driver element must not have a name")
	end
	scroll_driver.name = "scroll_driver"
	scroll_driver.on_resized = function(ew)
		print("called")
		local overflow = ew:get("overflow")
		if overflow == nil then
			return
		end
		print(overflow.overflow_y)

		local parent = ew:parent()

		print("END")
		parent:apply("free", {
			current_scroll_y = 0,
			max_scroll_y = overflow.overflow_y,
		})
		print("END")
		parent:requestInternalBoundsUpdate()
		print("END")
	end

	local ret = {
		on_scroll = function(ew, scroll_y)
			print("scrolling", scroll_y)
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
