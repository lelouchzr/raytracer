#!/usr/bin/env python3
"""Regenerate authored example JSONs, their catalog and the original sculpture mesh."""
import json
from pathlib import Path
from examples.head import head_mesh
from examples.physical import PHYSICAL
from examples.subjects import SUBJECTS

ROOT = Path(__file__).resolve().parents[1]


def write_scene(slug, title, data, entries, **metadata):
    path = ROOT / 'gallery' / f'{slug}.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    data['render']['output'] = f'gallery/{slug}.ppm'
    path.write_text(json.dumps(data, indent=2) + '\n')
    entries.append({'id': slug, 'title': title, 'scene': str(path.relative_to(ROOT)),
                    'image': f'gallery/{slug}.png', 'ppm': f'gallery/{slug}.ppm', **metadata})


def main():
    entries = []
    (ROOT / 'gallery').mkdir(exist_ok=True)
    (ROOT / 'gallery/07_cornell_sculpture.obj').write_text(head_mesh())
    for slug, title, factory in (*SUBJECTS, *PHYSICAL):
        write_scene(slug, title, factory(), entries,
                    purpose='subject illustration' if slug[:2] in ('01', '02', '03', '04') else 'physical rendering')
    catalog = ROOT / 'gallery/catalog.json'
    catalog.parent.mkdir(parents=True, exist_ok=True)
    catalog.write_text(json.dumps({'schema': 1, 'entries': entries}, indent=2) + '\n')
    print(f'Generated {len(entries)} standalone scenes and one original OBJ sculpture')


if __name__ == '__main__':
    main()
