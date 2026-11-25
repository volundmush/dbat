#!/usr/bin/env python3
"""check-bools.py [--update] [--list]: a yes or no is a bool. This counts, per file of the game (libs/dbat/src,
libs/dbat/include, apps), the integers that only ever hold 0 or 1:

  - an int function whose every return is 0, 1, true, false, a comparison, a logical expression, a bool function's
    result or another such function's;
  - an int parameter that every call gives a 0/1 literal (both seen);
  - an int local only ever set to 0/1 literals (or comparisons), never counted with ++, += and the like.

It reads the text, not the syntax tree, so it misses some and counts a few numbers that happen to be 0 or 1 (a count
that is only ever reset, say). Those stay int, and the baseline keeps them: check-bools.baseline holds each file's
count, and this fails if a file has more than its baseline (or any, if it isn't listed). After a change that converts
some, run it with --update to lower the baseline. --list prints each one found, as file:line: kind name.
"""
import collections
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE = os.path.join(ROOT, 'tools', 'check-bools.baseline')
INT = r'(?:int|long|short|unsigned|unsigned int|int8_t|uint8_t|int16_t|int32_t|int64_t|sbyte|byte|ubyte)'
LIT = {'0', '1', 'true', 'false'}
YES, NO = {'1', 'true'}, {'0', 'false'}


def strip(s):
    """the text without comments and string or char literals, its lines kept where they were"""
    s = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', '', m.group(0)), s, flags=re.S)
    s = re.sub(r'//[^\n]*', '', s)
    s = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', s)
    return re.sub(r"'(?:[^'\\\n]|\\.)+'", "' '", s)


def unparen(r):
    r = r.strip()
    while r.startswith('(') and r.endswith(')'):
        depth = 0
        for k, c in enumerate(r):
            depth += (c == '(') - (c == ')')
            if depth == 0 and k < len(r) - 1:
                return r
        r = r[1:-1].strip()
    return r


def args_of(text):
    """a call's arguments, split at its top-level commas"""
    out, depth, cur = [], 0, ''
    for c in text:
        depth += (c in '([{') - (c in ')]}')
        if c == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += c
    return out + [cur.strip()] if cur.strip() else out


def body_at(s, start):
    """the text from start (just past a '{') to its matching '}'"""
    i, depth = start, 1
    while depth and i < len(s):
        depth += (s[i] == '{') - (s[i] == '}')
        i += 1
    return s[start:i - 1]


def files():
    found = []
    for pattern in ('libs/dbat/src/**/*.cpp', 'libs/dbat/src/**/*.hpp', 'libs/dbat/include/**/*.hpp', 'apps/**/*.cpp', 'apps/**/*.hpp'):
        found += glob.glob(os.path.join(ROOT, pattern), recursive=True)
    return sorted(set(found))


DEF = re.compile(r'^[ \t]*(?:static\s+|inline\s+|constexpr\s+)*(' + INT + r'|bool)\s+\**\s*([A-Za-z_][\w:]*)\s*\(([^;{]*?)\)\s*(?:const\s*)?\n?\s*\{', re.M)


