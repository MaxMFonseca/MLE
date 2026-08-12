local color_picker = require("mle.ui.comp.color_picker")
local range_slider = require("mle.ui.comp.range_slider")
local scrollable_with_bar = require("mle.ui.comp.scrollable_with_bar")

local FONT = {
	small = "26px",
	body = "30px",
	button = "29px",
	section = "33px",
	title = "38px",
}

local RESOURCE_INPUT_NAMES = {
	"model_input",
	"held_item_input",
	"animation_input",
	"attachment_input",
}

local RESOURCE_TEXT_COLOR = {
	inactive = Colors.slate400,
	focused = Colors.WHITE,
	active = Colors.green300,
}

local function text_style(value, height, color, extra)
	local ret = {
		text = value,
		height = height,
		color = color or Colors.slate100,
		border_thickness = 0,
	}
	for key, item in pairs(extra or {}) do
		ret[key] = item
	end
	return ret
end

local function make_label(value, color, height, name)
	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		text = text_style(value, height or FONT.body, color),
	}
end

local function panel_button(value, callback, name)
	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		padding = { "10px", "7px" },
		text = text_style(value, FONT.button, Colors.slate50, { justify = "center" }),
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

local function selector_arrow(value, direction, name)
	local button = panel_button(value, function(ew)
		ew:parent():call("select", direction)
	end, name)
	button.size_x = "44px"
	return button
end

local function make_bool_toggle(name, label, enabled, callback)
	local function button_text(value)
		return label .. ": " .. (value and "On" or "Off")
	end

	local function button_color(value, hovered)
		if value then
			return (hovered and Colors.sky600 or Colors.sky700):withA(0.96)
		end
		return (hovered and Colors.slate700 or Colors.slate800):withA(0.92)
	end

	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		padding = { "10px", "7px" },
		text = text_style(button_text(enabled), FONT.button, Colors.slate50, { justify = "center" }),
		background = button_color(enabled, false),
		border = {
			thickness = "1px",
			color = Colors.slate400,
			roundness = "5px",
		},
		table = {
			enabled = enabled,
		},
		fn = {
			setEnabled = function(ew, value)
				local state = ew:get("table")
				state.enabled = value
				ew:apply("text", button_text(value))
				ew:apply("background", button_color(value, false))
				callback(value)
			end,
			toggle = function(ew)
				ew:call("setEnabled", not ew:get("table").enabled)
			end,
		},
		on_hover_in = function(ew)
			ew:apply("background", button_color(ew:get("table").enabled, true))
		end,
		on_hover_out = function(ew)
			ew:apply("background", button_color(ew:get("table").enabled, false))
		end,
		on_keys = {
			lmb = function(ew)
				ew:call("toggle")
			end,
		},
	}
end

local function make_section(title, children, name)
	local section_children = {
		make_label(title, Colors.sky200, FONT.section),
	}
	for _, child in ipairs(children) do
		table.insert(section_children, child)
	end
	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		padding = "10px",
		background = Colors.slate900:withA(0.76),
		border = {
			thickness = "1px",
			color = Colors.slate600,
			roundness = "6px",
		},
		list = {
			pack = true,
			gap = "8px",
		},
		table = {
			model_test_section = true,
		},
		c = section_children,
	}
end

local function make_value_slider(name, label, minimum, maximum, value, callback)
	local span = maximum - minimum
	local normalized = span > 0 and (value - minimum) / span or 0
	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		list = {
			pack = true,
			gap = "5px",
		},
		table = {
			minimum = minimum,
			maximum = maximum,
			value = value,
		},
		c = {
			make_label(label, Colors.slate200, FONT.small),
			{
				name = "slider",
				size_x = 1,
				size_y = "22px",
				padding = "1px",
				background = Colors.slate950:withA(0.72),
				border = {
					thickness = "1px",
					color = Colors.slate500,
					roundness = "8px",
				},
				comp = range_slider(function(_, new_normalized)
					local new_value = minimum + span * new_normalized
					callback(new_value)
				end, normalized),
			},
		},
	}
