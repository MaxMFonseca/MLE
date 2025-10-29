return {
	padding = "10px",
	container = {
		type = "free",
	},
	c = {
		text_c = {
			size_y = 0.06,
			size_x = 1,
			padding = "4px",
			border = {
				thickness = "2px",
				color = "NQB",
				roundness = "7px",
			},
			pos = "lb",
			origin = "lb",
			background = { "BLACK", 0.4 },
			container = {
				dir = "h",
				gap = "6px",
			},

			c = {
				car = {
					idx = 0,
					text = {
						"$>",
					},
				},
				text = {
					idx = 1,
					text = {
						".",
						input = {},
					},
				},
			},
		},
	},
}
