local sprite_sheet = "i/ui/animation_sprite_sheet.png"

local function make_label(text)
	return {
		size_x = "620px",
		size_y = "28px",
		text = {
			text = text,
			height = "18px",
			color = Colors.slate100,
			justify = "center",
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
		gap = "16px",
	},
	c = {
		{
			size_x = "620px",
			size_y = "36px",
			text = {
				text = "Native UI animations",
				height = "30px",
				color = Colors.WHITE,
				justify = "center",
			},
		},
		make_label("Position tween"),
		{
			size_x = "620px",
			size_y = "96px",
			free = {},
			c = {
				jumper = {
					size_x = "220px",
					size_y = "42px",
					pos = { "c", "70px" },
					origin = "c",
					background = Colors.cyan700,
					border = {
						thickness = "1px",
						color = Colors.cyan200,
						roundness = "8px",
					},
					text = {
						text = "Bouncing",
						height = "18px",
						color = Colors.WHITE,
						justify = "center",
					},
					animation = {
						tracks = {
							{
								target = "pos_y",
								from = "70px",
								to = "28px",
								duration = 0.45,
								ease = "out_quad",
								loop = true,
								yoyo = true,
							},
						},
					},
				},
			},
		},
		make_label("Scale and color tweens"),
		{
			size_x = "620px",
			size_y = "92px",
			free = {},
			c = {
				pulse = {
					size_x = "260px",
					size_y = "54px",
					pos = "c",
					origin = "c",
					background = Colors.emerald700,
					border = {
						thickness = "1px",
						color = Colors.emerald200,
						roundness = "8px",
					},
					text = {
						text = "Pulse",
						height = "20px",
						color = Colors.WHITE,
						justify = "center",
					},
					animation = {
						tracks = {
							{
								target = "render_scale",
								from = 0.92,
								to = 1.08,
								duration = 0.6,
								ease = "in_out_quad",
								loop = true,
								yoyo = true,
							},
							{
								target = "background",
								from = Colors.emerald800,
								to = Colors.fuchsia700,
								duration = 1.2,
								ease = "in_out_quad",
								loop = true,
								yoyo = true,
							},
						},
					},
				},
			},
		},
		make_label("Size tween"),
		{
			size_x = "620px",
			size_y = "100px",
			free = {},
			c = {
				sizer = {
					size_x = "100px",
					size_y = "40px",
					pos = "c",
					origin = "c",
					background = Colors.indigo700,
					border = {
						thickness = "1px",
						color = Colors.indigo200,
						roundness = "8px",
					},
					text = {
						text = "Resizing",
						height = "18px",
						color = Colors.WHITE,
						justify = "center",
					},
					animation = {
						tracks = {
							{
								target = "size_x",
								from = "100px",
								to = "300px",
								duration = 0.8,
								ease = "in_out_quad",
								loop = true,
								yoyo = true,
							},
						},
					},
				},
			},
		},
		make_label("Sprite sheet"),
		{
			size = "64px",
			sprite = {
				texture = sprite_sheet,
				uv_size_px = { 32, 32 },
				fit = true,
			},
			animation = {
				sprite = {
					frame_size = { 32, 32 },
					frames = 4,
					fps = 6,
					loop = true,
				},
			},
		},
		make_label("Typewriter"),
		{
			name = "typewriter_text",
			text = {
				text = [[Letters reveal without changing layout!
Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Duis nec justo eget felis facilisis fermentum.
Aliquam porttitor mauris sit amet orci. Aenean dignissim pellentesque felis.]],
				visible_chars = 0,
				height = "24px",
				color = Colors.WHITE,
				justify = "center",
			},
			animation = {
				typewriter = {
					cps = 18,
					start_delay = 0.25,
				},
			},
		},
		{
			size_x = "140px",
			size_y = "36px",
			background = Colors.indigo700,
			border = {
				thickness = "1px",
				color = Colors.indigo200,
				roundness = "6px",
			},
			text = {
				text = "Reset text",
				height = "16px",
				color = Colors.WHITE,
				justify = "center",
			},
			on_keys = {
				lmb = function(ew)
					local typewriter = ew:parent():getChild("typewriter_text")
					typewriter:apply("text", { visible_chars = 0 })
					typewriter:apply("animation", {
						typewriter = {
							cps = 18,
							start_delay = 0.25,
						},
					})
				end,
			},
		},
	},
}
