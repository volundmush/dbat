#!/usr/bin/env python3
"""dg_strict.py WORLD_DIR: the triggers the strict DG grammar refuses, and the lines it reads differently.

Reads every trigger in WORLD_DIR/world/trg (or WORLD_DIR/trg) and reports, by trigger vnum and line:
  - lines a strict keyword reads differently from the old driver, which matched keywords by their prefix and
    needed a space after if, while and switch ('if(' was a command; 'elseif(' and 'endx' were else and end);
  - what the strict grammar refuses: a block keyword with no block to belong to, a block still open at the end
    of the script, text after else, end, done, break or default on the same line, if, elseif, while, switch or
    case with nothing after it, and case or default anywhere but directly in a switch.
Exits 0 either way; the report is the point.
"""
import os
import re
import sys

KEYWORDS = ('if', 'elseif', 'else', 'end', 'while', 'done', 'switch', 'case', 'default', 'break')
NO_ARGUMENT = ('else', 'end', 'done', 'break', 'default')
NEEDS_ARGUMENT = ('if', 'elseif', 'while', 'switch', 'case')


def old_kind(line):
    """what the old driver took the line for, from its own tests in their order"""
    s = line.lstrip().lower()
    if s.startswith('*'):
        return 'comment'
    for prefix, kind in (('if ', 'if'), ('elseif ', 'elseif'), ('else', 'else'), ('while ', 'while'), ('switch ', 'switch'),
                         ('end', 'end'), ('done', 'done'), ('break', 'break'), ('case', 'case')):
        if s.startswith(prefix):
            return kind
    return 'command'


def new_kind(line):
    """the strict reading: a keyword is the whole first word, which ends at a space or '('"""
    s = line.strip()
    if not s:
        return 'blank', ''
    if s.startswith('*'):
        return 'comment', ''
    m = re.match(r'([A-Za-z_]+)(.*)$', s)
    if m and m[1].lower() in KEYWORDS and (not m[2] or m[2][0] in ' \t('):
        return m[1].lower(), m[2].strip()
    return 'command', ''


def triggers(directory):
    """(vnum, [script lines]) for each trigger in the .trg files"""
    for name in sorted(os.listdir(directory)):
        if not name.endswith('.trg'):
            continue
        text = open(os.path.join(directory, name), encoding='latin-1').read()
        for chunk in re.split(r'^#', text, flags=re.M)[1:]:
            vnum = chunk.split('\n', 1)[0].strip()
            if not vnum.isdigit():
                continue
            parts = chunk.split('~')
            if len(parts) < 3:
                continue
            body = parts[2]
            if body.startswith('\r\n'):
                body = body[2:]
            elif body.startswith('\n'):
                body = body[1:]
            yield int(vnum), body.replace('\r', '').split('\n')


def check(lines):
    """the strict grammar's errors, and the lines it reads differently from the old driver"""
    errors, changed, open_blocks = [], [], []  # open_blocks: (keyword, line)
    for number, line in enumerate(lines, 1):
        kind, rest = new_kind(line)
        old = old_kind(line)
        if kind in ('blank',):
            continue
        if old != kind and not (old == 'command' and kind == 'comment'):
            changed.append((number, f"read as {kind}, where the old driver ran it as {old}: {line.strip()[:60]}"))
        if kind in NO_ARGUMENT and rest:
            errors.append((number, f"text after '{kind}': {rest[:40]}"))
        if kind in NEEDS_ARGUMENT and not rest:
            errors.append((number, f"'{kind}' with nothing to test"))
        top = open_blocks[-1] if open_blocks else None
        if kind == 'if':
            open_blocks.append(('if', number))
        elif kind in ('elseif', 'else'):
            if not top or top[0] != 'if':
                errors.append((number, f"'{kind}' with no 'if' open" + (f" (the open block is the '{top[0]}' at line {top[1]})" if top else '')))
        elif kind == 'end':
            if not top or top[0] != 'if':
                errors.append((number, "'end' closes nothing" + (f" (the open block is the '{top[0]}' at line {top[1]})" if top else '')))
            else:
                open_blocks.pop()
        elif kind in ('while', 'switch'):
            open_blocks.append((kind, number))
        elif kind == 'done':
            if not top or top[0] not in ('while', 'switch'):
                errors.append((number, "'done' closes nothing" + (f" (the open block is the '{top[0]}' at line {top[1]})" if top else '')))
            else:
                open_blocks.pop()
        elif kind in ('case', 'default'):
            if not top or top[0] != 'switch':
                errors.append((number, f"'{kind}' outside a switch's own lines"))
        elif kind == 'break':
            if not any(k in ('while', 'switch') for k, _ in open_blocks):
                errors.append((number, "'break' outside a while or switch"))
    for keyword, number in open_blocks:
        errors.append((number, f"'{keyword}' never closed"))
    return sorted(errors), changed


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    directory = sys.argv[1]
    for sub in ('world/trg', 'trg'):
        if os.path.isdir(os.path.join(directory, sub)):
            directory = os.path.join(directory, sub)
            break
    total = refused = differ = 0
    for vnum, lines in triggers(directory):
        total += 1
        errors, changed = check(lines)
        if errors:
            refused += 1
            print(f'trigger {vnum}: refused')
            for number, why in errors:
                print(f'  line {number}: {why}')
        if changed:
            differ += 1
            if not errors:
                print(f'trigger {vnum}: compiles, but reads differently')
            for number, why in changed:
                print(f'  line {number}: {why}')
    print(f'{total} triggers: {refused} refused, {differ} with lines read differently')


if __name__ == '__main__':
    main()
