#!/usr/bin/env python3
"""Render the selected scenes and verify the published PNG/PPM pairs contain identical pixels."""
import json
from collections import Counter
from pathlib import Path
import subprocess
import sys
import tempfile
import struct
import zlib

binary = Path(sys.argv[1]).resolve()
root = Path(sys.argv[2]).resolve()
sys.path.insert(0, str(root / 'tools'))
from examples.rendering import read_ppm, resolve_scene

entries = json.loads((root / 'gallery/catalog.json').read_text())['entries']
assert len({e['id'] for e in entries}) == len(entries) == 7
assert {root / e['scene'] for e in entries} == set((root / 'gallery').glob('[0-9]*.json'))
assert not (root / 'screenshots').exists(), 'the old screenshot collection must not return'


def sculpture_topology():
    vertices, faces = [], []
    for line in (root / 'gallery/07_cornell_sculpture.obj').read_text().splitlines():
        fields = line.split()
        if fields and fields[0] == 'v':
            vertices.append(tuple(float(v) for v in fields[1:]))
        elif fields and fields[0] == 'f':
            faces.append(tuple(int(v) - 1 for v in fields[1:]))
    edges, winding = Counter(), Counter()
    volume = 0
    for face in faces:
        assert len(face) == 3 and len(set(face)) == 3
        a, b, c = (vertices[i] for i in face)
        u, v = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
        normal = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2],
                  u[0] * v[1] - u[1] * v[0])
        assert sum(n * n for n in normal) > 1e-12, 'degenerate sculpture triangle'
        volume += sum(a[i] * normal[i] for i in range(3)) / 6
        for i, j in zip(face, face[1:] + face[:1]):
            edge = tuple(sorted((i, j)))
            edges[edge] += 1
            winding[edge] += 1 if i < j else -1
    assert vertices and faces and volume > 0, 'sculpture must face outwards'
    assert all(v == 2 for v in edges.values()), 'sculpture is not closed'
    assert all(v == 0 for v in winding.values()), 'inconsistent sculpture face normals'


sculpture_topology()


def published_pixels(entry):
    width, height, pixels = read_ppm(root / entry['ppm'])
    png = (root / entry['image']).read_bytes()
    assert png[:8] == b'\x89PNG\r\n\x1a\n'
    offset, compressed, ended = 8, bytearray(), False
    while offset < len(png):
        size = struct.unpack_from('>I', png, offset)[0]
        kind = png[offset + 4:offset + 8]
        payload = png[offset + 8:offset + 8 + size]
        crc = struct.unpack_from('>I', png, offset + 8 + size)[0]
        assert crc == zlib.crc32(kind + payload), 'corrupt PNG chunk'
        if kind == b'IHDR':
            assert struct.unpack('>IIBBBBB', payload) == (width, height, 8, 2, 0, 0, 0)
        elif kind == b'IDAT':
            compressed.extend(payload)
        elif kind == b'IEND':
            ended = True
        offset += size + 12
    # Gallery PNGs use our lossless encoder's unfiltered RGB scanlines.
    expected = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    assert ended and zlib.decompress(compressed) == expected, 'PNG and PPM pixels differ'
    return width, height


def explicit_transforms(objects):
    for obj in objects:
        for key in ('translate', 'rotate', 'scale', 'shear'):
            assert len(obj[key]) == 3, (obj['type'], key)
        if obj['type'] == 'group':
            explicit_transforms(obj['children'])


with tempfile.TemporaryDirectory(prefix='raytracer-gallery-check-') as name:
    work = Path(name)
    image, config = work / 'image.ppm', work / 'scene.json'

    def render(data):
        image.unlink(missing_ok=True)
        config.write_text(json.dumps(data))
        result = subprocess.run([binary, config], cwd=work, capture_output=True, text=True, timeout=25)
        assert result.returncode == 0, result.stderr
        width, height, pixels = read_ppm(image)
        assert (width, height) == (48, 32)
        assert len(set(pixels)) >= 16, 'blank scene'
        return pixels

    for entry in entries:
        source = root / entry['scene']
        data = json.loads(source.read_text())
        assert published_pixels(entry) == (data['render']['width'], data['render']['height'])
        explicit_transforms(data['objects'])
        data = resolve_scene(data, source.parent)
        data['render'].update(width=48, height=32, samples=4, depth=4, threads=2, output=str(image))
        try:
            pixels = render(data)
            if entry['id'] == '07_cornell_sculpture':
                red_pixels = sum(r > 60 and r > 2 * max(g, b)
                                 for r, g, b in zip(pixels[::3], pixels[1::3], pixels[2::3]))
                assert red_pixels > 20, 'the red sculpture is not visible in the camera'
        except Exception as error:
            raise AssertionError(f"{entry['id']}: {error}") from error
print('Seven scenes rendered; PNG/PPM pixels match; all transforms are explicit')
