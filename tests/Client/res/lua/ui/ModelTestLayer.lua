local function make_camera_slider(label, callback)
	return {
		list = {
			pack = true,
			gap = "6px",
		},
		size_x = "1f",
		size_y = "50px",
		c = {
			{
				text = {
					text = label,
					height = "18px",
					color = Colors.slate100,
					border_thickness = 1,
					border_color = Colors.slate900,
				},
			},
			{
				size_x = "1f",
				size_y = "14px",
				background = Colors.slate800:withA(0.86),
				border = {
					thickness = "1px",
					color = Colors.slate300,
					roundness = "7px",
				},
				comp = require("mle.ui.comp.range_slider")(function(_, value)
					callback(value)
				end, 0.5),
			},
		},
	}
end

local animations = G.model_test_animation_names or {}

local function make_animation_selector()
	local initial_animation = animations[1] or "No animations"

	return {
		fn = {
			selectAnimation = function(ew, direction)
				local self_table = ew:get("table")
				if #self_table.options == 0 then
					return
				end

				self_table.current_index = (self_table.current_index + direction) % #self_table.options
				local selected = self_table.options[self_table.current_index + 1]

				ew:getChild("name"):apply("text", selected)
				G.model_test_set_animation(selected)
			end,
		},
		table = {
			options = animations,
			current_index = 0,
		},
		list = {
			dir = "h",
			cross_align = "c",
			gap = "8px",
		},
		size_x = "1f",
		size_y = "34px",
		padding = "2px",
		children_base = {
			text = {
				height = "22px",
				color = Colors.WHITE,
				border_thickness = 1,
				border_color = Colors.slate950,
			},
		},
		c = {
			{
				size_x = "28px",
				text = "<",
				background = Colors.slate800:withA(0.9),
				border = {
					thickness = "1px",
					color = Colors.slate300,
					roundness = "4px",
				},
				on_keys = {
					lmb = function(ew)
						ew:parent():call("selectAnimation", -1)
					end,
				},
			},
			{
				name = "name",
				size_x = "1f",
				text = initial_animation,
				background = Colors.slate900:withA(0.86),
				border = {
					thickness = "1px",
					color = Colors.slate500,
					roundness = "4px",
				},
			},
			{
				size_x = "28px",
				text = ">",
				background = Colors.slate800:withA(0.9),
				border = {
					thickness = "1px",
					color = Colors.slate300,
					roundness = "4px",
				},
				on_keys = {
					lmb = function(ew)
						ew:parent():call("selectAnimation", 1)
					end,
				},
			},
		},
	}
end

return {
	free_container = {},
	c = {
		camera_controls = {
			pos = { "20px", "20px" },
			size_x = "30%r",
			padding = "12px",
			background = Colors.slate900:withA(0.58),
			border = {
				thickness = "1px",
				color = Colors.slate400,
				roundness = "6px",
			},
			list = {
				pack = true,
				gap = "10px",
			},
			c = {
				{
					text = {
						text = "Camera",
						height = "20px",
						color = Colors.WHITE,
					},
				},
				make_camera_slider("Yaw", function(value)
					G.model_test_set_camera_yaw(value)
				end),
				make_camera_slider("Pitch", function(value)
					G.model_test_set_camera_pitch(value)
				end),
				{
					text = {
						text = "Animation",
						height = "18px",
						color = Colors.slate100,
						border_thickness = 1,
						border_color = Colors.slate950,
					},
				},
				make_animation_selector(),
			},
		},
	},
}
