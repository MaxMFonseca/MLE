return {
	background = "RED",
	padding = "10px",

	styles = {
		basic_round = {
			border = {
				roundness = "10%",
			},
		},
		basic_border = {
			border = {
				thickness = "3px",
				color = "NQB",
			},
		},
	},

	container = {
		type = "hybrid",
		dir = "v",
		wrap = "y",
		-- cross_max_size = 0.2,
		cross_align = "c",
		main_min_gap = "10px",
		cross_gap = 0.1,
		justify = "se",
		justify_last = "b",
	},
	children_base = {
		-- style = { "basic_round", "basic_border" },
		-- background = "GRAY",
	},
	c = {
		bg = {
			idx = 0,
			pos = { 0, 0 },
			size = { 1, 1 },
			sprite = { "i/ui/bg.png" },
		},
		blur = {
			pos = { 0, 0 },
			size = 0.5,
			blur = {
				radius = 20,
				sigma = 10,
			},
		},
	},
}
-- 		{
-- 			-- size = "80px",
-- 			sprite = { "mle/ui/mle.png", color = "RED" },
-- 		},
-- 		{
-- 			text = "Hello! World",
-- 			size_y = "100px",
-- 			size_x = "fit",
-- 		},
-- 		{
-- 			size = { "30px", "35px" },
-- 			background = "RED",
-- 		},
-- 		{
-- 			size = { "60px", "10px" },
-- 		},
-- 		{
-- 			size = { "80px", "60px" },
-- 		},
-- 		{
-- 			size = { "60px", "80px" },
-- 		},
-- 		{
-- 			size_x = 1,
-- 			aspect_ratio = 1,
-- 		},
-- 		{
-- 			size_x = 1,
-- 			aspect_ratio = 1,
-- 		},
-- 		{
-- 			size = "10px",
-- 			background = "RED",
-- 		},
-- 		{
-- 			size = "20px",
-- 			background = "GREEN",
-- 		},
-- 		{
-- 			size = "30px",
-- 			background = "BLUE",
-- 		},
-- 		{
-- 			size = "40px",
-- 			background = "YELLOW",
-- 		},
-- 		{
-- 			size = "50px",
-- 			background = "CYAN",
-- 		},
-- 		{
-- 			size = "50px",
-- 			background = "CYAN",
-- 		},
-- 		{
-- 			size = { "100px", "50px" },
-- 			background = "#dddddd",
-- 		},
-- 		{
-- 			size = "10px",
-- 			background = "#101010",
-- 		},
-- 		{
-- 			size = "20px",
-- 			background = "#202020",
-- 		},
-- 		{
-- 			size = "30px",
-- 			background = "#303030",
-- 		},
-- 		{
-- 			size = "40px",
-- 			background = "#404040",
-- 		},
-- 		{
-- 			size = "50px",
-- 			background = "#505050",
-- 		},
-- 		{
-- 			size = "60px",
-- 			background = "#606060",
-- 		},
-- 		{
-- 			size = "70px",
-- 			background = "#707070",
-- 		},
-- 		{
-- 			size = "80px",
-- 			background = "#808080",
-- 		},
-- 		{
-- 			size = "90px",
-- 			background = "#909090",
-- 		},
-- 		{
-- 			size = "100px",
-- 			background = "#a0a0a0",
-- 		},
-- 		{
-- 			size = { "100px", "100px" },
-- 			background = "#ff0000",
-- 		},
-- 		{
-- 			size = { "80px", "40px" },
-- 			background = "#00ff00",
-- 		},
-- 		{
-- 			size = { "40px", "80px" },
-- 			background = "#0000ff",
-- 		},
-- 		{
-- 			size = { "60px", "100px" },
-- 			background = "#ffff00",
-- 		},
-- 		{
-- 			size = { "100px", "60px" },
-- 			background = "#00ffff",
-- 		},
-- 		{
-- 			size = { "90px", "30px" },
-- 			background = "#ff00ff",
-- 		},
-- 		{
-- 			size = { "30px", "90px" },
-- 			background = "#ffffff",
-- 		},
-- 		{
-- 			size = { "70px", "70px" },
-- 			background = "#333333",
-- 		},
-- 		{
-- 			size = { "50px", "100px" },
-- 			background = "#888888",
-- 		},
-- 		{
-- 			size = { "100px", "50px" },
-- 			background = "#cccccc",
-- 		},
-- 	},
-- }
