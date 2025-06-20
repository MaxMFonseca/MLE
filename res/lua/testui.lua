return {
	root_image = {
		extent = { 1920, 1080 },
		clear_color = "NQB",
	},
	bounds = { 0, 0, 1920, 1080 },
	padding = 10,
	c = {
		direction = "col",
		child_gap = 10,
		{
			"1",
			bounds = { 1920 / 2, 1080 / 2, 1920 / 2 - 10, 1080 / 2 - 10 },
			background = "RED",
		},
		{
			sprite = {
				"mle",
				color = "MAGENTA",
			},
		},
		{
			-- size = { "1f", "1f" }, {0,DEFAULT} BEHAVIOR FOR LIST CONTAINER
			-- aspect_ratio = 1,

			sprite = {
				"star.png",
				color = "NQB",
			},

			-- margin = { b = 100 },
			background = "GREEN",
		},
		{
			size = { 100, "1f" },
			-- margin = { b = 100 },
			background = "GREEN",
		},
		{
			size = { 100, ".1f" },
			-- margin = { b = 100 },
			background = "GREEN",
		},
		{
			size_x = "0.9f",

			background = "BLUE",
			text = { "A ... HelloWorld! Now we can render text elements! ... A", color = "RED" },
		},
		{
			size = { 100, 100 },
			-- margin = { t = 100, l = 200 },
			background = "MAGENTA",
		},
		-- {
		-- 	"background",
		-- 	renderable = "shader",
		-- 	always_render = true,
		-- 	layer = -1,
		-- 	pipeline = {
		-- 		"init_bg",
		-- 		setup = { -- TODO: allow this to be done outside, for reusability
		-- 			frag = "i/bg.frag",
		-- 		},
		-- 	},
		-- 	onPipelineSet = function(self, shader)
		-- 		shader:setFieldVec4("in_color0", Color.getColor("ZERO"))
		-- 		shader:setFieldVec4("in_color1", Color.getColor("ZERO"))
		-- 	end,
		-- 	onShaderRender = function(self, shader)
		-- 		shader:setFieldF("in_time", mle.time)
		-- 		shader:setFieldF("in_aspect_ratio", self:getAspectRatio())
		-- 	end,
		-- 	listen = {
		-- 		uColorChanged = function(self, e)
		-- 			local s = self:getRenderable():asShader()
		-- 			s:setFieldVec4("in_color0", e)
		-- 			s:setFieldVec4("in_color1", e)
		-- 		end,
		-- 	},
		-- },
		--
		-- {
		-- 	pos = { 0.5, 0 },
		-- 	size_y = 0.45,
		-- 	aspect_ratio = 1,
		-- 	origin = "ct",
		-- 	background = "yellow_3",
		-- 	-- renderable = "textbox",
		-- 	text = {
		-- 		"1234567890\nabcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ\n!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\nAAAAAAAAAAA",
		-- 		color = "ZERO",
		-- 		font = "Nerd",
		-- 		char_height = 0.2,
		-- 		-- outline_thickness = 3,
		-- 		-- outline_color = "black",
		-- 	},
		-- 	listen = {
		-- 		uColorChanged = function(self, e)
		-- 			local t = self:getRenderable():asText()
		-- 			t.color = e
		-- 		end,
		-- 	},
		-- 	listen_keys_on_hover = {
		-- 		-- lmb = function(self)
		-- 		-- 	local t = self:getRenderable():asTextBox()
		-- 		-- 	local h = t:getHoveredChar()
		-- 		-- 	t:setRenderSelection(true)
		-- 		-- 	t:beginRangeSelection(h)
		-- 		-- end,
		-- 		["lmb:d"] = function(self)
		-- 			local t = self:getRenderable():asTextBox()
		-- 			local h = t:getHoveredChar()
		-- 			t:endRangeSelection(h)
		-- 		end,
		--
		-- 		rmb = function(self)
		-- 			local t = self:getRenderable():asTextBox()
		-- 			print(t:getText())
		-- 		end,
		-- 	},
		-- },
	},
}
