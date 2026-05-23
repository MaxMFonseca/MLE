local dropdown = require("mle.ui.comp.text_dropdown_selector")

local function make_selector(name, origin, options)
	return {
		name = name,
		size_x = "320px",
		size_y = "38px",
		padding = { "12px", "8px" },
		background = Colors.slate800,
		border = {
			thickness = "1px",
			color = Colors.slate400,
			roundness = "5px",
		},
		comp = dropdown(options, function(ew, selected)
			local status = ew:parent():getChild("status")
			status:apply("text", name .. " selected: " .. selected)
		end, origin),
	}
end

return {
	background = Colors.slate700,
	size = 1,
	list = {
		dir = "v",
		cross_align = "c",
		gap = "14px",
	},
	padding = "32px",
	c = {
		{
			name = "title",
			text = {
				text = "Text dropdown selector",
				height = "30px",
				color = Colors.WHITE,
			},
		},

		make_selector("bottom_left_selector", "bottom_left", {
			"Warrior",
			"Ranger",
			"Mage",
			"Cleric",
		}),

		make_selector("top_right_selector", "top_right", {
			"Small",
			"Medium",
			"Large",
			"Extra Large",
		}),

		{
			name = "replace_options",
			size_x = "320px",
			size_y = "36px",
			background = Colors.slate600,
			border = {
				thickness = "1px",
				color = Colors.slate300,
				roundness = "5px",
			},
			text = {
				text = "Replace bottom-left options",
				height = "16px",
				color = Colors.WHITE,
			},
			on_keys = {
				lmb = function(ew)
					ew:parent():getChild("bottom_left_selector"):call("set_options", {
						options = {
							"Oak",
							"Ash",
							"Birch",
						},
						selected = "Ash",
					})
				end,
			},
		},

		{
			name = "status",
			size_x = "320px",
			size_y = "28px",
			text = {
				text = "No selection yet",
				height = "15px",
				color = Colors.slate100,
			},
		},
	},
}
