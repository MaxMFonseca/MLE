local function make_button(text, color, onClick)
	return {
		size_y = "24px",
		background = color,
		border = {
			thickness = "2px",
			color = Colors.slate300,
			roundness = "4px",
		},
		text = {
			text = text,
			color = Colors.WHITE,
		},
		on_keys = {
			lmb = onClick,
		},
	}
end

local function make_popup_content(level)
	return {
		background = Colors.slate800,
		border = {
			thickness = "2px",
			color = Colors.slate400,
			roundness = "8px",
		},
		padding = "8px",

		-- Random position
		pos = {
			(math.random() * 0.7 + 0.15) .. "r",
			(math.random() * 0.7 + 0.15) .. "r",
		},

		list = {
			dir = "v",
			gap = "4px",
			cross_align = "c",
			pack = true,
		},
		c = {
			{
				text = {
					text = "Lvl " .. level,
					height = "24px",
					color = Colors.sky400,
				},
			},
			make_button("+", Colors.sky700, function(ew)
				ew:createPopup(make_popup_content(level + 1))
			end),
			make_button("x", Colors.rose700, function(ew)
				ew:parent():destroy()
			end),
		},
	}
end

return {
	free = {},
	size = 1,
	c = {
		icon = {
			pos = { "10px", "10px" },
			size_x = "64px",
			aspect_ratio = 1,
			sprite = "mle/mle.png",
			on_keys = {
				lmb = function(ew)
					ew:createPopup(make_popup_content(1))
				end,
			},
		},

		instructions = {
			pos = "c",
			origin = "c",
			text = {
				text = "Click icon in top-left to start popup stack test.",
				height = "20px",
				color = Colors.slate400,
			},
		},
	},
}
