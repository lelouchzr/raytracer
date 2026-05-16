#!/usr/bin/env python3
"""Run Memcheck without suppressions and reject errors, leaks or incomplete reports."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--valgrind', required=True)
parser.add_argument('--log', type=Path, required=True)
parser.add_argument('--allow-reachable', action='store_true',
                    help='Report third-party process caches, while rejecting all lost blocks')
parser.add_argument('--expect-exit', type=int, default=0)
parser.add_argument('command', nargs=argparse.REMAINDER)
args = parser.parse_args()
command = args.command[1:] if args.command[:1] == ['--'] else args.command
if not command:
    parser.error('a program is required after --')
args.log.parent.mkdir(parents=True, exist_ok=True)
xml_path = args.log.with_suffix('.xml').resolve()
summary_path = args.log.with_suffix('.json').resolve()
flags = ['--tool=memcheck', '--leak-check=full', '--show-leak-kinds=all',
         '--errors-for-leak-kinds=' + ('definite,indirect,possible' if args.allow_reachable else 'all'),
         '--default-suppressions=no', '--error-exitcode=99', '--keep-debuginfo=yes',
         '--num-callers=24', '--xml=yes', '--xml-file=' + str(xml_path)]
try:
    completed = subprocess.run([args.valgrind, *flags, *command], timeout=240,
                               env=dict(os.environ, EPITECH_MEMCHECK="1"))
    report = ET.parse(xml_path).getroot()
    errors = report.findall('error')
    reachable = [item for item in errors if item.findtext('kind') == 'Leak_StillReachable']
    failures = [item for item in errors if not
                (args.allow_reachable and item.findtext('kind') == 'Leak_StillReachable')]
    suppressed = sum(int(item.findtext('count', '0')) for item in report.findall('suppcounts/pair'))
    summary = {'command': command, 'exit': completed.returncode,
               'finished': bool(report.findall("status[state='FINISHED']")),
               'errors': [item.findtext('kind') for item in failures],
               'lostBytes': sum(int(item.findtext('xwhat/leakedbytes', '0')) for item in errors
                                if item.findtext('kind') != 'Leak_StillReachable'),
               'reachableBytes': sum(int(item.findtext('xwhat/leakedbytes', '0')) for item in reachable),
               'suppressed': suppressed, 'allowReachable': args.allow_reachable}
    summary_path.write_text(json.dumps(summary, indent=2) + '\n')
    if (completed.returncode != args.expect_exit or failures or suppressed or not summary['finished']):
        raise RuntimeError(f'Memory check failed; see {xml_path} and {summary_path}')
    print(f'Memcheck passed: 0 errors, 0 lost bytes; reachable={summary["reachableBytes"]}; suppressions=0')
except (OSError, subprocess.TimeoutExpired, ET.ParseError, RuntimeError) as error:
    print(error, file=sys.stderr)
    sys.exit(1)