def find(src):
    """(file, line, kind, name) for each one"""
    fns, bools = {}, set()
    for f, s in src.items():
        for m in DEF.finditer(s):
            name = m.group(2).split('::')[-1]
            if m.group(1) == 'bool':
                bools.add(name)
            elif name != 'main':
                fns[(f, name)] = (s[:m.start()].count('\n') + 1, body_at(s, m.end()), m.group(3))
    good = set()

    def boolean(r, body, top=True):
        r = unparen(r)
        if r in LIT or (r.startswith('!') and not r.startswith('!=')):
            return True
        depth = 0
        for k, c in enumerate(r):  # a choice (a ? b : c) is boolean if both answers are
            depth += (c in '([{') - (c in ')]}')
            if depth == 0 and c == '?':
                rest = r[k + 1:]
                d = 0
                for j, c2 in enumerate(rest):
                    d += (c2 in '([{?') - (c2 in ')]}')
                    if c2 == ':' and d == 0:
                        return boolean(rest[:j], body, top) and boolean(rest[j + 1:], body, top)
                    if c2 == ':':
                        d -= 1
                return False
        depth = 0
        for k, c in enumerate(r):
            depth += (c in '([{') - (c in ')]}')
            if depth == 0 and (r.startswith(('==', '!=', '<=', '>=', '&&', '||'), k) or (c in '<>' and r[k - 1:k] == ' ' and r[k + 1:k + 2] == ' ')):
                return True
        m = re.fullmatch(r'([A-Za-z_][\w:]*)\s*\(.*\)', r, re.S)
        if m:
            callee = m.group(1).split('::')[-1]
            return callee in bools or callee in good
        if top and re.fullmatch(r'[A-Za-z_]\w*', r):  # a local: what it is set to
            sets = re.findall(r'(?<![\w.>=!<])' + r + r'\s*=(?!=)\s*([^;]*);', body)
            if not sets or re.search(r'\b' + r + r'\s*(\+\+|--|\+=|-=|\*=|/=|\|=|&=)|(\+\+|--)' + r + r'\b|&' + r + r'\b', body):
                return False
            return all(boolean(v, body, False) for v in sets) and bool({unparen(v) for v in sets} & (YES | NO))
        return False

    while True:
        new = set()
        for (f, name), (line, body, _) in fns.items():
            rets = [unparen(r) for r in re.findall(r'\breturn\b([^;]*);', body)]
            only_literals = all(r in LIT for r in rets)
            one_answer = only_literals and not (set(rets) & YES and set(rets) & NO)  # e.g. a stub that always says 0
            if name not in good and rets and not one_answer and all(boolean(r, body) for r in rets):
                new.add(name)
        if not new:
            break
        good |= new
    out = [(f, line, 'function', name) for (f, name), (line, _, _) in fns.items() if name in good]

    # parameters: every call gives a literal 0/1, and both are seen
    every = '\n'.join(src.values())
    defs = {}
    for f, s in src.items():
        for m in re.finditer(r'^[ \t]*(?:static\s+|inline\s+)*[\w:<>]+[\s*&]+([a-z_]\w*)\s*\(([^;{)]*)\)\s*\n?\s*\{', s, re.M):
            params = args_of(m.group(2))
            ints = [k for k, p in enumerate(params) if re.fullmatch(INT + r'\s+[a-z_]\w*(\s*=\s*\w+)?', p)]
            if ints:
                defs[m.group(1)] = (f, s[:m.start()].count('\n') + 1, params, ints)
    callsites = collections.defaultdict(list)  # each name followed by '(', found in one pass
    for m in re.finditer(r'(?<![\w.>:])([a-z_]\w*)\s*\(', every):
        if m.group(1) in defs:
            callsites[m.group(1)].append(m)
    for name, (f, line, params, ints) in defs.items():
        calls = []
        for m in callsites[name]:
            before = every[max(0, m.start() - 30):m.start()]
            if re.search(r'[\w>*&]\s+\**$', before) and not re.search(r'(return|=|\(|,|!|&&|\|\|)\s*$', before):
                continue  # its definition or a declaration
            calls.append(_call_args(every, m.end()))
        for k in ints:
            given = [c[k] for c in calls if len(c) > k]
            if given and set(given) <= LIT and set(given) & YES and set(given) & NO:
                out.append((f, line, 'parameter', f'{name}:{params[k].split()[1].split("=")[0]}'))

    # locals
    for f, s in src.items():
        if not f.endswith('.cpp'):
            continue
        for m in re.finditer(r'^[ \t]*int\s+([^;()]*);', s, re.M):
            end = s.find('\n}\n', m.end())
            body = s[m.end():end if end > 0 else len(s)]
            for part in m.group(1).split(','):
                v = re.fullmatch(r'\s*([a-z_]\w*)\s*(?:=\s*(\S+))?\s*', part)
                if not v or (v.group(2) is not None and v.group(2) not in LIT):
                    continue
                name = v.group(1)
                sets = [unparen(x) for x in re.findall(r'(?<![\w.>=!<])' + name + r'\s*=(?!=)\s*([^;,)]*)', body)]
                if not sets and v.group(2) is None:
                    continue
                if not all(x in LIT for x in sets):
                    continue
                if not ({*sets, v.group(2) or ''} & YES) or not ({*sets, v.group(2) or ''} & NO):
                    continue
                if re.search(r'(?<![\w.>])' + name + r'\s*(\+\+|--|\+=|-=|\*=|/=|\|=|&=)|(\+\+|--)' + name + r'\b|&' + name + r'\b', body):
                    continue
                if len(re.findall(r'(?<![\w.>])' + name + r'\b', body)) < 2:
                    continue
                out.append((f, s[:m.start()].count('\n') + 1, 'local', name))
    return out


def _call_args(text, start):
    i, depth = start, 1
    while depth and i < len(text):
        depth += (text[i] == '(') - (text[i] == ')')
        i += 1
    return args_of(text[start:i - 1])


def main():
    src = {os.path.relpath(f, ROOT): strip(open(f, encoding='utf-8', errors='replace').read()) for f in files()}
    found = sorted(find(src))
    if '--list' in sys.argv:
        for f, line, kind, name in found:
            print(f'{f}:{line}: {kind} {name}')
    per = collections.Counter(f for f, *_ in found)
    if '--update' in sys.argv:
        with open(BASELINE, 'w') as out:
            out.write("# check-bools.py's counts: <ints that only hold 0 or 1> <file>. Lower it with tools/check-bools.py --update.\n")
            for f in sorted(per):
                out.write(f'{per[f]} {f}\n')
        print(f'check-bools: baseline updated, {sum(per.values())} in {len(per)} files')
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
            print(f'check-bools: {f} has {n}, more than its baseline of {base.get(f, 0)}')
            fail = True
    if any(per.get(f, 0) < n for f, n in base.items()):
        print('check-bools: some files are below their baseline; lower it with tools/check-bools.py --update')
    if not fail:
        print(f'check-bools: OK, {sum(per.values())} left')
    return 1 if fail else 0


if __name__ == '__main__':
    sys.exit(main())
