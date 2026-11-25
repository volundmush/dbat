#!/usr/bin/env python3
"""check-conversions.py [--update] [--show FILE]: implicit conversions and C-style casts only go down.

The game builds with -Wconversion, -Wsign-conversion and -Wold-style-cast as warnings, not errors, while the ones
already there are fixed. This counts each file's warning sites (a header's once, however many files include it), by
compiling every file in build/compile_commands.json (the debug build's) with -fsyntax-only, and fails if a file has
more than check-conversions.baseline says, or has any and isn't listed. --update lowers the baseline after a change
that removes some; --show FILE lists one file's sites. An index into a std::array or std::span (a signed int converted to
its size_type) doesn't count: with _GLIBCXX_ASSERTIONS, which every preset defines, [] checks it, so a negative index
stops the game instead of reading out of bounds. When every count is zero, the baseline goes and the warnings
become errors.

JOBS sets how many compilers run at once (default: one per core).
"""
import argparse, collections, concurrent.futures, json, os, re, shlex, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
BASELINE = os.path.join(ROOT, 'tools', 'check-conversions.baseline')
DATABASE = os.path.join(ROOT, 'build', 'compile_commands.json')
FLAGS = ['-Wconversion', '-Wsign-conversion', '-Wold-style-cast']
CHECKED_INDEX = re.compile(r'conversion to .std::(?:array|span)<.*>::size_type. .*may change the sign of the result \[-Wsign-conversion\]$')
WARNING = re.compile(r'^(?P<file>[^:\s]+):(?P<line>\d+):(?P<col>\d+): warning: .*\[(?P<flag>-W(?:conversion|float-conversion|sign-conversion|old-style-cast))\]$')


def sites(entry):
    """The warning sites one file's compile gives: (file, line, column, flag)"""
    args = shlex.split(entry['command'])
    out, skip = [], False
    for a in args:
        if skip:
            skip = False
        elif a == '-o':
            skip = True
        elif a not in ('-c', '-Werror') and not a.startswith('-fsanitize'):
            out.append(a)
    out += FLAGS + ['-fsyntax-only', '-fdiagnostics-plain-output', '-fno-diagnostics-show-caret']
    result = subprocess.run(out, cwd=entry['directory'], capture_output=True, text=True)
    found = set()
    for line in result.stderr.splitlines():
        m = WARNING.match(line)
        if m and not CHECKED_INDEX.search(line):
            path = os.path.relpath(os.path.realpath(os.path.join(entry['directory'], m['file'])), ROOT)
            found.add((path, int(m['line']), int(m['col']), m['flag']))
    return found


def count_all():
    with open(DATABASE) as f:
        entries = [e for e in json.load(f) if os.path.realpath(e['file']).startswith(ROOT + os.sep)]
    found = set()
    jobs = int(os.environ.get('JOBS', 0)) or os.cpu_count() or 1
    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        for s in pool.map(sites, entries):
            found |= s
    return found


def read_baseline():
    counts = {}
    if os.path.exists(BASELINE):
        for line in open(BASELINE):
            if line.strip() and not line.startswith('#'):
                n, path = line.split(None, 1)
                counts[path.strip()] = int(n)
    return counts


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--update', action='store_true')
    parser.add_argument('--show')
    args = parser.parse_args()
    if not os.path.exists(DATABASE):
        sys.exit(f'check-conversions: no {os.path.relpath(DATABASE, ROOT)}; configure the debug preset first')

    found = count_all()
    per_file = collections.Counter(path for path, *_ in found)
    if args.show:
        for path, line, col, flag in sorted(s for s in found if s[0] == args.show):
            print(f'{path}:{line}:{col}: {flag}')
        return 0
    if args.update:
        with open(BASELINE, 'w') as f:
            f.write('# check-conversions.py\'s counts: <warning sites> <file>. Lower it with tools/check-conversions.py --update.\n')
            for path in sorted(per_file):
                f.write(f'{per_file[path]} {path}\n')
        print(f'check-conversions: baseline updated, {sum(per_file.values())} sites in {len(per_file)} files')
        return 0

    baseline, fail, lower = read_baseline(), False, False
    for path, n in sorted(per_file.items()):
        was = baseline.get(path)
        if was is None:
            print(f'conversions: {path} has {n} and no baseline (it may have none)')
            fail = True
        elif n > was:
            print(f'conversions: {path} has {n}, more than its baseline of {was} (tools/check-conversions.py --show {path})')
            fail = True
        elif n < was:
            lower = True
    if any(path not in per_file for path in baseline):
        lower = True
    if lower:
        print('check-conversions: some files are below their baseline; lower it with tools/check-conversions.py --update')
    if not fail:
        print(f'check-conversions: OK, {sum(per_file.values())} sites left')
    return 1 if fail else 0


if __name__ == '__main__':
    sys.exit(main())
