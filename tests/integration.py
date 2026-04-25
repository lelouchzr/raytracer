#!/usr/bin/env python3
"""Exercise the real CLI, strict configuration, PPM output, and deterministic workers."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile

binary = Path(sys.argv[1]).resolve()
bonus = len(sys.argv) > 3 and sys.argv[3].upper() in {"ON", "TRUE", "1"}
checks = 0


def run(arguments, expected=0):
    global checks
    result = subprocess.run([str(binary), *map(str, arguments)], capture_output=True, timeout=60)
    assert result.returncode == expected, (arguments, result.returncode, result.stderr)
    if expected:
        assert result.stderr and not result.stdout, result
    checks += 1
    return result


with tempfile.TemporaryDirectory(prefix="raytracer-tests-") as tmp:
    directory = Path(tmp)
    output = directory / "image.ppm"
    scene = {
        "camera": {"position": [0, 1, 5], "target": [0, 0, 0]},
        "render": {"width": 40, "height": 24, "output": str(output)},
        "materials": {"red": {"color": [0.8, 0.1, 0.04]}},
        "objects": [{"type": "sphere", "material": "red"}],
        "lights": [{"type": "ambient", "intensity": 0.2},
                   {"type": "directional", "direction": [-1, -2, -1]}],
    }
    config = directory / "scene.json"

    def save(value):
        config.write_text(json.dumps(value))

    save(scene)
    run(["--help"])
    run([] ,84)
    run([config, config], 84)
    run([directory / "missing.json"], 84)
    run([config])
    data = output.read_bytes()
    header = b"P6\n40 24\n255\n"
    assert data.startswith(header) and len(data) == len(header) + 40 * 24 * 3
    assert len(set(data[len(header):])) > 30, "render appears blank or unlit"
    assert not list(directory.glob("*.tmp.*"))
    original = data
    dark = copy.deepcopy(scene)
    dark["lights"] = []
    save(dark)
    run([config])
    assert output.read_bytes() != original, "lighting does not affect output"

    for text in ('{', '[1,]', '{"a":1,"a":2}', '01', '1e999', '{"camera":null}',
                 '[' * 140 + '0' + ']' * 140):
        config.write_text(text)
        run([config], 84)

    for mutate in (
        lambda s: s["render"].update(width=0),
        lambda s: s["render"].update(width=3.5),
        lambda s: s["render"].update(width=8192, height=8192),
        lambda s: s["render"].update(typo=4),
        lambda s: s["camera"].update(target=[0, 1, 5]),
        lambda s: s["camera"].update(up=[0, -1, -5]),
        lambda s: s["objects"][0].update(radius=-1),
        lambda s: s["objects"][0].update(type="unknown"),
        lambda s: s["objects"][0].update(material="missing"),
        lambda s: s["materials"]["red"].update(color=[2, 0, 0]),
        lambda s: s.update(objects=[]),
        lambda s: s["render"].update(output=str(directory / "missing" / "out.ppm")),
    ):
        invalid = copy.deepcopy(scene)
        mutate(invalid)
        save(invalid)
        run([config], 84)
        assert not list(directory.glob("*.tmp.*"))

    if bonus:
        complex_scene = copy.deepcopy(scene)
        complex_scene["render"].update(samples=13, ao=3, threads=1)
        complex_scene["materials"]["red"].update(type="phong", transmission=0.75,
                                                reflection=0.05, ior=1.5)
        complex_scene["materials"]["floor"] = {"type": "phong", "pattern": "checker",
                                                  "color": [0.8, 0.8, 0.8], "second": [0.1, 0.1, 0.1]}
        complex_scene["objects"] += [{"type": "plane", "material": "floor", "translate": [0, -1, 0]}]
        save(complex_scene)
        run([config])
        one_worker = output.read_bytes()
        complex_scene["render"]["threads"] = 4
        save(complex_scene)
        run([config])
        assert output.read_bytes() == one_worker, "thread count changed deterministic output"
        complex_scene["render"]["samples"] = 0
        save(complex_scene)
        run([config], 84)
        cyclic = copy.deepcopy(scene)
        cyclic["include"] = ["scene.json"]
        save(cyclic)
        run([config], 84)
        imported = directory / "objects.json"
        imported.write_text(json.dumps({"materials": scene["materials"], "objects": scene["objects"]}))
        importer = copy.deepcopy(scene)
        del importer["materials"]
        del importer["objects"]
        importer["include"] = ["objects.json"]
        save(importer)
        run([config])
        invalid_obj = directory / "broken.obj"
        invalid_obj.write_text("v 0 0 0\nf 1 2 9\n")
        obj_scene = copy.deepcopy(scene)
        obj_scene["objects"] = [{"type": "obj", "file": str(invalid_obj), "material": "red"}]
        save(obj_scene)
        run([config], 84)
        for fixture in ("gallery", "observatory", "geometry", "obj", "texture", "bvh_garden"):
            source = Path(sys.argv[2]) / "bonus" / "scenes" / (fixture + ".json")
            contents = json.loads(source.read_text())
            # Preserve relative imports/assets by writing the temporary scene beside its source.
            contents["render"].update(width=48, height=32, samples=2, ao=0, output=str(output))
            # Root-relative paths are explicitly normalized for a temporary scene location.
            for obj in contents.get("objects", []):
                if "file" in obj:
                    obj["file"] = str((source.parent / obj["file"]).resolve())
            for material in contents.get("materials", {}).values():
                if "file" in material:
                    material["file"] = str((source.parent / material["file"]).resolve())
            if "include" in contents:
                contents["include"] = [str((source.parent / p).resolve()) for p in contents["include"]]
            save(contents)
            run([config])
            assert output.stat().st_size > 48 * 32 * 3

print(f"{checks} CLI scenarios passed; bonus={bonus}")
