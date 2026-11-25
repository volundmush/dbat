#!/usr/bin/env python3
"""check-arrays.py [--update] [--list]: an array is a std::array (or a std::span when a function takes one). This
counts, per file of the game and its tests (libs/dbat/src, libs/dbat/include, libs/dbat/tests, apps, tools/*.cpp),
the C arrays still declared: struct members, tables, locals (`int value[N];`, `static const T NAME[] = {...};`) and
array parameters (`T arr[]`), which decay to pointers.

With _GLIBCXX_ASSERTIONS (every preset defines it) std::array's and std::span's [] check the index, so an index out of
bounds stops with a message instead of reading or writing past the end; a C array doesn't.

It reads the text, not the syntax tree. The counts are a ratchet while the arrays convert: check-arrays.baseline holds
each file's, and this fails if a file has more than its baseline (or any, if it isn't listed). After a change that
converts some, run it with --update to lower the baseline. --list prints each one found, as file:line: name.
"""
import collections
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE = os.path.join(ROOT, 'tools', 'check-arrays.baseline')
NOT_A_TYPE = {'return', 'delete', 'case', 'else', 'goto', 'throw', 'new', 'co_return', 'co_yield', 'sizeof', 'typedef', 'using', 'operator', 'do'}
NOT_A_NAME = {'auto', 'operator', 'const', 'new', 'delete'}

# a declaration that starts a line: qualifiers, a type (with a template's arguments), pointers, a name, its bounds
LINE = re.compile(r'^[ \t]*(?:(?:static|const|constexpr|extern|inline|thread_local|mutable|volatile|struct|unsigned|signed|long|short)\s+)*'
                  r'([A-Za-z_][\w:]*(?:<[^;{}()\n]*>)?)(?:\s+const)?((?:\s*[*&]+\s*|\s+))([A-Za-z_]\w*)\s*\[[^\]\n]*\]\s*(?:\[[^\]\n]*\]\s*)*[;=,{)]', re.M)
# a parameter: `(T name[]` or `, T name[N]`, followed by `,` or `)`
PARAM = re.compile(r'[(,]\s*(?:const\s+)?(?:struct\s+)?([A-Za-z_][\w:]*(?:<[^;{}()\n]*>)?)\s*[*&\s]+\s*([A-Za-z_]\w*)\s*\[[^\]\n]*\]\s*(?=[,)])')


def strip(s):
    """the text without comments and string or char literals, its lines kept where they were"""
    s = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', '', m.group(0)), s, flags=re.S)
    s = re.sub(r'//[^\n]*', '', s)
    s = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', s)
    s = re.sub(r"'(?:[^'\\\n]|\\.)+'", "' '", s)
    return re.sub(r'^[ \t]*#[^\n]*', '', s, flags=re.M)  # the preprocessor's lines


def files():
    found = []
    for pattern in ('libs/dbat/src/**/*.cpp', 'libs/dbat/src/**/*.hpp', 'libs/dbat/include/**/*.hpp', 'libs/dbat/tests/**/*.cpp',
                    'libs/dbat/tests/**/*.hpp', 'apps/**/*.cpp', 'apps/**/*.hpp', 'tools/*.cpp'):
        found += glob.glob(os.path.join(ROOT, pattern), recursive=True)
    return sorted(set(found))


def find(f, s):
    """(file, line, name) for each C array declared"""
    out = set()
    for m in LINE.finditer(s):
        kind, name = m.group(1), m.group(3)
        if kind in NOT_A_TYPE or name in NOT_A_NAME or kind.startswith('std::array'):
            continue
        out.add((f, s[:m.start(3)].count('\n') + 1, name))
    for m in PARAM.finditer(s):
        if m.group(1) in NOT_A_TYPE or m.group(2) in NOT_A_NAME:
            continue
        out.add((f, s[:m.start(2)].count('\n') + 1, m.group(2)))
    return out


def main():
    found = []
    for path in files():
        f = os.path.relpath(path, ROOT)
        found += find(f, strip(open(path, encoding='utf-8', errors='replace').read()))
    found.sort()
    if '--list' in sys.argv:
        for f, line, name in found:
            print(f'{f}:{line}: {name}')
    per = collections.Counter(f for f, *_ in found)
    if '--update' in sys.argv:
        with open(BASELINE, 'w') as out:
            out.write("# check-arrays.py's counts: <C arrays declared> <file>. Lower it with tools/check-arrays.py --update.\n")
            for f in sorted(per):
                out.write(f'{per[f]} {f}\n')
        print(f'check-arrays: baseline updated, {sum(per.values())} in {len(per)} files')
        return 0
    base = {}
    if os.path.exists(BASELINE):
        for line in open(BASELINE):
            if line[:1].isdigit():
                n, f = line.split(maxsplit=1)
                base[f.strip()] = int(n)
    fail = False
    for f, n in sorted(per.items()):
        if n > base.get(f, 0):
            print(f'check-arrays: {f} has {n}, more than its baseline of {base.get(f, 0)}')
            fail = True
    if any(per.get(f, 0) < n for f, n in base.items()):
        print('check-arrays: some files are below their baseline; lower it with tools/check-arrays.py --update')
    if not fail:
        print(f'check-arrays: OK, {sum(per.values())} left')
    return 1 if fail else 0


if __name__ == '__main__':
    sys.exit(main())
