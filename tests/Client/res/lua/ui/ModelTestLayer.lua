local function make_camera_slider(label, callback)
	return {
		list = {
			pack = true,
			gap = "6px",
		},
		size_y = "50px",
		c = {
			label = {
				text = {
					text = label,
					height = "18px",
					color = Colors.slate100,
					border_thickness = 1,
					border_color = Colors.slate900,
				},
			},
			slider = {
				size_x = "250px",
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

return {
	free_container = {},
	c = {
		camera_controls = {
			pos = { "20px", "20px" },
			size_x = "300px",
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
				title = {
					text = {
						text = "Camera",
						height = "20px",
						color = Colors.WHITE,
					},
				},
				yaw = make_camera_slider("Yaw", function(value)
					G.model_test_set_camera_yaw(value)
				end),
				pitch = make_camera_slider("Pitch", function(value)
					G.model_test_set_camera_pitch(value)
				end),
			},
		},
	},
}
