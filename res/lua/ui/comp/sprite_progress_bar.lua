local function clamp01(value)
	return math.max(0, math.min(1, value or 0))
end

local function render_value(value)
	if value <= 0 then
		return 0.001
	end

	return value
end

return function(opts)
	opts = opts or {}

	local value = clamp01(opts.value)
	local initial_render_value = render_value(value)
	local left_texture = opts.left or opts.left_texture
	local center_texture = opts.center or opts.center_texture
	local fill_texture = opts.fill or opts.fill_texture
	local sparkle_texture = opts.sparkle or opts.sparkle_texture
	local track_children = {
		{
			name = "center",
			layer = -2,
			size = 1,
			sprite = {
				texture = center_texture,
				fit = true,
			},
		},

		{
			name = "fill",
			layer = -1,
			size_x = initial_render_value,
			size_y = 1,
			sprite = {
				texture = fill_texture,
				fit = true,
				uv_size = { initial_render_value, 1 },
			},
		},
	}

	if sparkle_texture then
		table.insert(track_children, {
			name = "sparkle",
			layer = 1,
			size_x = "fit",
			size_y = 1,
			pos = { value, "c" },
			origin = "c",
			sprite = {
				texture = sparkle_texture,
			},
		})
	end

	return {
		fn = {
			set_value = function(ew, new_value)
				local clamped_value = clamp01(new_value)
				local self_table = ew:get("table")
				self_table.value = clamped_value

				local track = ew:getChild("track")
				local fill = track:getChild("fill")
				local fill_render_value = render_value(clamped_value)

				fill:apply("size_x", fill_render_value)
				fill:apply("sprite", {
					texture = fill_texture,
					fit = true,
					uv_size = { fill_render_value, 1 },
				})

				if sparkle_texture then
					track:getChild("sparkle"):apply("pos_x", clamped_value)
				end
			end,
		},

		table = {
			value = value,
		},

		list = {
			dir = "h",
		},

		c = {
			{
				name = "left_edge",
				size_x = "fit",
				size_y = 1,
				sprite = {
					texture = left_texture,
				},
			},

			{
				name = "track",
				size_x = "1f",
				size_y = 1,
				free = {},
				c = track_children,
			},

			{
				name = "right_edge",
				size_x = "fit",
				size_y = 1,
				sprite = {
					texture = left_texture,
					flip_x = true,
				},
			},
		},
	}
end
