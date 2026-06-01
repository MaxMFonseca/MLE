local panel_texture = "i/ui/nine_slice_panel.png"

local function make_demo(name, label, width, height, slice, tint)
	return {
		name = name,
		size_x = width,
		size_y = height,
		free = {},
		c = {
			panel = {
				size = 1,
				nine_slice = {
					texture = panel_texture,
					slice = slice,
					color = tint or Colors.WHITE,
					fit = true,
				},
			},
			label = {
				layer = 1,
				pos = "c",
				origin = "c",
				size_x = 0.9,
				size_y = "28px",
				text = {
					text = label,
					height = "18px",
					color = Colors.WHITE,
					justify = "center",
				},
			},
		},
	}
end

return {
	background = Colors.slate800,
	size = 1,
	padding = "36px",
	list = {
		dir = "v",
		cross_align = "c",
		gap = "18px",
	},
	c = {
		{
			name = "title",
			size_x = "640px",
			size_y = "34px",
			text = {
				text = "Nine-slice panel",
				height = "30px",
				color = Colors.WHITE,
				justify = "center",
			},
		},
		make_demo("small_panel", "12px slices, compact", "280px", "96px", "12px"),
		make_demo(
			"wide_panel",
			"12px slices, wide stretch",
			"620px",
			"96px",
			{ t = 12, b = 12, l = 12, r = 12 },
			Colors.cyan100
		),
		make_demo(
			"tall_panel",
			"20px slices, tall stretch",
			"360px",
			"180px",
			{ t = 20, b = 20, l = 20, r = 20 },
			Colors.emerald100
		),
		{
			name = "atlas_demo",
			size_x = "360px",
			size_y = "112px",
			free = {},
			c = {
				panel = {
					size = 1,
					nine_slice = {
						texture = panel_texture,
						uv_px = { 0, 0 },
						uv_size_px = { 64, 64 },
						slice = { t = 12, b = 12, l = 12, r = 12 },
						fit = true,
					},
				},
				label = {
					layer = 1,
					pos = "c",
					origin = "c",
					size_x = 0.9,
					size_y = "28px",
					text = {
						text = "Atlas region via uv_px",
						height = "18px",
						color = Colors.WHITE,
						justify = "center",
					},
				},
			},
		},
	},
}
