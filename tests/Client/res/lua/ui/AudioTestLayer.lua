local function label(text, color)
	return {
		size_x = "1f",
		size_y = "24px",
		text = { text = text, height = "15px", color = color or Colors.slate100 },
	}
end

local function button(text, fn)
	return {
		size_x = "1f",
		size_y = "30px",
		padding = { "8px", "5px" },
		background = Colors.slate800,
		border = { thickness = "1px", color = Colors.slate500, roundness = "4px" },
		text = { text = text, height = "14px", color = Colors.WHITE },
		on_hover_in = function(ew) ew:apply("background", Colors.slate700) end,
		on_hover_out = function(ew) ew:apply("background", Colors.slate800) end,
		on_keys = { lmb = fn },
	}
end

local function section(title, children)
	local content = { label(title, Colors.cyan300) }
	for _, child in ipairs(children) do table.insert(content, child) end
	return {
		size_x = "1f", padding = "8px", background = Colors.slate950:withA(0.76),
		border = { thickness = "1px", color = Colors.slate700, roundness = "5px" },
		list = { pack = true, gap = "5px" }, c = content,
	}
end

local telemetry = label("Telemetry loading…")
telemetry.name = "telemetry"
telemetry.size_y = "96px"
telemetry.text.height = "13px"
telemetry.on_update = function(ew)
	local s = G.audio_test_status()
	local line = string.format(
		"Scenario: %s (%s)  bus=%d priority=%d cap=%d protected=%s rate=%d/s\nraw=%d aggregated=%d submitted=%d dropped=%d\nSlot 5: volume=%.2f pitch=%.2f duration=%dms | fade-in=%dms fade-out=%dms live-ramp=%dms",
		s.scenario, s.running and "running" or "stopped", s.bus, s.priority, s.cap,
		tostring(s.protected), s.rate, s.raw, s.aggregated, s.submitted, s.dropped,
		s.stream_volume, s.stream_pitch, s.stream_duration_ms,
		s.stream_fade_in_ms, s.stream_fade_out_ms, s.stream_ramp_fade_ms)
	ew:apply("text", line)
end

local shortcuts = label(
	"Keys: 1-6 policy/fade presets; 7/8/9 load rates; 0 corrupt | A/S/D/F shots | G/H policies, J/K priority\nQ/W load start/stop | Z/X/C/P stream start/pause/resume/stop | V/B live ramps, N fades, M/, duration\nR/T/Y duration streams on slots 3/4 | U Stop All, I reset, Space UI, Esc return",
	Colors.amber200)
shortcuts.size_y = "72px"
shortcuts.text.height = "12px"

