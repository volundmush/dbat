#!/usr/bin/env python3
"""check-members.py [--enforce STRUCT ...] [--show STRUCT[.FIELD]]

Reports direct member access to struct char_data, obj_data and room_data outside the _utils file that
owns each struct. It uses clang's syntax tree, so only real members of these structs count, including
those reached through macros.

--enforce STRUCT  exit non-zero if STRUCT (char_data, obj_data or room_data) has any such access
--show X          also list each access to struct X, or to one field with X = struct.field
"""
import argparse
import collections
import glob
import json
import multiprocessing
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB = os.path.join(ROOT, 'libs', 'dbat')
OWNERS = {'char_data': 'src/entities/character_utils.cpp', 'obj_data': 'src/entities/object_utils.cpp',
          'room_data': 'src/entities/room_utils.cpp'}
STRUCTS = re.compile(r'\b(char_data|obj_data|room_data)\b')


def scan(path):
    """Returns (struct, field, line) for each access in one source file. Headers are read as part of it."""
    dump = subprocess.run(['clang++', '-x', 'c++', '-std=gnu++2c', '-Iinclude', '-Xclang', '-ast-dump=json', '-fsyntax-only', '-w', path],
                          cwd=LIB, capture_output=True)
    if not dump.stdout:
        sys.exit(f'check-members: clang could not parse {path}: {dump.stderr.decode()[:400]}')
    found = []
    line = [0]

    def track(loc):
        """clang only prints a location's line when it changes, so it has to be followed in print order"""
        if not isinstance(loc, dict):
            return
        for key in ('spellingLoc', 'expansionLoc'):
            track(loc.get(key))
        if 'line' in loc:
            line[0] = loc['line']

    def walk(node):
        if node.get('isImplicit') and node.get('kind', '').endswith('Decl'):
            return  # e.g. the copy assignment C++ writes for a struct, which touches every member
        track(node.get('loc'))
        extent = node.get('range', {})
        track(extent.get('begin'))
        at = line[0]
        track(extent.get('end'))
        if node.get('kind') == 'MemberExpr' and node.get('inner'):
            base = node['inner'][0].get('type', {})
            text = base.get('qualType', '') + ' ' + base.get('desugaredQualType', '')
            match = STRUCTS.search(text)
            if match and '(' not in text and OWNERS[match.group(1)] != path:
                found.append((match.group(1), node.get('name'), at))
        for child in node.get('inner') or []:
            if child:
                walk(child)

    sys.setrecursionlimit(100000)
    walk(json.loads(dump.stdout))
    return [(path, *row) for row in found]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--enforce', nargs='*', default=[], choices=sorted(OWNERS))
    parser.add_argument('--show', default=None)
    args = parser.parse_args()

    files = sorted(os.path.relpath(p, LIB) for p in glob.glob(os.path.join(LIB, 'src', '**', '*.cpp'), recursive=True))
    with multiprocessing.get_context('fork').Pool(min(os.cpu_count() or 1, 24)) as pool:
        rows = [row for result in pool.imap_unordered(scan, files, chunksize=2) for row in result]

    totals = collections.Counter(row[1] for row in rows)
    for struct in sorted(OWNERS):
        users = len({row[0] for row in rows if row[1] == struct})
        print(f'check-members: {struct}: {totals[struct]} direct accesses in {users} files')
    if args.show:
        struct, _, field = args.show.partition('.')
        for path, s, f, line in sorted(rows, key=lambda r: (r[0], r[3])):
            if s == struct and (not field or f == field):
                print(f'  libs/dbat/{path}:{line}: {f}')
    failed = [s for s in args.enforce if totals[s]]
    if failed:
        print(f'check-members: FAIL: {", ".join(failed)} still accessed directly')
        sys.exit(1)
    print('check-members: OK')


if __name__ == '__main__':
    main()
