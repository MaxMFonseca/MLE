return {
	background = "NQW",
	container = {
		type = "hybrid",
		min_gap = "5px",
		dir = "v",
		justify = "c",
	},
	c = {
		hey = {
			size = "10px",
			pos = { dep_x = { "i", "2px" }, dep_y = "i:2px" },
			background = "RED",
		},
		world = {
			size = { 0.1, 0.1 },
			pos = { "30px", "30px" },
			background = "GREEN",
		},
		i = {
			size = { 0.02, 0.02 },
			pos = { dep_x = { "world", "5px" }, dep_y = { "world", "5px" } },
			background = "BLUE",
		},
		am = {
			pos = 0.5,
			background = "YELLOW",
		},
		Max = {
			pos = { "2px", "2px" },
			size = { dep_x = "am:0.2", dep_y = "am:0.2" },
			background = "CYAN",
			-- border = {thickness = "2px", color = "WHITE" },
		},
	},
}

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
--
-- 		{
-- 			size = "50px",
-- 			background = "CYAN",
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
-- 			size = "50px",
-- 			background = "CYAN",
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
-- 			size = "50px",
-- 			background = "CYAN",
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
-- 			size = "50px",
-- 			background = "CYAN",
-- 		},
-- 		{
-- 			size = "50px",
-- 			background = "CYAN",
-- 		},
-- 		{
-- 			size = "50px",
-- 			background = "CYAN",
-- 		},
-- 	},
-- }