end

local function make_normalized_slider(name, label, value, callback)
	return make_value_slider(name, label, 0, 1, value, callback)
end

local function make_cycle_selector(name, empty_label, options, initial_id, callback)
	local initial_index = 1
	for index, option in ipairs(options) do
		if option.id == initial_id then
			initial_index = index
			break
		end
	end
	local initial = options[initial_index]
	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		list = {
			dir = "h",
			cross_align = "start",
			gap = "8px",
		},
		table = {
			options = options,
			current_index = initial_index,
		},
		fn = {
			select = function(ew, direction)
				local state = ew:get("table")
				if #state.options == 0 then
					return
				end
				state.current_index = ((state.current_index - 1 + direction) % #state.options) + 1
				local selected = state.options[state.current_index]
				ew:getChild("value"):apply("text", selected.display_name)
				callback(selected, ew)
			end,
		},
		c = {
			selector_arrow("<", -1, "previous"),
			{
				name = "value",
				size_x = "1f",
				size_y = "fit",
				padding = { "8px", "7px" },
				text = text_style(
					initial and initial.display_name or empty_label,
					FONT.button,
					Colors.WHITE,
					{ justify = "center" }
				),
				background = Colors.slate950:withA(0.78),
				border = {
					thickness = "1px",
					color = Colors.slate500,
					roundness = "5px",
				},
			},
			selector_arrow(">", 1, "next"),
		},
	}
end

local function completion_status(result)
	local message = result.message or ""
	if result.suggestions and #result.suggestions > 0 then
		local suggestions = table.concat(result.suggestions, ", ")
		if message == "" then
			return suggestions
		end
		return message .. " — " .. suggestions
	end
	return message
end

local function make_resource_input(label, name, placeholder, submit, complete)
	return {
		name = name .. "_field",
		size_x = 1,
		size_y = "fit",
		list = {
			pack = true,
			gap = "5px",
		},
		c = {
			make_label(label, Colors.slate200, FONT.small),
			{
				name = name .. "_input",
				size_x = 1,
				size_y = "fit",
				table = {
					active_value = nil,
				},
				padding = { "10px", "8px" },
				background = Colors.slate950:withA(0.78),
				border = {
					thickness = "1px",
					color = Colors.slate500,
					roundness = "5px",
				},
				text = text_style(placeholder, FONT.button, RESOURCE_TEXT_COLOR.inactive, {
					input = {
						on_submit = function(ew, value)
							local activated = submit(value)
							ew:parent():getChild("status"):apply("text", G.model_test_resource_status())
							if activated then
								ew:get("table").active_value = value
								ew:apply("text", { color = RESOURCE_TEXT_COLOR.active })
								ew:apply("text_input_disable")
							end
						end,
						on_complete = function(ew, value)
							local result = complete(value) or {}
							if type(result.replacement) == "string" then
								ew:apply("text_input_set", result.replacement)
							end
							ew:parent():getChild("status"):apply("text", completion_status(result))
						end,
					},
				}),
				on_keys = {
					lmb = function(ew)
						ew:ui():getRoot():getChild("responsive_layout"):call("focusResourceInput", name .. "_input")
					end,
				},
			},
			make_label("", Colors.slate400, FONT.small, "status"),
		},
	}
end

local function make_color_picker_control(name, label, h, s, v, callback)
	return {
		name = name,
		size_x = 1,
		size_y = "fit",
		list = {
			pack = true,
			gap = "5px",
		},
		c = {
			make_label(label, Colors.slate200, FONT.small),
			{
				size_x = 1,
				size_y = "130px",
				comp = color_picker(h, s, v, function(_, new_color)
					callback(new_color)
				end),
			},
		},
	}
end

