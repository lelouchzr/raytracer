#!/usr/bin/env python3
"""Build labeled contact sheets from the actual rendered PNGs; requires Pillow."""
import json
import math
from pathlib import Path


def main():
    try:
        from PIL import Image, ImageDraw, ImageFont, ImageOps
    except ImportError as error:
        raise SystemExit('Contact sheets require Pillow: python3 -m pip install pillow') from error
    root = Path(__file__).resolve().parents[1]
    entries = json.loads((root / 'gallery/catalog.json').read_text())['entries']
    groups = {'overview': entries}
    for name, selected in groups.items():
        columns = 2
        cell_width = 640
        cell_height = round(cell_width * 0.72)
        margin, caption = 12, 42
        image = Image.new('RGB', (columns * cell_width,
                                  math.ceil(len(selected) / columns) * (cell_height + caption)), '#171c24')
        draw = ImageDraw.Draw(image)
        try:
            font = ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf', 19)
        except OSError:
            try:
                font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 18)
            except OSError:
                font = ImageFont.load_default()
        for i, entry in enumerate(selected):
            path = root / entry['image']
            if not path.is_file():
                raise SystemExit(f'Render the gallery first: missing {path}')
            rendered = Image.open(path).convert('RGB')
            rendered = ImageOps.contain(rendered, (cell_width - 2 * margin, cell_height - 2 * margin),
                                        Image.Resampling.LANCZOS)
            x, y = (i % columns) * cell_width, (i // columns) * (cell_height + caption)
            image.paste(rendered, (x + (cell_width - rendered.width) // 2,
                                   y + (cell_height - rendered.height) // 2))
            draw.text((x + margin, y + cell_height + 5), entry['title'], fill='#eef1f5', font=font)
        destination = root / 'gallery' / f'{name}.png'
        image.save(destination)
        print(destination.relative_to(root))


if __name__ == '__main__':
    main()
