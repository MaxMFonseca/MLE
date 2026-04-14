return function(h, s, v, callback)
	if h >= 1 then
		h = 0.999
	end

	local h360 = h * 360
	local color = Color.fromHSV(h360, s, v)

	return {
		fn = {
			setColor = function(ew, c)
				local self_table = ew:get("table")

				if c.h then
					self_table.h = c.h * 360
					if self_table.h >= 360 then
						self_table.h = 359.999
					end
				end

				self_table.s = c.s or self_table.s
				self_table.v = c.v or self_table.v

				local new_h = self_table.h
				local new_s = self_table.s
				local new_v = self_table.v

				local new_color = Color.fromHSV(new_h, new_s, new_v)
				local new_s_color = Color.fromHSV(new_h, 1, new_v)
				local new_v_color = Color.fromHSV(new_h, new_s, 1)

				local view = ew:getChild("view")
				view:apply("background", new_color)

				local s_slider = ew:getChild("s_slider")
				local s_bg = s_slider:getChild("bg")
				s_bg:apply("background", {
					rt = new_s_color,
					rb = new_s_color,
				})

				local v_slider = ew:getChild("v_slider")
				local v_bg = v_slider:getChild("bg")
				v_bg:apply("background", {
					rt = new_v_color,
					rb = new_v_color,
				})

				callback(ew, new_color)
			end,
		},

		table = {
			h = h * 360,
			s = s,
			v = v,
		},

		list = {
			gap = 10,
		},
		c = {
			{
				name = "view",
				size_y = "3f",
				size_x = 1,
				background = color,
			},

			{
				name = "h_slider",
				size_y = "1f",
				size_x = 1,

				comp = table.deep_merge(
					require("mle.ui.comp.range_slider")(function(ew, new_h)
						ew:parent():call("setColor", { h = new_h })
					end, h),
					{
						padding = "5px",
						c = {
							cursor = {
								size_y = 1,
								aspect_ratio = 0.2,
							},
							bg = {
								layer = -1,
								size = 1,
								sprite = "mle/hue_strip.png",
								border = {
									roundness = "10px",
									thickness = "1px",
									color = Colors.neutral900,
								},
							},
						},
					}
				),
			},

			{
				name = "s_slider",
				size = 1,

				comp = table.deep_merge(
					require("mle.ui.comp.range_slider")(function(ew, new_s)
						ew:parent():call("setColor", { s = new_s })
					end, s),
					{
						padding = "5px",
						c = {
							cursor = {
								size_y = 1,
								aspect_ratio = 0.2,
							},
							bg = {
								layer = -1,
								size = 1,
								background = {
									lt = Colors.WHITE,
									lb = Colors.WHITE,
									rb = Color.fromHSV(h360, 1, v),
									rt = Color.fromHSV(h360, 1, v),
								},
								border = {
									roundness = "10px",
									thickness = "1px",
									color = Colors.neutral900,
								},
							},
						},
					}
				),
			},

			{
				name = "v_slider",
				size = 1,

				comp = table.deep_merge(
					require("mle.ui.comp.range_slider")(function(ew, new_v)
						ew:parent():call("setColor", { v = new_v })
					end, v),
					{
						padding = "5px",
						c = {
							cursor = {
								size_y = 1,
								aspect_ratio = 0.2,
							},
							bg = {
								layer = -1,
								size = 1,
								background = {
									lt = Colors.BLACK,
									lb = Colors.BLACK,
									rb = Color.fromHSV(h360, s, 1),
									rt = Color.fromHSV(h360, s, 1),
								},
								border = {
									roundness = "10px",
									thickness = "1px",
									color = Colors.neutral900,
								},
							},
						},
					}
				),
			},
		},
	}
end
