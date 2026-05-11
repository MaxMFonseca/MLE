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
					text = "ModelTest",
					on_keys = {
						lmb = function(ew)
							G.init_model_test()
						end,
					},
				},
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

		range_slider_test = {
			padding = "10px",
			size_x = 0.6,
			size_y = 0.03,
			pos = { "c", 0.9 },
			origin = "c",

			comp = require("mle.ui.comp.range_slider")(),
		},

		-- carousel_test = {
		--
		-- 	size_y = 0.1,
		-- 	pos = { "c", 0.9 },
		-- 	origin = "cb",
		--
		-- 	list = { justify = "c", cross_align = "c", gap = 10, pack = true },
		--
		-- 	comp = require("mle.ui.comp.carousel_selector")(
		-- 		{
		-- 			"Lorem",
		-- 			"Ipsum",
		-- 			"Dolor",
		-- 			"Sit",
		-- 			"Amet",
		-- 			"Consectetur",
		-- 		},
		-- 		"mle/mle.png",
		--
		-- 		function(new_text)
		-- 			print("Selected:", new_text)
		-- 		end
		-- 	),
		-- },

		-- progress_bar = {
		-- 	size_y = 0.02,
		-- 	size_x = 0.8,
		-- 	pos = { "c", 0.9 },
		-- 	origin = "cb",
		--
		-- 	border = {
		-- 		thickness = "3px",
		-- 		color = Colors.neutral900,
		-- 		roundness = "10px",
		-- 	},
		-- 	comp = require("mle.ui.comp.progress_bar")(
		-- 		Colors.BLACK:withA(0.96),
		-- 		Colors.emerald800,
		-- 		Colors.emerald300,
		-- 		10,
		-- 		0.5
		-- 	),
		--
		-- 	on_hover = function(ew)
		-- 		local fn = ew:get("fn", "fill")
		-- 		local hovered = ew:get("hovered")
		-- 		local x = hovered.pos_self_norm.x
		-- 		fn(ew, x)
		-- 	end,
		-- },
		--
		mle = {
			size_x = 0.1,
			pos = "rb",
			origin = "rb",
			margin = "20px",

			background = {
				lt = Colors.emerald900,
				rt = Colors.red900,
				lb = Colors.blue900,
				rb = Colors.yellow900,
			},

			sprite = { "mle/mle.png", flip_x = true },
		},

		color_picker_test = {
			margin = "20px",
			size_y = 0.3,
			aspect_ratio = 1,

			pos = "lb",
			origin = "lb",

			comp = require("mle.ui.comp.color_picker")(0.5, 0.5, 0.5),
		},

		color_picker_test_2 = {
			margin = "20px",
			size_y = 0.1,
			aspect_ratio = 1,
			background = Color.fromHSV(0.0, 1.0, 1.0),

			table = {
				color = Color.fromHSV(0.0, 1.0, 1.0),
			},

			fn = {
				set_color = function(ew, color)
					ew:apply("background", color)
					ew:get("table").color = color
				end,
			},

			on_keys = {
				lmb = function(ew)
					local ew_table = ew:get("table")
					local initial_color = ew_table.color
					local hsv = initial_color:toHSV()
					local h = hsv.x / 360
					local s = hsv.y
					local v = hsv.z

					local local_bounds = ew:getBoundsOnRootNormalized()
					local target_pos = {
						x = local_bounds:pos().x + local_bounds:size().x,
						y = local_bounds:pos().y + local_bounds:size().y / 2,
					}
					ew:createPopup({
						child = {
							pos = target_pos,

							size_y = 0.2,
							size_x = 0.1,

							padding = "10px",

							border = {
								thickness = "3px",
								color = Colors.slate400,
								roundness = "10px",
							},

							on_keys = {
								lmb = function(popup_ew)
									-- dummy to prevent propagation
								end,
							},

							comp = require("mle.ui.comp.color_picker")(h, s, v, function(ew, color)
								local owner = ew:parent():get("table").owner
								owner:call("set_color", color)
							end),
						},
					})
				end,
			},
		},
	},
}
