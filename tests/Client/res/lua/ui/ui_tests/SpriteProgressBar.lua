local progress_bar = require("mle.ui.comp.sprite_progress_bar")
local range_slider = require("mle.ui.comp.range_slider")

local progress_texture_dir = "i/ui/progress_bar/"

local function make_progress_bar(name, initial_value)
	return {
		name = name,
		size_x = "520px",
		size_y = "56px",
		comp = progress_bar({
			value = initial_value,
			left = progress_texture_dir .. "left.png",
			center = progress_texture_dir .. "mid.png",
			fill = progress_texture_dir .. "fill.png",
			sparkle = progress_texture_dir .. "sparkle.png",
		}),
	}
end

return {
	background = Colors.slate700,
	size = 1,
	list = {
		dir = "v",
		cross_align = "c",
		gap = "18px",
	},
	padding = "40px",
	c = {
		{
			name = "title",
			text = {
				text = "Sprite progress bar",
				height = "30px",
				color = Colors.WHITE,
			},
		},

		make_progress_bar("sprite_progress_bar", 0.5),

		{
			name = "range_selector",
			size_x = "520px",
			size_y = "32px",
			padding = "4px",
			background = Colors.slate900:withA(0.55),
			border = {
				thickness = "1px",
				color = Colors.slate400,
				roundness = "5px",
			},
			comp = range_slider(function(ew, value)
				ew:parent():getChild("sprite_progress_bar"):call("set_value", value)
			end, 0.5),
		},
	},
}
