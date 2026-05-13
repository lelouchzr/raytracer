#!/usr/bin/env python3
"""Open the real native C++ studio and verify its actual framebuffer PNG."""

import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib

binary = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix="native-gui-") as name:
    folder = Path(name)
    scene = {
        "camera": {"position": [0, 1, 5], "target": [0, 0, 0]},
        "render": {
            "width": 80,
            "height": 48,
            "samples": 2,
            "threads": 1,
            "integrator": "pbr",
            "depth": 4,
            "toneMap": "neutral",
        },
        "materials": {
            "m": {"type": "pbr", "color": [0.7, 0.04, 0.02], "roughness": 0.35}
        },
        "objects": [{"type": "sphere", "material": "m"}],
        "lights": [
            {
                "type": "area",
                "position": [0, 4, 2],
                "u": [2, 0, 0],
                "v": [0, 0, 2],
                "intensity": 6,
            }
        ],
    }
    path = folder / "scène.json"
    path.write_text(json.dumps(scene))
    png = folder / "native.png"
    result = subprocess.run(
        [
            str(binary),
            "--studio",
            str(path),
            "--frames",
            "45",
            "--screenshot",
            str(png),
        ],
        capture_output=True,
        timeout=25,
    )
    assert result.returncode == 0, result.stderr.decode(errors="replace")
    data = png.read_bytes()
    assert data.startswith(b"\x89PNG\r\n\x1a\n")
    cursor, compressed = 8, bytearray()
    while cursor < len(data):
        length = struct.unpack_from(">I", data, cursor)[0]
        kind = data[cursor + 4 : cursor + 8]
        payload = data[cursor + 8 : cursor + 8 + length]
        expected = struct.unpack_from(">I", data, cursor + 8 + length)[0]
        assert zlib.crc32(kind + payload) == expected, "invalid PNG checksum"
        if kind == b"IHDR":
            width, height, depth, color, *_ = struct.unpack(">IIBBBBB", payload)
            assert width >= 1000 and height >= 650 and depth == 8 and color == 6
        if kind == b"IDAT":
            compressed.extend(payload)
        cursor += length + 12
    pixels = zlib.decompress(compressed)
    assert len(pixels) == height * (width * 4 + 1)
    assert len(set(pixels)) > 64, "native GUI framebuffer is blank"
    for arguments in (["--frames", "2junk"], ["--port", "0"]):
        bad = subprocess.run(
            [str(binary), "--studio", *arguments], capture_output=True, timeout=5
        )
        assert bad.returncode == 84
print(
    "Native SDL2/ImGui window, real framebuffer PNG, lifecycle and CLI validation passed"
)
