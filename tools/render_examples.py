#!/usr/bin/env python3
"""Render catalog scenes with the C++ engine; PNG conversion never alters pixel values."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from examples.rendering import read_ppm, resolve_scene, write_png

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-release/bin/raytracer')
    parser.add_argument('--profile', choices=('preview', 'final'), default='final')
    parser.add_argument('--select', default='', help='Substring of catalog id; empty selects everything')
    parser.add_argument('--jobs', type=int, default=1)
    parser.add_argument('--threads', type=int, default=0)
    parser.add_argument('--samples', type=int)
    parser.add_argument('--output', type=Path, help='Alternate image root; useful for previews')
    parser.add_argument('--timeout', type=int, default=1800)
    parser.add_argument('--report', type=Path, help='Optional local diagnostics JSON; keep outside Git')
    args = parser.parse_args()
    if (args.jobs < 1 or not 0 <= args.threads <= 256 or args.timeout <= 0 or
            (args.samples is not None and not 1 <= args.samples <= 4096)):
        parser.error('jobs >= 1, threads in [0,256], timeout > 0 and samples in [1,4096] are required')
    binary = args.binary.resolve()
    if not binary.is_file():
        parser.error(f'build the renderer first: {binary}')
    renderer_digest = digest(binary)
    entries = json.loads((ROOT / 'gallery/catalog.json').read_text())['entries']
    entries = [entry for entry in entries if args.select in entry['id']]
    if not entries:
        parser.error('selection matches no examples')
    output_root = args.output.resolve() if args.output else ROOT / 'gallery'
    records = []

    def render(entry):
        source = ROOT / entry['scene']
        source_digest = digest(source)
        data = resolve_scene(json.loads(source.read_text()), source.parent)
        settings = data['render']
        if args.profile == 'preview':
            settings['height'] = round(settings['height'] * 320 / settings['width'])
            settings['width'] = 320
            settings['samples'] = 8
            settings['depth'] = min(settings['depth'], 6)
        if args.samples is not None:
            settings['samples'] = args.samples
        settings['threads'] = args.threads or min(256, max(1, (os.cpu_count() or 1) // args.jobs))
        image = output_root / Path(entry['scene']).with_suffix('.png').name
        with tempfile.TemporaryDirectory(prefix='raytracer-example-') as temporary:
            work = Path(temporary)
            ppm = work / 'image.ppm'
            settings['output'] = str(ppm)
            config = work / 'scene.json'
            config.write_text(json.dumps(data))
            started = time.monotonic()
            result = subprocess.run([binary, config], cwd=ROOT, capture_output=True,
                                    text=True, timeout=args.timeout)
            elapsed = time.monotonic() - started
            if result.returncode:
                raise RuntimeError(f"{entry['id']}: {result.stderr.strip()}")
            width, height, pixels = read_ppm(ppm)
            if len(set(pixels)) < 16:
                raise RuntimeError(f"{entry['id']}: image appears empty")
            write_png(image, width, height, pixels)
            shutil.copyfile(ppm, image.with_suffix('.ppm'))
        record = {'id': entry['id'], 'renderer_sha256': renderer_digest,
                  'source_sha256': source_digest, 'image_sha256': digest(image),
                  'ppm_sha256': digest(image.with_suffix('.ppm')),
                  'width': width, 'height': height, 'samples': settings['samples'],
                  'depth': settings['depth'], 'threads': settings['threads'],
                  'seconds': round(elapsed, 3), 'integrator': settings['integrator']}
        print(f"{entry['id']}: {width}x{height}, {settings['samples']} spp, {elapsed:.2f}s", flush=True)
        return record

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for future in as_completed([pool.submit(render, entry) for entry in entries]):
            records.append(future.result())
    if digest(binary) != renderer_digest:
        raise RuntimeError('Renderer changed during the batch; rerun with a stable build')
    if args.report is None:
        return
    report = {'renderer_sha256': renderer_digest, 'profile': args.profile,
              'source_revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
              'dirty_source': bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT)),
              'pixel_processing': 'P6 RGB bytes encoded losslessly as PNG; no denoising or retouching',
              'images': sorted(records, key=lambda row: row['id'])}
    report_file = args.report.resolve()
    report_file.parent.mkdir(parents=True, exist_ok=True)
    if report_file.exists():
        previous = json.loads(report_file.read_text())
        catalog_ids = {e['id'] for e in json.loads((ROOT / 'gallery/catalog.json').read_text())['entries']}
        combined = {r['id']: dict(renderer_sha256=previous['renderer_sha256'], **r)
                    if 'renderer_sha256' not in r else r
                    for r in previous['images'] if r['id'] in catalog_ids}
        combined.update({r['id']: r for r in report['images']})
        report['images'] = sorted(combined.values(), key=lambda row: row['id'])
    report_file.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
