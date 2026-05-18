#!/usr/bin/env python3
"""Independent checks for the requested source organization and formatting."""
import pathlib
import re
import subprocess
import sys

root = pathlib.Path(sys.argv[1]).resolve()
files = []
for directory in ('include', 'src', 'bonus', 'tests'):
    base = root / directory
    if base.exists():
        files.extend(p for p in base.rglob('*') if p.suffix in ('.cpp', '.hpp') and 'vendor' not in p.parts)
errors = []
for path in files:
    result = subprocess.run(['clang-format', '--dry-run', '--Werror', str(path)],
                            text=True, capture_output=True)
    if result.returncode:
        errors.append(f'{path.relative_to(root)}: formatting differs from .clang-format')
    text = path.read_text()
    if path.suffix == '.cpp' and 'tests' not in path.relative_to(root).parts:
        # Only class bodies are flagged; template/function uses of `class T` are allowed.
        declaration = re.compile(r'^\s*(?:class|struct)\s+\w+(?:\s+final)?\s*(?::[^;{]+)?\s*\{', re.M)
        for match in declaration.finditer(text):
            line = text.count('\n', 0, match.start()) + 1
            errors.append(f'{path.relative_to(root)}:{line}: class/struct definition belongs in .hpp')
    if re.search(r'\b(?:TODO|FIXME|not implemented)\b', text, re.I):
        errors.append(f'{path.relative_to(root)}: unfinished marker requires review')
if errors:
    print('\n'.join(errors))
    sys.exit(1)
print(f'{root.name}: {len(files)} C++ files satisfy formatting and source-organization checks')
