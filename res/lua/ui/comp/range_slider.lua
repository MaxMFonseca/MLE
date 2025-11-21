return function()
	return {
		fn = {
			onCursorDrag = function(ew, cursor_pos_relative)
				local new_value = 0

				if cursor_pos_relative.x < 0 then
					new_value = 0
				elseif cursor_pos_relative.x > 1 then
					new_value = 1
				else
					new_value = cursor_pos_relative.x
				end

				local cursor = ew:getChild("cursor")
				cursor:apply("pos_x", new_value)
			end,
			onCursorDragEnd = function(ew) end,
		},

		on_keys = {
			lmb = function(ew)
				ew:beginCursorDrag()
			end,
		},

		free = {},
		c = {
			{
				pos = "c",
				origin = "c",

				background = Colors.neutral200,

				border = {
					roundness = 0.5,
					thickness = "3px",
					color = Colors.neutral800,
				},
			},

			{
				name = "cursor",
				aspect_ratio = 1,
				pos = "c",
				origin = "c",

				background = Colors.BLACK,

				border = {
					roundness = 0.5,
					thickness = "1px",
					color = Colors.red600,
				},
			},
		},
	}
end
