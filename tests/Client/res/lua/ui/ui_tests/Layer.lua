local function create_multipanel()
	local panel_names = {
		"Scrollable",
		"TextDropdown",
		"SpriteProgressBar",
		"Form",
		"PopupStack",
	}

	local ret = require("mle/ui/comp/multipanel")({
		{
			name = panel_names[1],
			comp = require("i/ui/ui_tests/Scrollable"),
		},
		{
			name = panel_names[2],
			comp = require("i/ui/ui_tests/TextDropdownSelector"),
		},
		{
			name = panel_names[3],
			comp = require("i/ui/ui_tests/SpriteProgressBar"),
		},
		{
			name = panel_names[4],
			comp = require("i/ui/ui_tests/FormPanel"),
		},
		{
			name = panel_names[5],
			comp = require("i/ui/ui_tests/PopupStack"),
		},
	}, true)

	ret.size = 1
	ret.table = {
		panel_index = 1,
		panel_names = panel_names,
	}
	ret.fn.setPanelByOffset = function(ew, offset)
		local ew_table = ew:get("table")
		local next_index = ((ew_table.panel_index - 1 + offset) % #ew_table.panel_names) + 1
		ew_table.panel_index = next_index
		ew:call("setPanel", ew_table.panel_names[next_index])
	end
	ret.on_keys = {
		left = function(ew)
			ew:call("setPanelByOffset", -1)
		end,
		right = function(ew)
			ew:call("setPanelByOffset", 1)
		end,
		escape = function()
			G.return_to_init()
		end,
	}

	return ret
end

return create_multipanel()
