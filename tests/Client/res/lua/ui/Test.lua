local color1 = Color.new("WHITE")
local color2 = Color.new("RED")
local color = color1

return {
	-- background = "RED",
	-- padding = "10px",
	-- size = { "300px", 1 },

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

	on_update = function(ew)
		local factor = math.abs((C.time / 2) % 2 - 1)
		color = Color.mix(color1, color2, factor)
	end,

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
			-- on_hover = function(ew)
			-- 	print("hovering bg", ew:fullName())
			-- 	local h = ew:get("hovered")
			-- 	print(h.pos_self.x, h.pos_self.y)
			-- 	print(h.pos_self_norm.x, h.pos_self_norm.y)
			-- 	print(h.sw:elapsedSecFloat())
			-- 	print(h.state)
			-- end,
			on_update = function(ew)
				ew:apply("shader_params", {
					pc_color = color,
				})
			end,
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
					pc_color = color,
				},
				fn = "TODO",
			},
		},
		blur = {
			pos = 0.5,
			size_y = 0.5,
			origin = "c",
			-- margin = "50px",
			padding = "13px",
			style = { "basic_round" },
			border = {
				thickness = "3px",
				color = color,
			},
			on_update = function(ew)
				ew:apply("border_color", color)

				local time_f = math.abs((C.time / 2) % 2 - 1)
				-- ew:apply("pos_x", time_f / 2 + 0.25)
			end,
			on_hover_in = function(ew)
				ew:apply("render_scale", 1.02)
			end,
			on_hover_out = function(ew)
				ew:apply("render_scale", 1.0)
			end,

			blur = {
				radius = 13,
				sigma = 5,
				before_children = true,
			},
			on_keys = {
				a = function()
					print("hello world")
				end,
				b = function()
					print("hello world BBBBBB")
				end,
			},

			container = {
				pack_children = true,
				cross_align = "c",
			},

			children_base = {
				border = {
					color = "NQB",
					thickness = "2px",
				},
				size_y = "1f",
				on_keys = {
					lmb = function(ew)
						print(ew:fullName())
						-- print(ew:get("text")) TODO
					end,
				},
			},
			c = {
				{
					name = "Hello",
					text = "Hello",
				},
				{
					name = "World",
					text = "World",
				},
				{
					name = "My",
					text = "My",
				},
				{
					name = "Name",
					text = "Name",
				},

				-- {
				-- 	on_hover_in = function(ew)
				-- 		print("in", ew:fullName())
				-- 	end,
				-- 	on_hover_out = function(ew)
				-- 		print("out", ew:fullName())
				-- 	end,
				-- 	size_y = 0.5,
				-- 	text = "World!",
				-- },
				-- {
				-- 	on_hover_in = function(ew)
				-- 		print("in", ew:fullName())
				-- 	end,
				-- 	on_hover_out = function(ew)
				-- 		print("out", ew:fullName())
				-- 	end,
				-- 	size_y = 0.5,
				-- 	text = "World!",
				-- },
				-- {
				-- 	on_hover_in = function(ew)
				-- 		print("in", ew:fullName())
				-- 	end,
				-- 	on_hover_out = function(ew)
				-- 		print("out", ew:fullName())
				-- 	end,
				-- 	size_y = 0.5,
				-- 	text = "World!",
				-- },
				-- {
				-- 	on_hover_in = function(ew)
				-- 		print("in", ew:fullName())
				-- 	end,
				-- 	on_hover_out = function(ew)
				-- 		print("out", ew:fullName())
				-- 	end,
				-- 	size_y = 0.5,
				-- 	text = "World!",
				-- },
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