local function make_color_parameter_row(descriptor, parameter)
	local value = parameter.value or parameter.default or {}
	return make_color_picker_control(
		"shader_parameter_" .. parameter.id,
		parameter.display_name,
		value.h or 0,
		value.s or 0,
		value.v or 1,
		function(new_color)
			G.model_test_set_shader_parameter(descriptor.id, parameter.id, new_color)
		end
	)
end

local function make_shader_parameter_row(descriptor, parameter)
	if parameter.type == "float" then
		return make_value_slider(
			"shader_parameter_" .. parameter.id,
			parameter.display_name,
			parameter.min,
			parameter.max,
			parameter.value,
			function(value)
				G.model_test_set_shader_parameter(descriptor.id, parameter.id, value)
			end
		)
	end
	if parameter.type == "color" then
		return make_color_parameter_row(descriptor, parameter)
	end
	return {
		name = "shader_parameter_" .. parameter.id,
		size_x = 1,
		size_y = "fit",
		list = {
			pack = true,
		},
		c = {
			make_label(
				parameter.display_name .. " (unsupported " .. tostring(parameter.type) .. ")",
				Colors.amber300,
				FONT.small
			),
		},
	}
end

local function make_shader_parameter_rows(descriptor)
	local rows = {}
	for _, parameter in ipairs(descriptor.parameters or {}) do
		table.insert(rows, make_shader_parameter_row(descriptor, parameter))
	end
	return rows
end

local function make_shader_rows()
	return {
		name = "shader_parameter_rows",
		size_x = 1,
		size_y = "fit",
		list = {
			pack = true,
			gap = "8px",
		},
		fn = {
			makeRows = function(_, descriptor)
				return make_shader_parameter_rows(descriptor)
			end,
			rebuild = function(ew, descriptor)
				descriptor = descriptor or G.model_test_get_shader_descriptor()
				ew:destroyAllChildren()
				for _, row in ipairs(make_shader_parameter_rows(descriptor)) do
					ew:addChild(row)
				end
				ew:requestExternalBoundsUpdate()
			end,
		},
		on_create = function(ew)
			ew:call("rebuild", G.model_test_get_shader_descriptor())
		end,
	}
end

local current_shader = G.model_test_get_shader_descriptor()
local shader_rows = make_shader_rows()
local shader_selector = make_cycle_selector(
	"shader_selector",
	"No shader modes",
	G.model_test_shader_modes or {},
	current_shader.id,
	function(selected, ew)
		G.model_test_set_shader_mode(selected.id)
		local descriptor = G.model_test_get_shader_descriptor()
		local section = ew:parent()
		section:getChild("shader_parameter_rows"):call("rebuild", descriptor)
		local status = G.model_test_shader_status()
		section:getChild("shader_status"):apply("text", status ~= "" and status or descriptor.display_name)
	end
)

local held_state = {
	translation = { 0, 0, 0 },
	rotation = { 0, 0, 0 },
	scale = 1,
}

local function set_held_translation(axis, value)
	held_state.translation[axis] = value
	G.model_test_set_held_item_translation(
		held_state.translation[1],
		held_state.translation[2],
		held_state.translation[3]
	)
end

local function set_held_rotation(axis, value)
	held_state.rotation[axis] = value
	G.model_test_set_held_item_rotation(held_state.rotation[1], held_state.rotation[2], held_state.rotation[3])
end

local transform_slider_defaults = {
	held_translation_x = 0.5,
	held_translation_y = 0.5,
	held_translation_z = 0.5,
	held_rotation_x = 0.5,
	held_rotation_y = 0.5,
	held_rotation_z = 0.5,
	held_scale = (1 - 0.1) / (3 - 0.1),
}

local function reset_held_transform(ew)
	held_state.translation = { 0, 0, 0 }
	held_state.rotation = { 0, 0, 0 }
	held_state.scale = 1
	G.model_test_set_held_item_translation(0, 0, 0)
	G.model_test_set_held_item_rotation(0, 0, 0)
	G.model_test_set_held_item_scale(1)
	local section = ew:parent()
	for name, normalized in pairs(transform_slider_defaults) do
		local slider = section:getChild(name):getChild("slider")
		local cursor = slider:getChild("cursor")
		cursor:apply("pos_x", normalized)
		cursor:requestExternalBoundsUpdate()
		slider:requestInternalBoundsUpdate()
	end
