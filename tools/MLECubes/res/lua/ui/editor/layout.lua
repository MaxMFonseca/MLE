return {
	root_image = {
		extent = { 1920, 1080 },
		clear_color = "NQB",
	},
	c = {
		require("i.ui.editor.header")({
			size_y = "3.5%",
		}),
		{
			size_y = "0.25%",
			background = "BLACK",
		},
		{
			"EditorView",
			size_y = "1f",
			-- size_x = "100%",
			sprite = {
				"noise",
			},
			-- on_init = function(self)
			-- 	print("EditorView created")
			-- 	self.dispatchEvent("editor_view_created")
			-- end,
		},
	},
}
