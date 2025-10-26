return {
	padding = "30px",
	container = {
		dir = "h",
	},
	c = {
		left = {
			idx = 0,
			size_x = "1f",
			size_y = 1,
			container = {
				dir = "v",
			},
			c = {
				{
					idx = 0,
					text = { "PerfTracker/s:", height = "20px" },
				},
				text = {
					idx = 1,
					text = { height = "20px" },
				},
			},
		},
		right = {
			idx = 1,
			size_x = "1f",
			size_y = 1,
			container = {
				dir = "v",
				cross_align = "e",
			},
			c = {
				{
					idx = 0,
					text = { "GpuPerf:", height = "20px" },
				},
				text = {
					idx = 1,
					text = { height = "20px", justify = "e" },
				},
			},
		},
	},
}