end

local assets_section = make_section("Resources", {
	make_resource_input("Model", "model", "i/path/model.glb#Node", function(value)
		return G.model_test_submit_model(value)
	end, function(value)
		return G.model_test_complete_model(value)
	end),
	make_resource_input("Held item", "held_item", "i/path/item.glb#Node", function(value)
		return G.model_test_submit_held_item(value)
	end, function(value)
		return G.model_test_complete_held_item(value)
	end),
	make_resource_input("Animation", "animation", "i/path/model.glb#Clip", function(value)
		return G.model_test_submit_animation(value)
	end, function(value)
		return G.model_test_complete_animation(value)
	end),
	make_resource_input("Attachment node", "attachment", "Hand.R", function(value)
		return G.model_test_submit_attachment(value)
	end, function(value)
		return G.model_test_complete_attachment(value)
	end),
	panel_button("Remove held item", function(ew)
		G.model_test_clear_held_item()
		local field = ew:parent():getChild("held_item_field")
		local input = field:getChild("held_item_input")
		input:get("table").active_value = nil
		input:apply("text_input_disable")
		input:apply("text", { color = RESOURCE_TEXT_COLOR.inactive })
		field:getChild("status"):apply("text", G.model_test_resource_status())
	end, "held_item_clear"),
	panel_button("T pose / clear animation", function(ew)
		G.model_test_clear_animation()
		ew:parent():getChild("animation_field"):getChild("status"):apply("text", G.model_test_resource_status())
	end),
	panel_button("Refresh resource paths", function(ew)
		G.model_test_refresh_resource_paths()
		local status = G.model_test_resource_status()
		local section = ew:parent()
		for _, name in ipairs({ "model_field", "held_item_field", "animation_field", "attachment_field" }) do
			section:getChild(name):getChild("status"):apply("text", status)
		end
	end),
}, "resources_section")

local held_transform_section = make_section("Held item transform", {
	make_value_slider("held_translation_x", "Translation X", -2, 2, 0, function(value)
		set_held_translation(1, value)
	end),
	make_value_slider("held_translation_y", "Translation Y", -2, 2, 0, function(value)
		set_held_translation(2, value)
	end),
	make_value_slider("held_translation_z", "Translation Z", -2, 2, 0, function(value)
		set_held_translation(3, value)
	end),
	make_value_slider("held_rotation_x", "Rotation X", -math.pi, math.pi, 0, function(value)
		set_held_rotation(1, value)
	end),
	make_value_slider("held_rotation_y", "Rotation Y", -math.pi, math.pi, 0, function(value)
		set_held_rotation(2, value)
	end),
	make_value_slider("held_rotation_z", "Rotation Z", -math.pi, math.pi, 0, function(value)
		set_held_rotation(3, value)
	end),
	make_value_slider("held_scale", "Scale", 0.1, 3, 1, function(value)
		held_state.scale = value
		G.model_test_set_held_item_scale(value)
	end),
	panel_button("Reset held transform", reset_held_transform, "held_transform_reset"),
}, "held_transform_section")

local camera_section = make_section("Viewport camera", {
	make_label("Left drag: orbit", Colors.slate200, FONT.small),
	make_label("Middle drag: pan", Colors.slate200, FONT.small),
	make_label("Wheel: zoom", Colors.slate200, FONT.small),
	make_label("Camera input pauses over inspector or focused text.", Colors.slate400, FONT.small),
	panel_button("Reset camera", function()
		G.model_test_reset_camera()
	end, "camera_reset"),
}, "camera_section")

local shader_section = make_section("Shader", {
	shader_selector,
	shader_rows,
	make_label("", Colors.slate400, FONT.small, "shader_status"),
}, "shader_section")

