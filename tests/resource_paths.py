#!/usr/bin/env python3
"""Verify installed, macOS bundle, source and explicit resource lookup paths."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


binary, source = (Path(value).resolve() for value in sys.argv[1:])
env = dict(os.environ)
env.pop('RAYTRACER_RESOURCE_ROOT', None)
with tempfile.TemporaryDirectory(prefix='raytracer resources ') as name:
    work = Path(name)

    def resources(root):
        (root / 'bonus/scenes').mkdir(parents=True)
        (root / 'bonus/assets').mkdir(parents=True)

    def check(executable, expected, environment=env):
        result = subprocess.run([executable], cwd=work, env=environment,
                                capture_output=True, text=True, timeout=10)
        assert result.returncode == 0, result.stderr
        assert Path(result.stdout.strip()) == expected.resolve(), result.stdout

    check(binary, source)
    package = work / 'moved package'
    resources(package)
    installed = package / 'raytracer'
    shutil.copy2(binary, installed)
    check(installed, package)
    contents = work / 'Atelier.app/Contents'
    resources(contents / 'Resources')
    (contents / 'MacOS').mkdir()
    app = contents / 'MacOS/Atelier'
    shutil.copy2(binary, app)
    check(app, contents / 'Resources')
    override = work / 'explicit assets'
    resources(override)
    check(installed, override, dict(env, RAYTRACER_RESOURCE_ROOT=str(override)))
    invalid = subprocess.run([installed], cwd=work,
                             env=dict(env, RAYTRACER_RESOURCE_ROOT=str(work / 'missing')),
                             capture_output=True, text=True, timeout=10)
    assert invalid.returncode == 84 and 'RAYTRACER_RESOURCE_ROOT' in invalid.stderr
print('Source fallback, relocated binary, app resources and resource override passed')
