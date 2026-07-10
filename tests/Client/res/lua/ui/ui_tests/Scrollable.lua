local next_row = 0

local function content(id)
	return {
		id = id,
		background = Colors.slate700:withA(0.3),
		list = {
			dir = "v",
			cross_align = "c",
		},
	}
end

local function make_row(index, width, color)
	return {
		name = "child_" .. index,
		size_y = "5%r",
		size_x = width,
		background = color,
		on_keys = {
			lmb = function(ew) end,
		},
	}
end

local function add_row()
	next_row = next_row + 1
	local width = 0.3 + math.random() * 0.7
	local color = Color.random()

	local legacy = G.ui:getElementById("legacy_scroll_content")
	legacy:addChild(make_row(next_row, width, color))
	legacy:requestInternalBoundsUpdate()

	local with_bar = G.ui:getElementById("bar_scroll_content")
	with_bar:addChild(make_row(next_row, width, color))
	with_bar:requestInternalBoundsUpdate()
end

local function clear_rows()
	local legacy = G.ui:getElementById("legacy_scroll_content")
	legacy:destroyAllChildren()
	legacy:requestInternalBoundsUpdate()

	local with_bar = G.ui:getElementById("bar_scroll_content")
	with_bar:destroyAllChildren()
	with_bar:requestInternalBoundsUpdate()

	next_row = 0
end

local legacy = require("mle.ui.comp.scrollable")(content("legacy_scroll_content"))
legacy.name = "legacy"
legacy.size = { 0.4, 0.72 }
legacy.pos = { 0.25, 0.46 }
legacy.origin = "c"

local with_bar = require("mle.ui.comp.scrollable_with_bar")(content("bar_scroll_content"))
with_bar.name = "with_bar"
with_bar.size = { 0.4, 0.72 }
with_bar.pos = { 0.75, 0.46 }
with_bar.origin = "c"

return {
	background = Colors.slate600,
	size = 1,
	free = {},
	c = {
		legacy,
		with_bar,
		{
			name = "legacy_label",
			text = "Scrollable",
			size = { 0.4, "5%r" },
			pos = { 0.25, 0.06 },
			origin = "c",
		},
		{
			name = "bar_label",
			text = "Scrollable with bar",
			size = { 0.4, "5%r" },
			pos = { 0.75, 0.06 },
			origin = "c",
		},
		{
			name = "add",
			text = "+",
			size = { "8%r", "7%r" },
			pos = { 0.44, 0.92 },
			origin = "c",
			background = Colors.slate700,
			on_keys = {
				lmb = function(ew)
					add_row()
				end,
			},
		},
		{
			name = "clear",
			text = "Clear",
			size = { "12%r", "7%r" },
			pos = { 0.56, 0.92 },
			origin = "c",
			background = Colors.slate700,
			on_keys = {
				lmb = function(ew)
					clear_rows()
				end,
			},
		},
	},
}
