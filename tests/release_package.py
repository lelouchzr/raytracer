#!/usr/bin/env python3
"""Verify the exact extracted release, including relocated resources and real rendering."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tarfile
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--gui', action='store_true', help='Also exercise actual SDL2 and SFML windows')
    args = parser.parse_args()
    archive = args.archive.resolve()
    checksum = archive.with_name(archive.name + '.sha256').read_text().split()[0]
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == checksum
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix='raytracer release ') as temporary:
        work = Path(temporary)
        with tarfile.open(archive) as source:
            members = source.getmembers()
            for member in members:
                path = Path(member.name)
                assert not path.is_absolute() and '..' not in path.parts, member.name
                assert member.isdir() or member.isfile(), member.name
            source.extractall(work, members=members)
        package = work / archive.name.removesuffix('.tar.gz')
        manifest = json.loads((package / 'manifest.json').read_text())
        assert len(manifest['commit']) == 40
        for name, expected in manifest['files'].items():
            assert hashlib.sha256((package / name).read_bytes()).hexdigest() == expected, name
        assert (package / 'licenses/imgui.txt').is_file()
        assert not list(package.rglob('*.log'))
        binary = package / 'raytracer'
        environment = dict(os.environ)
        environment.pop('RAYTRACER_RESOURCE_ROOT', None)

        def run(arguments, expected=0, env=environment, executable=binary):
            result = subprocess.run([executable, *map(str, arguments)], cwd=work,
                                    env=env, capture_output=True, timeout=60)
            assert result.returncode == expected, (arguments, result.returncode, result.stderr)
            return result

        assert b'--studio' in run(['--help']).stdout
        for arguments in ([], ['missing.json'], ['--unknown']):
            assert run(arguments, 84).stderr
        image = work / 'image.ppm'
        config = work / 'scene.json'
        scene = {
            'camera': {'position': [0, 1, 5], 'target': [0, 0, 0]},
            'render': {'width': 40, 'height': 24, 'samples': 4, 'threads': 1,
                       'depth': 4, 'photons': 1000, 'output': str(image)},
            'materials': {'m': {'type': 'phong', 'color': [0.8, 0.05, 0.02]}},
            'objects': [{'type': 'sphere', 'material': 'm'}],
            'lights': [{'type': 'ambient', 'intensity': 0.1},
                       {'type': 'point', 'position': [-2, 4, 3], 'intensity': 12}],
        }

        def render(document):
            config.write_text(json.dumps(document))
            image.unlink(missing_ok=True)
            run([config])
            pixels = image.read_bytes()
            header = b'P6\n40 24\n255\n'
            assert pixels.startswith(header) and len(pixels) == len(header) + 40 * 24 * 3
            assert len(set(pixels[len(header):])) > 20, 'blank or unlit render'
            return pixels

        for integrator in ('whitted', 'path', 'photon', 'pbr'):
            scene['render']['integrator'] = integrator
            original = render(scene)
            scene['render']['threads'] = 3
            assert render(scene) == original, 'thread count changed rendering'
            scene['render']['threads'] = 1
        plugin = copy.deepcopy(scene)
        plugin['plugins'] = [str(package / 'plugins/raytracer_example.so')]
        plugin['objects'][0]['type'] = 'plugin_sphere'
        render(plugin)
        mesh = copy.deepcopy(scene)
        mesh['objects'] = [{'type': 'obj', 'material': 'm',
                            'file': str(package / 'bonus/assets/crystal.obj')}]
        render(mesh)
        mesh['environment'] = {'file': str(package / 'bonus/assets/studio.hdr'), 'intensity': 0.5}
        render(mesh)

        # Use the extracted examples, not the source checkout, without a desktop dependency.
        screenshot = work / 'default studio.png'
        headless = dict(environment, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
        run(['--studio', '--frames', '3', '--screenshot', screenshot], env=headless)
        assert screenshot.read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
        if platform.system() == 'Darwin':
            app = package / 'Atelier.app/Contents/MacOS/Atelier'
            assert (app.parent.parent / 'Resources/licenses/imgui.txt').is_file()
            screenshot.unlink()
            run(['--frames', '3', '--screenshot', screenshot], env=headless, executable=app)
            assert screenshot.is_file()
        if args.gui:
            for script in ('native_gui.py', 'gui.py'):
                subprocess.run([sys.executable, root / 'tests' / script, binary],
                               cwd=work, env=environment, check=True, timeout=60)
        print('Release verified: SHA-256, relocation, CLI errors, four deterministic integrators, '
              'plugin, OBJ, HDR, default studio resources'
              + (', macOS app' if platform.system() == 'Darwin' else '')
              + (', real SDL2/SFML windows' if args.gui else ''))


if __name__ == '__main__':
    main()
