#!/usr/bin/env python3
"""Compare every demonstration scene across builds without changing tracked renders."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def resolve_assets(value: object, directory: Path) -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            if key in ("file", "normalFile") and isinstance(child, str):
                value[key] = str((directory / child).resolve())
            else:
                resolve_assets(child, directory)
    elif isinstance(value, list):
        for child in value:
            resolve_assets(child, directory)


def render(binary: Path, source: Path, folder: Path) -> dict:
    scene = json.loads(source.read_text())
    resolve_assets(scene, source.parent)
    if "plugins" in scene:
        plugin_directory = binary.parent.parent / "plugins"
        scene["plugins"] = [str(plugin_directory / Path(name).name)
                            for name in scene["plugins"]]
    image = folder / "result.ppm"
    settings = scene.setdefault("render", {})
    settings.update(width=80, height=48, samples=4, ao=1, threads=1,
                    output=str(image))
    if "minSamples" in settings:
        settings["minSamples"] = min(4, settings["minSamples"])
    if "photons" in settings:
        settings["photons"] = 5000
    config = folder / "scene.json"
    config.write_text(json.dumps(scene))
    image.unlink(missing_ok=True)
    result = subprocess.run([str(binary), str(config)], cwd=ROOT,
                            capture_output=True, text=True, timeout=120)
    if result.returncode:
        raise RuntimeError(f"{source.name}: {binary} exited {result.returncode}: "
                           f"{result.stderr.strip()}")
    if not image.is_file() or not image.read_bytes().startswith(b"P6\n80 48\n255\n"):
        raise RuntimeError(f"{source.name}: missing or invalid miniature PPM")
    return {"sha256": digest(image), "bytes": image.stat().st_size}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    binaries = {name: path.resolve(strict=True) for name, path in
                (("baseline", args.baseline), ("candidate", args.candidate))}
    scenes = sorted((ROOT / "scenes").glob("*.json"))
    scenes += sorted((ROOT / "bonus" / "scenes").glob("*.json"))
    if not scenes:
        raise RuntimeError("no demonstration scenes found")
    report = {
        "purpose": "Functional miniature comparison; these are not performance timings",
        "render_overrides": {"width": 80, "height": 48, "samples": 4,
                             "ao": 1, "threads": 1, "photons_if_present": 5000,
                             "minSamples_if_present": "capped at 4"},
        "binaries": {name: {"path": str(path), "sha256": digest(path)}
                     for name, path in binaries.items()},
        "scenes": [],
    }
    with tempfile.TemporaryDirectory(prefix="raytracer-compare-") as temporary:
        folder = Path(temporary)
        for source in scenes:
            results = {name: render(binary, source, folder)
                       for name, binary in binaries.items()}
            if results["baseline"] != results["candidate"]:
                raise RuntimeError(f"{source.relative_to(ROOT)}: image changed: {results}")
            report["scenes"].append({"source": str(source.relative_to(ROOT)),
                                     "source_sha256": digest(source), **results})
            print(f"IDENTICAL {source.relative_to(ROOT)}", flush=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(scenes)} demonstration scenes are byte-identical across builds")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"compare-scenes: {error}", file=sys.stderr)
        sys.exit(1)
