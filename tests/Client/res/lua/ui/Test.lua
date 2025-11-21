local color1 = Colors.RED
local color2 = Colors.YELLOW
local color = color1

return {
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
		local factor = math.abs((C.time / 4) % 2 - 1)
		color = Color.mix(color1, color2, factor)
	end,

	free_container = {},
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

		menu = {
			pos = "c",
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

			blur = {
				radius = 13,
				sigma = 5,
				before_children = true,
			},

			children_base = {
				size_y = "1f",
				text = {
					color = Colors.BLACK,
					border_thickness = 2,
					border_color = Colors.emerald900,
				},
				on_hover_in = function(ew)
					ew:apply("render_scale", 1.02)
				end,
				on_hover_out = function(ew)
					ew:apply("render_scale", 1.0)
				end,
			},

			list = {
				pack = true,
				cross_align = "c",
			},
			c = {
				{
					text = "OneShot",
					on_keys = {
						lmb = function(ew)
							C.Audio.playOneShot("mle/t")
						end,
					},
				},
				{
					text = "StartStream",
					on_keys = {
						lmb = function(ew)
							C.Audio.startStream({
								name = "i/ambient",
								bus = 0,
								id = 0,
							})
						end,
					},
				},
				{
					text = "StopStream",
					on_keys = {
						lmb = function(ew)
							C.Audio.stopStream(0)
						end,
					},
				},
				{
					text = "Volume up",
					on_keys = {
						lmb = function(ew)
							local current_volume = C.Audio.getVolume(0)
							C.Audio.setVolume(0, current_volume + 0.05)
						end,
					},
				},
				{
					text = "Volume down",
					on_keys = {
						lmb = function(ew)
							local current_volume = C.Audio.getVolume(0)
							C.Audio.setVolume(0, current_volume - 0.05)
						end,
					},
				},
			},
		},

		progress_bar = {
			size_y = 0.02,
			size_x = 0.8,
			pos = { "c", 0.9 },
			origin = "cb",

			border = {
				thickness = "3px",
				color = Colors.neutral900,
				roundness = "10px",
			},
			comp = require("mle.ui.comp.progress_bar")(
				Colors.BLACK:withA(0.96),
				Colors.emerald800,
				Colors.emerald300,
				10,
				0.5
			),

			on_hover = function(ew)
				local fn = ew:get("fn", "fill")
				local hovered = ew:get("hovered")
				local x = hovered.pos_self_norm.x
				fn(ew, x)
			end,
		},

		mle = {
			size_x = 0.1,
			pos = "rb",
			origin = "rb",
			margin = "20px",

			sprite = { "mle/mle.png", flip_x = true },
		},
	},
}
