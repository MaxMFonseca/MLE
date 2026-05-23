return {
	padding = "10px",

	list = {
		dir = "h",
	},
	c = {
		left = {
			size = "1f",
			idx = 0,
			list = {
				dir = "v",
			},
			c = {
				{
					idx = 0,
					text = {
						"PerfTracker/s:",
						height = "23px",
						color = Colors.green400,
						border_color = Colors.slate900,
						border_thickness = 2,
					},
				},
				text = {
					idx = 1,
					text = {
						height = "23px",
						color = Colors.green400,
						border_color = Colors.slate900,
						border_thickness = 2,
					},
				},
			},
		},
		right = {
			size = "1f",
			idx = 1,
			list = {
				dir = "v",
				cross_align = "e",
			},
			c = {
				{
					idx = 0,
					text = {
						"GpuPerf:",
						height = "23px",
						color = Colors.green400,
						border_color = Colors.slate900,
						border_thickness = 2,
					},
				},
				text = {
					idx = 1,
					text = {
						height = "23px",
						justify = "e",
						color = Colors.green400,
						border_color = Colors.slate900,
						border_thickness = 2,
					},
				},
			},
		},
	},
}
