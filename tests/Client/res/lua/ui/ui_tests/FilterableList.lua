local filterable_list = require("mle.ui.comp.filterable_list")

local names = {
	"Hello World",
	"Renderer Pipeline",
	"High Wind Damage",
	"Shadow Map",
	"Scrollable Container",
	"Text Dropdown Selector",
	"Sprite Progress Bar",
	"Nine Slice Panel",
	"Animation Timeline",
	"Inventory Grid",
	"Popup Stack",
	"Audio Engine",
	"Vulkan Context",
	"Lua Runtime",
	"Thread Pool",
	"mle_a",
	"mle_b",
	"mle_c",
	"mle_d",
	"mle_e",
	"Frame Renderer",
	"Texture Cache",
	"Shader Compiler",
	"Descriptor Pool",
	"Command Buffer",
	"Swapchain Manager",
	"Render Pass",
	"Pipeline Cache",
	"Font Atlas",
	"Text Layout",
	"Input Manager",
	"Keyboard State",
	"Mouse Capture",
	"Gamepad Mapper",
	"Window System",
	"Scene Graph",
	"Entity Registry",
	"Component Storage",
	"Event Dispatcher",
	"Job Scheduler",
	"Memory Arena",
	"Resource Loader",
	"Asset Database",
	"Model Importer",
	"Texture Streaming",
	"Audio Mixer",
	"Sound Cache",
	"Music Stream",
	"Lua Console",
	"Script Reload",
	"UI Layout",
	"UI Bounds",
	"UI Hover State",
	"UI Focus Manager",
	"UI Popup Layer",
	"UI Scroll State",
	"UI Text Input",
	"UI Color Picker",
	"UI Range Slider",
	"UI Carousel Selector",
	"Debug Overlay",
	"Performance Tracker",
	"Runtime Config",
	"Network Client",
	"Server Protocol",
}

local rows = {}
for index, name in ipairs(names) do
	rows[#rows + 1] = {
		name = name,
		size_x = "1f",
		size_y = "44px",
		padding = { "12px", "8px" },
		background = index % 2 == 0 and Colors.slate700 or Colors.slate800,
		border = {
			thickness = "1px",
			color = Colors.slate500,
			roundness = "4px",
		},
		text = {
			text = name,
			height = "16px",
			color = Colors.WHITE,
		},
		table = {
			row_index = index,
		},
	}
end

local list = filterable_list(rows, {
	input_text = {
		text = "",
		height = "17px",
		color = Colors.WHITE,
		input = {},
	},
	list_gap = "5px",
	scrollbar = {
		bar_width = "12px",
		min_thumb_px = 28,
	},
})
list.name = "filterable_list"
list.size_x = "520px"
list.size_y = "1f"

return {
	background = Colors.slate600,
	size = 1,
	padding = "28px",
	list = {
		dir = "v",
		cross_align = "c",
		gap = "12px",
	},
	c = {
		{
			name = "title",
			size_y = "30px",
			text = {
				text = "Filterable list — try hwd, rpl, spb, or mle_c",
				height = "22px",
				color = Colors.WHITE,
			},
		},
		list,
	},
}
