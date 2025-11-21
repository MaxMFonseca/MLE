return {
	padding = "10px",

	size = {},
	free_container = {},
	c = {
		text_c = {
			size_y = 0.06,
			size_x = 1,
			padding = "4px",
			border = {
				thickness = "2px",
				color = Colors.green500,
				roundness = "7px",
			},
			background = Colors.BLACK:withA(0.96),
			list = {
				dir = "h",
				gap = "6px",
			},

			c = {
				car = {
					idx = 0,
					text = {
						"$>",
						color = Colors.BLACK,
						bold = true,
						border_color = Colors.green700,
						border_thickness = 1,
					},
				},
				text = {
					idx = 1,
					text = {
						".",
						input = {},
						color = Colors.green700,
					},
				},
			},
		},
	},
}
