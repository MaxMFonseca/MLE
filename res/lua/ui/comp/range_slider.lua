return function(callback, value)
	return {
		fn = {
			onCursorDrag = function(ew, cursor_pos_relative)
				local new_value = 0

				print("Cursor pos relative:", cursor_pos_relative.x)

				if cursor_pos_relative.x < 0 then
					new_value = 0
				elseif cursor_pos_relative.x > 1 then
					new_value = 1
				else
					new_value = cursor_pos_relative.x
				end

				local cursor = ew:getChild("cursor")
				cursor:apply("pos_x", new_value)

				if callback then
					callback(ew, new_value)
				end
			end,
			onCursorDragEnd = function(ew) end,
		},

		on_keys = {
			lmb = function(ew)
				ew:beginCursorDrag()
			end,
		},

		table = {
			value = value or 0,
		},

		free = {},
		c = {
			cursor = {
				aspect_ratio = 1,
				pos = { value or 0, 0.5 },
				origin = "c",

				background = Colors.LIGHT_GRAY,

				border = {
					roundness = 0.5,
					thickness = "1px",
					color = Colors.BLACK,
				},
			},
		},
	}
end
