return function(options, right_arrow_texture_name, callback)
	if options == nil or #options == 0 then
		options = { "Option 1", "Option 2", "Option 3" }
	end

	return {
		fn = {
			nextItem = function(ew, direction)
				local self_table = ew:get("table")
				local l_options = self_table.options
				local options_count = #l_options

				self_table.current_index = (self_table.current_index + direction) % options_count
				local new_text = l_options[self_table.current_index + 1]

				local text_ew = ew:getChild("text")
				text_ew:apply("text", new_text)

				if callback then
					callback(new_text)
				end
			end,
		},

		table = {
			options = options,
			current_index = 0,
		},

		list = { dir = "h" },
		c = {
			{
				sprite = { texture = right_arrow_texture_name, flip_x = true },

				on_keys = {
					lmb = function(ew)
						ew:parent():call("nextItem", -1)
					end,
				},
			},
			{
				name = "text",
				text = options[1],
			},
			{
				sprite = { texture = right_arrow_texture_name },

				on_keys = {
					lmb = function(ew)
						ew:parent():call("nextItem", 1)
					end,
				},
			},
		},
	}
end
