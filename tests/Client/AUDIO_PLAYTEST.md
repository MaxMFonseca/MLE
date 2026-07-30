# Client Audio Manual Playtest

Last attempt: 2026-07-12 (America/Sao_Paulo)

Overall status: **PENDING USER PLAYTEST**. Current verification environment has no SDL video device, so Client aborts before UI and audio-device initialization. No audible result was observed.

## Launch

From repository root:

```sh
source scripts/envsetup.sh && mle_build -t Debug && mle_run_test -n Client -t Debug
```

Expected path: Client window opens, then select **Audio Test** from Init menu. Generated fixtures under `tests/Client/res/sounds/generated/` preload through runtime sound names prefixed `i/generated/`.

2026-07-12 environment result: **BLOCKED** — `SDL_Init failed: No available video device`; direct `build/Debug/tests/Client/Client` exited 134 after assertion. UI visibility and audio-device opening were not observed. Note: `mle_run_test` printed same abort but returned 0 because its trailing log-link operation masks executable status.

## Controls

| Key | Action |
| --- | --- |
| `1` / `2` / `3` / `4` | Cap rejection / equal-priority rejection / higher-priority replacement / protected UI preset |
| `5` / `6` | Texture fade / ramp replacement preset |
| `7` / `8` / `9` | Sparse 15/s / battle 120/s / saturation 600/s preset |
| `0` | Attempt corrupt fixture preload |
| `A` / `S` / `D` / `F` | Combat high / combat low / hit 01 / protected UI one-shot |
| `G` / `H` | Bus 0 cap 8 / protected bus 1 cap 2 |
| `J` / `K` | Priority 1 / priority 4 |
| `Q` / `W` | Start / stop synthetic load |
| `Z` / `X` / `C` / `P` | Start / pause / resume / fade-stop stream slot 5 |
| `V` / `B` | Ramp to volume 0.35, pitch 0.8 over 250 ms / volume 1.0, pitch 1.25 over 500 ms |
| `N` | Set next stream fade-in/out to 750/1000 ms |
| `M` / `,` | Set stream duration to full / 600 ms |
| `R` / `T` / `Y` | Start mono slot 3 at offset 0/full / mono slot 3 at offset 150 ms for 200 ms / stereo slot 4 at offset 250 ms for 500 ms |
| `U` / `I` | Stop all / reset scenario and counters |
| `Space` / `Esc` | Protected UI one-shot / return to Init |

Buttons in Audio Test mirror these shortcuts. Telemetry reports raw, aggregated, submitted, and dropped events plus stream parameters.

## Acceptance checklist

Use headphones or known-good speakers. Mark PASS or FAIL only after hearing and observing behavior. On failure, record reproduction steps and relevant `latest.log` lines.

| Status | Check | Procedure / expected result |
| --- | --- | --- |
| PENDING USER PLAYTEST | Bounded combat density | Press `9`, then `Q`; telemetry raw count grows rapidly while submitted cues remain bounded (maximum four per fixed update), and output remains intelligible rather than unbounded stacking. Press `W`. |
| PENDING USER PLAYTEST | Protected UI A/B | Press `4`. Phase 1 plays a long priority-1 UI tone on unprotected bus 1, submits 249 quiet five-second priority-2 fillers (0.015 PCM peak each) on bus 0 at no more than four/update, then submits one priority-4 combat attack. Engine attempts to create up to 250 one-shot sources, but device may expose fewer. Once saturated, an incoming filler or final combat attack selects UI as lowest-priority eligible victim, so UI cuts off early. After a 1.5 s comparison delay, playback resets and phase 2 repeats with bus 1 protected: fillers cannot steal UI and final combat replaces a filler while UI continues to its natural end. Scenario label identifies phase. Record generated-source count from log with any unexpected audible result. |
| PENDING USER PLAYTEST | Equal-priority behavior | Press `2`; with bus cap 1, second equal-priority hit is rejected and does not replace/restart first voice. |
| PENDING USER PLAYTEST | Higher-priority behavior | Press `3`; higher-frequency priority-4 cue replaces lower-frequency priority-1 cue at bus cap 1. |
| PENDING USER PLAYTEST | Click-free fades | Press `5`; texture fades in over 750 ms and stops after about two seconds with a 1000 ms fade, without clicks at either edge. |
| PENDING USER PLAYTEST | Smooth ramp replacement | Press `6`; initial long ramp is replaced about 0.5 s later by new 900 ms target. Transition remains continuous, with no gain/pitch jump or click. |
| PENDING USER PLAYTEST | Pause and resume | Press `Z`, wait for recognizable texture position, press `X`, wait, then `C`; playback pauses silently and resumes. |
| PENDING USER PLAYTEST | No playback-position reset | Record or loop back slot-5 output while using `Z`, `X`, then `C`. Compare waveform phase immediately before pause and after resume against the known start of `combat_texture`; resumed output must continue from paused phase rather than restart at frame zero. |
| PENDING USER PLAYTEST | Mono/stereo stream windows | Press `R` (slot 3, mono offset 0/full: about 750 ms), `T` (slot 3, mono offset 150 ms, duration 200 ms), and `Y` (slot 4, stereo offset 250 ms, duration 500 ms). Each non-looping stream starts at specified offset and ends at requested duration; stereo remains two-channel. Slot 5 texture may run concurrently. |
| PENDING USER PLAYTEST | Stop All | Start load and stream (`Q`, `Z`), then press `U`; all one-shots and streams stop completely. |
| PENDING USER PLAYTEST | Reset | Start load and stream, then press `I`; all playback stops, counters reset, and defaults return (bus 0, priority 2, cap 8, unprotected, 60/s). |
| PENDING USER PLAYTEST | Exit/shutdown stop | Start stream/load, press `Esc`, then exit Client; no audio continues after layer return or process shutdown. |
| PENDING USER PLAYTEST | Safe corrupt preload failure | Press `0`; Client remains responsive, no corrupt audio plays, and log records controlled decode/load failure without crash or assertion. |

## Automated fixture evidence

On 2026-07-12, exact pinned environment verification succeeded:

```sh
/tmp/mle-audio-fixtures/bin/python tests/Client/tools/generate_audio_fixtures.py --verify
```

Output: `verified: committed fixtures match deterministic regeneration`
