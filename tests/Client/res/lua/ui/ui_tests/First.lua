return {
	background = Colors.slate600,

	size = 1,

	free = {},
	c = {
		{
			name = "scrollable",
			id = "scrollable",
			background = Colors.slate700:withA(0.3),

			fn = {
				addChild = function(ew, child)
					ew:addChild(child)
					ew:requestInternalBoundsUpdate()
					--ew:requestExternalBoundsUpdate()
					print("Child added to scrollable")
				end,
			},

			list = {
				dir = "v",
				cross_align = "c",
				pack = true,
			},
		},
		{
			name = "add_to_scrollable",

			size_y = "0.1",
			text = "+",
			pos = "c",
			origin = "c",

			on_keys = {
				lmb = function(ew)
					local scrollable_ew = G.ui:getElementById("scrollable")

					scrollable_ew:call("addChild", {
						size_y = "5%r",
						size_x = 0.1 + math.random() * 0.2,
						background = Color.random(),
					})
				end,
			},
		},
	},
}
