Idx = 0
local function makeScrollable()
	local ret = require("mle.ui.comp.scrollable")({
		id = "scrollable_list",

		background = Colors.slate700:withA(0.3),

		fn = {
			addChild = function(ew, child)
				ew:addChild(child)
				ew:requestInternalBoundsUpdate()
			end,
		},

		list = {
			dir = "v",
			cross_align = "c",
		},
	})

	ret.size_x = 0.3
	ret.size_y = 0.8

	return ret
end

return {
	background = Colors.slate600,

	size = 1,

	free = {},
	c = {

		makeScrollable(),

		{
			name = "add_to_scrollable",

			size_y = "0.1",
			text = "+",
			pos = "c",
			origin = "c",

			on_keys = {
				lmb = function(ew)
					local scrollable_ew = G.ui:getElementById("scrollable_list")
					Idx = Idx + 1

					scrollable_ew:call("addChild", {
						name = "child_" .. Idx,
						size_y = "5%r",
						size_x = 0.3 + math.random() * 0.7,
						background = Color.random(),
						on_keys = {
							lmb = function(ew) end,
						},
					})
				end,
			},
		},
	},
}
