local dropdown = require("mle.ui.comp.text_dropdown_selector")

local text_fields = {
	"name_input",
	"email_input",
	"notes_input",
}

local function make_label(text)
	return {
		size_x = "420px",
		size_y = "20px",
		text = {
			text = text,
			height = "15px",
			color = Colors.slate100,
		},
	}
end

local function make_text_input(name, placeholder, multiline)
	return {
		name = name,
		size_x = "420px",
		size_y = multiline and "92px" or "38px",
		padding = { "10px", "8px" },
		background = Colors.slate800,
		border = {
			thickness = "1px",
			color = Colors.slate400,
			roundness = "5px",
		},
		text = {
			text = placeholder,
			height = "16px",
			color = Colors.WHITE,
			input = {
				multiline = multiline,
				ctrl_enter_newline = true,
			},
		},
		on_keys = {
			lmb = function(ew)
				ew:parent():call("focusTextInput", name)
			end,
		},
	}
end

local function make_checkbox(name, label, checked)
	return {
		name = name,
		size_x = "420px",
		size_y = "34px",
		padding = { "10px", "6px" },
		background = Colors.slate800,
		border = {
			thickness = "1px",
			color = Colors.slate500,
			roundness = "5px",
		},
		text = {
			text = (checked and "[x] " or "[ ] ") .. label,
			height = "16px",
			color = Colors.WHITE,
		},
		table = {
			label = label,
			checked = checked,
		},
		fn = {
			setChecked = function(ew, value)
				local state = ew:get("table")
				state.checked = value
				ew:apply("text", (value and "[x] " or "[ ] ") .. state.label)
			end,
			toggle = function(ew)
				ew:call("setChecked", not ew:get("table").checked)
			end,
		},
		on_keys = {
			lmb = function(ew)
				ew:call("toggle")
			end,
		},
	}
end

local function make_select(name, options)
	return {
		name = name,
		size_x = "420px",
		size_y = "38px",
		padding = { "10px", "8px" },
		background = Colors.slate800,
		border = {
			thickness = "1px",
			color = Colors.slate400,
			roundness = "5px",
		},
		comp = dropdown(options, nil, "bottom_left"),
	}
end

local function get_selected(form, name)
	return form:getChild(name):get("table").selected
end

return {
	background = Colors.slate700,
	size = 1,
	padding = "28px",
	list = {
		dir = "v",
		cross_align = "c",
		gap = "8px",
	},
	table = {
		text_fields = text_fields,
	},
	fn = {
		focusTextInput = function(ew, name)
			for _, field_name in ipairs(ew:get("table").text_fields) do
				ew:getChild(field_name):apply("text_input_disable")
			end
			ew:getChild(name):apply("text_input_enable")
		end,
		submit = function(ew)
			local name = ew:getChild("name_input"):get("text")
			local email = ew:getChild("email_input"):get("text")
			local notes = ew:getChild("notes_input"):get("text")
			local subscribed = ew:getChild("subscribe_checkbox"):get("table").checked
			local beta = ew:getChild("beta_checkbox"):get("table").checked
			local role = get_selected(ew, "role_select")
			local region = get_selected(ew, "region_select")
			local priority = get_selected(ew, "priority_select")

			print(
				"FormPanel submit:",
				"name=" .. name,
				"email=" .. email,
				"notes=" .. notes,
				"subscribed=" .. tostring(subscribed),
				"beta=" .. tostring(beta),
				"role=" .. role,
				"region=" .. region,
				"priority=" .. priority
			)

			ew:getChild("status"):apply("text", "Submitted. Values printed to console.")
		end,
	},
	c = {
		{
			name = "title",
			size_x = "420px",
			size_y = "28px",
			text = {
				text = "Form panel",
				height = "24px",
				color = Colors.WHITE,
			},
		},

		make_label("Name"),
		make_text_input("name_input", "Enter a display name", false),

		make_label("Email"),
		make_text_input("email_input", "Enter an email address", false),

		make_label("Notes"),
		make_text_input("notes_input", "Add notes", true),

		make_checkbox("subscribe_checkbox", "Subscribe to updates", true),
		make_checkbox("beta_checkbox", "Enable beta features", false),

		make_label("Role"),
		make_select("role_select", {
			"Designer",
			"Engineer",
			"Producer",
			"QA",
		}),

		make_label("Region"),
		make_select("region_select", {
			"Americas",
			"Europe",
			"Asia Pacific",
			"Remote",
		}),

		make_label("Priority"),
		make_select("priority_select", {
			"Low",
			"Normal",
			"High",
			"Urgent",
		}),

		{
			name = "submit_button",
			size_x = "420px",
			size_y = "40px",
			background = Colors.slate600,
			border = {
				thickness = "1px",
				color = Colors.slate300,
				roundness = "5px",
			},
			text = {
				text = "Submit",
				height = "17px",
				color = Colors.WHITE,
			},
			on_keys = {
				lmb = function(ew)
					ew:parent():call("submit")
				end,
			},
		},

		{
			name = "status",
			size_x = "420px",
			size_y = "24px",
			text = {
				text = "Fill the form and submit.",
				height = "14px",
				color = Colors.slate100,
			},
		},
	},
}
