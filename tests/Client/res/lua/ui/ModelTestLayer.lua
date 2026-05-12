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

local function make_asset_selector(empty_label, options, callback)
	local initial = options[1] or empty_label

	return {
		fn = {
			selectAsset = function(ew, direction)
				local self_table = ew:get("table")
				if #self_table.options == 0 then
					return
				end

				self_table.current_index = (self_table.current_index + direction) % #self_table.options
				local selected = self_table.options[self_table.current_index + 1]

				ew:getChild("name"):apply("text", selected)
				callback(selected)
			end,
			setOptions = function(ew, new_options)
				local self_table = ew:get("table")
				self_table.options = new_options or {}
				self_table.current_index = 0

				local selected = self_table.options[1]
				if selected == nil then
					ew:getChild("name"):apply("text", empty_label)
					return
				end

				ew:getChild("name"):apply("text", selected)
				callback(selected)
			end,
		},
		table = {
			options = options,
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
						ew:parent():call("selectAsset", -1)
					end,
				},
			},
			{
				name = "name",
				size_x = "1f",
				text = initial,
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
						ew:parent():call("selectAsset", 1)
					end,
				},
			},
		},
	}
end

local models = G.model_test_model_names or {}
local animations = G.model_test_animation_names or {}

return {
	free_container = {},
	c = {
		camera_controls = {
			pos = { "20px", "20px" },
			size_x = "34%r",
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
				make_camera_slider("Distance", function(value)
					G.model_test_set_camera_distance(value)
				end),
				{
					text = {
						text = "Model",
						height = "18px",
						color = Colors.slate100,
						border_thickness = 1,
						border_color = Colors.slate950,
					},
				},
				(function()
					local selector = make_asset_selector("No models", models, function(selected)
						G.model_test_set_model(selected)
					end)
					selector.name = "model_selector"
					return selector
				end)(),
				{
					text = {
						text = "Animation",
						height = "18px",
						color = Colors.slate100,
						border_thickness = 1,
						border_color = Colors.slate950,
					},
				},
				(function()
					local selector = make_asset_selector("No animations", animations, function(selected)
						G.model_test_set_animation(selected)
					end)
					selector.name = "animation_selector"
					return selector
				end)(),
				{
					size_x = "1f",
					size_y = "30px",
					text = "Refresh",
					background = Colors.slate800:withA(0.9),
					border = {
						thickness = "1px",
						color = Colors.slate300,
						roundness = "4px",
					},
					on_keys = {
						lmb = function(ew)
							local refreshed = G.model_test_refresh_assets()
							local panel = ew:parent()
							panel:getChild("model_selector"):call("setOptions", refreshed.models or {})
							panel:getChild("animation_selector"):call("setOptions", refreshed.animations or {})
						end,
					},
				},
			},
		},
	},
}
