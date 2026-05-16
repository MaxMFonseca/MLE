return function()
	return {
		fn = {
			setPanel = function(ew, child_name)
				print(child_name)
				ew:disableAllBut(child_name)
				print("e")
			end,

			addPanel = function(ew, child)
				ew:addChild(child)
			end,
		},

		free = {},
	}
end
