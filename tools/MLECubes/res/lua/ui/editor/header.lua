return function(extra)
	local i = table.deep_copy(extra)

	i.padding = 5

	i.c = {
		direction = "row",
		align = "center",
		gap = 5,
	}

	local idx = 0
	local lGen = function(text)
		idx = idx + 1
		local color = "cyan" .. idx .. "00"
		table.insert(i.c, {
			text = {
				text = text,
				color = color,
				outline = "BLACK:5",
			},
			size_y = "100%",
			on_hover_enter = function(self)
				self:apply("text", { outline_thickness = 15 })
			end,
			on_hover_leave = function(self)
				self:apply("text", { outline_thickness = 5 })
			end,
			on_click = function()
				mle.audio.play("i/menu_click.flac")
			end,
		})
	end

	lGen("lorem")
	lGen("ipsum")
	lGen("dolor")
	lGen("sit")
	lGen("amet")
	lGen("consectetur")
	lGen("adipiscing")
	lGen("elit")
	lGen("sed")

	return i
end
