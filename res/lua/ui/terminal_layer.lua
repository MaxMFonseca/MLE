return {
	padding = "10px",
	size = {},
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
				color = "green500",
				roundness = "7px",
			},
			background = { "BLACK", 0.96 },
			container = {
				dir = "h",
				gap = "6px",
			},

			c = {
				car = {
					idx = 0,
					text = {
						"$>",
						color = "BLACK",
						bold = true,
						border_color = "green700",
						border_thickness = 1,
					},
				},
				text = {
					idx = 1,
					text = {
						".",
						input = {},
						color = "green700",
					},
				},
			},
		},
	},
}
