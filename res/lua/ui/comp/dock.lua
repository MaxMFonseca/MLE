return function(children)
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
