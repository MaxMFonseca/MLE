return {
	root_image = {
		extent = { 1920, 1080 },
		clear_color = "NQB",
	},
	-- padding = 10,
	c = {
		{
			sprite = "colorBG.png",
			size = { "100%", "100%" },
			pos = { 0, 0 },
		},
		{
			"title",
			pos = { "50%", "6%" },
			origin = "ct",
			size_y = 256,
			-- blur = true,
			text = {
				"Hello MAX!",
				color = "BLACK",
			},
		},
		{
			rel = { "pos:title:cb" },
			pos_y = 60,
			origin = "ct",
			blur = true,
			padding = 10,

			c = {
				align = "center",
				gap = 20,
				children_base = {
					on_hover_enter = function(self)
						self:apply("text", { color = "NQB" })
						mle.audio.play("res/i/sounds/menu_click.flac")
					end,
					on_hover_leave = function(self)
						self:apply("text", { color = "WHITE" })
					end,
				},
				{
					text = { "Single Player" },
				},
				{
					text = { "Multiplayer" },
				},
				{
					text = { "Achievements" },
				},
				{
					text = { "... Texture Packs ..." },
				},
				{
					text = { "Settings" },
				},
				{
					text = { "Exit" },
					on_hover_enter = function(self)
						self:apply("text", { color = "RED" })
						mle.audio.play("res/i/sounds/menu_click.flac")
						mle.core.stop()
					end,
				},
			},
		},
	},
	-- 	},
	-- },
	-- c = {
	-- 	direction = "col",
	-- 	gap = 10,
	-- 	{
	-- 		"1",
	-- 		bounds = { 1920 / 2, 1080 / 2, 1920 / 2 - 10, 1080 / 2 - 10 },
	-- 		background = "RED",
	-- 	},
	-- 	{
	-- 		sprite = {
	-- 			"mle",
	-- 			color = "MAGENTA",
	-- 		},
	-- 	},
	-- 	{
	-- 		size = { 100, "1f" },
	-- 		-- margin = { b = 100 },
	-- 		background = "GREEN",
	-- 	},
	-- 	{
	-- 		size = { 100, ".1f" },
	-- 		-- margin = { b = 100 },
	-- 		background = "RED",
	-- 		origin = "c",
	-- 	},
	-- 	{
	-- 		size_x = "0.9f",
	--
	-- 		background = "BLUE",
	-- 		text = { "A ... HelloWorld! Now we can render text elements! ... A", color = "RED" },
	-- 	},
	-- 	{
	-- 		size = { 100, 100 },
	-- 		-- margin = { t = 100, l = 200 },
	-- 		background = "MAGENTA",
	-- 	},
	-- 	{
	-- 		"star",
	--
	-- 		pos = { 100, 100 },
	-- 		size = { "10%", "s" },
	--
	-- 		sprite = {
	-- 			"star.png",
	-- 			color = "NQB",
	-- 		},
	--
	-- 		-- margin = { b = 100 },
	-- 		background = "GREEN",
	-- 	},
	-- 	{
	-- 		pos = { 20, 20 },
	-- 		rel = { "size_x:star:1", "pos:star:br" },
	-- 		-- size = {},
	-- 		-- size_r = {},
	-- 		sprite = {
	-- 			"star.png",
	-- 			color = "NQW",
	-- 		},
	-- 	},
	-- 	{
	-- 		pos = { "100%", 0 },
	-- 		origin = "rt",
	-- 		size_x = 100,
	-- 		size_y = "70%",
	-- 		background = "NQW",
	--
	-- 		c = {
	-- 			{
	-- 				size = { "10%", "0.8f" },
	-- 				background = "BLUE",
	-- 			},
	-- 		},
	-- 	},
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
	-- },
}
