local range_slider = require("mle.ui.comp.range_slider")
local dock = require("mle.ui.comp.dock")

local function panel_button(text, callback)
	return {
		size_x = "1f",
		size_y = "30px",
		text = {
			text = text,
			height = "17px",
			color = Colors.slate50,
			border_thickness = 1,
			border_color = Colors.slate950,
		},
		background = Colors.slate800:withA(0.92),
		border = {
			thickness = "1px",
			color = Colors.slate400,
			roundness = "5px",
		},
		on_hover_in = function(ew)
			ew:apply("background", Colors.slate700:withA(0.96))
		end,
		on_hover_out = function(ew)
			ew:apply("background", Colors.slate800:withA(0.92))
		end,
		on_keys = {
			lmb = callback,
		},
	}
end

local function make_label(text, color, height)
	return {
		size_x = "1f",
		size_y = height or "20px",
		text = {
			text = text,
			height = height or "18px",
			color = color or Colors.slate100,
			border_thickness = 1,
			border_color = Colors.slate950,
		},
	}
end

local function make_slider(label, value, callback)
	return {
		size_x = "1f",
		size_y = "36px",
		list = {
			pack = true,
			gap = "4px",
		},
		c = {
			make_label(label, Colors.slate200, "15px"),
			{
				size_x = "1f",
				size_y = "16px",
				padding = "1px",
				background = Colors.slate950:withA(0.72),
				border = {
					thickness = "1px",
					color = Colors.slate500,
					roundness = "8px",
				},
				comp = range_slider(function(_, new_value)
					callback(new_value)
				end, value),
			},
		},
	}
end

local function make_section(title, children)
	local section_children = {
		make_label(title, Colors.sky200, "19px"),
	}

	for _, child in ipairs(children) do
		table.insert(section_children, child)
	end

	return {
		size_x = "1f",
		padding = "8px",
		background = Colors.slate900:withA(0.76),
		border = {
			thickness = "1px",
			color = Colors.slate600,
			roundness = "6px",
		},
		list = {
			pack = true,
			gap = "6px",
		},
		c = section_children,
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
		size_y = "36px",
		children_base = {
			text = {
				height = "17px",
				color = Colors.WHITE,
				border_thickness = 1,
				border_color = Colors.slate950,
			},
		},
		c = {
			{
				size_x = "32px",
				text = "<",
				background = Colors.slate800:withA(0.92),
				border = {
					thickness = "1px",
					color = Colors.slate400,
					roundness = "5px",
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
				background = Colors.slate950:withA(0.78),
				border = {
					thickness = "1px",
					color = Colors.slate500,
					roundness = "5px",
				},
			},
			{
				size_x = "32px",
				text = ">",
				background = Colors.slate800:withA(0.92),
				border = {
					thickness = "1px",
					color = Colors.slate400,
					roundness = "5px",
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

local function make_shader_options_dock()
	local shader_options = dock()
	shader_options.name = "shader_options"
	shader_options.size_x = "1f"
	shader_options.size_y = "0px"
	shader_options.on_create = function(ew)
		ew:call("setPanel", "PBR")
		ew:apply("size_y", "0px")
	end
	shader_options.c = {
		PBR = {
			name = "PBR",
			size_x = "1f",
			size_y = "0px",
		},
		Cartoon = {
			name = "Cartoon",
			size_x = "1f",
			size_y = "36px",
			disabled = true,
			list = {
				pack = true,
			},
			c = {
				make_slider("Outline width", 0.267, function(value)
					G.model_test_set_outline_width(value)
				end),
			},
		},
		Wireframe = {
			name = "Wireframe",
			size_x = "1f",
			size_y = "36px",
			disabled = true,
			list = {
				pack = true,
			},
			c = {
				make_slider("Line width", 0.071, function(value)
					G.model_test_set_wireframe_width(value)
				end),
			},
		},
		Normals = {
			name = "Normals",
			size_x = "1f",
			size_y = "0px",
			disabled = true,
		},
	}
	return shader_options
end

local models = G.model_test_model_names or {}
local animations = G.model_test_animation_names or {}
local shader_modes = G.model_test_shader_mode_names or { "PBR", "Cartoon", "Wireframe", "Normals" }

return {
	free_container = {},
	c = {
		panel = {
			pos = { "18px", "18px" },
			size_x = "360px",
			padding = "10px",
			background = Colors.slate950:withA(0.62),
			border = {
				thickness = "1px",
				color = Colors.slate400,
				roundness = "7px",
			},
			list = {
				pack = true,
				gap = "8px",
			},
			c = {
				make_label("Model Test", Colors.WHITE, "22px"),
				make_section("Camera", {
					make_slider("Orbit yaw", 0.5, function(value)
						G.model_test_set_camera_yaw(value)
					end),
					make_slider("Orbit pitch", 0.5, function(value)
						G.model_test_set_camera_pitch(value)
					end),
					make_slider("Distance", 0.01, function(value)
						G.model_test_set_camera_distance(value)
					end),
				}),
				make_section("Shader", {
					(function()
						local selector = make_asset_selector("No shader modes", shader_modes, function(selected)
							G.model_test_set_shader_mode(selected)
						end)
						selector.name = "shader_selector"
						selector.fn.setOptionsPanel = function(ew, selected)
							local shader_options = ew:parent():getChild("shader_options")
							shader_options:call("setPanel", selected)
							if selected == "Cartoon" or selected == "Wireframe" then
								shader_options:apply("size_y", "36px")
							else
								shader_options:apply("size_y", "0px")
							end
						end
						local old_select_asset = selector.fn.selectAsset
						selector.fn.selectAsset = function(ew, direction)
							old_select_asset(ew, direction)
							local selected = ew:get("table").options[ew:get("table").current_index + 1]
							ew:call("setOptionsPanel", selected)
						end
						local old_set_options = selector.fn.setOptions
						selector.fn.setOptions = function(ew, new_options)
							old_set_options(ew, new_options)
							local selected = ew:get("table").options[ew:get("table").current_index + 1]
							ew:call("setOptionsPanel", selected or "PBR")
						end
						return selector
					end)(),
					make_shader_options_dock(),
				}),
				make_section("Sun", {
					make_slider("Direction yaw", 0.403, function(value)
						G.model_test_set_sun_yaw(value)
					end),
					make_slider("Elevation", 0.5, function(value)
						G.model_test_set_sun_pitch(value)
					end),
					make_slider("Intensity", 0.25, function(value)
						G.model_test_set_sun_intensity(value)
					end),
					make_slider("Ambient", 0.18, function(value)
						G.model_test_set_ambient(value)
					end),
				}),
				make_section("Assets", {
					make_label("Model", Colors.slate200, "15px"),
					(function()
						local selector = make_asset_selector("No GLB models", models, function(selected)
							G.model_test_set_model(selected)
						end)
						selector.name = "model_selector"
						return selector
					end)(),
					make_label("Animation", Colors.slate200, "15px"),
					(function()
						local selector = make_asset_selector("No GLB animations", animations, function(selected)
							G.model_test_set_animation(selected)
						end)
						selector.name = "animation_selector"
						return selector
					end)(),
					panel_button("Refresh assets", function(ew)
						local refreshed = G.model_test_refresh_assets()
						local assets = ew:parent()
						assets:getChild("model_selector"):call("setOptions", refreshed.models or {})
						assets:getChild("animation_selector"):call("setOptions", refreshed.animations or {})
					end),
				}),
			},
		},
	},
}