return {
	size = 1,
	free_container = {},
	on_keys = {
		escape = function() G.return_to_init() end,
		space = function() G.audio_test_trigger_ui() end,
		["1"] = function() G.audio_test_load_preset("Cap rejection") end,
		["2"] = function() G.audio_test_load_preset("Equal-priority rejection") end,
		["3"] = function() G.audio_test_load_preset("Higher-priority replacement") end,
		["4"] = function() G.audio_test_load_preset("UI protection") end,
		["5"] = function() G.audio_test_load_preset("Texture fade") end,
		["6"] = function() G.audio_test_load_preset("Ramp replacement") end,
		["7"] = function() G.audio_test_load_preset("Sparse") end,
		["8"] = function() G.audio_test_load_preset("Battle") end,
		["9"] = function() G.audio_test_load_preset("Saturation") end,
		["0"] = function() G.audio_test_load_preset("Corrupt fixture") end,
		a = function() G.audio_test_play("i/generated/combat_high") end,
		s = function() G.audio_test_play("i/generated/combat_low") end,
		d = function() G.audio_test_play("i/generated/hit_01") end,
		f = function() G.audio_test_trigger_ui() end,
		g = function() G.audio_test_set_bus(0); G.audio_test_set_policy(8, false) end,
		h = function() G.audio_test_set_bus(1); G.audio_test_set_policy(2, true) end,
		j = function() G.audio_test_set_priority(1) end,
		k = function() G.audio_test_set_priority(4) end,
		q = function() G.audio_test_start_load() end,
		w = function() G.audio_test_stop_load() end,
		z = function() G.audio_test_stream_start() end,
		x = function() G.audio_test_stream_pause() end,
		c = function() G.audio_test_stream_resume() end,
		v = function() G.audio_test_stream_params(0.35, 0.8, 250) end,
		b = function() G.audio_test_stream_params(1.0, 1.25, 500) end,
		n = function() G.audio_test_set_stream_fades(750, 1000) end,
		m = function() G.audio_test_set_stream_duration(0) end,
		comma = function() G.audio_test_set_stream_duration(600) end,
		p = function() G.audio_test_stream_stop() end,
		r = function() G.audio_test_duration("duration_mono", 3, 0, 0) end,
		t = function() G.audio_test_duration("duration_mono", 3, 150, 200) end,
		y = function() G.audio_test_duration("duration_stereo", 4, 250, 500) end,
		u = function() G.audio_test_stop_all() end,
		i = function() G.audio_test_reset() end,
	},
	c = {
		panel = {
			pos = "c", origin = "c", size_x = 0.92, size_y = 0.94, padding = "10px",
			background = Colors.slate900:withA(0.94),
			border = { thickness = "2px", color = Colors.cyan700, roundness = "8px" },
			list = { dir = "h", gap = "9px" },
			children_base = { size_x = "1f", list = { pack = true, gap = "8px" } },
			c = {
				{
					c = {
						label("Interactive Audio Test", Colors.WHITE),
						shortcuts,
						section("One-shots", {
							button("Combat high", function() G.audio_test_play("i/generated/combat_high") end),
							button("Combat low", function() G.audio_test_play("i/generated/combat_low") end),
							button("Hit 01", function() G.audio_test_play("i/generated/hit_01") end),
							button("Protected UI (Space)", function() G.audio_test_trigger_ui() end),
						}),
						section("Policies", {
							button("Bus 0 / cap 8", function() G.audio_test_set_bus(0); G.audio_test_set_policy(8, false) end),
							button("Bus 1 protected / cap 2", function() G.audio_test_set_bus(1); G.audio_test_set_policy(2, true) end),
							button("Priority 1", function() G.audio_test_set_priority(1) end),
							button("Priority 4", function() G.audio_test_set_priority(4) end),
						}),
					},
				},
				{
					c = {
						section("Load Presets", {
							button("1 Cap rejection", function() G.audio_test_load_preset("Cap rejection") end),
							button("2 Equal-priority rejection", function() G.audio_test_load_preset("Equal-priority rejection") end),
							button("3 Higher-priority replacement", function() G.audio_test_load_preset("Higher-priority replacement") end),
							button("4 UI protection — unprotected/protected A/B", function() G.audio_test_load_preset("UI protection") end),
							button("5 Texture fade", function() G.audio_test_load_preset("Texture fade") end),
							button("6 Ramp replacement", function() G.audio_test_load_preset("Ramp replacement") end),
							button("Sparse — 15/s", function() G.audio_test_load_preset("Sparse") end),
							button("Battle — 120/s", function() G.audio_test_load_preset("Battle") end),
							button("Saturation — 600/s", function() G.audio_test_load_preset("Saturation") end),
							button("Start synthetic load", function() G.audio_test_start_load() end),
							button("Stop synthetic load", function() G.audio_test_stop_load() end),
							button("Explicit corrupt-load test", function() G.audio_test_load_preset("Corrupt fixture") end),
						}),
						section("Stream Slot 5", {
							button("Start (loop + fade in)", function() G.audio_test_stream_start() end),
							button("Pause", function() G.audio_test_stream_pause() end),
							button("Resume", function() G.audio_test_stream_resume() end),
							button("Volume 0.35 / pitch 0.8 / fade 250", function() G.audio_test_stream_params(0.35, 0.8, 250) end),
							button("Volume 1.0 / pitch 1.25 / fade 500", function() G.audio_test_stream_params(1.0, 1.25, 500) end),
							button("Fades 750 in / 1000 out", function() G.audio_test_set_stream_fades(750, 1000) end),
							button("Duration full", function() G.audio_test_set_stream_duration(0) end),
							button("Duration 600 ms", function() G.audio_test_set_stream_duration(600) end),
							button("Stop (fade out)", function() G.audio_test_stream_stop() end),
						}),
					},
				},
				{
					c = {
						section("Duration", {
							button("Mono full: slot 3, offset 0", function() G.audio_test_duration("duration_mono", 3, 0, 0) end),
							button("Mono: slot 3, offset 150, duration 200", function() G.audio_test_duration("duration_mono", 3, 150, 200) end),
							button("Stereo: slot 4, offset 250, duration 500", function() G.audio_test_duration("duration_stereo", 4, 250, 500) end),
						}),
						section("Telemetry (client submissions)", { telemetry }),
						section("Actions", {
							button("Stop All", function() G.audio_test_stop_all() end),
							button("Reset scenario + counters", function() G.audio_test_reset() end),
							button("Return to Init (Esc)", function() G.return_to_init() end),
						}),
					},
				},
			},
		},
	},
}
