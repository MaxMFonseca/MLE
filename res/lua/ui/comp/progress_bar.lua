return function(color_base, color_fill_1, color_fill_2, roundness_px, initial_fill)
	return {
		fn = {
			fill = function(ew, fraction)
				fraction = math.max(0.005, math.min(1, fraction or 0))
				local child = ew:getChild("fill")
				child:apply("size_x", fraction)
				child:apply("background", Color.mix(color_fill_1, color_fill_2, fraction))
			end,
		},

		container = { type = "free" },
		c = {
			base = {
				layer = -1,
				size = { 1, 1 },
				background = color_base,
				border = {
					roundness = roundness_px .. "px",
				},
			},
			fill = {
				layer = 0,
				size = { initial_fill, 1 },
				background = Color.mix(color_fill_1, color_fill_2, initial_fill),
				border = {
					roundness = roundness_px .. "px",
				},
			},
		},
	}
end
