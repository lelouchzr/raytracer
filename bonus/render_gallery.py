#!/usr/bin/env python3
"""Compatibility entry point for the selected root gallery; forwards CLI arguments."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
raise SystemExit(subprocess.call([sys.executable, str(root / 'tools/render_examples.py'), *sys.argv[1:]], cwd=root))
