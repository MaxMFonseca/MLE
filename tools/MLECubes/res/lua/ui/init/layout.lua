local color = C.Colors.ZERO

return {
	on_init = function(self)
		print("on_init layout")
	end,
	on_update = function(self)
		color = Color.mix(C.Colors.red800, C.Colors.blue800, (math.sin(C.time) + 1) / 2)
	end,

	root_image = {
		extent = { 1920, 1080 },
		clear_color = C.Colors.slate300,
	},
	c = {
		{
			"BG",
			size = "100%",
			shader = {
				vert = "mle/ui/quad",
				frag = "i/ui/bg",

				fn = function(self)
					return {
						pc = {
							canvas_size = { 190 * 7, 100 * 7 },
							color = color,
							time = C.time * 1.5,
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
				". MLECubes .",
				color = C.Colors.emerald300,
				outline = "BLACK:2",
			},
			on_update = function(self)
				self:apply("text", { color = color:withA(0.5) })
			end,
		},
	},
}
