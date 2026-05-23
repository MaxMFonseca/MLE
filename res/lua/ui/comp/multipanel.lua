return function(children, one_active)
	if one_active ~= nil and one_active then
		for _, child in ipairs(children) do
			child.active = false
		end
		children[1].active = true
	end

	return {
		fn = {
			setPanel = function(ew, child_name)
				ew:disableAllBut(child_name)
			end,

			setEnabledChild = function(ew, child_name)
				ew:disableAllBut(child_name)
			end,

			addPanel = function(ew, child)
				ew:addChild(child)
			end,
		},

		free = {},
		c = children,
	}
end
