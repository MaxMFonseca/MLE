return function(children)
	return {
		fn = {
			setEnabledChild = function(ew, child_name)
				print("here1")
				ew:applyOnChildren({ enabled = false })
				print("here2")
				ew:getChild(child_name):apply("enabled", true)
				print("here3")
			end,

			addChild = function(ew, child)
				ew:addChild(child)
				ew:call("setEnabledChild", child.name)
			end,
		},

		free = {},
		c = children,
	}
end