local sun_section = make_section("Sun", {
	make_normalized_slider("sun_yaw", "Direction yaw", 0.403, function(value)
		G.model_test_set_sun_yaw(value)
	end),
	make_normalized_slider("sun_pitch", "Elevation", 0.5, function(value)
		G.model_test_set_sun_pitch(value)
	end),
	make_normalized_slider("sun_intensity", "Intensity", 0.25, function(value)
		G.model_test_set_sun_intensity(value)
	end),
	make_normalized_slider("ambient", "Ambient", 0.18, function(value)
		G.model_test_set_ambient(value)
	end),
}, "sun_section")

local clear_color_section = make_section("Clear color", {
	make_color_picker_control("clear_color_picker", "Background", 0, 0, 1, function(color)
		G.model_test_set_clear_color(color)
	end),
}, "clear_color_section")

local projection_section = make_section("Projection", {
	make_bool_toggle("projection_toggle", "Projection", false, function(enabled)
		G.model_test_set_show_projection(enabled)
	end),
	make_normalized_slider("projection_epsilon", "Simplification", 0, function(value)
		G.model_test_set_projection_epsilon(value * 2)
	end),
	make_color_picker_control("projection_color_picker", "Color", 192 / 360, 1, 1, function(color)
		G.model_test_set_projection_color(color)
	end),
}, "projection_section")

local inspector_content = {
	size_x = 1,
	size_y = "fit",
	padding = "12px",
	list = {
		pack = true,
		gap = "10px",
	},
	c = {
		make_label("Model Test", Colors.WHITE, FONT.title),
		camera_section,
		shader_section,
		sun_section,
		clear_color_section,
		assets_section,
		held_transform_section,
		projection_section,
	},
}

local inspector = scrollable_with_bar(inspector_content, {
	bar_width = "12px",
	min_thumb_px = 32,
	wheel_speed = 48,
})
inspector.name = "panel"
inspector.size_x = "420px"
inspector.size_y = 1
inspector.background = Colors.slate950:withA(0.86)
inspector.border = {
	thickness = "1px",
	color = Colors.slate400,
}

local function apply_responsive_layout(ew)
	local size = ew:getBoundsOnRoot():size()
	local viewport = ew:getChild("viewport")
	local panel = ew:getChild("panel")
	if size.x < 760 then
		ew:apply("list", { dir = "v" })
		viewport:apply("size_x", 1)
		viewport:apply("size_y", "34%")
		panel:apply("size_x", 1)
		panel:apply("size_y", "66%")
	else
		ew:apply("list", { dir = "h" })
		viewport:apply("size_x", "1f")
		viewport:apply("size_y", 1)
		panel:apply("size_x", "420px")
		panel:apply("size_y", 1)
	end
	ew:requestInternalBoundsUpdate()
end

return {
	size = 1,
	list = {},
	on_keys = {
		escape = function()
			G.return_to_init()
		end,
	},
	c = {
		{
			name = "responsive_layout",
			size = 1,
			list = {
				dir = "h",
			},
			fn = {
				focusResourceInput = function(ew, selected_name)
					for _, input_name in ipairs(RESOURCE_INPUT_NAMES) do
						for _, input in ipairs(ew:getChildrenNamedRecursive(input_name)) do
							input:apply("text_input_disable")
							local active_value = input:get("table").active_value
							local color = active_value ~= nil
									and input:get("text") == active_value
									and RESOURCE_TEXT_COLOR.active
								or RESOURCE_TEXT_COLOR.inactive
							input:apply("text", { color = color })
						end
					end
					for _, input in ipairs(ew:getChildrenNamedRecursive(selected_name)) do
						input:apply("text", { color = RESOURCE_TEXT_COLOR.focused })
						input:apply("text_input_enable")
					end
				end,
			},
			on_resized = apply_responsive_layout,
			c = {
				{
					name = "viewport",
					size_x = "1f",
					size_y = 1,
					free = {},
				},
				inspector,
			},
		},
	},
}
