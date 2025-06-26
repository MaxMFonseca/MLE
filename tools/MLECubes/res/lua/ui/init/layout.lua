return {
	root_image = {
		extent = { 1920, 1080 },
		clear_color = "slate300",
	},
	c = {
		-- {
		-- 	background = "emerald500",
		--
		-- 	text = {
		-- 		"Max!",
		-- 		color = "amber500",
		-- 	},
		-- },
		{
			"BG",
			size = "100%",
			shader = {
				vert = "mle/ui/quad",
				frag = "i/ui/bg",

				fn = function(self)
					return {
						pc = {
							canvas_size = { 1920, 1080 },
							color = Color.fromString("cyan800"),
							time = mle.time,
						},
					}
				end,
			},
		},
		{
			pos_x = "c",
			pos_y = "15%",
			size_y = 300,
			origin = "ct",
			text = {
				"Hello World!",
				color = "cyan400",
			},
		},
	},
}
