local function color()
	return {
		withA = function(self, alpha)
			return { alpha = alpha }
		end,
	}
end

local function assert_names(actual, expected)
	assert(#actual == #expected, "name count mismatch")
	for i, name in ipairs(expected) do
		assert(actual[i] == name, "name mismatch at " .. i)
	end
end

local function make_factory()
	Colors = {
		WHITE = color(),
		slate300 = color(),
		slate400 = color(),
		slate800 = color(),
	}
	return require("mle.ui.comp.filterable_list")
end

local function expect_error(fn, needle)
	local ok, message = pcall(fn)
	assert(not ok, "expected error containing " .. needle)
	assert(string.find(tostring(message), needle, 1, true), tostring(message))
end

local function run_validation()
	local factory = make_factory()
	expect_error(function() factory(42) end, "rows must be a table")
	expect_error(function() factory({}, 42) end, "options must be a table")
	expect_error(function() factory({ 42 }) end, "row 1 must be a table")
	expect_error(function() factory({ {} }) end, "row 1 name must be a nonempty string")
	expect_error(function() factory({ { name = "same" }, { name = "same" } }) end, "duplicate row name: same")
	expect_error(function() factory({}, { scrollbar = 42 }) end, "scrollbar must be a table")
end

local function run_factory_shape_and_immutability()
	local factory = make_factory()
	local rows = {
		{ name = "Hello World", table = { marker = 1 } },
		{ name = "Renderer", table = { marker = 2 } },
	}
	local root_def = factory(rows)

	assert(root_def.list.dir == "v")
	assert(root_def.c[1].name == "query_input")
	assert(root_def.c[2].name == "results")
	assert(root_def.c[2].c[1].c[1].name == "scroll_driver")
	assert(root_def.c[2].c[1].c[1].c[1].name == "Hello World")
	assert(root_def.c[2].c[1].c[1].c[2].name == "Renderer")

	root_def.table.rows[1].table.marker = 99
	assert(rows[1].table.marker == 1, "component state must not mutate caller rows")
	root_def.c[2].c[1].c[1].c[1].table.marker = 77
	assert(rows[1].table.marker == 1, "initial children must be deep copies")
end

local function run_filtering_and_update_noop()
	local factory = make_factory()
	local root_def = factory({
		{ name = "Hello World", table = { marker = 1 } },
		{ name = "Renderer", table = { marker = 2 } },
		{ name = "High Wind Damage", table = { marker = 3 } },
		{ name = "Shadow", table = { marker = 4 } },
	})

	local events, children = {}, {}
	local query = "hwd"
	local query_input = {}
	function query_input:get(key) assert(key == "text") return query end
	local scroll_driver = {}
	function scroll_driver:destroyAllChildren() children = {} events[#events + 1] = "destroy" end
	function scroll_driver:addChild(row) children[#children + 1] = row events[#events + 1] = "add:" .. row.name end
	function scroll_driver:requestInternalBoundsUpdate() events[#events + 1] = "bounds" end
	local viewport = {}
	function viewport:getChild(name) assert(name == "scroll_driver") return scroll_driver end
	local results = {}
	function results:getChild(name) assert(name == "viewport") return viewport end
	function results:call(name, value) assert(name == "setOffset") assert(value == 0) events[#events + 1] = "offset:0" end
	local root = {}
	function root:get(key) assert(key == "table") return root_def.table end
	function root:getChild(name)
		if name == "query_input" then return query_input end
		if name == "results" then return results end
		error("unexpected root child " .. tostring(name))
	end
	function root:call(name, ...) return root_def.fn[name](self, ...) end

	root_def.on_update(root)
	assert_names({ children[1].name, children[2].name }, { "Hello World", "High Wind Damage" })
	assert_names(events, { "destroy", "add:Hello World", "add:High Wind Damage", "bounds", "offset:0" })
	children[1].table.marker = 100
	assert(root_def.table.rows[1].table.marker == 1, "filtered children must be deep copies")
	root_def.on_update(root)
	assert(#events == 5, "unchanged query must not rebuild")
	query = "xyz"
	root_def.on_update(root)
	assert(#children == 0, "no-match query must empty results")
	assert(events[#events - 1] == "bounds")
	assert(events[#events] == "offset:0")
	query = ""
	root_def.on_update(root)
	assert_names({ children[1].name, children[2].name, children[3].name, children[4].name },
		{ "Hello World", "Renderer", "High Wind Damage", "Shadow" })
end

local function run_row_update_survives_filter_rebuild()
	local factory = make_factory()
	local root_def = factory({
		{ name = "Alpha A", table = { id = "A" }, background = "unselected" },
		{ name = "Beta B", table = { id = "B" }, background = "unselected" },
	})
	local children = {}
	local scroll_driver = {}
	function scroll_driver:destroyAllChildren() children = {} end
	function scroll_driver:addChild(row) children[#children + 1] = row end
	function scroll_driver:requestInternalBoundsUpdate() end
	local viewport = {}
	function viewport:getChild(name) assert(name == "scroll_driver") return scroll_driver end
	local results = {}
	function results:getChild(name) assert(name == "viewport") return viewport end
	function results:call(name, value) assert(name == "setOffset" and value == 0) end
	local root = {}
	function root:get(key) assert(key == "table") return root_def.table end
	function root:getChild(name)
		if name == "results" then return results end
		error("unexpected root child " .. tostring(name))
	end
	function root:call(name, ...) return root_def.fn[name](self, ...) end

	root:call("updateRows", function(row)
		row.background = row.table.id == "B" and "selected" or "unselected"
	end)
	root:call("applyFilter", "b")
	assert(#children == 1 and children[1].table.id == "B")
	assert(children[1].background == "selected", "selected style must survive a filter rebuild")
	root:call("applyFilter", "")
	assert(children[1].table.id == "A" and children[1].background == "unselected")
	assert(children[2].table.id == "B" and children[2].background == "selected")
end

local function run_row_update_survives_filter_rebuild()
	local factory = make_factory()
	local root_def = factory({
		{ name = "Alpha A", table = { id = "A" }, background = "unselected" },
		{ name = "Beta B", table = { id = "B" }, background = "unselected" },
	})

	local children = {}
	local scroll_driver = {}
	function scroll_driver:destroyAllChildren() children = {} end
	function scroll_driver:addChild(row) children[#children + 1] = row end
	function scroll_driver:requestInternalBoundsUpdate() end
	local viewport = {}
	function viewport:getChild(name) assert(name == "scroll_driver") return scroll_driver end
	local results = {}
	function results:getChild(name) assert(name == "viewport") return viewport end
	function results:call(name, value) assert(name == "setOffset" and value == 0) end
	local root = {}
	function root:get(key) assert(key == "table") return root_def.table end
	function root:getChild(name)
		if name == "results" then return results end
		error("unexpected root child " .. tostring(name))
	end
	function root:call(name, ...) return root_def.fn[name](self, ...) end

	root:call("updateRows", function(row)
		row.background = row.table.id == "B" and "selected" or "unselected"
	end)
	root:call("applyFilter", "b")
	assert(#children == 1 and children[1].table.id == "B")
	assert(children[1].background == "selected", "selected style must survive a filter rebuild")
	root:call("applyFilter", "")
	assert(children[1].table.id == "A" and children[1].background == "unselected")
	assert(children[2].table.id == "B" and children[2].background == "selected")
end

local M = {}
function M.run()
	run_validation()
	run_factory_shape_and_immutability()
	run_filtering_and_update_noop()
	run_row_update_survives_filter_rebuild()
	run_row_update_survives_filter_rebuild()
	return true
end
return M
