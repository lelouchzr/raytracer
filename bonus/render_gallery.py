#!/usr/bin/env python3
"""Regenerate defense PPMs with the compiled engine; optionally create PNG previews."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
scenes = sorted((root / "scenes").glob("*.json"))
scenes += sorted((root / 'bonus' / 'scenes').glob('*.json'))
scenes += [root / 'bonus' / 'quality' / f'{name}.json' for name in ('bsdf', 'nee')]
try:
    from PIL import Image
except ImportError:
    Image = None

binary = root / "raytracer"
if not binary.is_file():
    raise SystemExit("Build the bonus engine first: make")
(root / "screenshots").mkdir(exist_ok=True)
for scene in scenes:
    subprocess.run([str(binary), str(scene)], cwd=root, check=True)
captures = sorted((root / "screenshots").glob("*.ppm"))
captures += sorted((root / "bonus" / "quality").glob("*.ppm"))
for ppm in captures:
    if Image:
        with Image.open(ppm) as image:
            image.save(ppm.with_suffix(".png"))
print("Gallery regenerated." + (" PNG previews saved." if Image else " Install Pillow for optional PNG previews."))
