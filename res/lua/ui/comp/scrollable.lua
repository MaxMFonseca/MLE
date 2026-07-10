local scroll_state = require("mle.ui.comp.scroll_state")

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
		scroll_state.apply_legacy(parent, 0, overflow.overflow_y)
		parent:requestInternalBoundsUpdate()
	end

	return {
		on_scroll = function(ew, scroll_y)
			scroll_state.add_native_wheel(ew, scroll_y)
		end,

		free = {},

		c = {
			scroll_driver,
		},
	}
end
