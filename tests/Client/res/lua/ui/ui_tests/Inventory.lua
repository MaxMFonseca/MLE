local onCursorDragBegin = function(ew)
	local tbl = ew:get("table")
	local root = G.ui:getRoot()
	tbl.original_parent = ew:parent()
	root:addExistingChild(ew)
	ew:apply("layer", 1000)
	ew:apply("size", "48px")

	local root = G.ui:getElementById("inventory_root")
	root:call("makeAllItemsIgnoreHover")
end

local onCursorDrag = function(ew, cursor_pos_relative)
	local mouse_pos = G.window:getCursorPos()
	local hit = G.ui:hitTest(mouse_pos.x, mouse_pos.y, ew)

	local tbl = ew:get("table")
	local last_hit = tbl.last_hit

	-- Handle Hover state on slots
	if hit and hit:entt() ~= entt.null then
		local name = hit:name()
		if name and string.sub(name, 1, 5) == "slot_" then
			if last_hit and last_hit:entt() ~= hit:entt() then
				last_hit:call("onDropHoverOut")
			end
			hit:call("onDropHover")
			tbl.last_hit = hit
		else
			if last_hit then
				last_hit:call("onDropHoverOut")
				tbl.last_hit = nil
			end
		end
	else
		if last_hit then
			last_hit:call("onDropHoverOut")
			tbl.last_hit = nil
		end
	end

	ew:apply("pos_x", mouse_pos.x .. "px")
	ew:apply("pos_y", mouse_pos.y .. "px")
	ew:requestExternalBoundsUpdate()
end

local onCursorDragEnd = function(ew)
	local tbl = ew:get("table")
	local hit = tbl.last_hit

	ew:apply("size", "32px")
	ew:apply("pos", "c")

	if hit then
		hit:call("onDropHoverOut")

		local existing_item = hit:getChild("item")
		local orig_parent = tbl.original_parent

		if existing_item and existing_item:valid() then
			if orig_parent and orig_parent:valid() then
				orig_parent:addExistingChild(existing_item)
				existing_item:apply("pos", "c")
			end
		end

		print("Reparenting to hit:", hit:name())
		hit:addExistingChild(ew)
	else
		tbl.original_parent:addExistingChild(ew)
	end

	tbl.last_hit = nil
	tbl.original_parent = nil

	local root = G.ui:getElementById("inventory_root")
	root:call("makeAllItemsHoverableAgain")
end

local ItemColor = {
	Colors.red500,
	Colors.blue500,
	Colors.green500,
	Colors.yellow500,
	Colors.purple500,
}

local function makeInventory()
	local slots = {}
	for i = 1, 100 do
		slots[i] = {
			name = "slot_" .. i,
			size = "40px",
			background = Colors.slate800,
			border = { thickness = "2px", color = Colors.slate900 },
			free = {}, -- make it a free container so items can be positioned center
			fn = {
				onDropHover = function(ew)
					ew:apply("background", Colors.slate600)
				end,
				onDropHoverOut = function(ew)
					ew:apply("background", Colors.slate800)
				end,
			},
			hoverable = true,
			c = {},
		}
	end

	for i = 1, 5 do
		local c = ItemColor[i]
		slots[i].c.item = {
			name = "item",
			size = "32px",
			pos = "c",
			origin = "c",
			background = c,
			table = {
				original_parent = nil,
				last_hit = nil,
			},
			fn = {
				onCursorDragBegin = onCursorDragBegin,
				onCursorDrag = onCursorDrag,
				onCursorDragEnd = onCursorDragEnd,
			},
			on_keys = {
				lmb = function(ew)
					ew:beginCursorDrag()
				end,
			},
		}
	end

	return {
		id = "inventory_root",
		size = 1,
		background = Colors.slate900,
		free = {},
		c = {
			{
				-- size = "480px", -- 10 * 40px + 9 * 4px gap + padding
				size_x = 0.5,
				pos = "c",
				origin = "c",
				background = Colors.slate700,
				padding = "8px",
				border = { roundness = "8px" },

				list = {
					dir = "h",
					wrap = "y",
					justify = "sb",
					justify_last = "sb",
					cross_gap = "4px",
					cross_max_size = "40px",
					pack = true,
				},
				c = slots,
			},
			{
				background = Colors.BLUE,
				pos = "lb",
				origin = "lb",
				size = "20px",
				on_keys = {
					lmb = function(ew)
						ew:ui():logAllBounds()
					end,
				},
			},
		},

		fn = {
			makeAllItemsIgnoreHover = function(ew)
				local items = ew:getChildrenNamedRecursive("item")
				print(#items)
				for _, item in ipairs(items) do
					item:ignoreHover(true)
				end
				print("done1")
			end,
			makeAllItemsHoverableAgain = function(ew)
				local items = ew:getChildrenNamedRecursive("item")
				print(#items)
				for _, item in ipairs(items) do
					item:ignoreHover(false)
				end
				print("done2")
			end,
		},
	}
end

return makeInventory()
