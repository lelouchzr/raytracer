#!/usr/bin/env python3
"""Create a versioned Raytracer archive with native studio, resources and checksums."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile


def command(*args):
    return subprocess.check_output(args, text=True).strip()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build', type=Path)
    parser.add_argument('--version', required=True)
    parser.add_argument('--output', type=Path, default=Path('dist'))
    args = parser.parse_args()
    if not re.fullmatch(r'v[0-9]+\.[0-9]+\.[0-9]+', args.version):
        parser.error('version must use vMAJOR.MINOR.PATCH')
    root = Path(__file__).resolve().parents[1]
    build = args.build.resolve()
    cache = (build / 'CMakeCache.txt').read_text().splitlines()
    for setting in ('CMAKE_BUILD_TYPE:STRING=Release', 'RAYTRACER_BONUS:BOOL=ON',
                    'RAYTRACER_SANITIZE:BOOL=OFF', 'RAYTRACER_WERROR:BOOL=ON',
                    'RAYTRACER_REQUIRE_GUI:BOOL=ON'):
        if setting not in cache:
            raise RuntimeError(f'Release build requires {setting}')
    if command('git', '-C', str(root), 'status', '--porcelain'):
        raise RuntimeError('Release packaging requires a clean source checkout')
    system = {'Darwin': 'macos', 'Linux': 'linux'}.get(platform.system())
    architecture = {'aarch64': 'arm64', 'arm64': 'arm64', 'x86_64': 'x86_64'}.get(platform.machine())
    if not system or not architecture:
        raise RuntimeError('Supported release platforms: Linux/macOS, arm64/x86_64')
    deployment = next((line.split('=', 1)[1] for line in cache
                       if line.startswith('CMAKE_OSX_DEPLOYMENT_TARGET:')), '')
    if system == 'macos' and not re.fullmatch(r'[0-9]+(?:\.[0-9]+){0,2}', deployment):
        raise RuntimeError('Set an explicit CMAKE_OSX_DEPLOYMENT_TARGET for macOS packaging')
    libc_name, libc_version = platform.libc_ver()
    name = f'raytracer-{args.version}-{system}-{architecture}'
    commit = command('git', '-C', str(root), 'rev-parse', 'HEAD')
    epoch = int(command('git', '-C', str(root), 'show', '-s', '--format=%ct', 'HEAD'))
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f'{name}.tar.gz'
    if destination.exists():
        raise RuntimeError(f'Refusing to replace existing archive: {destination}')
    with tempfile.TemporaryDirectory(prefix='raytracer-package-') as temporary:
        package = Path(temporary) / name
        package.mkdir()
        shutil.copy2(build / 'bin/raytracer', package / 'raytracer')
        (package / 'plugins').mkdir()
        shutil.copy2(build / 'plugins/raytracer_example.so', package / 'plugins/raytracer_example.so')
        shutil.copytree(root / 'scenes', package / 'scenes')
        for directory in ('assets', 'scenes'):
            shutil.copytree(root / 'bonus' / directory, package / 'bonus' / directory)
        (package / 'screenshots').mkdir()
        for notice in ('LICENSE', 'THIRD_PARTY_NOTICES.md'):
            shutil.copy2(root / notice, package / notice)
        (package / 'licenses').mkdir()
        shutil.copy2(root / 'bonus/vendor/imgui/LICENSE.txt', package / 'licenses/imgui.txt')
        binaries = [package / 'raytracer', package / 'plugins/raytracer_example.so']
        if system == 'macos':
            app = package / 'Atelier.app'
            shutil.copytree(build / 'Atelier.app', app, symlinks=True)
            notices = app / 'Contents/Resources/licenses'
            notices.mkdir(parents=True, exist_ok=True)
            for notice in ('LICENSE', 'THIRD_PARTY_NOTICES.md'):
                shutil.copy2(root / notice, notices / notice)
            shutil.copy2(root / 'bonus/vendor/imgui/LICENSE.txt', notices / 'imgui.txt')
            binaries.append(app / 'Contents/MacOS/Atelier')
        (package / 'README.txt').write_text(f'''Raytracer / Atelier {args.version} — {system} {architecture}

Extract the entire directory. Keep bonus/, scenes/ and plugins/ beside raytracer.
    ./raytracer --studio
    ./raytracer --preview bonus/scenes/pbr_showcase.json
    ./raytracer --gui --watch bonus/scenes/pbr_showcase.json
macOS also includes Atelier.app. The application is unsigned and not notarized.

Resources are located beside the executable or in Atelier.app/Contents/Resources.
RAYTRACER_RESOURCE_ROOT overrides the resource directory. Explicit scene arguments
are relative to your current directory; scene assets are relative to that scene.
render.output is relative to the working directory, which must be writable.
The native studio can save JSON scenes and export PNG/PFM images.

Runtime dependencies are NOT bundled:
- Linux: {libc_name or 'glibc'} >= {libc_version or 'see manifest'} and a compatible
  libstdc++; SDL2, SFML graphics/window/system and zlib with the SONAMEs listed
  in manifest.json. A desktop display is needed for GUI modes.
- macOS: minimum target {deployment or 'see manifest'}, on the matching architecture.
  Install Homebrew SDL2, SFML
  and zlib (brew install sdl2 sfml zlib). Match manifest.json runtime versions.

The manifest records source commit, build platform, dynamic dependencies and hashes.
License: MIT for remake code; Dear ImGui license is in licenses/imgui.txt.
Other runtime licenses remain applicable: see THIRD_PARTY_NOTICES.md.
Source and controls: https://github.com/lelouchzr/raytracer
''')
        manifest = {
            'version': args.version, 'commit': commit, 'system': system,
            'architecture': architecture, 'build_os': platform.platform(),
            'minimum_macos': deployment if system == 'macos' else None,
            'libc': {'name': libc_name, 'version': libc_version} if system == 'linux' else None,
            'features': ['whitted', 'path', 'photon', 'pbr', 'native_studio', 'sfml_viewer',
                         'tcp_workers', 'example_plugin'],
            'runtime_dependencies': {
                str(p.relative_to(package)): command(*(['otool', '-L'] if system == 'macos' else ['ldd']),
                                                     str(p)).splitlines()[1:]
                for p in binaries},
            'files': {str(p.relative_to(package)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in sorted(package.rglob('*')) if p.is_file()},
        }
        (package / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')

        def normalize(info):
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mtime = epoch
            info.pax_headers = {}
            return info

        temporary_archive = destination.with_suffix('.gz.tmp')
        try:
            with temporary_archive.open('wb') as raw:
                with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=epoch) as compressed:
                    with tarfile.open(fileobj=compressed, mode='w') as archive:
                        for path in (package, *sorted(package.rglob('*'))):
                            archive.add(path, arcname=str(path.relative_to(package.parent)),
                                        recursive=False, filter=normalize)
            temporary_archive.replace(destination)
        finally:
            temporary_archive.unlink(missing_ok=True)
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    destination.with_name(destination.name + '.sha256').write_text(f'{digest}  {destination.name}\n')
    print(destination)


if __name__ == '__main__':
    main()
