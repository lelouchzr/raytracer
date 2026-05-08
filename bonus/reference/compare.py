#!/usr/bin/env python3
"""Compare the C++ renderer with a linear PFM previously rendered by PBRT."""

import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile


def mean_pfm(path):
    with path.open("rb") as source:
        if source.readline().strip() != b"PF":
            raise ValueError("RGB PFM required")
        width, height = map(int, source.readline().split())
        scale = float(source.readline())
        data = source.read()
    values = struct.unpack(
        ("<" if scale < 0 else ">") + "f" * (width * height * 3), data
    )
    return [sum(values[channel::3]) / (width * height) for channel in range(3)]


parser = argparse.ArgumentParser()
parser.add_argument(
    "--raytracer", type=Path, default=Path(__file__).resolve().parents[2] / "raytracer"
)
parser.add_argument(
    "--reference", type=Path, default=Path(__file__).with_name("pbrt-diffuse.pfm")
)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix="rt-reference-") as temporary:
    folder = Path(temporary)
    scene = json.loads(Path(__file__).with_name("diffuse.json").read_text())
    output = folder / "render.pfm"
    scene["render"]["output"] = str(output)
    config = folder / "scene.json"
    config.write_text(json.dumps(scene))
    subprocess.run([str(args.raytracer.resolve()), str(config)], check=True)
    actual, reference = mean_pfm(output), mean_pfm(args.reference)
relative = max(abs(a - b) / max(abs(b), 1e-12) for a, b in zip(actual, reference))
print(
    json.dumps(
        {
            "raytracer_rgb": actual,
            "pbrt_rgb": reference,
            "maximum_relative_channel_error": relative,
        },
        indent=2,
    )
)
if relative > 0.01:
    raise SystemExit("Diffuse reference differs by more than 1%")
