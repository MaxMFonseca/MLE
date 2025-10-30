return {
	padding = "10px",
	size = {},
	container = {
		dir = "h",
	},
	c = {
		left = {
			idx = 0,
			size_x = "1f",
			container = {
				dir = "v",
			},
			c = {
				{
					idx = 0,
					text = {
						"PerfTracker/s:",
						height = "23px",
						color = "green400",
					},
				},
				text = {
					idx = 1,
					text = {
						height = "23px",
						color = "green400",
					},
				},
			},
		},
		right = {
			idx = 1,
			size_x = "1f",
			container = {
				dir = "v",
				cross_align = "e",
			},
			c = {
				{
					idx = 0,
					text = { "GpuPerf:", height = "23px", color = "green400" },
				},
				text = {
					idx = 1,
					text = { height = "23px", justify = "e", color = "green400" },
				},
			},
		},
	},
}
