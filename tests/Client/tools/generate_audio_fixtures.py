#!/usr/bin/env python3
"""Generate deterministic developer-only WAV fixtures for Client audio tests.

Environment and regeneration:
  python3 -m venv /tmp/mle-audio-fixtures
  /tmp/mle-audio-fixtures/bin/python -m pip install \
      -r tests/Client/audio_requirements.txt
  /tmp/mle-audio-fixtures/bin/python \
      tests/Client/tools/generate_audio_fixtures.py
  /tmp/mle-audio-fixtures/bin/python \
      tests/Client/tools/generate_audio_fixtures.py --verify

Normal builds and playtests consume committed assets and require no Python packages.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from scipy.io import wavfile


SAMPLE_RATE = 48_000
SEED = 0x4D4C45
PEAK = 0.88
PROTECTION_FILLER_PEAK = 0.015
OUTPUT_DIR = Path(__file__).resolve().parents[1] / "res" / "sounds" / "generated"
EXPECTED_FILENAMES = {
    "audio_manifest.json",
    "combat_high.wav",
    "combat_low.wav",
    "combat_texture.wav",
    "corrupt.wav",
    "duration_mono.wav",
    "duration_stereo.wav",
    "hit_01.wav",
    "hit_02.wav",
    "hit_03.wav",
    "hit_04.wav",
    "protection_filler.wav",
    "ui_protected.wav",
}


def envelope(frames: int, attack_ms: float = 8.0, release_ms: float = 20.0) -> np.ndarray:
    """Raised-cosine attack/release envelope with zero-valued endpoints."""
    result = np.ones(frames, dtype=np.float64)
    attack = min(frames // 2, max(2, round(SAMPLE_RATE * attack_ms / 1000.0)))
    release = min(frames // 2, max(2, round(SAMPLE_RATE * release_ms / 1000.0)))
    result[:attack] = 0.5 - 0.5 * np.cos(np.linspace(0.0, np.pi, attack))
    result[-release:] = 0.5 + 0.5 * np.cos(np.linspace(0.0, np.pi, release))
    return result


def normalize(samples: np.ndarray, target_peak: float = PEAK) -> np.ndarray:
    if not 0.0 < target_peak <= 1.0:
        raise ValueError(f"target_peak must be in (0, 1], got {target_peak}")
    peak = float(np.max(np.abs(samples)))
    if peak == 0.0:
        return samples
    return samples * (target_peak / peak)


def pcm16(samples: np.ndarray, target_peak: float = PEAK) -> np.ndarray:
    normalized = normalize(samples, target_peak)
    return np.rint(normalized * np.iinfo(np.int16).max).astype("<i2")


def tone(duration: float, frequencies: tuple[float, ...], amplitudes: tuple[float, ...]) -> np.ndarray:
    frames = round(SAMPLE_RATE * duration)
    time = np.arange(frames, dtype=np.float64) / SAMPLE_RATE
    signal = sum(a * np.sin(2.0 * np.pi * f * time) for f, a in zip(frequencies, amplitudes))
    return signal * envelope(frames)


def hit(rng: np.random.Generator, index: int) -> np.ndarray:
    duration = 0.16 + index * 0.018
    frames = round(SAMPLE_RATE * duration)
    time = np.arange(frames, dtype=np.float64) / SAMPLE_RATE
    decay = np.exp(-(22.0 - index) * time)
    pitch = 145.0 + 47.0 * index
    body = np.sin(2.0 * np.pi * (pitch * time + 210.0 * time * time))
    noise = rng.standard_normal(frames)
    signal = (0.72 * body + 0.28 * noise) * decay
    return signal * envelope(frames, attack_ms=1.5, release_ms=12.0)


def seamless_texture() -> np.ndarray:
    """One-second periodic tile: integer-cycle partials align across loop boundary."""
    frames = SAMPLE_RATE
    phase = 2.0 * np.pi * np.arange(frames, dtype=np.float64) / frames
    left = 0.46 * np.sin(53 * phase) + 0.27 * np.sin(127 * phase + 0.3) + 0.12 * np.sin(389 * phase)
    right = 0.44 * np.sin(59 * phase + 0.5) + 0.25 * np.sin(131 * phase) + 0.14 * np.sin(383 * phase + 0.2)
    return np.column_stack((left, right))


@dataclass(frozen=True)
class Asset:
    filename: str
    samples: np.ndarray
    role: str
    target_peak: float = PEAK


def assets() -> list[Asset]:
    rng = np.random.default_rng(SEED)
    mono_duration = tone(0.75, (330.0, 660.0), (0.8, 0.2))
    stereo_duration = np.column_stack(
        (tone(1.25, (220.0, 440.0), (0.75, 0.25)), tone(1.25, (277.0, 554.0), (0.75, 0.25)))
    )
    return [
        Asset("combat_low.wav", tone(0.45, (90.0, 135.0), (0.8, 0.2)), "low-frequency combat layer"),
        Asset("combat_high.wav", tone(0.45, (1800.0, 2700.0), (0.7, 0.3)), "high-frequency combat layer"),
        Asset("ui_protected.wav", tone(1.4, (880.0, 1320.0), (0.75, 0.25)), "long protected UI voice for saturation A/B test"),
        Asset("protection_filler.wav", tone(5.0, (55.0, 82.5), (0.07, 0.03)), "quiet long filler for protected-bus saturation A/B test",
              target_peak=PROTECTION_FILLER_PEAK),
        *(Asset(f"hit_{i:02d}.wav", hit(rng, i), f"combat one-shot variant {i}") for i in range(1, 5)),
        Asset("duration_mono.wav", mono_duration, "known-duration mono fixture"),
        Asset("duration_stereo.wav", stereo_duration, "known-duration stereo fixture"),
        Asset("combat_texture.wav", seamless_texture(), "phase-aligned seamless stereo combat texture"),
    ]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate(output_dir: Path, report: bool = True) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    manifest: list[dict[str, object]] = []
    for asset in assets():
        data = pcm16(asset.samples, asset.target_peak)
        pcm_peak = float(np.max(np.abs(data.astype(np.int32)))) / np.iinfo(np.int16).max
        if asset.filename == "protection_filler.wav":
            assert asset.target_peak == PROTECTION_FILLER_PEAK
            assert abs(pcm_peak - PROTECTION_FILLER_PEAK) <= 1.0 / np.iinfo(np.int16).max
        path = output_dir / asset.filename
        wavfile.write(path, SAMPLE_RATE, data)
        channels = 1 if data.ndim == 1 else int(data.shape[1])
        manifest.append({
            "filename": asset.filename,
            "sha256": sha256(path),
            "sample_rate": SAMPLE_RATE,
            "channels": channels,
            "frames": int(data.shape[0]),
            "role": asset.role,
            "target_peak": asset.target_peak,
            "pcm_peak": pcm_peak,
        })
        if report:
            print(asset.filename)

    corrupt_path = output_dir / "corrupt.wav"
    corrupt_path.write_bytes(b"RIFF\x24\x00\x00\x00WAVEfmt \x10\x00")
    manifest.append({
        "filename": "corrupt.wav",
        "sha256": sha256(corrupt_path),
        "sample_rate": None,
        "channels": None,
        "frames": None,
        "role": "intentionally truncated RIFF header; invalid audio fixture",
    })
    if report:
        print("corrupt.wav")

    manifest_path = output_dir / "audio_manifest.json"
    manifest_path.write_text(json.dumps({"assets": manifest}, indent=2) + "\n", encoding="utf-8")
    if report:
        print("audio_manifest.json")
    return manifest_path


def verify() -> int:
    committed_names = {path.name for path in OUTPUT_DIR.iterdir() if path.is_file()}
    if committed_names != EXPECTED_FILENAMES:
        print(f"committed fixture set differs: expected={sorted(EXPECTED_FILENAMES)} committed={sorted(committed_names)}")
        return 1
    with tempfile.TemporaryDirectory(prefix="mle-audio-fixtures-") as temp:
        generated_dir = Path(temp)
        generate(generated_dir, report=False)
        actual_names = {path.name for path in generated_dir.iterdir() if path.is_file()}
        if actual_names != EXPECTED_FILENAMES:
            print(f"generated fixture set differs: expected={sorted(EXPECTED_FILENAMES)} generated={sorted(actual_names)}")
            return 1
        mismatches = [name for name in sorted(actual_names) if (OUTPUT_DIR / name).read_bytes() != (generated_dir / name).read_bytes()]
        if mismatches:
            print("non-deterministic or stale fixtures: " + ", ".join(mismatches))
            return 1
    print("verified: committed fixtures match deterministic regeneration")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--verify", action="store_true", help="regenerate in a temp directory and byte-compare")
    args = parser.parse_args()
    if args.verify:
        return verify()
    generate(OUTPUT_DIR)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
