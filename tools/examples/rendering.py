"""Resolve authored scene resources and encode unmodified renderer pixels as PNG."""
import copy
from pathlib import Path
import struct
import zlib


def resolve_scene(data, base):
    result = copy.deepcopy(data)

    def visit(value):
        if isinstance(value, dict):
            for key, child in value.items():
                if key in ('file', 'normalFile') and isinstance(child, str):
                    value[key] = str((base / child).resolve())
                elif key in ('plugins', 'include'):
                    value[key] = [str((base / path).resolve()) for path in child]
                else:
                    visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)
    visit(result)
    return result


def read_ppm(path):
    with Path(path).open('rb') as source:
        if source.readline() != b'P6\n':
            raise ValueError(f'{path}: expected renderer P6 output')
        width, height = map(int, source.readline().split())
        if source.readline() != b'255\n':
            raise ValueError(f'{path}: expected eight-bit RGB')
        pixels = source.read()
    if len(pixels) != width * height * 3:
        raise ValueError(f'{path}: truncated pixel data')
    return width, height, pixels


def write_png(path, width, height, pixels):
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + pixels[row * width * 3:(row + 1) * width * 3] for row in range(height))
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(rows, 6)) + chunk(b'IEND', b'')
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
