return {
	-- background = "RED",
	-- padding = "10px",

	styles = {
		basic_round = {
			border = {
				roundness = "20px",
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
	c = {
		bg = {
			idx = 0,
			pos = { 0, 0 },
			size = { 1, 1 },
			shader = {
				pipeline = {
					name = "bg",
					vert = "mle/ui/rect.vert",
					frag = "i/ui/bg.frag",
					colors = { "color" },
					topology = "triangle_strip",
					cull = "none",
				},
				params = {
					pc_color = Color.fromString("RED"),
				},
				fn = "TODO",
			},
		},
		blur = {
			pos = 0.5,
			size = "400px",
			origin = "c",
			margin = "50px",
			style = { "basic_round" },
			border = {
				thickness = "3px",
				color = "RED",
			},
			blur = {
				radius = 13,
				sigma = 5,
			},
			on_keys = {
				a = function()
					print("hello world")
				end,
				b = function()
					print("hello world BBBBBB")
				end,
			},
		},
		blur2 = {
			pos = { xrel = "blur", yrel = "blur:c" },
			size = { xrel = "blur", yrel = "blur:0.2" },
			origin = "lc",
			style = { "basic_round", "basic_border" },
			blur = {
				radius = 13,
				sigma = 5,
			},
			on_hover = function()
				return 12
			end,
			on_hover_in = function()
				print("on_hover_in")
			end,
			on_hover_out = function()
				print("on_hover_out")
			end,
		},
		how = {
			pos = "br",
			origin = "br",
			size = "30px",
			background = "NQB",
			table = { nnn = 0 },
			on_keys = {
				lmb = function(ew)
					print(ew:fullName())
					local t = ew:get("table")
					print(t.nnn)
					t.nnn = t.nnn + 1
				end,
			},
		},
		health_bar = {
			pos = "rt",
			origin = "rt",
			size = { 0.2, 0.05 },
			margin = "20px",
			style = { "basic_round", "basic_border" },
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
