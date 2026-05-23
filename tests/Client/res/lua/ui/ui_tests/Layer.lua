local function create_multipanel()
	local ret = require("mle/ui/comp/multipanel")({
		require("i/ui/ui_tests/First"),
	})

	ret.size = 1

	return ret
end

return create_multipanel()
